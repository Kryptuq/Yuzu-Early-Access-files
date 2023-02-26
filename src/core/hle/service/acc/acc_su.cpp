// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/acc/acc_su.h"

namespace Service::Account {

ACC_SU::ACC_SU(std::shared_ptr<Module> module_, std::shared_ptr<ProfileManager> profile_manager_,
               Core::System& system_)
    : Interface(std::move(module_), std::move(profile_manager_), system_, "acc:su") {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, &ACC_SU::GetUserCount, "GetUserCount"},
        {1, &ACC_SU::GetUserExistence, "GetUserExistence"},
        {2, &ACC_SU::ListAllUsers, "ListAllUsers"},
        {3, &ACC_SU::ListOpenUsers, "ListOpenUsers"},
        {4, &ACC_SU::GetLastOpenedUser, "GetLastOpenedUser"},
        {5, &ACC_SU::GetProfile, "GetProfile"},
        {6, nullptr, "GetProfileDigest"},
        {50, &ACC_SU::IsUserRegistrationRequestPermitted, "IsUserRegistrationRequestPermitted"},
        {51, &ACC_SU::TrySelectUserWithoutInteraction, "TrySelectUserWithoutInteraction"},
        {60, &ACC_SU::ListOpenContextStoredUsers, "ListOpenContextStoredUsers"},
        {99, nullptr, "DebugActivateOpenContextRetention"},
        {100, nullptr, "GetUserRegistrationNotifier"},
        {101, nullptr, "GetUserStateChangeNotifier"},
        {102, nullptr, "GetBaasAccountManagerForSystemService"},
        {103, nullptr, "GetBaasUserAvailabilityChangeNotifier"},
        {104, nullptr, "GetProfileUpdateNotifier"},
        {105, nullptr, "CheckNetworkServiceAvailabilityAsync"},
        {106, nullptr, "GetProfileSyncNotifier"},
        {110, &ACC_SU::StoreSaveDataThumbnailSystem, "StoreSaveDataThumbnail"},
        {111, nullptr, "ClearSaveDataThumbnail"},
        {112, nullptr, "LoadSaveDataThumbnail"},
        {113, nullptr, "GetSaveDataThumbnailExistence"},
        {120, nullptr, "ListOpenUsersInApplication"},
        {130, nullptr, "ActivateOpenContextRetention"},
        {140, &ACC_SU::ListQualifiedUsers, "ListQualifiedUsers"},
        {150, nullptr, "AuthenticateApplicationAsync"},
        {151, nullptr, "EnsureSignedDeviceIdentifierCacheForNintendoAccountAsync"},
        {152, nullptr, "LoadSignedDeviceIdentifierCacheForNintendoAccount"},
        {190, nullptr, "GetUserLastOpenedApplication"},
        {191, nullptr, "ActivateOpenContextHolder"},
        {200, nullptr, "BeginUserRegistration"},
        {201, nullptr, "CompleteUserRegistration"},
        {202, nullptr, "CancelUserRegistration"},
        {203, nullptr, "DeleteUser"},
        {204, nullptr, "SetUserPosition"},
        {205, &ACC_SU::GetProfileEditor, "GetProfileEditor"},
        {206, nullptr, "CompleteUserRegistrationForcibly"},
        {210, nullptr, "CreateFloatingRegistrationRequest"},
        {211, nullptr, "CreateProcedureToRegisterUserWithNintendoAccount"},
        {212, nullptr, "ResumeProcedureToRegisterUserWithNintendoAccount"},
        {230, nullptr, "AuthenticateServiceAsync"},
        {250, nullptr, "GetBaasAccountAdministrator"},
        {290, nullptr, "ProxyProcedureForGuestLoginWithNintendoAccount"},
        {291, nullptr, "ProxyProcedureForFloatingRegistrationWithNintendoAccount"},
        {299, nullptr, "SuspendBackgroundDaemon"},
        {900, nullptr, "SetUserUnqualifiedForDebug"},
        {901, nullptr, "UnsetUserUnqualifiedForDebug"},
        {902, nullptr, "ListUsersUnqualifiedForDebug"},
        {910, nullptr, "RefreshFirmwareSettingsForDebug"},
        {997, nullptr, "DebugInvalidateTokenCacheForUser"},
        {998, nullptr, "DebugSetUserStateClose"},
        {999, nullptr, "DebugSetUserStateOpen"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

ACC_SU::~ACC_SU() = default;

} // namespace Service::Account
