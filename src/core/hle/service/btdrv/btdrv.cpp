// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/core.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/service/btdrv/btdrv.h"
#include "core/hle/service/kernel_helpers.h"
#include "core/hle/service/service.h"
#include "core/hle/service/sm/sm.h"

namespace Service::BtDrv {

class Bt final : public ServiceFramework<Bt> {
public:
    explicit Bt(Core::System& system_)
        : ServiceFramework{system_, "bt"}, service_context{system_, "bt"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "LeClientReadCharacteristic"},
            {1, nullptr, "LeClientReadDescriptor"},
            {2, nullptr, "LeClientWriteCharacteristic"},
            {3, nullptr, "LeClientWriteDescriptor"},
            {4, nullptr, "LeClientRegisterNotification"},
            {5, nullptr, "LeClientDeregisterNotification"},
            {6, nullptr, "SetLeResponse"},
            {7, nullptr, "LeSendIndication"},
            {8, nullptr, "GetLeEventInfo"},
            {9, &Bt::RegisterBleEvent, "RegisterBleEvent"},
        };
        // clang-format on
        RegisterHandlers(functions);

        register_event = service_context.CreateEvent("BT:RegisterEvent");
    }

    ~Bt() override {
        service_context.CloseEvent(register_event);
    }

private:
    void RegisterBleEvent(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_BTM, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2, 1};
        rb.Push(ResultSuccess);
        rb.PushCopyObjects(register_event->GetReadableEvent());
    }

    KernelHelpers::ServiceContext service_context;

    Kernel::KEvent* register_event;
};

class BtDrv final : public ServiceFramework<BtDrv> {
public:
    explicit BtDrv(Core::System& system_) : ServiceFramework{system_, "btdrv"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "InitializeBluetoothDriver"},
            {1, nullptr, "InitializeBluetooth"},
            {2, nullptr, "EnableBluetooth"},
            {3, nullptr, "DisableBluetooth"},
            {4, nullptr, "FinalizeBluetooth"},
            {5, nullptr, "GetAdapterProperties"},
            {6, nullptr, "GetAdapterProperty"},
            {7, nullptr, "SetAdapterProperty"},
            {8, nullptr, "StartInquiry"},
            {9, nullptr, "StopInquiry"},
            {10, nullptr, "CreateBond"},
            {11, nullptr, "RemoveBond"},
            {12, nullptr, "CancelBond"},
            {13, nullptr, "RespondToPinRequest"},
            {14, nullptr, "RespondToSspRequest"},
            {15, nullptr, "GetEventInfo"},
            {16, nullptr, "InitializeHid"},
            {17, nullptr, "OpenHidConnection"},
            {18, nullptr, "CloseHidConnection"},
            {19, nullptr, "WriteHidData"},
            {20, nullptr, "WriteHidData2"},
            {21, nullptr, "SetHidReport"},
            {22, nullptr, "GetHidReport"},
            {23, nullptr, "TriggerConnection"},
            {24, nullptr, "AddPairedDeviceInfo"},
            {25, nullptr, "GetPairedDeviceInfo"},
            {26, nullptr, "FinalizeHid"},
            {27, nullptr, "GetHidEventInfo"},
            {28, nullptr, "SetTsi"},
            {29, nullptr, "EnableBurstMode"},
            {30, nullptr, "SetZeroRetransmission"},
            {31, nullptr, "EnableMcMode"},
            {32, nullptr, "EnableLlrScan"},
            {33, nullptr, "DisableLlrScan"},
            {34, nullptr, "EnableRadio"},
            {35, nullptr, "SetVisibility"},
            {36, nullptr, "EnableTbfcScan"},
            {37, nullptr, "RegisterHidReportEvent"},
            {38, nullptr, "GetHidReportEventInfo"},
            {39, nullptr, "GetLatestPlr"},
            {40, nullptr, "GetPendingConnections"},
            {41, nullptr, "GetChannelMap"},
            {42, nullptr, "EnableTxPowerBoostSetting"},
            {43, nullptr, "IsTxPowerBoostSettingEnabled"},
            {44, nullptr, "EnableAfhSetting"},
            {45, nullptr, "IsAfhSettingEnabled"},
            {46, nullptr, "InitializeBle"},
            {47, nullptr, "EnableBle"},
            {48, nullptr, "DisableBle"},
            {49, nullptr, "FinalizeBle"},
            {50, nullptr, "SetBleVisibility"},
            {51, nullptr, "SetBleConnectionParameter"},
            {52, nullptr, "SetBleDefaultConnectionParameter"},
            {53, nullptr, "SetBleAdvertiseData"},
            {54, nullptr, "SetBleAdvertiseParameter"},
            {55, nullptr, "StartBleScan"},
            {56, nullptr, "StopBleScan"},
            {57, nullptr, "AddBleScanFilterCondition"},
            {58, nullptr, "DeleteBleScanFilterCondition"},
            {59, nullptr, "DeleteBleScanFilter"},
            {60, nullptr, "ClearBleScanFilters"},
            {61, nullptr, "EnableBleScanFilter"},
            {62, nullptr, "RegisterGattClient"},
            {63, nullptr, "UnregisterGattClient"},
            {64, nullptr, "UnregisterAllGattClients"},
            {65, nullptr, "ConnectGattServer"},
            {66, nullptr, "CancelConnectGattServer"},
            {67, nullptr, "DisconnectGattServer"},
            {68, nullptr, "GetGattAttribute"},
            {69, nullptr, "GetGattService"},
            {70, nullptr, "ConfigureAttMtu"},
            {71, nullptr, "RegisterGattServer"},
            {72, nullptr, "UnregisterGattServer"},
            {73, nullptr, "ConnectGattClient"},
            {74, nullptr, "DisconnectGattClient"},
            {75, nullptr, "AddGattService"},
            {76, nullptr, "EnableGattService"},
            {77, nullptr, "AddGattCharacteristic"},
            {78, nullptr, "AddGattDescriptor"},
            {79, nullptr, "GetBleManagedEventInfo"},
            {80, nullptr, "GetGattFirstCharacteristic"},
            {81, nullptr, "GetGattNextCharacteristic"},
            {82, nullptr, "GetGattFirstDescriptor"},
            {83, nullptr, "GetGattNextDescriptor"},
            {84, nullptr, "RegisterGattManagedDataPath"},
            {85, nullptr, "UnregisterGattManagedDataPath"},
            {86, nullptr, "RegisterGattHidDataPath"},
            {87, nullptr, "UnregisterGattHidDataPath"},
            {88, nullptr, "RegisterGattDataPath"},
            {89, nullptr, "UnregisterGattDataPath"},
            {90, nullptr, "ReadGattCharacteristic"},
            {91, nullptr, "ReadGattDescriptor"},
            {92, nullptr, "WriteGattCharacteristic"},
            {93, nullptr, "WriteGattDescriptor"},
            {94, nullptr, "RegisterGattNotification"},
            {95, nullptr, "UnregisterGattNotification"},
            {96, nullptr, "GetLeHidEventInfo"},
            {97, nullptr, "RegisterBleHidEvent"},
            {98, nullptr, "SetBleScanParameter"},
            {99, nullptr, "MoveToSecondaryPiconet"},
            {100, nullptr, "IsBluetoothEnabled"},
            {128, nullptr, "AcquireAudioEvent"},
            {129, nullptr, "GetAudioEventInfo"},
            {130, nullptr, "OpenAudioConnection"},
            {131, nullptr, "CloseAudioConnection"},
            {132, nullptr, "OpenAudioOut"},
            {133, nullptr, "CloseAudioOut"},
            {134, nullptr, "AcquireAudioOutStateChangedEvent"},
            {135, nullptr, "StartAudioOut"},
            {136, nullptr, "StopAudioOut"},
            {137, nullptr, "GetAudioOutState"},
            {138, nullptr, "GetAudioOutFeedingCodec"},
            {139, nullptr, "GetAudioOutFeedingParameter"},
            {140, nullptr, "AcquireAudioOutBufferAvailableEvent"},
            {141, nullptr, "SendAudioData"},
            {142, nullptr, "AcquireAudioControlInputStateChangedEvent"},
            {143, nullptr, "GetAudioControlInputState"},
            {144, nullptr, "AcquireAudioConnectionStateChangedEvent"},
            {145, nullptr, "GetConnectedAudioDevice"},
            {146, nullptr, "CloseAudioControlInput"},
            {147, nullptr, "RegisterAudioControlNotification"},
            {148, nullptr, "SendAudioControlPassthroughCommand"},
            {149, nullptr, "SendAudioControlSetAbsoluteVolumeCommand"},
            {256, nullptr, "IsManufacturingMode"},
            {257, nullptr, "EmulateBluetoothCrash"},
            {258, nullptr, "GetBleChannelMap"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

void InstallInterfaces(SM::ServiceManager& sm, Core::System& system) {
    std::make_shared<BtDrv>(system)->InstallAsService(sm);
    std::make_shared<Bt>(system)->InstallAsService(sm);
}

} // namespace Service::BtDrv
