// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include "common/common_funcs.h"
#include "common/typed_address.h"
#include "core/hle/service/hid/controllers/controller_base.h"
#include "core/hle/service/hid/errors.h"

namespace Kernel {
class KEvent;
class KReadableEvent;
} // namespace Kernel

namespace Service::KernelHelpers {
class ServiceContext;
}

namespace Core::HID {
class EmulatedController;
} // namespace Core::HID

namespace Service::HID {
class Controller_Palma final : public ControllerBase {
public:
    using PalmaOperationData = std::array<u8, 0x140>;

    // This is nn::hid::PalmaOperationType
    enum class PalmaOperationType {
        PlayActivity,
        SetFrModeType,
        ReadStep,
        EnableStep,
        ResetStep,
        ReadApplicationSection,
        WriteApplicationSection,
        ReadUniqueCode,
        SetUniqueCodeInvalid,
        WriteActivityEntry,
        WriteRgbLedPatternEntry,
        WriteWaveEntry,
        ReadDataBaseIdentificationVersion,
        WriteDataBaseIdentificationVersion,
        SuspendFeature,
        ReadPlayLog,
        ResetPlayLog,
    };

    // This is nn::hid::PalmaWaveSet
    enum class PalmaWaveSet : u64 {
        Small,
        Medium,
        Large,
    };

    // This is nn::hid::PalmaFrModeType
    enum class PalmaFrModeType : u64 {
        Off,
        B01,
        B02,
        B03,
        Downloaded,
    };

    // This is nn::hid::PalmaFeature
    enum class PalmaFeature : u64 {
        FrMode,
        RumbleFeedback,
        Step,
        MuteSwitch,
    };

    // This is nn::hid::PalmaOperationInfo
    struct PalmaOperationInfo {
        PalmaOperationType operation{};
        Result result{PalmaResultSuccess};
        PalmaOperationData data{};
    };
    static_assert(sizeof(PalmaOperationInfo) == 0x148, "PalmaOperationInfo is an invalid size");

    // This is nn::hid::PalmaActivityEntry
    struct PalmaActivityEntry {
        u32 rgb_led_pattern_index;
        INSERT_PADDING_BYTES(2);
        PalmaWaveSet wave_set;
        u32 wave_index;
        INSERT_PADDING_BYTES(12);
    };
    static_assert(sizeof(PalmaActivityEntry) == 0x20, "PalmaActivityEntry is an invalid size");

    struct PalmaConnectionHandle {
        Core::HID::NpadIdType npad_id;
        INSERT_PADDING_BYTES(4); // Unknown
    };
    static_assert(sizeof(PalmaConnectionHandle) == 0x8,
                  "PalmaConnectionHandle has incorrect size.");

    explicit Controller_Palma(Core::HID::HIDCore& hid_core_, u8* raw_shared_memory_,
                              KernelHelpers::ServiceContext& service_context_);
    ~Controller_Palma() override;

    // Called when the controller is initialized
    void OnInit() override;

    // When the controller is released
    void OnRelease() override;

    // When the controller is requesting an update for the shared memory
    void OnUpdate(const Core::Timing::CoreTiming& core_timing) override;

    Result GetPalmaConnectionHandle(Core::HID::NpadIdType npad_id, PalmaConnectionHandle& handle);
    Result InitializePalma(const PalmaConnectionHandle& handle);
    Kernel::KReadableEvent& AcquirePalmaOperationCompleteEvent(
        const PalmaConnectionHandle& handle) const;
    Result GetPalmaOperationInfo(const PalmaConnectionHandle& handle,
                                 PalmaOperationType& operation_type,
                                 PalmaOperationData& data) const;
    Result PlayPalmaActivity(const PalmaConnectionHandle& handle, u64 palma_activity);
    Result SetPalmaFrModeType(const PalmaConnectionHandle& handle, PalmaFrModeType fr_mode_);
    Result ReadPalmaStep(const PalmaConnectionHandle& handle);
    Result EnablePalmaStep(const PalmaConnectionHandle& handle, bool is_enabled);
    Result ResetPalmaStep(const PalmaConnectionHandle& handle);
    Result ReadPalmaUniqueCode(const PalmaConnectionHandle& handle);
    Result SetPalmaUniqueCodeInvalid(const PalmaConnectionHandle& handle);
    Result WritePalmaRgbLedPatternEntry(const PalmaConnectionHandle& handle, u64 unknown);
    Result WritePalmaWaveEntry(const PalmaConnectionHandle& handle, PalmaWaveSet wave,
                               Common::ProcessAddress t_mem, u64 size);
    Result SetPalmaDataBaseIdentificationVersion(const PalmaConnectionHandle& handle,
                                                 s32 database_id_version_);
    Result GetPalmaDataBaseIdentificationVersion(const PalmaConnectionHandle& handle);
    Result GetPalmaOperationResult(const PalmaConnectionHandle& handle) const;
    void SetIsPalmaAllConnectable(bool is_all_connectable);
    Result PairPalma(const PalmaConnectionHandle& handle);
    void SetPalmaBoostMode(bool boost_mode);

private:
    void ReadPalmaApplicationSection();
    void WritePalmaApplicationSection();
    void WritePalmaActivityEntry();
    void SuspendPalmaFeature();
    void ReadPalmaPlayLog();
    void ResetPalmaPlayLog();
    void SetIsPalmaPairedConnectable();
    void CancelWritePalmaWaveEntry();
    void EnablePalmaBoostMode();
    void GetPalmaBluetoothAddress();
    void SetDisallowedPalmaConnection();

    bool is_connectable{};
    s32 database_id_version{};
    PalmaOperationInfo operation{};
    PalmaFrModeType fr_mode{};
    PalmaConnectionHandle active_handle{};

    Core::HID::EmulatedController* controller;

    Kernel::KEvent* operation_complete_event;
    KernelHelpers::ServiceContext& service_context;
};

} // namespace Service::HID
