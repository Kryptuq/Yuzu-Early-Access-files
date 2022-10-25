// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/settings.h"
#include "common/string_util.h"
#include "common/swap.h"
#include "core/file_sys/control_metadata.h"
#include "core/file_sys/vfs.h"

namespace FileSys {

const std::array<const char*, 16> LANGUAGE_NAMES{{
    "AmericanEnglish",
    "BritishEnglish",
    "Japanese",
    "French",
    "German",
    "LatinAmericanSpanish",
    "Spanish",
    "Italian",
    "Dutch",
    "CanadianFrench",
    "Portuguese",
    "Russian",
    "Korean",
    "Taiwanese",
    "Chinese",
    "BrazilianPortuguese",
}};

std::string LanguageEntry::GetApplicationName() const {
    return Common::StringFromFixedZeroTerminatedBuffer(application_name.data(),
                                                       application_name.size());
}

std::string LanguageEntry::GetDeveloperName() const {
    return Common::StringFromFixedZeroTerminatedBuffer(developer_name.data(),
                                                       developer_name.size());
}

constexpr std::array<Language, 18> language_to_codes = {{
    Language::Japanese,
    Language::AmericanEnglish,
    Language::French,
    Language::German,
    Language::Italian,
    Language::Spanish,
    Language::Chinese,
    Language::Korean,
    Language::Dutch,
    Language::Portuguese,
    Language::Russian,
    Language::Taiwanese,
    Language::BritishEnglish,
    Language::CanadianFrench,
    Language::LatinAmericanSpanish,
    Language::Chinese,
    Language::Taiwanese,
    Language::BrazilianPortuguese,
}};

NACP::NACP() = default;

NACP::NACP(VirtualFile file) {
    file->ReadObject(&raw);
}

NACP::~NACP() = default;

const LanguageEntry& NACP::GetLanguageEntry() const {
    Language language = language_to_codes[Settings::values.language_index.GetValue()];

    {
        const auto& language_entry = raw.language_entries.at(static_cast<u8>(language));
        if (!language_entry.GetApplicationName().empty())
            return language_entry;
    }

    for (const auto& language_entry : raw.language_entries) {
        if (!language_entry.GetApplicationName().empty())
            return language_entry;
    }

    return raw.language_entries.at(static_cast<u8>(Language::AmericanEnglish));
}

std::string NACP::GetApplicationName() const {
    return GetLanguageEntry().GetApplicationName();
}

std::string NACP::GetDeveloperName() const {
    return GetLanguageEntry().GetDeveloperName();
}

u64 NACP::GetTitleId() const {
    return raw.save_data_owner_id;
}

u64 NACP::GetDLCBaseTitleId() const {
    return raw.dlc_base_title_id;
}

std::string NACP::GetVersionString() const {
    return Common::StringFromFixedZeroTerminatedBuffer(raw.version_string.data(),
                                                       raw.version_string.size());
}

u64 NACP::GetDefaultNormalSaveSize() const {
    return raw.user_account_save_data_size;
}

u64 NACP::GetDefaultJournalSaveSize() const {
    return raw.user_account_save_data_journal_size;
}

bool NACP::GetUserAccountSwitchLock() const {
    return raw.user_account_switch_lock != 0;
}

u32 NACP::GetSupportedLanguages() const {
    return raw.supported_languages;
}

u64 NACP::GetDeviceSaveDataSize() const {
    return raw.device_save_data_size;
}

u32 NACP::GetParentalControlFlag() const {
    return raw.parental_control;
}

const std::array<u8, 0x20>& NACP::GetRatingAge() const {
    return raw.rating_age;
}

std::vector<u8> NACP::GetRawBytes() const {
    std::vector<u8> out(sizeof(RawNACP));
    std::memcpy(out.data(), &raw, sizeof(RawNACP));
    return out;
}
} // namespace FileSys
