// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/core_timing.h"
#include "core/hid/emulated_controller.h"
#include "core/hid/hid_core.h"
#include "core/hid/hid_types.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/kernel/k_readable_event.h"
#include "core/hle/service/hid/controllers/palma.h"
#include "core/hle/service/kernel_helpers.h"

namespace Service::HID {

Controller_Palma::Controller_Palma(Core::HID::HIDCore& hid_core_, u8* raw_shared_memory_,
                                   KernelHelpers::ServiceContext& service_context_)
    : ControllerBase{hid_core_}, service_context{service_context_} {
    controller = hid_core.GetEmulatedController(Core::HID::NpadIdType::Other);
    operation_complete_event = service_context.CreateEvent("hid:PalmaOperationCompleteEvent");
}

Controller_Palma::~Controller_Palma() = default;

void Controller_Palma::OnInit() {}

void Controller_Palma::OnRelease() {}

void Controller_Palma::OnUpdate(const Core::Timing::CoreTiming& core_timing) {
    if (!IsControllerActivated()) {
        return;
    }
}

Result Controller_Palma::GetPalmaConnectionHandle(Core::HID::NpadIdType npad_id,
                                                  PalmaConnectionHandle& handle) {
    active_handle.npad_id = npad_id;
    handle = active_handle;
    return ResultSuccess;
}

Result Controller_Palma::InitializePalma(const PalmaConnectionHandle& handle) {
    if (handle.npad_id != active_handle.npad_id) {
        return InvalidPalmaHandle;
    }
    ActivateController();
    return ResultSuccess;
}

Kernel::KReadableEvent& Controller_Palma::AcquirePalmaOperationCompleteEvent(
    const PalmaConnectionHandle& handle) const {
    if (handle.npad_id != active_handle.npad_id) {
        LOG_ERROR(Service_HID, "Invalid npad id {}", handle.npad_id);
    }
    return operation_complete_event->GetReadableEvent();
}

Result Controller_Palma::GetPalmaOperationInfo(const PalmaConnectionHandle& handle,
                                               PalmaOperationType& operation_type,
                                               PalmaOperationData& data) const {
    if (handle.npad_id != active_handle.npad_id) {
        return InvalidPalmaHandle;
    }
    operation_type = operation.operation;
    data = operation.data;
    return ResultSuccess;
}

Result Controller_Palma::PlayPalmaActivity(const PalmaConnectionHandle& handle,
                                           u64 palma_activity) {
    if (handle.npad_id != active_handle.npad_id) {
        return InvalidPalmaHandle;
    }
    operation.operation = PalmaOperationType::PlayActivity;
    operation.result = PalmaResultSuccess;
    operation.data = {};
    operation_complete_event->GetWritableEvent().Signal();
    return ResultSuccess;
}

Result Controller_Palma::SetPalmaFrModeType(const PalmaConnectionHandle& handle,
                                            PalmaFrModeType fr_mode_) {
    if (handle.npad_id != active_handle.npad_id) {
        return InvalidPalmaHandle;
    }
    fr_mode = fr_mode_;
    return ResultSuccess;
}

Result Controller_Palma::ReadPalmaStep(const PalmaConnectionHandle& handle) {
    if (handle.npad_id != active_handle.npad_id) {
        return InvalidPalmaHandle;
    }
    operation.operation = PalmaOperationType::ReadStep;
    operation.result = PalmaResultSuccess;
    operation.data = {};
    operation_complete_event->GetWritableEvent().Signal();
    return ResultSuccess;
}

Result Controller_Palma::EnablePalmaStep(const PalmaConnectionHandle& handle, bool is_enabled) {
    if (handle.npad_id != active_handle.npad_id) {
        return InvalidPalmaHandle;
    }
    return ResultSuccess;
}

Result Controller_Palma::ResetPalmaStep(const PalmaConnectionHandle& handle) {
    if (handle.npad_id != active_handle.npad_id) {
        return InvalidPalmaHandle;
    }
    return ResultSuccess;
}

void Controller_Palma::ReadPalmaApplicationSection() {}

void Controller_Palma::WritePalmaApplicationSection() {}

Result Controller_Palma::ReadPalmaUniqueCode(const PalmaConnectionHandle& handle) {
    if (handle.npad_id != active_handle.npad_id) {
        return InvalidPalmaHandle;
    }
    operation.operation = PalmaOperationType::ReadUniqueCode;
    operation.result = PalmaResultSuccess;
    operation.data = {};
    operation_complete_event->GetWritableEvent().Signal();
    return ResultSuccess;
}

Result Controller_Palma::SetPalmaUniqueCodeInvalid(const PalmaConnectionHandle& handle) {
    if (handle.npad_id != active_handle.npad_id) {
        return InvalidPalmaHandle;
    }
    operation.operation = PalmaOperationType::SetUniqueCodeInvalid;
    operation.result = PalmaResultSuccess;
    operation.data = {};
    operation_complete_event->GetWritableEvent().Signal();
    return ResultSuccess;
}

void Controller_Palma::WritePalmaActivityEntry() {}

Result Controller_Palma::WritePalmaRgbLedPatternEntry(const PalmaConnectionHandle& handle,
                                                      u64 unknown) {
    if (handle.npad_id != active_handle.npad_id) {
        return InvalidPalmaHandle;
    }
    operation.operation = PalmaOperationType::WriteRgbLedPatternEntry;
    operation.result = PalmaResultSuccess;
    operation.data = {};
    operation_complete_event->GetWritableEvent().Signal();
    return ResultSuccess;
}

Result Controller_Palma::WritePalmaWaveEntry(const PalmaConnectionHandle& handle, PalmaWaveSet wave,
                                             u8* t_mem, u64 size) {
    if (handle.npad_id != active_handle.npad_id) {
        return InvalidPalmaHandle;
    }
    operation.operation = PalmaOperationType::WriteWaveEntry;
    operation.result = PalmaResultSuccess;
    operation.data = {};
    operation_complete_event->GetWritableEvent().Signal();
    return ResultSuccess;
}

Result Controller_Palma::SetPalmaDataBaseIdentificationVersion(const PalmaConnectionHandle& handle,
                                                               s32 database_id_version_) {
    if (handle.npad_id != active_handle.npad_id) {
        return InvalidPalmaHandle;
    }
    database_id_version = database_id_version_;
    operation.operation = PalmaOperationType::ReadDataBaseIdentificationVersion;
    operation.result = PalmaResultSuccess;
    operation.data[0] = {};
    operation_complete_event->GetWritableEvent().Signal();
    return ResultSuccess;
}

Result Controller_Palma::GetPalmaDataBaseIdentificationVersion(
    const PalmaConnectionHandle& handle) {
    if (handle.npad_id != active_handle.npad_id) {
        return InvalidPalmaHandle;
    }
    operation.operation = PalmaOperationType::ReadDataBaseIdentificationVersion;
    operation.result = PalmaResultSuccess;
    operation.data = {};
    operation.data[0] = static_cast<u8>(database_id_version);
    operation_complete_event->GetWritableEvent().Signal();
    return ResultSuccess;
}

void Controller_Palma::SuspendPalmaFeature() {}

Result Controller_Palma::GetPalmaOperationResult(const PalmaConnectionHandle& handle) const {
    if (handle.npad_id != active_handle.npad_id) {
        return InvalidPalmaHandle;
    }
    return operation.result;
}
void Controller_Palma::ReadPalmaPlayLog() {}

void Controller_Palma::ResetPalmaPlayLog() {}

void Controller_Palma::SetIsPalmaAllConnectable(bool is_all_connectable) {
    // If true controllers are able to be paired
    is_connectable = is_all_connectable;
}

void Controller_Palma::SetIsPalmaPairedConnectable() {}

Result Controller_Palma::PairPalma(const PalmaConnectionHandle& handle) {
    if (handle.npad_id != active_handle.npad_id) {
        return InvalidPalmaHandle;
    }
    // TODO: Do something
    return ResultSuccess;
}

void Controller_Palma::SetPalmaBoostMode(bool boost_mode) {}

void Controller_Palma::CancelWritePalmaWaveEntry() {}

void Controller_Palma::EnablePalmaBoostMode() {}

void Controller_Palma::GetPalmaBluetoothAddress() {}

void Controller_Palma::SetDisallowedPalmaConnection() {}

} // namespace Service::HID
