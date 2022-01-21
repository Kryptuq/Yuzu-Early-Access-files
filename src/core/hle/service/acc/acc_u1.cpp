// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/acc/acc_u1.h"

namespace Service::Account {

ACC_U1::ACC_U1(std::shared_ptr<Module> module_, std::shared_ptr<ProfileManager> profile_manager_,
               Core::System& system_)
    : Interface(std::move(module_), std::move(profile_manager_), system_, "acc:u1") {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, &ACC_U1::GetUserCount, "GetUserCount"},
        {1, &ACC_U1::GetUserExistence, "GetUserExistence"},
        {2, &ACC_U1::ListAllUsers, "ListAllUsers"},
        {3, &ACC_U1::ListOpenUsers, "ListOpenUsers"},
        {4, &ACC_U1::GetLastOpenedUser, "GetLastOpenedUser"},
        {5, &ACC_U1::GetProfile, "GetProfile"},
        {6, nullptr, "GetProfileDigest"},
        {50, &ACC_U1::IsUserRegistrationRequestPermitted, "IsUserRegistrationRequestPermitted"},
        {51, &ACC_U1::TrySelectUserWithoutInteraction, "TrySelectUserWithoutInteraction"},
        {60, &ACC_U1::ListOpenContextStoredUsers, "ListOpenContextStoredUsers"},
        {99, nullptr, "DebugActivateOpenContextRetention"},
        {100, nullptr, "GetUserRegistrationNotifier"},
        {101, nullptr, "GetUserStateChangeNotifier"},
        {102, nullptr, "GetBaasAccountManagerForSystemService"},
        {103, nullptr, "GetBaasUserAvailabilityChangeNotifier"},
        {104, nullptr, "GetProfileUpdateNotifier"},
        {105, nullptr, "CheckNetworkServiceAvailabilityAsync"},
        {106, nullptr, "GetProfileSyncNotifier"},
        {110, &ACC_U1::StoreSaveDataThumbnailApplication, "StoreSaveDataThumbnail"},
        {111, nullptr, "ClearSaveDataThumbnail"},
        {112, nullptr, "LoadSaveDataThumbnail"},
        {113, nullptr, "GetSaveDataThumbnailExistence"},
        {120, nullptr, "ListOpenUsersInApplication"},
        {130, nullptr, "ActivateOpenContextRetention"},
        {140, &ACC_U1::ListQualifiedUsers, "ListQualifiedUsers"},
        {150, nullptr, "AuthenticateApplicationAsync"},
        {151, nullptr, "EnsureSignedDeviceIdentifierCacheForNintendoAccountAsync"},
        {152, nullptr, "LoadSignedDeviceIdentifierCacheForNintendoAccount"},
        {190, nullptr, "GetUserLastOpenedApplication"},
        {191, nullptr, "ActivateOpenContextHolder"},
        {997, nullptr, "DebugInvalidateTokenCacheForUser"},
        {998, nullptr, "DebugSetUserStateClose"},
        {999, nullptr, "DebugSetUserStateOpen"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

ACC_U1::~ACC_U1() = default;

} // namespace Service::Account
