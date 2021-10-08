// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/ipc_helpers.h"
#include "core/hle/service/service.h"
#include "core/hle/service/sm/sm.h"
#include "core/hle/service/ssl/ssl.h"

namespace Service::SSL {

enum class CertificateFormat : u32 {
    Pem = 1,
    Der = 2,
};

class ISslConnection final : public ServiceFramework<ISslConnection> {
public:
    explicit ISslConnection(Core::System& system_) : ServiceFramework{system_, "ISslConnection"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "SetSocketDescriptor"},
            {1, nullptr, "SetHostName"},
            {2, nullptr, "SetVerifyOption"},
            {3, nullptr, "SetIoMode"},
            {4, nullptr, "GetSocketDescriptor"},
            {5, nullptr, "GetHostName"},
            {6, nullptr, "GetVerifyOption"},
            {7, nullptr, "GetIoMode"},
            {8, nullptr, "DoHandshake"},
            {9, nullptr, "DoHandshakeGetServerCert"},
            {10, nullptr, "Read"},
            {11, nullptr, "Write"},
            {12, nullptr, "Pending"},
            {13, nullptr, "Peek"},
            {14, nullptr, "Poll"},
            {15, nullptr, "GetVerifyCertError"},
            {16, nullptr, "GetNeededServerCertBufferSize"},
            {17, nullptr, "SetSessionCacheMode"},
            {18, nullptr, "GetSessionCacheMode"},
            {19, nullptr, "FlushSessionCache"},
            {20, nullptr, "SetRenegotiationMode"},
            {21, nullptr, "GetRenegotiationMode"},
            {22, nullptr, "SetOption"},
            {23, nullptr, "GetOption"},
            {24, nullptr, "GetVerifyCertErrors"},
            {25, nullptr, "GetCipherInfo"},
            {26, nullptr, "SetNextAlpnProto"},
            {27, nullptr, "GetNextAlpnProto"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class ISslContext final : public ServiceFramework<ISslContext> {
public:
    explicit ISslContext(Core::System& system_) : ServiceFramework{system_, "ISslContext"} {
        static const FunctionInfo functions[] = {
            {0, &ISslContext::SetOption, "SetOption"},
            {1, nullptr, "GetOption"},
            {2, &ISslContext::CreateConnection, "CreateConnection"},
            {3, nullptr, "GetConnectionCount"},
            {4, &ISslContext::ImportServerPki, "ImportServerPki"},
            {5, &ISslContext::ImportClientPki, "ImportClientPki"},
            {6, nullptr, "RemoveServerPki"},
            {7, nullptr, "RemoveClientPki"},
            {8, nullptr, "RegisterInternalPki"},
            {9, nullptr, "AddPolicyOid"},
            {10, nullptr, "ImportCrl"},
            {11, nullptr, "RemoveCrl"},
        };
        RegisterHandlers(functions);
    }

private:
    void SetOption(Kernel::HLERequestContext& ctx) {
        struct Parameters {
            u8 enable;
            u32 option;
        };

        IPC::RequestParser rp{ctx};
        const auto parameters = rp.PopRaw<Parameters>();

        LOG_WARNING(Service_SSL, "(STUBBED) called. enable={}, option={}", parameters.enable,
                    parameters.option);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void CreateConnection(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_SSL, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<ISslConnection>(system);
    }

    void ImportServerPki(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto certificate_format = rp.PopEnum<CertificateFormat>();
        const auto pkcs_12_certificates = ctx.ReadBuffer(0);

        constexpr u64 server_id = 0;

        LOG_WARNING(Service_SSL, "(STUBBED) called, certificate_format={}", certificate_format);

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(ResultSuccess);
        rb.Push(server_id);
    }

    void ImportClientPki(Kernel::HLERequestContext& ctx) {
        const auto pkcs_12_certificate = ctx.ReadBuffer(0);
        const auto ascii_password = [&ctx] {
            if (ctx.CanReadBuffer(1)) {
                return ctx.ReadBuffer(1);
            }

            return std::vector<u8>{};
        }();

        constexpr u64 client_id = 0;

        LOG_WARNING(Service_SSL, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(ResultSuccess);
        rb.Push(client_id);
    }
};

class SSL final : public ServiceFramework<SSL> {
public:
    explicit SSL(Core::System& system_) : ServiceFramework{system_, "ssl"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &SSL::CreateContext, "CreateContext"},
            {1, nullptr, "GetContextCount"},
            {2, nullptr, "GetCertificates"},
            {3, nullptr, "GetCertificateBufSize"},
            {4, nullptr, "DebugIoctl"},
            {5, &SSL::SetInterfaceVersion, "SetInterfaceVersion"},
            {6, nullptr, "FlushSessionCache"},
            {7, nullptr, "SetDebugOption"},
            {8, nullptr, "GetDebugOption"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    u32 ssl_version{};
    void CreateContext(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_SSL, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<ISslContext>(system);
    }

    void SetInterfaceVersion(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_SSL, "called");

        IPC::RequestParser rp{ctx};
        ssl_version = rp.Pop<u32>();

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }
};

void InstallInterfaces(SM::ServiceManager& service_manager, Core::System& system) {
    std::make_shared<SSL>(system)->InstallAsService(service_manager);
}

} // namespace Service::SSL
