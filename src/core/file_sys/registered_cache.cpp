// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <random>
#include <regex>
#include <mbedtls/sha256.h>
#include "common/assert.h"
#include "common/fs/path_util.h"
#include "common/hex_util.h"
#include "common/logging/log.h"
#include "core/crypto/key_manager.h"
#include "core/file_sys/card_image.h"
#include "core/file_sys/common_funcs.h"
#include "core/file_sys/content_archive.h"
#include "core/file_sys/nca_metadata.h"
#include "core/file_sys/registered_cache.h"
#include "core/file_sys/submission_package.h"
#include "core/file_sys/vfs_concat.h"
#include "core/loader/loader.h"

namespace FileSys {

// The size of blocks to use when vfs raw copying into nand.
constexpr size_t VFS_RC_LARGE_COPY_BLOCK = 0x400000;

std::string ContentProviderEntry::DebugInfo() const {
    return fmt::format("title_id={:016X}, content_type={:02X}", title_id, static_cast<u8>(type));
}

bool operator<(const ContentProviderEntry& lhs, const ContentProviderEntry& rhs) {
    return (lhs.title_id < rhs.title_id) || (lhs.title_id == rhs.title_id && lhs.type < rhs.type);
}

bool operator==(const ContentProviderEntry& lhs, const ContentProviderEntry& rhs) {
    return std::tie(lhs.title_id, lhs.type) == std::tie(rhs.title_id, rhs.type);
}

bool operator!=(const ContentProviderEntry& lhs, const ContentProviderEntry& rhs) {
    return !operator==(lhs, rhs);
}

static bool FollowsTwoDigitDirFormat(std::string_view name) {
    static const std::regex two_digit_regex("000000[0-9A-F]{2}", std::regex_constants::ECMAScript |
                                                                     std::regex_constants::icase);
    return std::regex_match(name.begin(), name.end(), two_digit_regex);
}

static bool FollowsNcaIdFormat(std::string_view name) {
    static const std::regex nca_id_regex("[0-9A-F]{32}\\.nca", std::regex_constants::ECMAScript |
                                                                   std::regex_constants::icase);
    static const std::regex nca_id_cnmt_regex(
        "[0-9A-F]{32}\\.cnmt.nca", std::regex_constants::ECMAScript | std::regex_constants::icase);
    return (name.size() == 36 && std::regex_match(name.begin(), name.end(), nca_id_regex)) ||
           (name.size() == 41 && std::regex_match(name.begin(), name.end(), nca_id_cnmt_regex));
}

static std::string GetRelativePathFromNcaID(const std::array<u8, 16>& nca_id, bool second_hex_upper,
                                            bool within_two_digit, bool cnmt_suffix) {
    if (!within_two_digit) {
        const auto format_str = fmt::runtime(cnmt_suffix ? "{}.cnmt.nca" : "/{}.nca");
        return fmt::format(format_str, Common::HexToString(nca_id, second_hex_upper));
    }

    Core::Crypto::SHA256Hash hash{};
    mbedtls_sha256_ret(nca_id.data(), nca_id.size(), hash.data(), 0);

    const auto format_str =
        fmt::runtime(cnmt_suffix ? "/000000{:02X}/{}.cnmt.nca" : "/000000{:02X}/{}.nca");
    return fmt::format(format_str, hash[0], Common::HexToString(nca_id, second_hex_upper));
}

static std::string GetCNMTName(TitleType type, u64 title_id) {
    constexpr std::array<const char*, 9> TITLE_TYPE_NAMES{
        "SystemProgram",
        "SystemData",
        "SystemUpdate",
        "BootImagePackage",
        "BootImagePackageSafe",
        "Application",
        "Patch",
        "AddOnContent",
        "" ///< Currently unknown 'DeltaTitle'
    };

    auto index = static_cast<std::size_t>(type);
    // If the index is after the jump in TitleType, subtract it out.
    if (index >= static_cast<std::size_t>(TitleType::Application)) {
        index -= static_cast<std::size_t>(TitleType::Application) -
                 static_cast<std::size_t>(TitleType::FirmwarePackageB);
    }
    return fmt::format("{}_{:016x}.cnmt", TITLE_TYPE_NAMES[index], title_id);
}

ContentRecordType GetCRTypeFromNCAType(NCAContentType type) {
    switch (type) {
    case NCAContentType::Program:
        // TODO(DarkLordZach): Differentiate between Program and Patch
        return ContentRecordType::Program;
    case NCAContentType::Meta:
        return ContentRecordType::Meta;
    case NCAContentType::Control:
        return ContentRecordType::Control;
    case NCAContentType::Data:
    case NCAContentType::PublicData:
        return ContentRecordType::Data;
    case NCAContentType::Manual:
        // TODO(DarkLordZach): Peek at NCA contents to differentiate Manual and Legal.
        return ContentRecordType::HtmlDocument;
    default:
        UNREACHABLE_MSG("Invalid NCAContentType={:02X}", type);
        return ContentRecordType{};
    }
}

ContentProvider::~ContentProvider() = default;

bool ContentProvider::HasEntry(ContentProviderEntry entry) const {
    return HasEntry(entry.title_id, entry.type);
}

VirtualFile ContentProvider::GetEntryUnparsed(ContentProviderEntry entry) const {
    return GetEntryUnparsed(entry.title_id, entry.type);
}

VirtualFile ContentProvider::GetEntryRaw(ContentProviderEntry entry) const {
    return GetEntryRaw(entry.title_id, entry.type);
}

std::unique_ptr<NCA> ContentProvider::GetEntry(ContentProviderEntry entry) const {
    return GetEntry(entry.title_id, entry.type);
}

std::vector<ContentProviderEntry> ContentProvider::ListEntries() const {
    return ListEntriesFilter(std::nullopt, std::nullopt, std::nullopt);
}

PlaceholderCache::PlaceholderCache(VirtualDir dir_) : dir(std::move(dir_)) {}

bool PlaceholderCache::Create(const NcaID& id, u64 size) const {
    const auto path = GetRelativePathFromNcaID(id, false, true, false);

    if (dir->GetFileRelative(path) != nullptr) {
        return false;
    }

    Core::Crypto::SHA256Hash hash{};
    mbedtls_sha256_ret(id.data(), id.size(), hash.data(), 0);
    const auto dirname = fmt::format("000000{:02X}", hash[0]);

    const auto dir2 = GetOrCreateDirectoryRelative(dir, dirname);

    if (dir2 == nullptr)
        return false;

    const auto file = dir2->CreateFile(fmt::format("{}.nca", Common::HexToString(id, false)));

    if (file == nullptr)
        return false;

    return file->Resize(size);
}

bool PlaceholderCache::Delete(const NcaID& id) const {
    const auto path = GetRelativePathFromNcaID(id, false, true, false);

    if (dir->GetFileRelative(path) == nullptr) {
        return false;
    }

    Core::Crypto::SHA256Hash hash{};
    mbedtls_sha256_ret(id.data(), id.size(), hash.data(), 0);
    const auto dirname = fmt::format("000000{:02X}", hash[0]);

    const auto dir2 = GetOrCreateDirectoryRelative(dir, dirname);

    const auto res = dir2->DeleteFile(fmt::format("{}.nca", Common::HexToString(id, false)));

    return res;
}

bool PlaceholderCache::Exists(const NcaID& id) const {
    const auto path = GetRelativePathFromNcaID(id, false, true, false);

    return dir->GetFileRelative(path) != nullptr;
}

bool PlaceholderCache::Write(const NcaID& id, u64 offset, const std::vector<u8>& data) const {
    const auto path = GetRelativePathFromNcaID(id, false, true, false);
    const auto file = dir->GetFileRelative(path);

    if (file == nullptr)
        return false;

    return file->WriteBytes(data, offset) == data.size();
}

bool PlaceholderCache::Register(RegisteredCache* cache, const NcaID& placeholder,
                                const NcaID& install) const {
    const auto path = GetRelativePathFromNcaID(placeholder, false, true, false);
    const auto file = dir->GetFileRelative(path);

    if (file == nullptr)
        return false;

    const auto res = cache->RawInstallNCA(NCA{file}, &VfsRawCopy, false, install);

    if (res != InstallResult::Success)
        return false;

    return Delete(placeholder);
}

bool PlaceholderCache::CleanAll() const {
    return dir->GetParentDirectory()->CleanSubdirectoryRecursive(dir->GetName());
}

std::optional<std::array<u8, 0x10>> PlaceholderCache::GetRightsID(const NcaID& id) const {
    const auto path = GetRelativePathFromNcaID(id, false, true, false);
    const auto file = dir->GetFileRelative(path);

    if (file == nullptr)
        return std::nullopt;

    NCA nca{file};

    if (nca.GetStatus() != Loader::ResultStatus::Success &&
        nca.GetStatus() != Loader::ResultStatus::ErrorMissingBKTRBaseRomFS) {
        return std::nullopt;
    }

    const auto rights_id = nca.GetRightsId();
    if (rights_id == NcaID{})
        return std::nullopt;

    return rights_id;
}

u64 PlaceholderCache::Size(const NcaID& id) const {
    const auto path = GetRelativePathFromNcaID(id, false, true, false);
    const auto file = dir->GetFileRelative(path);

    if (file == nullptr)
        return 0;

    return file->GetSize();
}

bool PlaceholderCache::SetSize(const NcaID& id, u64 new_size) const {
    const auto path = GetRelativePathFromNcaID(id, false, true, false);
    const auto file = dir->GetFileRelative(path);

    if (file == nullptr)
        return false;

    return file->Resize(new_size);
}

std::vector<NcaID> PlaceholderCache::List() const {
    std::vector<NcaID> out;
    for (const auto& sdir : dir->GetSubdirectories()) {
        for (const auto& file : sdir->GetFiles()) {
            const auto name = file->GetName();
            if (name.length() == 36 && name.ends_with(".nca")) {
                out.push_back(Common::HexStringToArray<0x10>(name.substr(0, 32)));
            }
        }
    }
    return out;
}

NcaID PlaceholderCache::Generate() {
    std::random_device device;
    std::mt19937 gen(device());
    std::uniform_int_distribution<u64> distribution(1, std::numeric_limits<u64>::max());

    NcaID out{};

    const auto v1 = distribution(gen);
    const auto v2 = distribution(gen);
    std::memcpy(out.data(), &v1, sizeof(u64));
    std::memcpy(out.data() + sizeof(u64), &v2, sizeof(u64));

    return out;
}

VirtualFile RegisteredCache::OpenFileOrDirectoryConcat(const VirtualDir& open_dir,
                                                       std::string_view path) const {
    const auto file = open_dir->GetFileRelative(path);
    if (file != nullptr) {
        return file;
    }

    const auto nca_dir = open_dir->GetDirectoryRelative(path);
    if (nca_dir == nullptr) {
        return nullptr;
    }

    const auto files = nca_dir->GetFiles();
    if (files.size() == 1 && files[0]->GetName() == "00") {
        return files[0];
    }

    std::vector<VirtualFile> concat;
    // Since the files are a two-digit hex number, max is FF.
    for (std::size_t i = 0; i < 0x100; ++i) {
        auto next = nca_dir->GetFile(fmt::format("{:02X}", i));
        if (next != nullptr) {
            concat.push_back(std::move(next));
        } else {
            next = nca_dir->GetFile(fmt::format("{:02x}", i));
            if (next != nullptr) {
                concat.push_back(std::move(next));
            } else {
                break;
            }
        }
    }

    if (concat.empty()) {
        return nullptr;
    }

    return ConcatenatedVfsFile::MakeConcatenatedFile(concat, concat.front()->GetName());
}

VirtualFile RegisteredCache::GetFileAtID(NcaID id) const {
    VirtualFile file;
    // Try all five relevant modes of file storage:
    // (bit 2 = uppercase/lower, bit 1 = within a two-digit dir, bit 0 = .cnmt suffix)
    // 000: /000000**/{:032X}.nca
    // 010: /{:032X}.nca
    // 100: /000000**/{:032x}.nca
    // 110: /{:032x}.nca
    // 111: /{:032x}.cnmt.nca
    for (u8 i = 0; i < 8; ++i) {
        if ((i % 2) == 1 && i != 7)
            continue;
        const auto path =
            GetRelativePathFromNcaID(id, (i & 0b100) == 0, (i & 0b010) == 0, (i & 0b001) == 0b001);
        file = OpenFileOrDirectoryConcat(dir, path);
        if (file != nullptr)
            return file;
    }
    return file;
}

static std::optional<NcaID> CheckMapForContentRecord(const std::map<u64, CNMT>& map, u64 title_id,
                                                     ContentRecordType type) {
    const auto cmnt_iter = map.find(title_id);
    if (cmnt_iter == map.cend()) {
        return std::nullopt;
    }

    const auto& cnmt = cmnt_iter->second;
    const auto& content_records = cnmt.GetContentRecords();
    const auto iter = std::find_if(content_records.cbegin(), content_records.cend(),
                                   [type](const ContentRecord& rec) { return rec.type == type; });
    if (iter == content_records.cend()) {
        return std::nullopt;
    }

    return std::make_optional(iter->nca_id);
}

std::optional<NcaID> RegisteredCache::GetNcaIDFromMetadata(u64 title_id,
                                                           ContentRecordType type) const {
    if (type == ContentRecordType::Meta && meta_id.find(title_id) != meta_id.end())
        return meta_id.at(title_id);

    const auto res1 = CheckMapForContentRecord(yuzu_meta, title_id, type);
    if (res1)
        return res1;
    return CheckMapForContentRecord(meta, title_id, type);
}

std::vector<NcaID> RegisteredCache::AccumulateFiles() const {
    std::vector<NcaID> ids;
    for (const auto& d2_dir : dir->GetSubdirectories()) {
        if (FollowsNcaIdFormat(d2_dir->GetName())) {
            ids.push_back(Common::HexStringToArray<0x10, true>(d2_dir->GetName().substr(0, 0x20)));
            continue;
        }

        if (!FollowsTwoDigitDirFormat(d2_dir->GetName()))
            continue;

        for (const auto& nca_dir : d2_dir->GetSubdirectories()) {
            if (nca_dir == nullptr || !FollowsNcaIdFormat(nca_dir->GetName())) {
                continue;
            }

            ids.push_back(Common::HexStringToArray<0x10, true>(nca_dir->GetName().substr(0, 0x20)));
        }

        for (const auto& nca_file : d2_dir->GetFiles()) {
            if (nca_file == nullptr || !FollowsNcaIdFormat(nca_file->GetName())) {
                continue;
            }

            ids.push_back(
                Common::HexStringToArray<0x10, true>(nca_file->GetName().substr(0, 0x20)));
        }
    }

    for (const auto& d2_file : dir->GetFiles()) {
        if (FollowsNcaIdFormat(d2_file->GetName()))
            ids.push_back(Common::HexStringToArray<0x10, true>(d2_file->GetName().substr(0, 0x20)));
    }
    return ids;
}

void RegisteredCache::ProcessFiles(const std::vector<NcaID>& ids) {
    for (const auto& id : ids) {
        const auto file = GetFileAtID(id);

        if (file == nullptr)
            continue;
        const auto nca = std::make_shared<NCA>(parser(file, id), nullptr, 0);
        if (nca->GetStatus() != Loader::ResultStatus::Success ||
            nca->GetType() != NCAContentType::Meta) {
            continue;
        }

        const auto section0 = nca->GetSubdirectories()[0];

        for (const auto& section0_file : section0->GetFiles()) {
            if (section0_file->GetExtension() != "cnmt")
                continue;

            meta.insert_or_assign(nca->GetTitleId(), CNMT(section0_file));
            meta_id.insert_or_assign(nca->GetTitleId(), id);
            break;
        }
    }
}

void RegisteredCache::AccumulateYuzuMeta() {
    const auto meta_dir = dir->GetSubdirectory("yuzu_meta");
    if (meta_dir == nullptr) {
        return;
    }

    for (const auto& file : meta_dir->GetFiles()) {
        if (file->GetExtension() != "cnmt") {
            continue;
        }

        CNMT cnmt(file);
        yuzu_meta.insert_or_assign(cnmt.GetTitleID(), std::move(cnmt));
    }
}

void RegisteredCache::Refresh() {
    if (dir == nullptr) {
        return;
    }

    const auto ids = AccumulateFiles();
    ProcessFiles(ids);
    AccumulateYuzuMeta();
}

RegisteredCache::RegisteredCache(VirtualDir dir_, ContentProviderParsingFunction parsing_function)
    : dir(std::move(dir_)), parser(std::move(parsing_function)) {
    Refresh();
}

RegisteredCache::~RegisteredCache() = default;

bool RegisteredCache::HasEntry(u64 title_id, ContentRecordType type) const {
    return GetEntryRaw(title_id, type) != nullptr;
}

VirtualFile RegisteredCache::GetEntryUnparsed(u64 title_id, ContentRecordType type) const {
    const auto id = GetNcaIDFromMetadata(title_id, type);
    return id ? GetFileAtID(*id) : nullptr;
}

std::optional<u32> RegisteredCache::GetEntryVersion(u64 title_id) const {
    const auto meta_iter = meta.find(title_id);
    if (meta_iter != meta.cend()) {
        return meta_iter->second.GetTitleVersion();
    }

    const auto yuzu_meta_iter = yuzu_meta.find(title_id);
    if (yuzu_meta_iter != yuzu_meta.cend()) {
        return yuzu_meta_iter->second.GetTitleVersion();
    }

    return std::nullopt;
}

VirtualFile RegisteredCache::GetEntryRaw(u64 title_id, ContentRecordType type) const {
    const auto id = GetNcaIDFromMetadata(title_id, type);
    return id ? parser(GetFileAtID(*id), *id) : nullptr;
}

std::unique_ptr<NCA> RegisteredCache::GetEntry(u64 title_id, ContentRecordType type) const {
    const auto raw = GetEntryRaw(title_id, type);
    if (raw == nullptr)
        return nullptr;
    return std::make_unique<NCA>(raw, nullptr, 0);
}

template <typename T>
void RegisteredCache::IterateAllMetadata(
    std::vector<T>& out, std::function<T(const CNMT&, const ContentRecord&)> proc,
    std::function<bool(const CNMT&, const ContentRecord&)> filter) const {
    for (const auto& kv : meta) {
        const auto& cnmt = kv.second;
        if (filter(cnmt, EMPTY_META_CONTENT_RECORD))
            out.push_back(proc(cnmt, EMPTY_META_CONTENT_RECORD));
        for (const auto& rec : cnmt.GetContentRecords()) {
            if (GetFileAtID(rec.nca_id) != nullptr && filter(cnmt, rec)) {
                out.push_back(proc(cnmt, rec));
            }
        }
    }
    for (const auto& kv : yuzu_meta) {
        const auto& cnmt = kv.second;
        for (const auto& rec : cnmt.GetContentRecords()) {
            if (GetFileAtID(rec.nca_id) != nullptr && filter(cnmt, rec)) {
                out.push_back(proc(cnmt, rec));
            }
        }
    }
}

std::vector<ContentProviderEntry> RegisteredCache::ListEntriesFilter(
    std::optional<TitleType> title_type, std::optional<ContentRecordType> record_type,
    std::optional<u64> title_id) const {
    std::vector<ContentProviderEntry> out;
    IterateAllMetadata<ContentProviderEntry>(
        out,
        [](const CNMT& c, const ContentRecord& r) {
            return ContentProviderEntry{c.GetTitleID(), r.type};
        },
        [&title_type, &record_type, &title_id](const CNMT& c, const ContentRecord& r) {
            if (title_type && *title_type != c.GetType())
                return false;
            if (record_type && *record_type != r.type)
                return false;
            if (title_id && *title_id != c.GetTitleID())
                return false;
            return true;
        });
    return out;
}

static std::shared_ptr<NCA> GetNCAFromNSPForID(const NSP& nsp, const NcaID& id) {
    auto file = nsp.GetFile(fmt::format("{}.nca", Common::HexToString(id, false)));
    if (file == nullptr) {
        return nullptr;
    }
    return std::make_shared<NCA>(std::move(file));
}

InstallResult RegisteredCache::InstallEntry(const XCI& xci, bool overwrite_if_exists,
                                            const VfsCopyFunction& copy) {
    return InstallEntry(*xci.GetSecurePartitionNSP(), overwrite_if_exists, copy);
}

InstallResult RegisteredCache::InstallEntry(const NSP& nsp, bool overwrite_if_exists,
                                            const VfsCopyFunction& copy) {
    const auto ncas = nsp.GetNCAsCollapsed();
    const auto meta_iter = std::find_if(ncas.begin(), ncas.end(), [](const auto& nca) {
        return nca->GetType() == NCAContentType::Meta;
    });

    if (meta_iter == ncas.end()) {
        LOG_ERROR(Loader, "The file you are attempting to install does not have a metadata NCA and "
                          "is therefore malformed. Check your encryption keys.");
        return InstallResult::ErrorMetaFailed;
    }

    const auto meta_id_raw = (*meta_iter)->GetName().substr(0, 32);
    const auto meta_id_data = Common::HexStringToArray<16>(meta_id_raw);

    if ((*meta_iter)->GetSubdirectories().empty()) {
        LOG_ERROR(Loader,
                  "The file you are attempting to install does not contain a section0 within the "
                  "metadata NCA and is therefore malformed. Verify that the file is valid.");
        return InstallResult::ErrorMetaFailed;
    }

    const auto section0 = (*meta_iter)->GetSubdirectories()[0];

    if (section0->GetFiles().empty()) {
        LOG_ERROR(Loader,
                  "The file you are attempting to install does not contain a CNMT within the "
                  "metadata NCA and is therefore malformed. Verify that the file is valid.");
        return InstallResult::ErrorMetaFailed;
    }

    const auto cnmt_file = section0->GetFiles()[0];
    const CNMT cnmt(cnmt_file);

    const auto title_id = cnmt.GetTitleID();
    const auto version = cnmt.GetTitleVersion();

    if (title_id == GetBaseTitleID(title_id) && version == 0) {
        return InstallResult::ErrorBaseInstall;
    }

    const auto result = RemoveExistingEntry(title_id);

    // Install Metadata File
    const auto res = RawInstallNCA(**meta_iter, copy, overwrite_if_exists, meta_id_data);
    if (res != InstallResult::Success) {
        return res;
    }

    // Install all the other NCAs
    for (const auto& record : cnmt.GetContentRecords()) {
        // Ignore DeltaFragments, they are not useful to us
        if (record.type == ContentRecordType::DeltaFragment) {
            continue;
        }
        const auto nca = GetNCAFromNSPForID(nsp, record.nca_id);
        if (nca == nullptr) {
            return InstallResult::ErrorCopyFailed;
        }
        const auto res2 = RawInstallNCA(*nca, copy, overwrite_if_exists, record.nca_id);
        if (res2 != InstallResult::Success) {
            return res2;
        }
    }

    Refresh();
    if (result) {
        return InstallResult::OverwriteExisting;
    }
    return InstallResult::Success;
}

InstallResult RegisteredCache::InstallEntry(const NCA& nca, TitleType type,
                                            bool overwrite_if_exists, const VfsCopyFunction& copy) {
    const CNMTHeader header{
        .title_id = nca.GetTitleId(),
        .title_version = 0,
        .type = type,
        .reserved = {},
        .table_offset = 0x10,
        .number_content_entries = 1,
        .number_meta_entries = 0,
        .attributes = 0,
        .reserved2 = {},
        .is_committed = 0,
        .required_download_system_version = 0,
        .reserved3 = {},
    };
    const OptionalHeader opt_header{0, 0};
    ContentRecord c_rec{{}, {}, {}, GetCRTypeFromNCAType(nca.GetType()), {}};
    const auto& data = nca.GetBaseFile()->ReadBytes(0x100000);
    mbedtls_sha256_ret(data.data(), data.size(), c_rec.hash.data(), 0);
    std::memcpy(&c_rec.nca_id, &c_rec.hash, 16);
    const CNMT new_cnmt(header, opt_header, {c_rec}, {});
    if (!RawInstallYuzuMeta(new_cnmt)) {
        return InstallResult::ErrorMetaFailed;
    }
    return RawInstallNCA(nca, copy, overwrite_if_exists, c_rec.nca_id);
}

bool RegisteredCache::RemoveExistingEntry(u64 title_id) const {
    const auto delete_nca = [this](const NcaID& id) {
        const auto path = GetRelativePathFromNcaID(id, false, true, false);

        const bool isFile = dir->GetFileRelative(path) != nullptr;
        const bool isDir = dir->GetDirectoryRelative(path) != nullptr;

        if (isFile) {
            return dir->DeleteFile(path);
        } else if (isDir) {
            return dir->DeleteSubdirectoryRecursive(path);
        }

        return false;
    };

    // If an entry exists in the registered cache, remove it
    if (HasEntry(title_id, ContentRecordType::Meta)) {
        LOG_INFO(Loader,
                 "Previously installed entry (v{}) for title_id={:016X} detected! "
                 "Attempting to remove...",
                 GetEntryVersion(title_id).value_or(0), title_id);

        // Get all the ncas associated with the current CNMT and delete them
        const auto meta_old_id =
            GetNcaIDFromMetadata(title_id, ContentRecordType::Meta).value_or(NcaID{});
        const auto program_id =
            GetNcaIDFromMetadata(title_id, ContentRecordType::Program).value_or(NcaID{});
        const auto data_id =
            GetNcaIDFromMetadata(title_id, ContentRecordType::Data).value_or(NcaID{});
        const auto control_id =
            GetNcaIDFromMetadata(title_id, ContentRecordType::Control).value_or(NcaID{});
        const auto html_id =
            GetNcaIDFromMetadata(title_id, ContentRecordType::HtmlDocument).value_or(NcaID{});
        const auto legal_id =
            GetNcaIDFromMetadata(title_id, ContentRecordType::LegalInformation).value_or(NcaID{});

        const auto deleted_meta = delete_nca(meta_old_id);
        const auto deleted_program = delete_nca(program_id);
        const auto deleted_data = delete_nca(data_id);
        const auto deleted_control = delete_nca(control_id);
        const auto deleted_html = delete_nca(html_id);
        const auto deleted_legal = delete_nca(legal_id);

        return deleted_meta && (deleted_meta || deleted_program || deleted_data ||
                                deleted_control || deleted_html || deleted_legal);
    }

    return false;
}

InstallResult RegisteredCache::RawInstallNCA(const NCA& nca, const VfsCopyFunction& copy,
                                             bool overwrite_if_exists,
                                             std::optional<NcaID> override_id) {
    const auto in = nca.GetBaseFile();
    Core::Crypto::SHA256Hash hash{};

    // Calculate NcaID
    // NOTE: Because computing the SHA256 of an entire NCA is quite expensive (especially if the
    // game is massive), we're going to cheat and only hash the first MB of the NCA.
    // Also, for XCIs the NcaID matters, so if the override id isn't none, use that.
    NcaID id{};
    if (override_id) {
        id = *override_id;
    } else {
        const auto& data = in->ReadBytes(0x100000);
        mbedtls_sha256_ret(data.data(), data.size(), hash.data(), 0);
        memcpy(id.data(), hash.data(), 16);
    }

    std::string path = GetRelativePathFromNcaID(id, false, true, false);

    if (GetFileAtID(id) != nullptr && !overwrite_if_exists) {
        LOG_WARNING(Loader, "Attempting to overwrite existing NCA. Skipping...");
        return InstallResult::ErrorAlreadyExists;
    }

    if (GetFileAtID(id) != nullptr) {
        LOG_WARNING(Loader, "Overwriting existing NCA...");
        VirtualDir c_dir;
        { c_dir = dir->GetFileRelative(path)->GetContainingDirectory(); }
        c_dir->DeleteFile(Common::FS::GetFilename(path));
    }

    auto out = dir->CreateFileRelative(path);
    if (out == nullptr) {
        return InstallResult::ErrorCopyFailed;
    }
    return copy(in, out, VFS_RC_LARGE_COPY_BLOCK) ? InstallResult::Success
                                                  : InstallResult::ErrorCopyFailed;
}

bool RegisteredCache::RawInstallYuzuMeta(const CNMT& cnmt) {
    // Reasoning behind this method can be found in the comment for InstallEntry, NCA overload.
    const auto meta_dir = dir->CreateDirectoryRelative("yuzu_meta");
    const auto filename = GetCNMTName(cnmt.GetType(), cnmt.GetTitleID());
    if (meta_dir->GetFile(filename) == nullptr) {
        auto out = meta_dir->CreateFile(filename);
        const auto buffer = cnmt.Serialize();
        out->Resize(buffer.size());
        out->WriteBytes(buffer);
    } else {
        auto out = meta_dir->GetFile(filename);
        CNMT old_cnmt(out);
        // Returns true on change
        if (old_cnmt.UnionRecords(cnmt)) {
            out->Resize(0);
            const auto buffer = old_cnmt.Serialize();
            out->Resize(buffer.size());
            out->WriteBytes(buffer);
        }
    }
    Refresh();
    return std::find_if(yuzu_meta.begin(), yuzu_meta.end(),
                        [&cnmt](const std::pair<u64, CNMT>& kv) {
                            return kv.second.GetType() == cnmt.GetType() &&
                                   kv.second.GetTitleID() == cnmt.GetTitleID();
                        }) != yuzu_meta.end();
}

ContentProviderUnion::~ContentProviderUnion() = default;

void ContentProviderUnion::SetSlot(ContentProviderUnionSlot slot, ContentProvider* provider) {
    providers[slot] = provider;
}

void ContentProviderUnion::ClearSlot(ContentProviderUnionSlot slot) {
    providers[slot] = nullptr;
}

void ContentProviderUnion::Refresh() {
    for (auto& provider : providers) {
        if (provider.second == nullptr)
            continue;

        provider.second->Refresh();
    }
}

bool ContentProviderUnion::HasEntry(u64 title_id, ContentRecordType type) const {
    for (const auto& provider : providers) {
        if (provider.second == nullptr)
            continue;

        if (provider.second->HasEntry(title_id, type))
            return true;
    }

    return false;
}

std::optional<u32> ContentProviderUnion::GetEntryVersion(u64 title_id) const {
    for (const auto& provider : providers) {
        if (provider.second == nullptr)
            continue;

        const auto res = provider.second->GetEntryVersion(title_id);
        if (res != std::nullopt)
            return res;
    }

    return std::nullopt;
}

VirtualFile ContentProviderUnion::GetEntryUnparsed(u64 title_id, ContentRecordType type) const {
    for (const auto& provider : providers) {
        if (provider.second == nullptr)
            continue;

        const auto res = provider.second->GetEntryUnparsed(title_id, type);
        if (res != nullptr)
            return res;
    }

    return nullptr;
}

VirtualFile ContentProviderUnion::GetEntryRaw(u64 title_id, ContentRecordType type) const {
    for (const auto& provider : providers) {
        if (provider.second == nullptr)
            continue;

        const auto res = provider.second->GetEntryRaw(title_id, type);
        if (res != nullptr)
            return res;
    }

    return nullptr;
}

std::unique_ptr<NCA> ContentProviderUnion::GetEntry(u64 title_id, ContentRecordType type) const {
    for (const auto& provider : providers) {
        if (provider.second == nullptr)
            continue;

        auto res = provider.second->GetEntry(title_id, type);
        if (res != nullptr)
            return res;
    }

    return nullptr;
}

std::vector<ContentProviderEntry> ContentProviderUnion::ListEntriesFilter(
    std::optional<TitleType> title_type, std::optional<ContentRecordType> record_type,
    std::optional<u64> title_id) const {
    std::vector<ContentProviderEntry> out;

    for (const auto& provider : providers) {
        if (provider.second == nullptr)
            continue;

        const auto vec = provider.second->ListEntriesFilter(title_type, record_type, title_id);
        std::copy(vec.begin(), vec.end(), std::back_inserter(out));
    }

    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

std::vector<std::pair<ContentProviderUnionSlot, ContentProviderEntry>>
ContentProviderUnion::ListEntriesFilterOrigin(std::optional<ContentProviderUnionSlot> origin,
                                              std::optional<TitleType> title_type,
                                              std::optional<ContentRecordType> record_type,
                                              std::optional<u64> title_id) const {
    std::vector<std::pair<ContentProviderUnionSlot, ContentProviderEntry>> out;

    for (const auto& provider : providers) {
        if (provider.second == nullptr)
            continue;

        if (origin.has_value() && *origin != provider.first)
            continue;

        const auto vec = provider.second->ListEntriesFilter(title_type, record_type, title_id);
        std::transform(vec.begin(), vec.end(), std::back_inserter(out),
                       [&provider](const ContentProviderEntry& entry) {
                           return std::make_pair(provider.first, entry);
                       });
    }

    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

std::optional<ContentProviderUnionSlot> ContentProviderUnion::GetSlotForEntry(
    u64 title_id, ContentRecordType type) const {
    const auto iter =
        std::find_if(providers.begin(), providers.end(), [title_id, type](const auto& provider) {
            return provider.second != nullptr && provider.second->HasEntry(title_id, type);
        });

    if (iter == providers.end()) {
        return std::nullopt;
    }

    return iter->first;
}

ManualContentProvider::~ManualContentProvider() = default;

void ManualContentProvider::AddEntry(TitleType title_type, ContentRecordType content_type,
                                     u64 title_id, VirtualFile file) {
    entries.insert_or_assign({title_type, content_type, title_id}, file);
}

void ManualContentProvider::ClearAllEntries() {
    entries.clear();
}

void ManualContentProvider::Refresh() {}

bool ManualContentProvider::HasEntry(u64 title_id, ContentRecordType type) const {
    return GetEntryRaw(title_id, type) != nullptr;
}

std::optional<u32> ManualContentProvider::GetEntryVersion(u64 title_id) const {
    return std::nullopt;
}

VirtualFile ManualContentProvider::GetEntryUnparsed(u64 title_id, ContentRecordType type) const {
    return GetEntryRaw(title_id, type);
}

VirtualFile ManualContentProvider::GetEntryRaw(u64 title_id, ContentRecordType type) const {
    const auto iter =
        std::find_if(entries.begin(), entries.end(), [title_id, type](const auto& entry) {
            const auto content_type = std::get<1>(entry.first);
            const auto e_title_id = std::get<2>(entry.first);
            return content_type == type && e_title_id == title_id;
        });
    if (iter == entries.end())
        return nullptr;
    return iter->second;
}

std::unique_ptr<NCA> ManualContentProvider::GetEntry(u64 title_id, ContentRecordType type) const {
    const auto res = GetEntryRaw(title_id, type);
    if (res == nullptr)
        return nullptr;
    return std::make_unique<NCA>(res, nullptr, 0);
}

std::vector<ContentProviderEntry> ManualContentProvider::ListEntriesFilter(
    std::optional<TitleType> title_type, std::optional<ContentRecordType> record_type,
    std::optional<u64> title_id) const {
    std::vector<ContentProviderEntry> out;

    for (const auto& entry : entries) {
        const auto [e_title_type, e_content_type, e_title_id] = entry.first;
        if ((title_type == std::nullopt || e_title_type == *title_type) &&
            (record_type == std::nullopt || e_content_type == *record_type) &&
            (title_id == std::nullopt || e_title_id == *title_id)) {
            out.emplace_back(ContentProviderEntry{e_title_id, e_content_type});
        }
    }

    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}
} // namespace FileSys
