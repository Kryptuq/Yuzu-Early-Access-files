// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstring>
#include <fmt/format.h>

#include "common/fs/file.h"
#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "common/logging/log.h"
#include "common/settings.h"
#include "input_common/drivers/virtual_amiibo.h"

namespace InputCommon {
constexpr PadIdentifier identifier = {
    .guid = Common::UUID{},
    .port = 0,
    .pad = 0,
};

VirtualAmiibo::VirtualAmiibo(std::string input_engine_) : InputEngine(std::move(input_engine_)) {}

VirtualAmiibo::~VirtualAmiibo() = default;

Common::Input::DriverResult VirtualAmiibo::SetPollingMode(
    [[maybe_unused]] const PadIdentifier& identifier_,
    const Common::Input::PollingMode polling_mode_) {
    polling_mode = polling_mode_;

    switch (polling_mode) {
    case Common::Input::PollingMode::NFC:
        if (state == State::Initialized) {
            state = State::WaitingForAmiibo;
        }
        return Common::Input::DriverResult::Success;
    default:
        if (state == State::AmiiboIsOpen) {
            CloseAmiibo();
        }
        return Common::Input::DriverResult::NotSupported;
    }
}

Common::Input::NfcState VirtualAmiibo::SupportsNfc(
    [[maybe_unused]] const PadIdentifier& identifier_) const {
    return Common::Input::NfcState::Success;
}

Common::Input::NfcState VirtualAmiibo::WriteNfcData(
    [[maybe_unused]] const PadIdentifier& identifier_, const std::vector<u8>& data) {
    const Common::FS::IOFile nfc_file{file_path, Common::FS::FileAccessMode::ReadWrite,
                                      Common::FS::FileType::BinaryFile};

    if (!nfc_file.IsOpen()) {
        LOG_ERROR(Core, "Amiibo is already on use");
        return Common::Input::NfcState::WriteFailed;
    }

    if (!nfc_file.Write(data)) {
        LOG_ERROR(Service_NFP, "Error writting to file");
        return Common::Input::NfcState::WriteFailed;
    }

    nfc_data = data;

    return Common::Input::NfcState::Success;
}

VirtualAmiibo::State VirtualAmiibo::GetCurrentState() const {
    return state;
}

VirtualAmiibo::Info VirtualAmiibo::LoadAmiibo(const std::string& filename) {
    const Common::FS::IOFile nfc_file{filename, Common::FS::FileAccessMode::Read,
                                      Common::FS::FileType::BinaryFile};

    if (state != State::WaitingForAmiibo) {
        return Info::WrongDeviceState;
    }

    if (!nfc_file.IsOpen()) {
        return Info::UnableToLoad;
    }

    switch (nfc_file.GetSize()) {
    case AmiiboSize:
    case AmiiboSizeWithoutPassword:
        nfc_data.resize(AmiiboSize);
        if (nfc_file.Read(nfc_data) < AmiiboSizeWithoutPassword) {
            return Info::NotAnAmiibo;
        }
        break;
    case MifareSize:
        nfc_data.resize(MifareSize);
        if (nfc_file.Read(nfc_data) < MifareSize) {
            return Info::NotAnAmiibo;
        }
        break;
    default:
        return Info::NotAnAmiibo;
    }

    file_path = filename;
    state = State::AmiiboIsOpen;
    SetNfc(identifier, {Common::Input::NfcState::NewAmiibo, nfc_data});
    return Info::Success;
}

VirtualAmiibo::Info VirtualAmiibo::ReloadAmiibo() {
    if (state == State::AmiiboIsOpen) {
        SetNfc(identifier, {Common::Input::NfcState::NewAmiibo, nfc_data});
        return Info::Success;
    }

    return LoadAmiibo(file_path);
}

VirtualAmiibo::Info VirtualAmiibo::CloseAmiibo() {
    state = polling_mode == Common::Input::PollingMode::NFC ? State::WaitingForAmiibo
                                                            : State::Initialized;
    SetNfc(identifier, {Common::Input::NfcState::AmiiboRemoved, {}});
    return Info::Success;
}

std::string VirtualAmiibo::GetLastFilePath() const {
    return file_path;
}

} // namespace InputCommon
