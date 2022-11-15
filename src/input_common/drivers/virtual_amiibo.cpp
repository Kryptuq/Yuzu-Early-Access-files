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

Common::Input::PollingError VirtualAmiibo::SetPollingMode(
    [[maybe_unused]] const PadIdentifier& identifier_,
    const Common::Input::PollingMode polling_mode_) {
    polling_mode = polling_mode_;

    if (polling_mode == Common::Input::PollingMode::NFC) {
        if (state == State::Initialized) {
            state = State::WaitingForAmiibo;
        }
    } else {
        if (state == State::AmiiboIsOpen) {
            CloseAmiibo();
        }
    }

    return Common::Input::PollingError::None;
}

Common::Input::NfcState VirtualAmiibo::SupportsNfc(
    [[maybe_unused]] const PadIdentifier& identifier_) const {
    return Common::Input::NfcState::Success;
}

Common::Input::NfcState VirtualAmiibo::WriteNfcData(
    [[maybe_unused]] const PadIdentifier& identifier_, const std::vector<u8>& data) {
    const Common::FS::IOFile amiibo_file{file_path, Common::FS::FileAccessMode::ReadWrite,
                                         Common::FS::FileType::BinaryFile};

    if (!amiibo_file.IsOpen()) {
        LOG_ERROR(Core, "Amiibo is already on use");
        return Common::Input::NfcState::WriteFailed;
    }

    if (!amiibo_file.Write(data)) {
        LOG_ERROR(Service_NFP, "Error writting to file");
        return Common::Input::NfcState::WriteFailed;
    }

    amiibo_data = data;

    return Common::Input::NfcState::Success;
}

VirtualAmiibo::State VirtualAmiibo::GetCurrentState() const {
    return state;
}

VirtualAmiibo::Info VirtualAmiibo::LoadAmiibo(const std::string& filename) {
    const Common::FS::IOFile amiibo_file{filename, Common::FS::FileAccessMode::Read,
                                         Common::FS::FileType::BinaryFile};

    if (state != State::WaitingForAmiibo) {
        return Info::WrongDeviceState;
    }

    if (!amiibo_file.IsOpen()) {
        return Info::UnableToLoad;
    }

    amiibo_data.resize(amiibo_size);

    if (amiibo_file.Read(amiibo_data) < amiibo_size_without_password) {
        return Info::NotAnAmiibo;
    }

    file_path = filename;
    state = State::AmiiboIsOpen;
    SetNfc(identifier, {Common::Input::NfcState::NewAmiibo, amiibo_data});
    return Info::Success;
}

VirtualAmiibo::Info VirtualAmiibo::ReloadAmiibo() {
    if (state == State::AmiiboIsOpen) {
        SetNfc(identifier, {Common::Input::NfcState::NewAmiibo, amiibo_data});
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
