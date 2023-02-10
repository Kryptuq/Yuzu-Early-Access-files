// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/scope_exit.h"
#include "core/core.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_scoped_resource_reservation.h"
#include "core/hle/kernel/k_session.h"
#include "core/hle/kernel/svc.h"

namespace Kernel::Svc {
namespace {

template <typename T>
Result CreateSession(Core::System& system, Handle* out_server, Handle* out_client, u64 name) {
    auto& process = *system.CurrentProcess();
    auto& handle_table = process.GetHandleTable();

    // Declare the session we're going to allocate.
    T* session;

    // Reserve a new session from the process resource limit.
    // FIXME: LimitableResource_SessionCountMax
    KScopedResourceReservation session_reservation(&process, LimitableResource::SessionCountMax);
    if (session_reservation.Succeeded()) {
        session = T::Create(system.Kernel());
    } else {
        return ResultLimitReached;

        // // We couldn't reserve a session. Check that we support dynamically expanding the
        // // resource limit.
        // R_UNLESS(process.GetResourceLimit() ==
        //          &system.Kernel().GetSystemResourceLimit(), ResultLimitReached);
        // R_UNLESS(KTargetSystem::IsDynamicResourceLimitsEnabled(), ResultLimitReached());

        // // Try to allocate a session from unused slab memory.
        // session = T::CreateFromUnusedSlabMemory();
        // R_UNLESS(session != nullptr, ResultLimitReached);
        // ON_RESULT_FAILURE { session->Close(); };

        // // If we're creating a KSession, we want to add two KSessionRequests to the heap, to
        // // prevent request exhaustion.
        // // NOTE: Nintendo checks if session->DynamicCast<KSession *>() != nullptr, but there's
        // // no reason to not do this statically.
        // if constexpr (std::same_as<T, KSession>) {
        //     for (size_t i = 0; i < 2; i++) {
        //         KSessionRequest* request = KSessionRequest::CreateFromUnusedSlabMemory();
        //         R_UNLESS(request != nullptr, ResultLimitReached);
        //         request->Close();
        //     }
        // }

        // We successfully allocated a session, so add the object we allocated to the resource
        // limit.
        // system.Kernel().GetSystemResourceLimit().Reserve(LimitableResource::SessionCountMax, 1);
    }

    // Check that we successfully created a session.
    R_UNLESS(session != nullptr, ResultOutOfResource);

    // Initialize the session.
    session->Initialize(nullptr, fmt::format("{}", name));

    // Commit the session reservation.
    session_reservation.Commit();

    // Ensure that we clean up the session (and its only references are handle table) on function
    // end.
    SCOPE_EXIT({
        session->GetClientSession().Close();
        session->GetServerSession().Close();
    });

    // Register the session.
    T::Register(system.Kernel(), session);

    // Add the server session to the handle table.
    R_TRY(handle_table.Add(out_server, &session->GetServerSession()));

    // Add the client session to the handle table.
    const auto result = handle_table.Add(out_client, &session->GetClientSession());

    if (!R_SUCCEEDED(result)) {
        // Ensure that we maintaing a clean handle state on exit.
        handle_table.Remove(*out_server);
    }

    return result;
}

} // namespace

Result CreateSession(Core::System& system, Handle* out_server, Handle* out_client, bool is_light,
                     u64 name) {
    if (is_light) {
        // return CreateSession<KLightSession>(system, out_server, out_client, name);
        return ResultNotImplemented;
    } else {
        return CreateSession<KSession>(system, out_server, out_client, name);
    }
}

Result AcceptSession(Core::System& system, Handle* out_handle, Handle port_handle) {
    UNIMPLEMENTED();
    R_THROW(ResultNotImplemented);
}

Result CreateSession64(Core::System& system, Handle* out_server_session_handle,
                       Handle* out_client_session_handle, bool is_light, uint64_t name) {
    R_RETURN(CreateSession(system, out_server_session_handle, out_client_session_handle, is_light,
                           name));
}

Result AcceptSession64(Core::System& system, Handle* out_handle, Handle port) {
    R_RETURN(AcceptSession(system, out_handle, port));
}

Result CreateSession64From32(Core::System& system, Handle* out_server_session_handle,
                             Handle* out_client_session_handle, bool is_light, uint32_t name) {
    R_RETURN(CreateSession(system, out_server_session_handle, out_client_session_handle, is_light,
                           name));
}

Result AcceptSession64From32(Core::System& system, Handle* out_handle, Handle port) {
    R_RETURN(AcceptSession(system, out_handle, port));
}

} // namespace Kernel::Svc
