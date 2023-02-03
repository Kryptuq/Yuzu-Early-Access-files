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
    explicit IJitEnvironment(Core::System& system_, Kernel::KProcess& process_, CodeRange user_rx,
                             CodeRange user_ro)
        : ServiceFramework{system_, "IJitEnvironment", ServiceThreadType::CreateNew},
          process{&process_}, context{system_.Memory()} {
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
        LOG_DEBUG(Service_JIT, "called");

        struct InputParameters {
            u32 data_size;
            u64 command;
            std::array<CodeRange, 2> ranges;
            Struct32 data;
        };

        struct OutputParameters {
            s32 return_value;
            std::array<CodeRange, 2> ranges;
        };

        IPC::RequestParser rp{ctx};
        const auto parameters{rp.PopRaw<InputParameters>()};

        // Optional input/output buffers
        std::vector<u8> input_buffer{ctx.CanReadBuffer() ? ctx.ReadBuffer() : std::vector<u8>()};
        std::vector<u8> output_buffer(ctx.CanWriteBuffer() ? ctx.GetWriteBufferSize() : 0);

        // Function call prototype:
        // void GenerateCode(s32* ret, CodeRange* c0_out, CodeRange* c1_out, JITConfiguration* cfg,
        //                   u64 cmd, u8* input_buf, size_t input_size, CodeRange* c0_in,
        //                   CodeRange* c1_in, Struct32* data, size_t data_size, u8* output_buf,
        //                   size_t output_size);
        //
        // The command argument is used to control the behavior of the plugin during code
        // generation. The configuration allows the plugin to access the output code ranges, and the
        // other arguments are used to transfer state between the game and the plugin.

        const VAddr ret_ptr{context.AddHeap(0u)};
        const VAddr c0_in_ptr{context.AddHeap(parameters.ranges[0])};
        const VAddr c1_in_ptr{context.AddHeap(parameters.ranges[1])};
        const VAddr c0_out_ptr{context.AddHeap(ClearSize(parameters.ranges[0]))};
        const VAddr c1_out_ptr{context.AddHeap(ClearSize(parameters.ranges[1]))};

        const VAddr input_ptr{context.AddHeap(input_buffer.data(), input_buffer.size())};
        const VAddr output_ptr{context.AddHeap(output_buffer.data(), output_buffer.size())};
        const VAddr data_ptr{context.AddHeap(parameters.data)};
        const VAddr configuration_ptr{context.AddHeap(configuration)};

        // The callback does not directly return a value, it only writes to the output pointer
        context.CallFunction(callbacks.GenerateCode, ret_ptr, c0_out_ptr, c1_out_ptr,
                             configuration_ptr, parameters.command, input_ptr, input_buffer.size(),
                             c0_in_ptr, c1_in_ptr, data_ptr, parameters.data_size, output_ptr,
                             output_buffer.size());

        const s32 return_value{context.GetHeap<s32>(ret_ptr)};

        if (return_value == 0) {
            // The callback has written to the output executable code range,
            // requiring an instruction cache invalidation
            system.InvalidateCpuInstructionCacheRange(configuration.user_rx_memory.offset,
                                                      configuration.user_rx_memory.size);

            // Write back to the IPC output buffer, if provided
            if (ctx.CanWriteBuffer()) {
                context.GetHeap(output_ptr, output_buffer.data(), output_buffer.size());
                ctx.WriteBuffer(output_buffer.data(), output_buffer.size());
            }

            const OutputParameters out{
                .return_value = return_value,
                .ranges =
                    {
                        context.GetHeap<CodeRange>(c0_out_ptr),
                        context.GetHeap<CodeRange>(c1_out_ptr),
                    },
            };

            IPC::ResponseBuilder rb{ctx, 8};
            rb.Push(ResultSuccess);
            rb.PushRaw(out);
        } else {
            LOG_WARNING(Service_JIT, "plugin GenerateCode callback failed");
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultUnknown);
        }
    };

    void Control(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_JIT, "called");

        IPC::RequestParser rp{ctx};
        const auto command{rp.PopRaw<u64>()};

        // Optional input/output buffers
        std::vector<u8> input_buffer{ctx.CanReadBuffer() ? ctx.ReadBuffer() : std::vector<u8>()};
        std::vector<u8> output_buffer(ctx.CanWriteBuffer() ? ctx.GetWriteBufferSize() : 0);

        // Function call prototype:
        // u64 Control(s32* ret, JITConfiguration* cfg, u64 cmd, u8* input_buf, size_t input_size,
        //             u8* output_buf, size_t output_size);
        //
        // This function is used to set up the state of the plugin before code generation, generally
        // passing objects like pointers to VM state from the game. It is usually called once.

        const VAddr ret_ptr{context.AddHeap(0u)};
        const VAddr configuration_ptr{context.AddHeap(configuration)};
        const VAddr input_ptr{context.AddHeap(input_buffer.data(), input_buffer.size())};
        const VAddr output_ptr{context.AddHeap(output_buffer.data(), output_buffer.size())};

        const u64 wrapper_value{context.CallFunction(callbacks.Control, ret_ptr, configuration_ptr,
                                                     command, input_ptr, input_buffer.size(),
                                                     output_ptr, output_buffer.size())};

        const s32 return_value{context.GetHeap<s32>(ret_ptr)};

        if (wrapper_value == 0 && return_value == 0) {
            // Write back to the IPC output buffer, if provided
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
        LOG_DEBUG(Service_JIT, "called");

        IPC::RequestParser rp{ctx};
        const auto tmem_size{rp.PopRaw<u64>()};
        const auto tmem_handle{ctx.GetCopyHandle(0)};
        const auto nro_plugin{ctx.ReadBuffer(1)};

        if (tmem_size == 0) {
            LOG_ERROR(Service_JIT, "attempted to load plugin with empty transfer memory");
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultUnknown);
            return;
        }

        auto tmem{process->GetHandleTable().GetObject<Kernel::KTransferMemory>(tmem_handle)};
        if (tmem.IsNull()) {
            LOG_ERROR(Service_JIT, "attempted to load plugin with invalid transfer memory handle");
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultUnknown);
            return;
        }

        // Set up the configuration with the required TransferMemory address
        configuration.transfer_memory.offset = tmem->GetSourceAddress();
        configuration.transfer_memory.size = tmem_size;

        // Gather up all the callbacks from the loaded plugin
        auto symbols{Core::Symbols::GetSymbols(nro_plugin, true)};
        const auto GetSymbol{[&](const std::string& name) { return symbols[name].first; }};

        callbacks.rtld_fini = GetSymbol("_fini");
        callbacks.rtld_init = GetSymbol("_init");
        callbacks.Control = GetSymbol("nnjitpluginControl");
        callbacks.ResolveBasicSymbols = GetSymbol("nnjitpluginResolveBasicSymbols");
        callbacks.SetupDiagnostics = GetSymbol("nnjitpluginSetupDiagnostics");
        callbacks.Configure = GetSymbol("nnjitpluginConfigure");
        callbacks.GenerateCode = GetSymbol("nnjitpluginGenerateCode");
        callbacks.GetVersion = GetSymbol("nnjitpluginGetVersion");
        callbacks.OnPrepared = GetSymbol("nnjitpluginOnPrepared");
        callbacks.Keeper = GetSymbol("nnjitpluginKeeper");

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
        context.MapProcessMemory(configuration.transfer_memory.offset,
                                 configuration.transfer_memory.size);

        // Run ELF constructors, if needed
        if (callbacks.rtld_init != 0) {
            context.CallFunction(callbacks.rtld_init);
        }

        // Function prototype:
        // u64 GetVersion();
        const auto version{context.CallFunction(callbacks.GetVersion)};
        if (version != 1) {
            LOG_ERROR(Service_JIT, "unknown plugin version {}", version);
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultUnknown);
            return;
        }

        // Function prototype:
        // void ResolveBasicSymbols(void (*resolver)(const char* name));
        const auto resolve{context.GetHelper("_resolve")};
        if (callbacks.ResolveBasicSymbols != 0) {
            context.CallFunction(callbacks.ResolveBasicSymbols, resolve);
        }

        // Function prototype:
        // void SetupDiagnostics(u32 enabled, void (**resolver)(const char* name));
        const auto resolve_ptr{context.AddHeap(resolve)};
        if (callbacks.SetupDiagnostics != 0) {
            context.CallFunction(callbacks.SetupDiagnostics, 0u, resolve_ptr);
        }

        // Function prototype:
        // void Configure(u32* memory_flags);
        context.CallFunction(callbacks.Configure, 0ull);

        // Function prototype:
        // void OnPrepared(JITConfiguration* cfg);
        const auto configuration_ptr{context.AddHeap(configuration)};
        context.CallFunction(callbacks.OnPrepared, configuration_ptr);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void GetCodeAddress(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_JIT, "called");

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
        CodeRange transfer_memory;
        CodeRange sys_rx_memory;
        CodeRange sys_ro_memory;
    };

    static CodeRange ClearSize(CodeRange in) {
        in.size = 0;
        return in;
    }

    Kernel::KScopedAutoObject<Kernel::KProcess> process;
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
        const auto process_handle{ctx.GetCopyHandle(0)};
        const auto rx_mem_handle{ctx.GetCopyHandle(1)};
        const auto ro_mem_handle{ctx.GetCopyHandle(2)};

        if (parameters.rx_size == 0 || parameters.ro_size == 0) {
            LOG_ERROR(Service_JIT, "attempted to init with empty code regions");
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultUnknown);
            return;
        }

        // Fetch using the handle table for the current process here,
        // since we are not multiprocess yet.
        const auto& handle_table{system.CurrentProcess()->GetHandleTable()};

        auto process{handle_table.GetObject<Kernel::KProcess>(process_handle)};
        if (process.IsNull()) {
            LOG_ERROR(Service_JIT, "process is null for handle=0x{:08X}", process_handle);
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultUnknown);
            return;
        }

        auto rx_mem{handle_table.GetObject<Kernel::KCodeMemory>(rx_mem_handle)};
        if (rx_mem.IsNull()) {
            LOG_ERROR(Service_JIT, "rx_mem is null for handle=0x{:08X}", rx_mem_handle);
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultUnknown);
            return;
        }

        auto ro_mem{handle_table.GetObject<Kernel::KCodeMemory>(ro_mem_handle)};
        if (ro_mem.IsNull()) {
            LOG_ERROR(Service_JIT, "ro_mem is null for handle=0x{:08X}", ro_mem_handle);
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultUnknown);
            return;
        }

        const CodeRange user_rx{
            .offset = rx_mem->GetSourceAddress(),
            .size = parameters.rx_size,
        };

        const CodeRange user_ro{
            .offset = ro_mem->GetSourceAddress(),
            .size = parameters.ro_size,
        };

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<IJitEnvironment>(system, *process, user_rx, user_ro);
    }
};

void InstallInterfaces(SM::ServiceManager& sm, Core::System& system) {
    std::make_shared<JITU>(system)->InstallAsService(sm);
}

} // namespace Service::JIT
