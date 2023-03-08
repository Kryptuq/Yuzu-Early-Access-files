// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fmt/format.h>
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/settings.h"
#include "core/core.h"
#include "core/hle/ipc.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_server_port.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/service/acc/acc.h"
#include "core/hle/service/am/am.h"
#include "core/hle/service/aoc/aoc_u.h"
#include "core/hle/service/apm/apm.h"
#include "core/hle/service/audio/audio.h"
#include "core/hle/service/bcat/bcat_module.h"
#include "core/hle/service/bpc/bpc.h"
#include "core/hle/service/btdrv/btdrv.h"
#include "core/hle/service/btm/btm.h"
#include "core/hle/service/caps/caps.h"
#include "core/hle/service/erpt/erpt.h"
#include "core/hle/service/es/es.h"
#include "core/hle/service/eupld/eupld.h"
#include "core/hle/service/fatal/fatal.h"
#include "core/hle/service/fgm/fgm.h"
#include "core/hle/service/filesystem/filesystem.h"
#include "core/hle/service/friend/friend.h"
#include "core/hle/service/glue/glue.h"
#include "core/hle/service/grc/grc.h"
#include "core/hle/service/hid/hid.h"
#include "core/hle/service/ipc_helpers.h"
#include "core/hle/service/jit/jit.h"
#include "core/hle/service/lbl/lbl.h"
#include "core/hle/service/ldn/ldn.h"
#include "core/hle/service/ldr/ldr.h"
#include "core/hle/service/lm/lm.h"
#include "core/hle/service/mig/mig.h"
#include "core/hle/service/mii/mii.h"
#include "core/hle/service/mm/mm_u.h"
#include "core/hle/service/mnpp/mnpp_app.h"
#include "core/hle/service/ncm/ncm.h"
#include "core/hle/service/nfc/nfc.h"
#include "core/hle/service/nfp/nfp.h"
#include "core/hle/service/ngct/ngct.h"
#include "core/hle/service/nifm/nifm.h"
#include "core/hle/service/nim/nim.h"
#include "core/hle/service/npns/npns.h"
#include "core/hle/service/ns/ns.h"
#include "core/hle/service/nvdrv/nvdrv.h"
#include "core/hle/service/nvnflinger/hos_binder_driver_server.h"
#include "core/hle/service/nvnflinger/nvnflinger.h"
#include "core/hle/service/olsc/olsc.h"
#include "core/hle/service/pcie/pcie.h"
#include "core/hle/service/pctl/pctl_module.h"
#include "core/hle/service/pcv/pcv.h"
#include "core/hle/service/pm/pm.h"
#include "core/hle/service/prepo/prepo.h"
#include "core/hle/service/psc/psc.h"
#include "core/hle/service/ptm/ptm.h"
#include "core/hle/service/service.h"
#include "core/hle/service/set/settings.h"
#include "core/hle/service/sm/sm.h"
#include "core/hle/service/sockets/sockets.h"
#include "core/hle/service/spl/spl_module.h"
#include "core/hle/service/ssl/ssl.h"
#include "core/hle/service/time/time.h"
#include "core/hle/service/usb/usb.h"
#include "core/hle/service/vi/vi.h"
#include "core/reporter.h"

namespace Service {

/**
 * Creates a function string for logging, complete with the name (or header code, depending
 * on what's passed in) the port name, and all the cmd_buff arguments.
 */
[[maybe_unused]] static std::string MakeFunctionString(std::string_view name,
                                                       std::string_view port_name,
                                                       const u32* cmd_buff) {
    // Number of params == bits 0-5 + bits 6-11
    int num_params = (cmd_buff[0] & 0x3F) + ((cmd_buff[0] >> 6) & 0x3F);

    std::string function_string = fmt::format("function '{}': port={}", name, port_name);
    for (int i = 1; i <= num_params; ++i) {
        function_string += fmt::format(", cmd_buff[{}]=0x{:X}", i, cmd_buff[i]);
    }
    return function_string;
}

ServiceFrameworkBase::ServiceFrameworkBase(Core::System& system_, const char* service_name_,
                                           u32 max_sessions_, InvokerFn* handler_invoker_)
    : SessionRequestHandler(system_.Kernel(), service_name_), system{system_},
      service_name{service_name_}, max_sessions{max_sessions_}, handler_invoker{handler_invoker_} {}

ServiceFrameworkBase::~ServiceFrameworkBase() {
    // Wait for other threads to release access before destroying
    const auto guard = LockService();
}

void ServiceFrameworkBase::RegisterHandlersBase(const FunctionInfoBase* functions, std::size_t n) {
    handlers.reserve(handlers.size() + n);
    for (std::size_t i = 0; i < n; ++i) {
        // Usually this array is sorted by id already, so hint to insert at the end
        handlers.emplace_hint(handlers.cend(), functions[i].expected_header, functions[i]);
    }
}

void ServiceFrameworkBase::RegisterHandlersBaseTipc(const FunctionInfoBase* functions,
                                                    std::size_t n) {
    handlers_tipc.reserve(handlers_tipc.size() + n);
    for (std::size_t i = 0; i < n; ++i) {
        // Usually this array is sorted by id already, so hint to insert at the end
        handlers_tipc.emplace_hint(handlers_tipc.cend(), functions[i].expected_header,
                                   functions[i]);
    }
}

void ServiceFrameworkBase::ReportUnimplementedFunction(HLERequestContext& ctx,
                                                       const FunctionInfoBase* info) {
    auto cmd_buf = ctx.CommandBuffer();
    std::string function_name = info == nullptr ? fmt::format("{}", ctx.GetCommand()) : info->name;

    fmt::memory_buffer buf;
    fmt::format_to(std::back_inserter(buf), "function '{}': port='{}' cmd_buf={{[0]=0x{:X}",
                   function_name, service_name, cmd_buf[0]);
    for (int i = 1; i <= 8; ++i) {
        fmt::format_to(std::back_inserter(buf), ", [{}]=0x{:X}", i, cmd_buf[i]);
    }
    buf.push_back('}');

    system.GetReporter().SaveUnimplementedFunctionReport(ctx, ctx.GetCommand(), function_name,
                                                         service_name);
    UNIMPLEMENTED_MSG("Unknown / unimplemented {}", fmt::to_string(buf));
    if (Settings::values.use_auto_stub) {
        LOG_WARNING(Service, "Using auto stub fallback!");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }
}

void ServiceFrameworkBase::InvokeRequest(HLERequestContext& ctx) {
    auto itr = handlers.find(ctx.GetCommand());
    const FunctionInfoBase* info = itr == handlers.end() ? nullptr : &itr->second;
    if (info == nullptr || info->handler_callback == nullptr) {
        return ReportUnimplementedFunction(ctx, info);
    }

    LOG_TRACE(Service, "{}", MakeFunctionString(info->name, GetServiceName(), ctx.CommandBuffer()));
    handler_invoker(this, info->handler_callback, ctx);
}

void ServiceFrameworkBase::InvokeRequestTipc(HLERequestContext& ctx) {
    boost::container::flat_map<u32, FunctionInfoBase>::iterator itr;

    itr = handlers_tipc.find(ctx.GetCommand());

    const FunctionInfoBase* info = itr == handlers_tipc.end() ? nullptr : &itr->second;
    if (info == nullptr || info->handler_callback == nullptr) {
        return ReportUnimplementedFunction(ctx, info);
    }

    LOG_TRACE(Service, "{}", MakeFunctionString(info->name, GetServiceName(), ctx.CommandBuffer()));
    handler_invoker(this, info->handler_callback, ctx);
}

Result ServiceFrameworkBase::HandleSyncRequest(Kernel::KServerSession& session,
                                               HLERequestContext& ctx) {
    const auto guard = LockService();

    Result result = ResultSuccess;

    switch (ctx.GetCommandType()) {
    case IPC::CommandType::Close:
    case IPC::CommandType::TIPC_Close: {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
        result = IPC::ResultSessionClosed;
        break;
    }
    case IPC::CommandType::ControlWithContext:
    case IPC::CommandType::Control: {
        system.ServiceManager().InvokeControlRequest(ctx);
        break;
    }
    case IPC::CommandType::RequestWithContext:
    case IPC::CommandType::Request: {
        InvokeRequest(ctx);
        break;
    }
    default:
        if (ctx.IsTipc()) {
            InvokeRequestTipc(ctx);
            break;
        }

        UNIMPLEMENTED_MSG("command_type={}", ctx.GetCommandType());
        break;
    }

    // If emulation was shutdown, we are closing service threads, do not write the response back to
    // memory that may be shutting down as well.
    if (system.IsPoweredOn()) {
        ctx.WriteToOutgoingCommandBuffer(ctx.GetThread());
    }

    return result;
}

/// Initialize Services
Services::Services(std::shared_ptr<SM::ServiceManager>& sm, Core::System& system)
    : hos_binder_driver_server{std::make_unique<Nvnflinger::HosBinderDriverServer>(system)},
      nv_flinger{std::make_unique<Nvnflinger::Nvnflinger>(system, *hos_binder_driver_server)} {

    auto& kernel = system.Kernel();

    // Nvnflinger needs to be accessed by several services like Vi and AppletOE so we instantiate it
    // here and pass it into the respective InstallInterfaces functions.
    system.GetFileSystemController().CreateFactories(*system.GetFilesystem(), false);

    // clang-format off
    kernel.RunOnHostCoreProcess("audio",      [&] { Audio::LoopProcess(system); }).detach();
    kernel.RunOnHostCoreProcess("FS",         [&] { FileSystem::LoopProcess(system); }).detach();
    kernel.RunOnHostCoreProcess("jit",        [&] { JIT::LoopProcess(system); }).detach();
    kernel.RunOnHostCoreProcess("ldn",        [&] { LDN::LoopProcess(system); }).detach();
    kernel.RunOnHostCoreProcess("Loader",     [&] { LDR::LoopProcess(system); }).detach();
    kernel.RunOnHostCoreProcess("nvservices", [&] { Nvidia::LoopProcess(*nv_flinger, system); }).detach();
    kernel.RunOnHostCoreProcess("bsdsocket",  [&] { Sockets::LoopProcess(system); }).detach();
    kernel.RunOnHostCoreProcess("vi",         [&] { VI::LoopProcess(system, *nv_flinger, *hos_binder_driver_server); }).detach();

    kernel.RunOnGuestCoreProcess("sm",         [&] { SM::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("account",    [&] { Account::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("am",         [&] { AM::LoopProcess(*nv_flinger, system); });
    kernel.RunOnGuestCoreProcess("aoc",        [&] { AOC::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("apm",        [&] { APM::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("bcat",       [&] { BCAT::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("bpc",        [&] { BPC::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("btdrv",      [&] { BtDrv::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("btm",        [&] { BTM::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("capsrv",     [&] { Capture::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("erpt",       [&] { ERPT::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("es",         [&] { ES::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("eupld",      [&] { EUPLD::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("fatal",      [&] { Fatal::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("fgm",        [&] { FGM::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("friends",    [&] { Friend::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("glue",       [&] { Glue::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("grc",        [&] { GRC::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("hid",        [&] { HID::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("lbl",        [&] { LBL::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("LogManager.Prod", [&] { LM::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("mig",        [&] { Migration::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("mii",        [&] { Mii::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("mm",         [&] { MM::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("mnpp",       [&] { MNPP::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("NCM",        [&] { NCM::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("nfc",        [&] { NFC::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("nfp",        [&] { NFP::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("ngct",       [&] { NGCT::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("nifm",       [&] { NIFM::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("nim",        [&] { NIM::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("npns",       [&] { NPNS::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("ns",         [&] { NS::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("olsc",       [&] { OLSC::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("pcie",       [&] { PCIe::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("pctl",       [&] { PCTL::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("pcv",        [&] { PCV::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("prepo",      [&] { PlayReport::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("ProcessManager", [&] { PM::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("psc",        [&] { PSC::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("ptm",        [&] { PTM::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("settings",   [&] { Set::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("spl",        [&] { SPL::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("ssl",        [&] { SSL::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("time",       [&] { Time::LoopProcess(system); });
    kernel.RunOnGuestCoreProcess("usb",        [&] { USB::LoopProcess(system); });
    // clang-format on
}

Services::~Services() = default;

void Services::KillNVNFlinger() {
    nv_flinger->ShutdownLayers();
}

} // namespace Service
