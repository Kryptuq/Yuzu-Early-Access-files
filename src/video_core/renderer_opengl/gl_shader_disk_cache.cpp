// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>

#include <fmt/format.h>

#include "common/assert.h"
#include "common/common_types.h"
#include "common/fs/file.h"
#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "common/logging/log.h"
#include "common/scm_rev.h"
#include "common/settings.h"
#include "common/zstd_compression.h"
#include "core/core.h"
#include "core/hle/kernel/k_process.h"
#include "video_core/engines/shader_type.h"
#include "video_core/renderer_opengl/gl_shader_cache.h"
#include "video_core/renderer_opengl/gl_shader_disk_cache.h"

namespace OpenGL {

using Tegra::Engines::ShaderType;
using VideoCommon::Shader::BindlessSamplerMap;
using VideoCommon::Shader::BoundSamplerMap;
using VideoCommon::Shader::KeyMap;
using VideoCommon::Shader::SeparateSamplerKey;
using ShaderCacheVersionHash = std::array<u8, 64>;

struct ConstBufferKey {
    u32 cbuf = 0;
    u32 offset = 0;
    u32 value = 0;
};

struct BoundSamplerEntry {
    u32 offset = 0;
    Tegra::Engines::SamplerDescriptor sampler;
};

struct SeparateSamplerEntry {
    u32 cbuf1 = 0;
    u32 cbuf2 = 0;
    u32 offset1 = 0;
    u32 offset2 = 0;
    Tegra::Engines::SamplerDescriptor sampler;
};

struct BindlessSamplerEntry {
    u32 cbuf = 0;
    u32 offset = 0;
    Tegra::Engines::SamplerDescriptor sampler;
};

namespace {

constexpr u32 NativeVersion = 21;

ShaderCacheVersionHash GetShaderCacheVersionHash() {
    ShaderCacheVersionHash hash{};
    const std::size_t length = std::min(std::strlen(Common::g_shader_cache_version), hash.size());
    std::memcpy(hash.data(), Common::g_shader_cache_version, length);
    return hash;
}

} // Anonymous namespace

ShaderDiskCacheEntry::ShaderDiskCacheEntry() = default;

ShaderDiskCacheEntry::~ShaderDiskCacheEntry() = default;

bool ShaderDiskCacheEntry::Load(Common::FS::IOFile& file) {
    if (!file.ReadObject(type)) {
        return false;
    }
    u32 code_size;
    u32 code_size_b;
    if (!file.ReadObject(code_size) || !file.ReadObject(code_size_b)) {
        return false;
    }
    code.resize(code_size);
    code_b.resize(code_size_b);
    if (file.Read(code) != code_size) {
        return false;
    }
    if (HasProgramA() && file.Read(code_b) != code_size_b) {
        return false;
    }

    u8 is_texture_handler_size_known;
    u32 texture_handler_size_value;
    u32 num_keys;
    u32 num_bound_samplers;
    u32 num_separate_samplers;
    u32 num_bindless_samplers;
    if (!file.ReadObject(unique_identifier) || !file.ReadObject(bound_buffer) ||
        !file.ReadObject(is_texture_handler_size_known) ||
        !file.ReadObject(texture_handler_size_value) || !file.ReadObject(graphics_info) ||
        !file.ReadObject(compute_info) || !file.ReadObject(num_keys) ||
        !file.ReadObject(num_bound_samplers) || !file.ReadObject(num_separate_samplers) ||
        !file.ReadObject(num_bindless_samplers)) {
        return false;
    }
    if (is_texture_handler_size_known) {
        texture_handler_size = texture_handler_size_value;
    }

    std::vector<ConstBufferKey> flat_keys(num_keys);
    std::vector<BoundSamplerEntry> flat_bound_samplers(num_bound_samplers);
    std::vector<SeparateSamplerEntry> flat_separate_samplers(num_separate_samplers);
    std::vector<BindlessSamplerEntry> flat_bindless_samplers(num_bindless_samplers);
    if (file.Read(flat_keys) != flat_keys.size() ||
        file.Read(flat_bound_samplers) != flat_bound_samplers.size() ||
        file.Read(flat_separate_samplers) != flat_separate_samplers.size() ||
        file.Read(flat_bindless_samplers) != flat_bindless_samplers.size()) {
        return false;
    }
    for (const auto& entry : flat_keys) {
        keys.insert({{entry.cbuf, entry.offset}, entry.value});
    }
    for (const auto& entry : flat_bound_samplers) {
        bound_samplers.emplace(entry.offset, entry.sampler);
    }
    for (const auto& entry : flat_separate_samplers) {
        SeparateSamplerKey key;
        key.buffers = {entry.cbuf1, entry.cbuf2};
        key.offsets = {entry.offset1, entry.offset2};
        separate_samplers.emplace(key, entry.sampler);
    }
    for (const auto& entry : flat_bindless_samplers) {
        bindless_samplers.insert({{entry.cbuf, entry.offset}, entry.sampler});
    }

    return true;
}

bool ShaderDiskCacheEntry::Save(Common::FS::IOFile& file) const {
    if (!file.WriteObject(static_cast<u32>(type)) ||
        !file.WriteObject(static_cast<u32>(code.size())) ||
        !file.WriteObject(static_cast<u32>(code_b.size()))) {
        return false;
    }
    if (file.Write(code) != code.size()) {
        return false;
    }
    if (HasProgramA() && file.Write(code_b) != code_b.size()) {
        return false;
    }

    if (!file.WriteObject(unique_identifier) || !file.WriteObject(bound_buffer) ||
        !file.WriteObject(static_cast<u8>(texture_handler_size.has_value())) ||
        !file.WriteObject(texture_handler_size.value_or(0)) || !file.WriteObject(graphics_info) ||
        !file.WriteObject(compute_info) || !file.WriteObject(static_cast<u32>(keys.size())) ||
        !file.WriteObject(static_cast<u32>(bound_samplers.size())) ||
        !file.WriteObject(static_cast<u32>(separate_samplers.size())) ||
        !file.WriteObject(static_cast<u32>(bindless_samplers.size()))) {
        return false;
    }

    std::vector<ConstBufferKey> flat_keys;
    flat_keys.reserve(keys.size());
    for (const auto& [address, value] : keys) {
        flat_keys.push_back(ConstBufferKey{address.first, address.second, value});
    }

    std::vector<BoundSamplerEntry> flat_bound_samplers;
    flat_bound_samplers.reserve(bound_samplers.size());
    for (const auto& [address, sampler] : bound_samplers) {
        flat_bound_samplers.push_back(BoundSamplerEntry{address, sampler});
    }

    std::vector<SeparateSamplerEntry> flat_separate_samplers;
    flat_separate_samplers.reserve(separate_samplers.size());
    for (const auto& [key, sampler] : separate_samplers) {
        SeparateSamplerEntry entry;
        std::tie(entry.cbuf1, entry.cbuf2) = key.buffers;
        std::tie(entry.offset1, entry.offset2) = key.offsets;
        entry.sampler = sampler;
        flat_separate_samplers.push_back(entry);
    }

    std::vector<BindlessSamplerEntry> flat_bindless_samplers;
    flat_bindless_samplers.reserve(bindless_samplers.size());
    for (const auto& [address, sampler] : bindless_samplers) {
        flat_bindless_samplers.push_back(
            BindlessSamplerEntry{address.first, address.second, sampler});
    }

    return file.Write(flat_keys) == flat_keys.size() &&
           file.Write(flat_bound_samplers) == flat_bound_samplers.size() &&
           file.Write(flat_separate_samplers) == flat_separate_samplers.size() &&
           file.Write(flat_bindless_samplers) == flat_bindless_samplers.size();
}

ShaderDiskCacheOpenGL::ShaderDiskCacheOpenGL() = default;

ShaderDiskCacheOpenGL::~ShaderDiskCacheOpenGL() = default;

void ShaderDiskCacheOpenGL::BindTitleID(u64 title_id_) {
    title_id = title_id_;
}

std::optional<std::vector<ShaderDiskCacheEntry>> ShaderDiskCacheOpenGL::LoadTransferable() {
    // Skip games without title id
    const bool has_title_id = title_id != 0;
    if (!Settings::values.use_disk_shader_cache.GetValue() || !has_title_id) {
        return std::nullopt;
    }

    Common::FS::IOFile file{GetTransferablePath(), Common::FS::FileAccessMode::Read,
                            Common::FS::FileType::BinaryFile};
    if (!file.IsOpen()) {
        LOG_INFO(Render_OpenGL, "No transferable shader cache found");
        is_usable = true;
        return std::nullopt;
    }

    u32 version{};
    if (!file.ReadObject(version)) {
        LOG_ERROR(Render_OpenGL, "Failed to get transferable cache version, skipping it");
        return std::nullopt;
    }

    if (version < NativeVersion) {
        LOG_INFO(Render_OpenGL, "Transferable shader cache is old, removing");
        file.Close();
        InvalidateTransferable();
        is_usable = true;
        return std::nullopt;
    }
    if (version > NativeVersion) {
        LOG_WARNING(Render_OpenGL, "Transferable shader cache was generated with a newer version "
                                   "of the emulator, skipping");
        return std::nullopt;
    }

    // Version is valid, load the shaders
    std::vector<ShaderDiskCacheEntry> entries;
    while (static_cast<u64>(file.Tell()) < file.GetSize()) {
        ShaderDiskCacheEntry& entry = entries.emplace_back();
        if (!entry.Load(file)) {
            LOG_ERROR(Render_OpenGL, "Failed to load transferable raw entry, skipping");
            return std::nullopt;
        }
    }

    is_usable = true;
    return {std::move(entries)};
}

std::vector<ShaderDiskCachePrecompiled> ShaderDiskCacheOpenGL::LoadPrecompiled() {
    if (!is_usable) {
        return {};
    }

    Common::FS::IOFile file{GetPrecompiledPath(), Common::FS::FileAccessMode::Read,
                            Common::FS::FileType::BinaryFile};
    if (!file.IsOpen()) {
        LOG_INFO(Render_OpenGL, "No precompiled shader cache found");
        return {};
    }

    if (const auto result = LoadPrecompiledFile(file)) {
        return *result;
    }

    LOG_INFO(Render_OpenGL, "Failed to load precompiled cache");
    file.Close();
    InvalidatePrecompiled();
    return {};
}

std::optional<std::vector<ShaderDiskCachePrecompiled>> ShaderDiskCacheOpenGL::LoadPrecompiledFile(
    Common::FS::IOFile& file) {
    // Read compressed file from disk and decompress to virtual precompiled cache file
    std::vector<u8> compressed(file.GetSize());
    if (file.Read(compressed) != file.GetSize()) {
        return std::nullopt;
    }
    const std::vector<u8> decompressed = Common::Compression::DecompressDataZSTD(compressed);
    SaveArrayToPrecompiled(decompressed.data(), decompressed.size());
    precompiled_cache_virtual_file_offset = 0;

    ShaderCacheVersionHash file_hash{};
    if (!LoadArrayFromPrecompiled(file_hash.data(), file_hash.size())) {
        precompiled_cache_virtual_file_offset = 0;
        return std::nullopt;
    }
    if (GetShaderCacheVersionHash() != file_hash) {
        LOG_INFO(Render_OpenGL, "Precompiled cache is from another version of the emulator");
        precompiled_cache_virtual_file_offset = 0;
        return std::nullopt;
    }

    std::vector<ShaderDiskCachePrecompiled> entries;
    while (precompiled_cache_virtual_file_offset < precompiled_cache_virtual_file.GetSize()) {
        u32 binary_size;
        auto& entry = entries.emplace_back();
        if (!LoadObjectFromPrecompiled(entry.unique_identifier) ||
            !LoadObjectFromPrecompiled(entry.binary_format) ||
            !LoadObjectFromPrecompiled(binary_size)) {
            return std::nullopt;
        }

        entry.binary.resize(binary_size);
        if (!LoadArrayFromPrecompiled(entry.binary.data(), entry.binary.size())) {
            return std::nullopt;
        }
    }
    return entries;
}

void ShaderDiskCacheOpenGL::InvalidateTransferable() {
    if (!Common::FS::RemoveFile(GetTransferablePath())) {
        LOG_ERROR(Render_OpenGL, "Failed to invalidate transferable file={}",
                  Common::FS::PathToUTF8String(GetTransferablePath()));
    }
    InvalidatePrecompiled();
}

void ShaderDiskCacheOpenGL::InvalidatePrecompiled() {
    // Clear virtaul precompiled cache file
    precompiled_cache_virtual_file.Resize(0);

    if (!Common::FS::RemoveFile(GetPrecompiledPath())) {
        LOG_ERROR(Render_OpenGL, "Failed to invalidate precompiled file={}",
                  Common::FS::PathToUTF8String(GetPrecompiledPath()));
    }
}

void ShaderDiskCacheOpenGL::SaveEntry(const ShaderDiskCacheEntry& entry) {
    if (!is_usable) {
        return;
    }

    const u64 id = entry.unique_identifier;
    if (stored_transferable.contains(id)) {
        // The shader already exists
        return;
    }

    Common::FS::IOFile file = AppendTransferableFile();
    if (!file.IsOpen()) {
        return;
    }
    if (!entry.Save(file)) {
        LOG_ERROR(Render_OpenGL, "Failed to save raw transferable cache entry, removing");
        file.Close();
        InvalidateTransferable();
        return;
    }

    stored_transferable.insert(id);
}

void ShaderDiskCacheOpenGL::SavePrecompiled(u64 unique_identifier, GLuint program) {
    if (!is_usable) {
        return;
    }

    // TODO(Rodrigo): This is a design smell. I shouldn't be having to manually write the header
    // when writing the dump. This should be done the moment I get access to write to the virtual
    // file.
    if (precompiled_cache_virtual_file.GetSize() == 0) {
        SavePrecompiledHeaderToVirtualPrecompiledCache();
    }

    GLint binary_length;
    glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &binary_length);

    GLenum binary_format;
    std::vector<u8> binary(binary_length);
    glGetProgramBinary(program, binary_length, nullptr, &binary_format, binary.data());

    if (!SaveObjectToPrecompiled(unique_identifier) || !SaveObjectToPrecompiled(binary_format) ||
        !SaveObjectToPrecompiled(static_cast<u32>(binary.size())) ||
        !SaveArrayToPrecompiled(binary.data(), binary.size())) {
        LOG_ERROR(Render_OpenGL, "Failed to save binary program file in shader={:016X}, removing",
                  unique_identifier);
        InvalidatePrecompiled();
    }
}

Common::FS::IOFile ShaderDiskCacheOpenGL::AppendTransferableFile() const {
    if (!EnsureDirectories()) {
        return {};
    }

    const auto transferable_path{GetTransferablePath()};
    const bool existed = Common::FS::Exists(transferable_path);

    Common::FS::IOFile file{transferable_path, Common::FS::FileAccessMode::Append,
                            Common::FS::FileType::BinaryFile};
    if (!file.IsOpen()) {
        LOG_ERROR(Render_OpenGL, "Failed to open transferable cache in path={}",
                  Common::FS::PathToUTF8String(transferable_path));
        return {};
    }
    if (!existed || file.GetSize() == 0) {
        // If the file didn't exist, write its version
        if (!file.WriteObject(NativeVersion)) {
            LOG_ERROR(Render_OpenGL, "Failed to write transferable cache version in path={}",
                      Common::FS::PathToUTF8String(transferable_path));
            return {};
        }
    }
    return file;
}

void ShaderDiskCacheOpenGL::SavePrecompiledHeaderToVirtualPrecompiledCache() {
    const auto hash{GetShaderCacheVersionHash()};
    if (!SaveArrayToPrecompiled(hash.data(), hash.size())) {
        LOG_ERROR(
            Render_OpenGL,
            "Failed to write precompiled cache version hash to virtual precompiled cache file");
    }
}

void ShaderDiskCacheOpenGL::SaveVirtualPrecompiledFile() {
    precompiled_cache_virtual_file_offset = 0;
    const std::vector<u8> uncompressed = precompiled_cache_virtual_file.ReadAllBytes();
    const std::vector<u8> compressed =
        Common::Compression::CompressDataZSTDDefault(uncompressed.data(), uncompressed.size());

    const auto precompiled_path = GetPrecompiledPath();
    Common::FS::IOFile file{precompiled_path, Common::FS::FileAccessMode::Write,
                            Common::FS::FileType::BinaryFile};

    if (!file.IsOpen()) {
        LOG_ERROR(Render_OpenGL, "Failed to open precompiled cache in path={}",
                  Common::FS::PathToUTF8String(precompiled_path));
        return;
    }
    if (file.Write(compressed) != compressed.size()) {
        LOG_ERROR(Render_OpenGL, "Failed to write precompiled cache version in path={}",
                  Common::FS::PathToUTF8String(precompiled_path));
    }
}

bool ShaderDiskCacheOpenGL::EnsureDirectories() const {
    const auto CreateDir = [](const std::filesystem::path& dir) {
        if (!Common::FS::CreateDir(dir)) {
            LOG_ERROR(Render_OpenGL, "Failed to create directory={}",
                      Common::FS::PathToUTF8String(dir));
            return false;
        }
        return true;
    };

    return CreateDir(Common::FS::GetYuzuPath(Common::FS::YuzuPath::ShaderDir)) &&
           CreateDir(GetBaseDir()) && CreateDir(GetTransferableDir()) &&
           CreateDir(GetPrecompiledDir());
}

std::filesystem::path ShaderDiskCacheOpenGL::GetTransferablePath() const {
    return GetTransferableDir() / fmt::format("{}.bin", GetTitleID());
}

std::filesystem::path ShaderDiskCacheOpenGL::GetPrecompiledPath() const {
    return GetPrecompiledDir() / fmt::format("{}.bin", GetTitleID());
}

std::filesystem::path ShaderDiskCacheOpenGL::GetTransferableDir() const {
    return GetBaseDir() / "transferable";
}

std::filesystem::path ShaderDiskCacheOpenGL::GetPrecompiledDir() const {
    return GetBaseDir() / "precompiled";
}

std::filesystem::path ShaderDiskCacheOpenGL::GetBaseDir() const {
    return Common::FS::GetYuzuPath(Common::FS::YuzuPath::ShaderDir) / "opengl";
}

std::string ShaderDiskCacheOpenGL::GetTitleID() const {
    return fmt::format("{:016X}", title_id);
}

} // namespace OpenGL
