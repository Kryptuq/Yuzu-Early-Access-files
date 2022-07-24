// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/spl/spl.h"

namespace Service::SPL {

SPL::SPL(Core::System& system_, std::shared_ptr<Module> module_)
    : Interface(system_, std::move(module_), "spl:") {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, &SPL::GetConfig, "GetConfig"},
        {1, &SPL::ModularExponentiate, "ModularExponentiate"},
        {5, &SPL::SetConfig, "SetConfig"},
        {7, &SPL::GenerateRandomBytes, "GenerateRandomBytes"},
        {11, &SPL::IsDevelopment, "IsDevelopment"},
        {24, &SPL::SetBootReason, "SetBootReason"},
        {25, &SPL::GetBootReason, "GetBootReason"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

SPL_MIG::SPL_MIG(Core::System& system_, std::shared_ptr<Module> module_)
    : Interface(system_, std::move(module_), "spl:mig") {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, &SPL::GetConfig, "GetConfig"},
        {1, &SPL::ModularExponentiate, "ModularExponentiate"},
        {2, nullptr, "GenerateAesKek"},
        {3, nullptr, "LoadAesKey"},
        {4, nullptr, "GenerateAesKey"},
        {5, &SPL::SetConfig, "SetConfig"},
        {7, &SPL::GenerateRandomBytes, "GenerateRandomBytes"},
        {11, &SPL::IsDevelopment, "IsDevelopment"},
        {14, nullptr, "DecryptAesKey"},
        {15, nullptr, "CryptAesCtr"},
        {16, nullptr, "ComputeCmac"},
        {21, nullptr, "AllocateAesKeyslot"},
        {22, nullptr, "DeallocateAesKeySlot"},
        {23, nullptr, "GetAesKeyslotAvailableEvent"},
        {24, &SPL::SetBootReason, "SetBootReason"},
        {25, &SPL::GetBootReason, "GetBootReason"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

SPL_FS::SPL_FS(Core::System& system_, std::shared_ptr<Module> module_)
    : Interface(system_, std::move(module_), "spl:fs") {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, &SPL::GetConfig, "GetConfig"},
        {1, &SPL::ModularExponentiate, "ModularExponentiate"},
        {2, nullptr, "GenerateAesKek"},
        {3, nullptr, "LoadAesKey"},
        {4, nullptr, "GenerateAesKey"},
        {5, &SPL::SetConfig, "SetConfig"},
        {7, &SPL::GenerateRandomBytes, "GenerateRandomBytes"},
        {9, nullptr, "ImportLotusKey"},
        {10, nullptr, "DecryptLotusMessage"},
        {11, &SPL::IsDevelopment, "IsDevelopment"},
        {12, nullptr, "GenerateSpecificAesKey"},
        {14, nullptr, "DecryptAesKey"},
        {15, nullptr, "CryptAesCtr"},
        {16, nullptr, "ComputeCmac"},
        {19, nullptr, "LoadTitleKey"},
        {21, nullptr, "AllocateAesKeyslot"},
        {22, nullptr, "DeallocateAesKeySlot"},
        {23, nullptr, "GetAesKeyslotAvailableEvent"},
        {24, &SPL::SetBootReason, "SetBootReason"},
        {25, &SPL::GetBootReason, "GetBootReason"},
        {31, nullptr, "GetPackage2Hash"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

SPL_SSL::SPL_SSL(Core::System& system_, std::shared_ptr<Module> module_)
    : Interface(system_, std::move(module_), "spl:ssl") {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, &SPL::GetConfig, "GetConfig"},
        {1, &SPL::ModularExponentiate, "ModularExponentiate"},
        {2, nullptr, "GenerateAesKek"},
        {3, nullptr, "LoadAesKey"},
        {4, nullptr, "GenerateAesKey"},
        {5, &SPL::SetConfig, "SetConfig"},
        {7, &SPL::GenerateRandomBytes, "GenerateRandomBytes"},
        {11, &SPL::IsDevelopment, "IsDevelopment"},
        {13, nullptr, "DecryptDeviceUniqueData"},
        {14, nullptr, "DecryptAesKey"},
        {15, nullptr, "CryptAesCtr"},
        {16, nullptr, "ComputeCmac"},
        {21, nullptr, "AllocateAesKeyslot"},
        {22, nullptr, "DeallocateAesKeySlot"},
        {23, nullptr, "GetAesKeyslotAvailableEvent"},
        {24, &SPL::SetBootReason, "SetBootReason"},
        {25, &SPL::GetBootReason, "GetBootReason"},
        {26, nullptr, "DecryptAndStoreSslClientCertKey"},
        {27, nullptr, "ModularExponentiateWithSslClientCertKey"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

SPL_ES::SPL_ES(Core::System& system_, std::shared_ptr<Module> module_)
    : Interface(system_, std::move(module_), "spl:es") {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, &SPL::GetConfig, "GetConfig"},
        {1, &SPL::ModularExponentiate, "ModularExponentiate"},
        {2, nullptr, "GenerateAesKek"},
        {3, nullptr, "LoadAesKey"},
        {4, nullptr, "GenerateAesKey"},
        {5, &SPL::SetConfig, "SetConfig"},
        {7, &SPL::GenerateRandomBytes, "GenerateRandomBytes"},
        {11, &SPL::IsDevelopment, "IsDevelopment"},
        {13, nullptr, "DecryptDeviceUniqueData"},
        {14, nullptr, "DecryptAesKey"},
        {15, nullptr, "CryptAesCtr"},
        {16, nullptr, "ComputeCmac"},
        {17, nullptr, "ImportEsKey"},
        {18, nullptr, "UnwrapTitleKey"},
        {20, nullptr, "PrepareEsCommonKey"},
        {21, nullptr, "AllocateAesKeyslot"},
        {22, nullptr, "DeallocateAesKeySlot"},
        {23, nullptr, "GetAesKeyslotAvailableEvent"},
        {24, &SPL::SetBootReason, "SetBootReason"},
        {25, &SPL::GetBootReason, "GetBootReason"},
        {28, nullptr, "DecryptAndStoreDrmDeviceCertKey"},
        {29, nullptr, "ModularExponentiateWithDrmDeviceCertKey"},
        {31, nullptr, "PrepareEsArchiveKey"},
        {32, nullptr, "LoadPreparedAesKey"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

SPL_MANU::SPL_MANU(Core::System& system_, std::shared_ptr<Module> module_)
    : Interface(system_, std::move(module_), "spl:manu") {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, &SPL::GetConfig, "GetConfig"},
        {1, &SPL::ModularExponentiate, "ModularExponentiate"},
        {2, nullptr, "GenerateAesKek"},
        {3, nullptr, "LoadAesKey"},
        {4, nullptr, "GenerateAesKey"},
        {5, &SPL::SetConfig, "SetConfig"},
        {7, &SPL::GenerateRandomBytes, "GenerateRandomBytes"},
        {11, &SPL::IsDevelopment, "IsDevelopment"},
        {13, nullptr, "DecryptDeviceUniqueData"},
        {14, nullptr, "DecryptAesKey"},
        {15, nullptr, "CryptAesCtr"},
        {16, nullptr, "ComputeCmac"},
        {21, nullptr, "AllocateAesKeyslot"},
        {22, nullptr, "DeallocateAesKeySlot"},
        {23, nullptr, "GetAesKeyslotAvailableEvent"},
        {24, &SPL::SetBootReason, "SetBootReason"},
        {25, &SPL::GetBootReason, "GetBootReason"},
        {30, nullptr, "ReencryptDeviceUniqueData"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

SPL::~SPL() = default;

SPL_MIG::~SPL_MIG() = default;

SPL_FS::~SPL_FS() = default;

SPL_SSL::~SPL_SSL() = default;

SPL_ES::~SPL_ES() = default;

SPL_MANU::~SPL_MANU() = default;

} // namespace Service::SPL
