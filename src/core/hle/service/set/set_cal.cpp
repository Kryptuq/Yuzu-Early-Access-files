// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/set/set_cal.h"

namespace Service::Set {

SET_CAL::SET_CAL(Core::System& system_) : ServiceFramework{system_, "set:cal"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "GetBluetoothBdAddress"},
        {1, nullptr, "GetConfigurationId1"},
        {2, nullptr, "GetAccelerometerOffset"},
        {3, nullptr, "GetAccelerometerScale"},
        {4, nullptr, "GetGyroscopeOffset"},
        {5, nullptr, "GetGyroscopeScale"},
        {6, nullptr, "GetWirelessLanMacAddress"},
        {7, nullptr, "GetWirelessLanCountryCodeCount"},
        {8, nullptr, "GetWirelessLanCountryCodes"},
        {9, nullptr, "GetSerialNumber"},
        {10, nullptr, "SetInitialSystemAppletProgramId"},
        {11, nullptr, "SetOverlayDispProgramId"},
        {12, nullptr, "GetBatteryLot"},
        {14, nullptr, "GetEciDeviceCertificate"},
        {15, nullptr, "GetEticketDeviceCertificate"},
        {16, nullptr, "GetSslKey"},
        {17, nullptr, "GetSslCertificate"},
        {18, nullptr, "GetGameCardKey"},
        {19, nullptr, "GetGameCardCertificate"},
        {20, nullptr, "GetEciDeviceKey"},
        {21, nullptr, "GetEticketDeviceKey"},
        {22, nullptr, "GetSpeakerParameter"},
        {23, nullptr, "GetLcdVendorId"},
        {24, nullptr, "GetEciDeviceCertificate2"},
        {25, nullptr, "GetEciDeviceKey2"},
        {26, nullptr, "GetAmiiboKey"},
        {27, nullptr, "GetAmiiboEcqvCertificate"},
        {28, nullptr, "GetAmiiboEcdsaCertificate"},
        {29, nullptr, "GetAmiiboEcqvBlsKey"},
        {30, nullptr, "GetAmiiboEcqvBlsCertificate"},
        {31, nullptr, "GetAmiiboEcqvBlsRootCertificate"},
        {32, nullptr, "GetUsbTypeCPowerSourceCircuitVersion"},
        {33, nullptr, "GetAnalogStickModuleTypeL"},
        {34, nullptr, "GetAnalogStickModelParameterL"},
        {35, nullptr, "GetAnalogStickFactoryCalibrationL"},
        {36, nullptr, "GetAnalogStickModuleTypeR"},
        {37, nullptr, "GetAnalogStickModelParameterR"},
        {38, nullptr, "GetAnalogStickFactoryCalibrationR"},
        {39, nullptr, "GetConsoleSixAxisSensorModuleType"},
        {40, nullptr, "GetConsoleSixAxisSensorHorizontalOffset"},
        {41, nullptr, "GetBatteryVersion"},
        {42, nullptr, "GetDeviceId"},
        {43, nullptr, "GetConsoleSixAxisSensorMountType"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

SET_CAL::~SET_CAL() = default;

} // namespace Service::Set
