// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "core/hid/emulated_console.h"
#include "core/hid/emulated_controller.h"
#include "core/hid/emulated_devices.h"
#include "core/hid/hid_core.h"

namespace Core::HID {

HIDCore::HIDCore()
    : player_1{std::make_unique<EmulatedController>(NpadIdType::Player1)},
      player_2{std::make_unique<EmulatedController>(NpadIdType::Player2)},
      player_3{std::make_unique<EmulatedController>(NpadIdType::Player3)},
      player_4{std::make_unique<EmulatedController>(NpadIdType::Player4)},
      player_5{std::make_unique<EmulatedController>(NpadIdType::Player5)},
      player_6{std::make_unique<EmulatedController>(NpadIdType::Player6)},
      player_7{std::make_unique<EmulatedController>(NpadIdType::Player7)},
      player_8{std::make_unique<EmulatedController>(NpadIdType::Player8)},
      other{std::make_unique<EmulatedController>(NpadIdType::Other)},
      handheld{std::make_unique<EmulatedController>(NpadIdType::Handheld)},
      console{std::make_unique<EmulatedConsole>()}, devices{std::make_unique<EmulatedDevices>()} {}

HIDCore::~HIDCore() = default;

EmulatedController* HIDCore::GetEmulatedController(NpadIdType npad_id_type) {
    switch (npad_id_type) {
    case NpadIdType::Player1:
        return player_1.get();
    case NpadIdType::Player2:
        return player_2.get();
    case NpadIdType::Player3:
        return player_3.get();
    case NpadIdType::Player4:
        return player_4.get();
    case NpadIdType::Player5:
        return player_5.get();
    case NpadIdType::Player6:
        return player_6.get();
    case NpadIdType::Player7:
        return player_7.get();
    case NpadIdType::Player8:
        return player_8.get();
    case NpadIdType::Other:
        return other.get();
    case NpadIdType::Handheld:
        return handheld.get();
    case NpadIdType::Invalid:
    default:
        UNREACHABLE_MSG("Invalid NpadIdType={}", npad_id_type);
        return nullptr;
    }
}

const EmulatedController* HIDCore::GetEmulatedController(NpadIdType npad_id_type) const {
    switch (npad_id_type) {
    case NpadIdType::Player1:
        return player_1.get();
    case NpadIdType::Player2:
        return player_2.get();
    case NpadIdType::Player3:
        return player_3.get();
    case NpadIdType::Player4:
        return player_4.get();
    case NpadIdType::Player5:
        return player_5.get();
    case NpadIdType::Player6:
        return player_6.get();
    case NpadIdType::Player7:
        return player_7.get();
    case NpadIdType::Player8:
        return player_8.get();
    case NpadIdType::Other:
        return other.get();
    case NpadIdType::Handheld:
        return handheld.get();
    case NpadIdType::Invalid:
    default:
        UNREACHABLE_MSG("Invalid NpadIdType={}", npad_id_type);
        return nullptr;
    }
}
EmulatedConsole* HIDCore::GetEmulatedConsole() {
    return console.get();
}

const EmulatedConsole* HIDCore::GetEmulatedConsole() const {
    return console.get();
}

EmulatedDevices* HIDCore::GetEmulatedDevices() {
    return devices.get();
}

const EmulatedDevices* HIDCore::GetEmulatedDevices() const {
    return devices.get();
}

EmulatedController* HIDCore::GetEmulatedControllerByIndex(std::size_t index) {
    return GetEmulatedController(IndexToNpadIdType(index));
}

const EmulatedController* HIDCore::GetEmulatedControllerByIndex(std::size_t index) const {
    return GetEmulatedController(IndexToNpadIdType(index));
}

void HIDCore::SetSupportedStyleTag(NpadStyleTag style_tag) {
    supported_style_tag.raw = style_tag.raw;
    player_1->SetSupportedNpadStyleTag(supported_style_tag);
    player_2->SetSupportedNpadStyleTag(supported_style_tag);
    player_3->SetSupportedNpadStyleTag(supported_style_tag);
    player_4->SetSupportedNpadStyleTag(supported_style_tag);
    player_5->SetSupportedNpadStyleTag(supported_style_tag);
    player_6->SetSupportedNpadStyleTag(supported_style_tag);
    player_7->SetSupportedNpadStyleTag(supported_style_tag);
    player_8->SetSupportedNpadStyleTag(supported_style_tag);
    other->SetSupportedNpadStyleTag(supported_style_tag);
    handheld->SetSupportedNpadStyleTag(supported_style_tag);
}

NpadStyleTag HIDCore::GetSupportedStyleTag() const {
    return supported_style_tag;
}

s8 HIDCore::GetPlayerCount() const {
    s8 active_players = 0;
    for (std::size_t player_index = 0; player_index < available_controllers - 2; ++player_index) {
        const auto* const controller = GetEmulatedControllerByIndex(player_index);
        if (controller->IsConnected()) {
            active_players++;
        }
    }
    return active_players;
}

NpadIdType HIDCore::GetFirstNpadId() const {
    for (std::size_t player_index = 0; player_index < available_controllers; ++player_index) {
        const auto* const controller = GetEmulatedControllerByIndex(player_index);
        if (controller->IsConnected()) {
            return controller->GetNpadIdType();
        }
    }
    return NpadIdType::Player1;
}

NpadIdType HIDCore::GetFirstDisconnectedNpadId() const {
    for (std::size_t player_index = 0; player_index < available_controllers; ++player_index) {
        const auto* const controller = GetEmulatedControllerByIndex(player_index);
        if (!controller->IsConnected()) {
            return controller->GetNpadIdType();
        }
    }
    return NpadIdType::Player1;
}

void HIDCore::EnableAllControllerConfiguration() {
    player_1->EnableConfiguration();
    player_2->EnableConfiguration();
    player_3->EnableConfiguration();
    player_4->EnableConfiguration();
    player_5->EnableConfiguration();
    player_6->EnableConfiguration();
    player_7->EnableConfiguration();
    player_8->EnableConfiguration();
    other->EnableConfiguration();
    handheld->EnableConfiguration();
}

void HIDCore::DisableAllControllerConfiguration() {
    player_1->DisableConfiguration();
    player_2->DisableConfiguration();
    player_3->DisableConfiguration();
    player_4->DisableConfiguration();
    player_5->DisableConfiguration();
    player_6->DisableConfiguration();
    player_7->DisableConfiguration();
    player_8->DisableConfiguration();
    other->DisableConfiguration();
    handheld->DisableConfiguration();
}

void HIDCore::ReloadInputDevices() {
    player_1->ReloadFromSettings();
    player_2->ReloadFromSettings();
    player_3->ReloadFromSettings();
    player_4->ReloadFromSettings();
    player_5->ReloadFromSettings();
    player_6->ReloadFromSettings();
    player_7->ReloadFromSettings();
    player_8->ReloadFromSettings();
    other->ReloadFromSettings();
    handheld->ReloadFromSettings();
    console->ReloadFromSettings();
    devices->ReloadFromSettings();
}

void HIDCore::UnloadInputDevices() {
    player_1->UnloadInput();
    player_2->UnloadInput();
    player_3->UnloadInput();
    player_4->UnloadInput();
    player_5->UnloadInput();
    player_6->UnloadInput();
    player_7->UnloadInput();
    player_8->UnloadInput();
    other->UnloadInput();
    handheld->UnloadInput();
    console->UnloadInput();
    devices->UnloadInput();
}

} // namespace Core::HID
