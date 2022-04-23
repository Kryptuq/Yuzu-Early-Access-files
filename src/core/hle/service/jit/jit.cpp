// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/arm/symbols.h"
#include "core/core.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/k_code_memory.h"
#include "core/hle/kernel/k_transfer_memory.h"
#include "core/hle/result.h"
#include "core/hle/service/jit/jit.h"
#include "core/hle/service/jit/jit_context.h"
#include "core/hle/service/service.h"
#include "core/memory.h"

namespace Service::JIT {

struct CodeRange {
    u64 offset;
    u64 size;
};

class IJitEnvironment final : public ServiceFramework<IJitEnvironment> {
public:
    explicit IJitEnvironment(Core::System& system_, CodeRange user_rx, CodeRange user_ro)
        : ServiceFramework{system_, "IJitEnvironment", ServiceThreadType::CreateNew},
          context{system_.Memory()} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &IJitEnvironment::GenerateCode, "GenerateCode"},
            {1, &IJitEnvironment::Control, "Control"},
            {1000, &IJitEnvironment::LoadPlugin, "LoadPlugin"},
            {1001, &IJitEnvironment::GetCodeAddress, "GetCodeAddress"},
        };
        // clang-format on

        RegisterHandlers(functions);

        // Identity map user code range into sysmodule context
        configuration.user_ro_memory = user_ro;
        configuration.user_rx_memory = user_rx;
        configuration.sys_ro_memory = user_ro;
        configuration.sys_rx_memory = user_rx;
    }

    void GenerateCode(Kernel::HLERequestContext& ctx) {
        struct Parameters {
            u32 data_size;
            u64 command;
            CodeRange cr1;
            CodeRange cr2;
            Struct32 data;
        };

        IPC::RequestParser rp{ctx};
        const auto parameters{rp.PopRaw<Parameters>()};
        std::vector<u8> input_buffer{ctx.CanReadBuffer() ? ctx.ReadBuffer() : std::vector<u8>()};
        std::vector<u8> output_buffer(ctx.CanWriteBuffer() ? ctx.GetWriteBufferSize() : 0);

        const VAddr return_ptr{context.AddHeap(0u)};
        const VAddr cr1_in_ptr{context.AddHeap(parameters.cr1)};
        const VAddr cr2_in_ptr{context.AddHeap(parameters.cr2)};
        const VAddr cr1_out_ptr{
            context.AddHeap(CodeRange{.offset = parameters.cr1.offset, .size = 0})};
        const VAddr cr2_out_ptr{
            context.AddHeap(CodeRange{.offset = parameters.cr2.offset, .size = 0})};
        const VAddr input_ptr{context.AddHeap(input_buffer.data(), input_buffer.size())};
        const VAddr output_ptr{context.AddHeap(output_buffer.data(), output_buffer.size())};
        const VAddr data_ptr{context.AddHeap(parameters.data)};
        const VAddr configuration_ptr{context.AddHeap(configuration)};

        context.CallFunction(callbacks.GenerateCode, return_ptr, cr1_out_ptr, cr2_out_ptr,
                             configuration_ptr, parameters.command, input_ptr, input_buffer.size(),
                             cr1_in_ptr, cr2_in_ptr, data_ptr, parameters.data_size, output_ptr,
                             output_buffer.size());

        const s32 return_value{context.GetHeap<s32>(return_ptr)};

        if (return_value == 0) {
            system.InvalidateCpuInstructionCacheRange(configuration.user_rx_memory.offset,
                                                      configuration.user_rx_memory.size);

            if (ctx.CanWriteBuffer()) {
                context.GetHeap(output_ptr, output_buffer.data(), output_buffer.size());
                ctx.WriteBuffer(output_buffer.data(), output_buffer.size());
            }
            const auto cr1_out{context.GetHeap<CodeRange>(cr1_out_ptr)};
            const auto cr2_out{context.GetHeap<CodeRange>(cr2_out_ptr)};

            IPC::ResponseBuilder rb{ctx, 8};
            rb.Push(ResultSuccess);
            rb.Push<u64>(return_value);
            rb.PushRaw(cr1_out);
            rb.PushRaw(cr2_out);
        } else {
            LOG_WARNING(Service_JIT, "plugin GenerateCode callback failed");
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultUnknown);
        }
    };

    void Control(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto command{rp.PopRaw<u64>()};
        const auto input_buffer{ctx.ReadBuffer()};
        std::vector<u8> output_buffer(ctx.CanWriteBuffer() ? ctx.GetWriteBufferSize() : 0);

        const VAddr return_ptr{context.AddHeap(0u)};
        const VAddr configuration_ptr{context.AddHeap(configuration)};
        const VAddr input_ptr{context.AddHeap(input_buffer.data(), input_buffer.size())};
        const VAddr output_ptr{context.AddHeap(output_buffer.data(), output_buffer.size())};
        const u64 wrapper_value{
            context.CallFunction(callbacks.Control, return_ptr, configuration_ptr, command,
                                 input_ptr, input_buffer.size(), output_ptr, output_buffer.size())};
        const s32 return_value{context.GetHeap<s32>(return_ptr)};

        if (wrapper_value == 0 && return_value == 0) {
            if (ctx.CanWriteBuffer()) {
                context.GetHeap(output_ptr, output_buffer.data(), output_buffer.size());
                ctx.WriteBuffer(output_buffer.data(), output_buffer.size());
            }
            IPC::ResponseBuilder rb{ctx, 3};
            rb.Push(ResultSuccess);
            rb.Push(return_value);
        } else {
            LOG_WARNING(Service_JIT, "plugin Control callback failed");
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultUnknown);
        }
    }

    void LoadPlugin(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto tmem_size{rp.PopRaw<u64>()};
        if (tmem_size == 0) {
            LOG_ERROR(Service_JIT, "attempted to load plugin with empty transfer memory");
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultUnknown);
            return;
        }

        const auto tmem_handle{ctx.GetCopyHandle(0)};
        auto tmem{system.CurrentProcess()->GetHandleTable().GetObject<Kernel::KTransferMemory>(
            tmem_handle)};
        if (tmem.IsNull()) {
            LOG_ERROR(Service_JIT, "attempted to load plugin with invalid transfer memory handle");
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultUnknown);
            return;
        }

        configuration.work_memory.offset = tmem->GetSourceAddress();
        configuration.work_memory.size = tmem_size;

        const auto nro_plugin{ctx.ReadBuffer(1)};
        auto symbols{Core::Symbols::GetSymbols(nro_plugin, true)};
        const auto GetSymbol{[&](std::string name) { return symbols[name].first; }};

        callbacks =
            GuestCallbacks{.rtld_fini = GetSymbol("_fini"),
                           .rtld_init = GetSymbol("_init"),
                           .Control = GetSymbol("nnjitpluginControl"),
                           .ResolveBasicSymbols = GetSymbol("nnjitpluginResolveBasicSymbols"),
                           .SetupDiagnostics = GetSymbol("nnjitpluginSetupDiagnostics"),
                           .Configure = GetSymbol("nnjitpluginConfigure"),
                           .GenerateCode = GetSymbol("nnjitpluginGenerateCode"),
                           .GetVersion = GetSymbol("nnjitpluginGetVersion"),
                           .Keeper = GetSymbol("nnjitpluginKeeper"),
                           .OnPrepared = GetSymbol("nnjitpluginOnPrepared")};

        if (callbacks.GetVersion == 0 || callbacks.Configure == 0 || callbacks.GenerateCode == 0 ||
            callbacks.OnPrepared == 0) {
            LOG_ERROR(Service_JIT, "plugin does not implement all necessary functionality");
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultUnknown);
            return;
        }

        if (!context.LoadNRO(nro_plugin)) {
            LOG_ERROR(Service_JIT, "failed to load plugin");
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultUnknown);
            return;
        }

        context.MapProcessMemory(configuration.sys_ro_memory.offset,
                                 configuration.sys_ro_memory.size);
        context.MapProcessMemory(configuration.sys_rx_memory.offset,
                                 configuration.sys_rx_memory.size);
        context.MapProcessMemory(configuration.work_memory.offset, configuration.work_memory.size);

        if (callbacks.rtld_init != 0) {
            context.CallFunction(callbacks.rtld_init);
        }

        const auto version{context.CallFunction(callbacks.GetVersion)};
        if (version != 1) {
            LOG_ERROR(Service_JIT, "unknown plugin version {}", version);
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultUnknown);
            return;
        }

        const auto resolve{context.GetHelper("_resolve")};
        if (callbacks.ResolveBasicSymbols != 0) {
            context.CallFunction(callbacks.ResolveBasicSymbols, resolve);
        }
        const auto resolve_ptr{context.AddHeap(resolve)};
        if (callbacks.SetupDiagnostics != 0) {
            context.CallFunction(callbacks.SetupDiagnostics, 0u, resolve_ptr);
        }

        context.CallFunction(callbacks.Configure, 0u);
        const auto configuration_ptr{context.AddHeap(configuration)};
        context.CallFunction(callbacks.OnPrepared, configuration_ptr);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void GetCodeAddress(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 6};
        rb.Push(ResultSuccess);
        rb.Push(configuration.user_rx_memory.offset);
        rb.Push(configuration.user_ro_memory.offset);
    }

private:
    using Struct32 = std::array<u8, 32>;

    struct GuestCallbacks {
        VAddr rtld_fini;
        VAddr rtld_init;
        VAddr Control;
        VAddr ResolveBasicSymbols;
        VAddr SetupDiagnostics;
        VAddr Configure;
        VAddr GenerateCode;
        VAddr GetVersion;
        VAddr Keeper;
        VAddr OnPrepared;
    };

    struct JITConfiguration {
        CodeRange user_rx_memory;
        CodeRange user_ro_memory;
        CodeRange work_memory;
        CodeRange sys_rx_memory;
        CodeRange sys_ro_memory;
    };

    GuestCallbacks callbacks;
    JITConfiguration configuration;
    JITContext context;
};

class JITU final : public ServiceFramework<JITU> {
public:
    explicit JITU(Core::System& system_) : ServiceFramework{system_, "jit:u"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &JITU::CreateJitEnvironment, "CreateJitEnvironment"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

    void CreateJitEnvironment(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_JIT, "called");

        struct Parameters {
            u64 rx_size;
            u64 ro_size;
        };

        IPC::RequestParser rp{ctx};
        const auto parameters{rp.PopRaw<Parameters>()};
        const auto executable_mem_handle{ctx.GetCopyHandle(1)};
        const auto readable_mem_handle{ctx.GetCopyHandle(2)};

        if (parameters.rx_size == 0 || parameters.ro_size == 0) {
            LOG_ERROR(Service_JIT, "attempted to init with empty code regions");
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultUnknown);
            return;
        }

        // The copy handle at index 0 is the process handle, but handle tables are
        // per-process, so there is no point reading it here until we are multiprocess
        const auto& process{*system.CurrentProcess()};

        auto executable_mem{
            process.GetHandleTable().GetObject<Kernel::KCodeMemory>(executable_mem_handle)};
        if (executable_mem.IsNull()) {
            LOG_ERROR(Service_JIT, "executable_mem is null for handle=0x{:08X}",
                      executable_mem_handle);
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultUnknown);
            return;
        }

        auto readable_mem{
            process.GetHandleTable().GetObject<Kernel::KCodeMemory>(readable_mem_handle)};
        if (readable_mem.IsNull()) {
            LOG_ERROR(Service_JIT, "readable_mem is null for handle=0x{:08X}", readable_mem_handle);
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultUnknown);
            return;
        }

        const CodeRange user_rx{
            .offset = executable_mem->GetSourceAddress(),
            .size = parameters.rx_size,
        };

        const CodeRange user_ro{
            .offset = readable_mem->GetSourceAddress(),
            .size = parameters.ro_size,
        };

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<IJitEnvironment>(system, user_rx, user_ro);
    }
};

void InstallInterfaces(SM::ServiceManager& sm, Core::System& system) {
    std::make_shared<JITU>(system)->InstallAsService(sm);
}

} // namespace Service::JIT
