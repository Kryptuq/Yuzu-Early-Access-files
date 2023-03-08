// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <string>
#include <vector>

#include "common/common_types.h"
#include "input_common/input_engine.h"

namespace Common::FS {
class IOFile;
}

namespace InputCommon {

class VirtualAmiibo final : public InputEngine {
public:
    enum class State {
        Initialized,
        WaitingForAmiibo,
        AmiiboIsOpen,
    };

    enum class Info {
        Success,
        UnableToLoad,
        NotAnAmiibo,
        WrongDeviceState,
        Unknown,
    };

    explicit VirtualAmiibo(std::string input_engine_);
    ~VirtualAmiibo() override;

    // Sets polling mode to a controller
    Common::Input::DriverResult SetPollingMode(
        const PadIdentifier& identifier_, const Common::Input::PollingMode polling_mode_) override;

    Common::Input::NfcState SupportsNfc(const PadIdentifier& identifier_) const override;

    Common::Input::NfcState WriteNfcData(const PadIdentifier& identifier_,
                                         const std::vector<u8>& data) override;

    State GetCurrentState() const;

    Info LoadAmiibo(const std::string& amiibo_file);
    Info ReloadAmiibo();
    Info CloseAmiibo();

    std::string GetLastFilePath() const;

private:
    static constexpr std::size_t AmiiboSize = 0x21C;
    static constexpr std::size_t AmiiboSizeWithoutPassword = AmiiboSize - 0x8;
    static constexpr std::size_t MifareSize = 0x400;

    std::string file_path{};
    State state{State::Initialized};
    std::vector<u8> nfc_data;
    Common::Input::PollingMode polling_mode{Common::Input::PollingMode::Passive};
};
} // namespace InputCommon
