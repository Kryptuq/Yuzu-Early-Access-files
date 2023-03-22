// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <span>
#include "common/typed_address.h"
#include "core/hle/result.h"

namespace Core {
class System;
}

namespace Kernel {
class KEvent;
class KReadableEvent;
} // namespace Kernel

namespace Service::KernelHelpers {
class ServiceContext;
}

namespace Service::HID {

// This is nn::hidbus::JoyPollingMode
enum class JoyPollingMode : u32 {
    SixAxisSensorDisable,
    SixAxisSensorEnable,
    ButtonOnly,
};

struct DataAccessorHeader {
    Result result{ResultUnknown};
    INSERT_PADDING_WORDS(0x1);
    std::array<u8, 0x18> unused{};
    u64 latest_entry{};
    u64 total_entries{};
};
static_assert(sizeof(DataAccessorHeader) == 0x30, "DataAccessorHeader is an invalid size");

struct JoyDisableSixAxisPollingData {
    std::array<u8, 0x26> data;
    u8 out_size;
    INSERT_PADDING_BYTES(0x1);
    u64 sampling_number;
};
static_assert(sizeof(JoyDisableSixAxisPollingData) == 0x30,
              "JoyDisableSixAxisPollingData is an invalid size");

struct JoyEnableSixAxisPollingData {
    std::array<u8, 0x8> data;
    u8 out_size;
    INSERT_PADDING_BYTES(0x7);
    u64 sampling_number;
};
static_assert(sizeof(JoyEnableSixAxisPollingData) == 0x18,
              "JoyEnableSixAxisPollingData is an invalid size");

struct JoyButtonOnlyPollingData {
    std::array<u8, 0x2c> data;
    u8 out_size;
    INSERT_PADDING_BYTES(0x3);
    u64 sampling_number;
};
static_assert(sizeof(JoyButtonOnlyPollingData) == 0x38,
              "JoyButtonOnlyPollingData is an invalid size");

struct JoyDisableSixAxisPollingEntry {
    u64 sampling_number;
    JoyDisableSixAxisPollingData polling_data;
};
static_assert(sizeof(JoyDisableSixAxisPollingEntry) == 0x38,
              "JoyDisableSixAxisPollingEntry is an invalid size");

struct JoyEnableSixAxisPollingEntry {
    u64 sampling_number;
    JoyEnableSixAxisPollingData polling_data;
};
static_assert(sizeof(JoyEnableSixAxisPollingEntry) == 0x20,
              "JoyEnableSixAxisPollingEntry is an invalid size");

struct JoyButtonOnlyPollingEntry {
    u64 sampling_number;
    JoyButtonOnlyPollingData polling_data;
};
static_assert(sizeof(JoyButtonOnlyPollingEntry) == 0x40,
              "JoyButtonOnlyPollingEntry is an invalid size");

struct JoyDisableSixAxisDataAccessor {
    DataAccessorHeader header{};
    std::array<JoyDisableSixAxisPollingEntry, 0xb> entries{};
};
static_assert(sizeof(JoyDisableSixAxisDataAccessor) == 0x298,
              "JoyDisableSixAxisDataAccessor is an invalid size");

struct JoyEnableSixAxisDataAccessor {
    DataAccessorHeader header{};
    std::array<JoyEnableSixAxisPollingEntry, 0xb> entries{};
};
static_assert(sizeof(JoyEnableSixAxisDataAccessor) == 0x190,
              "JoyEnableSixAxisDataAccessor is an invalid size");

struct ButtonOnlyPollingDataAccessor {
    DataAccessorHeader header;
    std::array<JoyButtonOnlyPollingEntry, 0xb> entries;
};
static_assert(sizeof(ButtonOnlyPollingDataAccessor) == 0x2F0,
              "ButtonOnlyPollingDataAccessor is an invalid size");

class HidbusBase {
public:
    explicit HidbusBase(Core::System& system_, KernelHelpers::ServiceContext& service_context_);
    virtual ~HidbusBase();

    void ActivateDevice();

    void DeactivateDevice();

    bool IsDeviceActivated() const;

    // Enables/disables the device
    void Enable(bool enable);

    // returns true if device is enabled
    bool IsEnabled() const;

    // returns true if polling mode is enabled
    bool IsPollingMode() const;

    // returns polling mode
    JoyPollingMode GetPollingMode() const;

    // Sets and enables JoyPollingMode
    void SetPollingMode(JoyPollingMode mode);

    // Disables JoyPollingMode
    void DisablePollingMode();

    // Called on EnableJoyPollingReceiveMode
    void SetTransferMemoryAddress(Common::ProcessAddress t_mem);

    Kernel::KReadableEvent& GetSendCommandAsycEvent() const;

    virtual void OnInit() {}

    virtual void OnRelease() {}

    // Updates device transfer memory
    virtual void OnUpdate() {}

    // Returns the device ID of the joycon
    virtual u8 GetDeviceId() const {
        return {};
    }

    // Assigns a command from data
    virtual bool SetCommand(std::span<const u8> data) {
        return {};
    }

    // Returns a reply from a command
    virtual std::vector<u8> GetReply() const {
        return {};
    }

protected:
    bool is_activated{};
    bool device_enabled{};
    bool polling_mode_enabled{};
    JoyPollingMode polling_mode = {};
    // TODO(German77): All data accessors need to be replaced with a ring lifo object
    JoyDisableSixAxisDataAccessor disable_sixaxis_data{};
    JoyEnableSixAxisDataAccessor enable_sixaxis_data{};
    ButtonOnlyPollingDataAccessor button_only_data{};

    Common::ProcessAddress transfer_memory{};

    Core::System& system;
    Kernel::KEvent* send_command_async_event;
    KernelHelpers::ServiceContext& service_context;
};
} // namespace Service::HID
