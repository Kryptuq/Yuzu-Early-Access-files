// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <optional>
#include "common/assert.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "video_core/dirty_flags.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/gpu.h"
#include "video_core/memory_manager.h"
#include "video_core/rasterizer_interface.h"
#include "video_core/textures/texture.h"

namespace Tegra::Engines {

using VideoCore::QueryType;

/// First register id that is actually a Macro call.
constexpr u32 MacroRegistersStart = 0xE00;

Maxwell3D::Maxwell3D(Core::System& system_, MemoryManager& memory_manager_)
    : system{system_}, memory_manager{memory_manager_}, macro_engine{GetMacroEngine(*this)},
      upload_state{memory_manager, regs.upload} {
    dirty.flags.flip();
    InitializeRegisterDefaults();
}

Maxwell3D::~Maxwell3D() = default;

void Maxwell3D::BindRasterizer(VideoCore::RasterizerInterface* rasterizer_) {
    rasterizer = rasterizer_;
    upload_state.BindRasterizer(rasterizer_);
}

void Maxwell3D::InitializeRegisterDefaults() {
    // Initializes registers to their default values - what games expect them to be at boot. This is
    // for certain registers that may not be explicitly set by games.

    // Reset all registers to zero
    std::memset(&regs, 0, sizeof(regs));

    // Depth range near/far is not always set, but is expected to be the default 0.0f, 1.0f. This is
    // needed for ARMS.
    for (auto& viewport : regs.viewports) {
        viewport.depth_range_near = 0.0f;
        viewport.depth_range_far = 1.0f;
    }
    for (auto& viewport : regs.viewport_transform) {
        viewport.swizzle.x.Assign(Regs::ViewportSwizzle::PositiveX);
        viewport.swizzle.y.Assign(Regs::ViewportSwizzle::PositiveY);
        viewport.swizzle.z.Assign(Regs::ViewportSwizzle::PositiveZ);
        viewport.swizzle.w.Assign(Regs::ViewportSwizzle::PositiveW);
    }

    // Doom and Bomberman seems to use the uninitialized registers and just enable blend
    // so initialize blend registers with sane values
    regs.blend.color_op = Regs::Blend::Equation::Add_D3D;
    regs.blend.color_source = Regs::Blend::Factor::One_D3D;
    regs.blend.color_dest = Regs::Blend::Factor::Zero_D3D;
    regs.blend.alpha_op = Regs::Blend::Equation::Add_D3D;
    regs.blend.alpha_source = Regs::Blend::Factor::One_D3D;
    regs.blend.alpha_dest = Regs::Blend::Factor::Zero_D3D;
    for (auto& blend : regs.blend_per_target) {
        blend.color_op = Regs::Blend::Equation::Add_D3D;
        blend.color_source = Regs::Blend::Factor::One_D3D;
        blend.color_dest = Regs::Blend::Factor::Zero_D3D;
        blend.alpha_op = Regs::Blend::Equation::Add_D3D;
        blend.alpha_source = Regs::Blend::Factor::One_D3D;
        blend.alpha_dest = Regs::Blend::Factor::Zero_D3D;
    }
    regs.stencil_front_op.fail = Regs::StencilOp::Op::Keep_D3D;
    regs.stencil_front_op.zfail = Regs::StencilOp::Op::Keep_D3D;
    regs.stencil_front_op.zpass = Regs::StencilOp::Op::Keep_D3D;
    regs.stencil_front_op.func = Regs::ComparisonOp::Always_GL;
    regs.stencil_front_func.func_mask = 0xFFFFFFFF;
    regs.stencil_front_func.mask = 0xFFFFFFFF;
    regs.stencil_two_side_enable = 1;
    regs.stencil_back_op.fail = Regs::StencilOp::Op::Keep_D3D;
    regs.stencil_back_op.zfail = Regs::StencilOp::Op::Keep_D3D;
    regs.stencil_back_op.zpass = Regs::StencilOp::Op::Keep_D3D;
    regs.stencil_back_op.func = Regs::ComparisonOp::Always_GL;
    regs.stencil_back_func.func_mask = 0xFFFFFFFF;
    regs.stencil_back_func.mask = 0xFFFFFFFF;

    regs.depth_test_func = Regs::ComparisonOp::Always_GL;
    regs.gl_front_face = Regs::FrontFace::CounterClockWise;
    regs.gl_cull_face = Regs::CullFace::Back;

    // TODO(Rodrigo): Most games do not set a point size. I think this is a case of a
    // register carrying a default value. Assume it's OpenGL's default (1).
    regs.point_size = 1.0f;

    // TODO(bunnei): Some games do not initialize the color masks (e.g. Sonic Mania). Assuming a
    // default of enabled fixes rendering here.
    for (auto& color_mask : regs.color_mask) {
        color_mask.R.Assign(1);
        color_mask.G.Assign(1);
        color_mask.B.Assign(1);
        color_mask.A.Assign(1);
    }

    for (auto& format : regs.vertex_attrib_format) {
        format.constant.Assign(1);
    }

    // NVN games expect these values to be enabled at boot
    regs.rasterize_enable = 1;
    regs.color_target_mrt_enable = 1;
    regs.framebuffer_srgb = 1;
    regs.line_width_aliased = 1.0f;
    regs.line_width_smooth = 1.0f;
    regs.gl_front_face = Maxwell3D::Regs::FrontFace::ClockWise;
    regs.polygon_mode_back = Maxwell3D::Regs::PolygonMode::Fill;
    regs.polygon_mode_front = Maxwell3D::Regs::PolygonMode::Fill;

    shadow_state = regs;

    inline_draw[MAXWELL3D_REG_INDEX(draw.end)] = true;
    inline_draw[MAXWELL3D_REG_INDEX(draw.begin)] = true;
    inline_draw[MAXWELL3D_REG_INDEX(vertex_buffer.first)] = true;
    inline_draw[MAXWELL3D_REG_INDEX(vertex_buffer.count)] = true;
    inline_draw[MAXWELL3D_REG_INDEX(index_buffer.first)] = true;
    inline_draw[MAXWELL3D_REG_INDEX(index_buffer.count)] = true;
}

void Maxwell3D::ProcessMacro(u32 method, const u32* base_start, u32 amount, bool is_last_call) {
    if (executing_macro == 0) {
        // A macro call must begin by writing the macro method's register, not its argument.
        ASSERT_MSG((method % 2) == 0,
                   "Can't start macro execution by writing to the ARGS register");
        executing_macro = method;
    }

    macro_params.insert(macro_params.end(), base_start, base_start + amount);

    // Call the macro when there are no more parameters in the command buffer
    if (is_last_call) {
        CallMacroMethod(executing_macro, macro_params);
        macro_params.clear();
    }
}

u32 Maxwell3D::ProcessShadowRam(u32 method, u32 argument) {
    // Keep track of the register value in shadow_state when requested.
    const auto control = shadow_state.shadow_ram_control;
    if (control == Regs::ShadowRamControl::Track ||
        control == Regs::ShadowRamControl::TrackWithFilter) {
        shadow_state.reg_array[method] = argument;
        return argument;
    }
    if (control == Regs::ShadowRamControl::Replay) {
        return shadow_state.reg_array[method];
    }
    return argument;
}

void Maxwell3D::ProcessDirtyRegisters(u32 method, u32 argument) {
    if (regs.reg_array[method] == argument) {
        return;
    }
    regs.reg_array[method] = argument;

    for (const auto& table : dirty.tables) {
        dirty.flags[table[method]] = true;
    }
}

void Maxwell3D::ProcessMethodCall(u32 method, u32 argument, u32 nonshadow_argument,
                                  bool is_last_call) {
    switch (method) {
    case MAXWELL3D_REG_INDEX(wait_for_idle):
        return rasterizer->WaitForIdle();
    case MAXWELL3D_REG_INDEX(shadow_ram_control):
        shadow_state.shadow_ram_control = static_cast<Regs::ShadowRamControl>(nonshadow_argument);
        return;
    case MAXWELL3D_REG_INDEX(load_mme.instruction_ptr):
        return macro_engine->ClearCode(regs.load_mme.instruction_ptr);
    case MAXWELL3D_REG_INDEX(load_mme.instruction):
        return macro_engine->AddCode(regs.load_mme.instruction_ptr, argument);
    case MAXWELL3D_REG_INDEX(load_mme.start_address):
        return ProcessMacroBind(argument);
    case MAXWELL3D_REG_INDEX(falcon[4]):
        return ProcessFirmwareCall4();
    case MAXWELL3D_REG_INDEX(const_buffer.buffer):
    case MAXWELL3D_REG_INDEX(const_buffer.buffer) + 1:
    case MAXWELL3D_REG_INDEX(const_buffer.buffer) + 2:
    case MAXWELL3D_REG_INDEX(const_buffer.buffer) + 3:
    case MAXWELL3D_REG_INDEX(const_buffer.buffer) + 4:
    case MAXWELL3D_REG_INDEX(const_buffer.buffer) + 5:
    case MAXWELL3D_REG_INDEX(const_buffer.buffer) + 6:
    case MAXWELL3D_REG_INDEX(const_buffer.buffer) + 7:
    case MAXWELL3D_REG_INDEX(const_buffer.buffer) + 8:
    case MAXWELL3D_REG_INDEX(const_buffer.buffer) + 9:
    case MAXWELL3D_REG_INDEX(const_buffer.buffer) + 10:
    case MAXWELL3D_REG_INDEX(const_buffer.buffer) + 11:
    case MAXWELL3D_REG_INDEX(const_buffer.buffer) + 12:
    case MAXWELL3D_REG_INDEX(const_buffer.buffer) + 13:
    case MAXWELL3D_REG_INDEX(const_buffer.buffer) + 14:
    case MAXWELL3D_REG_INDEX(const_buffer.buffer) + 15:
        return ProcessCBData(argument);
    case MAXWELL3D_REG_INDEX(bind_groups[0].raw_config):
        return ProcessCBBind(0);
    case MAXWELL3D_REG_INDEX(bind_groups[1].raw_config):
        return ProcessCBBind(1);
    case MAXWELL3D_REG_INDEX(bind_groups[2].raw_config):
        return ProcessCBBind(2);
    case MAXWELL3D_REG_INDEX(bind_groups[3].raw_config):
        return ProcessCBBind(3);
    case MAXWELL3D_REG_INDEX(bind_groups[4].raw_config):
        return ProcessCBBind(4);
    case MAXWELL3D_REG_INDEX(index_buffer32_first):
        regs.index_buffer.count = regs.index_buffer32_first.count;
        regs.index_buffer.first = regs.index_buffer32_first.first;
        dirty.flags[VideoCommon::Dirty::IndexBuffer] = true;
        return FlushInlineDraw();
    case MAXWELL3D_REG_INDEX(index_buffer16_first):
        regs.index_buffer.count = regs.index_buffer16_first.count;
        regs.index_buffer.first = regs.index_buffer16_first.first;
        dirty.flags[VideoCommon::Dirty::IndexBuffer] = true;
        return FlushInlineDraw();
    case MAXWELL3D_REG_INDEX(index_buffer8_first):
        regs.index_buffer.count = regs.index_buffer8_first.count;
        regs.index_buffer.first = regs.index_buffer8_first.first;
        dirty.flags[VideoCommon::Dirty::IndexBuffer] = true;
        return FlushInlineDraw();
    case MAXWELL3D_REG_INDEX(topology_override):
        use_topology_override = true;
        return;
    case MAXWELL3D_REG_INDEX(clear_surface):
        return ProcessClearBuffers();
    case MAXWELL3D_REG_INDEX(report_semaphore.query):
        return ProcessQueryGet();
    case MAXWELL3D_REG_INDEX(render_enable.mode):
        return ProcessQueryCondition();
    case MAXWELL3D_REG_INDEX(clear_report_value):
        return ProcessCounterReset();
    case MAXWELL3D_REG_INDEX(sync_info):
        return ProcessSyncPoint();
    case MAXWELL3D_REG_INDEX(launch_dma):
        return upload_state.ProcessExec(regs.launch_dma.memory_layout.Value() ==
                                        Regs::LaunchDMA::Layout::Pitch);
    case MAXWELL3D_REG_INDEX(inline_data):
        upload_state.ProcessData(argument, is_last_call);
        return;
    case MAXWELL3D_REG_INDEX(fragment_barrier):
        return rasterizer->FragmentBarrier();
    }
}

void Maxwell3D::CallMacroMethod(u32 method, const std::vector<u32>& parameters) {
    // Reset the current macro.
    executing_macro = 0;

    // Lookup the macro offset
    const u32 entry =
        ((method - MacroRegistersStart) >> 1) % static_cast<u32>(macro_positions.size());

    // Execute the current macro.
    macro_engine->Execute(macro_positions[entry], parameters);
    if (draw_state.current_mode != DrawMode::Undefined) {
        FlushInlineDraw();
    }
}

void Maxwell3D::CallMethod(u32 method, u32 method_argument, bool is_last_call) {
    // It is an error to write to a register other than the current macro's ARG register before
    // it has finished execution.
    if (executing_macro != 0) {
        ASSERT(method == executing_macro + 1);
    }

    // Methods after 0xE00 are special, they're actually triggers for some microcode that was
    // uploaded to the GPU during initialization.
    if (method >= MacroRegistersStart) {
        ProcessMacro(method, &method_argument, 1, is_last_call);
        return;
    }

    ASSERT_MSG(method < Regs::NUM_REGS,
               "Invalid Maxwell3D register, increase the size of the Regs structure");

    if (inline_draw[method]) {
        regs.reg_array[method] = method_argument;
        switch (method) {
        case MAXWELL3D_REG_INDEX(vertex_buffer.count):
        case MAXWELL3D_REG_INDEX(index_buffer.count): {
            const DrawMode expected_mode = method == MAXWELL3D_REG_INDEX(vertex_buffer.count)
                                               ? DrawMode::Array
                                               : DrawMode::Indexed;
            StepInstance(expected_mode, method_argument);
            break;
        }
        case MAXWELL3D_REG_INDEX(draw.begin):
            draw_state.instance_mode =
                (regs.draw.instance_id == Maxwell3D::Regs::Draw::InstanceId::Subsequent) ||
                (regs.draw.instance_id == Maxwell3D::Regs::Draw::InstanceId::Unchanged);
            draw_state.gl_begin_consume = true;
            break;
        case MAXWELL3D_REG_INDEX(draw.end):
            draw_state.gl_end_count++;
            break;
        case MAXWELL3D_REG_INDEX(vertex_buffer.first):
        case MAXWELL3D_REG_INDEX(index_buffer.first):
            break;
        }
    } else {
        if (draw_state.current_mode != DrawMode::Undefined) {
            FlushInlineDraw();
        }

        const u32 argument = ProcessShadowRam(method, method_argument);
        ProcessDirtyRegisters(method, argument);
        ProcessMethodCall(method, argument, method_argument, is_last_call);
    }
}

void Maxwell3D::CallMultiMethod(u32 method, const u32* base_start, u32 amount,
                                u32 methods_pending) {
    // Methods after 0xE00 are special, they're actually triggers for some microcode that was
    // uploaded to the GPU during initialization.
    if (method >= MacroRegistersStart) {
        ProcessMacro(method, base_start, amount, amount == methods_pending);
        return;
    }
    switch (method) {
    case MAXWELL3D_REG_INDEX(const_buffer.buffer):
    case MAXWELL3D_REG_INDEX(const_buffer.buffer) + 1:
    case MAXWELL3D_REG_INDEX(const_buffer.buffer) + 2:
    case MAXWELL3D_REG_INDEX(const_buffer.buffer) + 3:
    case MAXWELL3D_REG_INDEX(const_buffer.buffer) + 4:
    case MAXWELL3D_REG_INDEX(const_buffer.buffer) + 5:
    case MAXWELL3D_REG_INDEX(const_buffer.buffer) + 6:
    case MAXWELL3D_REG_INDEX(const_buffer.buffer) + 7:
    case MAXWELL3D_REG_INDEX(const_buffer.buffer) + 8:
    case MAXWELL3D_REG_INDEX(const_buffer.buffer) + 9:
    case MAXWELL3D_REG_INDEX(const_buffer.buffer) + 10:
    case MAXWELL3D_REG_INDEX(const_buffer.buffer) + 11:
    case MAXWELL3D_REG_INDEX(const_buffer.buffer) + 12:
    case MAXWELL3D_REG_INDEX(const_buffer.buffer) + 13:
    case MAXWELL3D_REG_INDEX(const_buffer.buffer) + 14:
    case MAXWELL3D_REG_INDEX(const_buffer.buffer) + 15:
        ProcessCBMultiData(base_start, amount);
        break;
    case MAXWELL3D_REG_INDEX(inline_data):
        upload_state.ProcessData(base_start, static_cast<size_t>(amount));
        return;
    default:
        for (std::size_t i = 0; i < amount; i++) {
            CallMethod(method, base_start[i], methods_pending - static_cast<u32>(i) <= 1);
        }
        break;
    }
}

void Maxwell3D::StepInstance(const DrawMode expected_mode, const u32 count) {
    if (draw_state.current_mode == DrawMode::Undefined) {
        if (draw_state.gl_begin_consume) {
            draw_state.current_mode = expected_mode;
            draw_state.current_count = count;
            draw_state.instance_count = 1;
            draw_state.gl_begin_consume = false;
            draw_state.gl_end_count = 0;
        }
        return;
    } else {
        if (draw_state.current_mode == expected_mode && count == draw_state.current_count &&
            draw_state.instance_mode && draw_state.gl_begin_consume) {
            draw_state.instance_count++;
            draw_state.gl_begin_consume = false;
            return;
        } else {
            FlushInlineDraw();
        }
    }
    // Tail call in case it needs to retry.
    StepInstance(expected_mode, count);
}

void Maxwell3D::ProcessTopologyOverride() {
    using PrimitiveTopology = Maxwell3D::Regs::PrimitiveTopology;
    using PrimitiveTopologyOverride = Maxwell3D::Regs::PrimitiveTopologyOverride;

    PrimitiveTopology topology{};

    switch (regs.topology_override) {
    case PrimitiveTopologyOverride::None:
        topology = regs.draw.topology;
        break;
    case PrimitiveTopologyOverride::Points:
        topology = PrimitiveTopology::Points;
        break;
    case PrimitiveTopologyOverride::Lines:
        topology = PrimitiveTopology::Lines;
        break;
    case PrimitiveTopologyOverride::LineStrip:
        topology = PrimitiveTopology::LineStrip;
        break;
    default:
        topology = static_cast<PrimitiveTopology>(regs.topology_override);
        break;
    }

    if (use_topology_override) {
        regs.draw.topology.Assign(topology);
    }
}

void Maxwell3D::FlushInlineDraw() {
    LOG_TRACE(HW_GPU, "called, topology={}, count={}", regs.draw.topology.Value(),
              regs.vertex_buffer.count);

    ASSERT_MSG(!(regs.index_buffer.count && regs.vertex_buffer.count), "Both indexed and direct?");
    ASSERT(draw_state.instance_count == draw_state.gl_end_count);

    // Both instance configuration registers can not be set at the same time.
    ASSERT_MSG(regs.draw.instance_id == Maxwell3D::Regs::Draw::InstanceId::First ||
                   regs.draw.instance_id != Maxwell3D::Regs::Draw::InstanceId::Unchanged,
               "Illegal combination of instancing parameters");

    ProcessTopologyOverride();

    const bool is_indexed = draw_state.current_mode == DrawMode::Indexed;
    if (ShouldExecute()) {
        rasterizer->Draw(is_indexed);
    }

    // TODO(bunnei): Below, we reset vertex count so that we can use these registers to determine if
    // the game is trying to draw indexed or direct mode. This needs to be verified on HW still -
    // it's possible that it is incorrect and that there is some other register used to specify the
    // drawing mode.
    if (is_indexed) {
        regs.index_buffer.count = 0;
    } else {
        regs.vertex_buffer.count = 0;
    }
    draw_state.current_mode = DrawMode::Undefined;
    draw_state.current_count = 0;
    draw_state.instance_count = 0;
    draw_state.instance_mode = false;
    draw_state.gl_begin_consume = false;
    draw_state.gl_end_count = 0;
}

void Maxwell3D::ProcessMacroUpload(u32 data) {
    macro_engine->AddCode(regs.load_mme.instruction_ptr++, data);
}

void Maxwell3D::ProcessMacroBind(u32 data) {
    macro_positions[regs.load_mme.start_address_ptr++] = data;
}

void Maxwell3D::ProcessFirmwareCall4() {
    LOG_WARNING(HW_GPU, "(STUBBED) called");

    // Firmware call 4 is a blob that changes some registers depending on its parameters.
    // These registers don't affect emulation and so are stubbed by setting 0xd00 to 1.
    regs.shadow_scratch[0] = 1;
}

void Maxwell3D::StampQueryResult(u64 payload, bool long_query) {
    const GPUVAddr sequence_address{regs.report_semaphore.Address()};
    if (long_query) {
        memory_manager.Write<u64>(sequence_address + sizeof(u64), system.GPU().GetTicks());
        memory_manager.Write<u64>(sequence_address, payload);
    } else {
        memory_manager.Write<u32>(sequence_address, static_cast<u32>(payload));
    }
}

void Maxwell3D::ProcessQueryGet() {
    // TODO(Subv): Support the other query units.
    if (regs.report_semaphore.query.location != Regs::ReportSemaphore::Location::All) {
        LOG_DEBUG(HW_GPU, "Locations other than ALL are unimplemented");
    }

    switch (regs.report_semaphore.query.operation) {
    case Regs::ReportSemaphore::Operation::Release:
        if (regs.report_semaphore.query.release ==
                Regs::ReportSemaphore::Release::AfterAllPreceedingWrites ||
            regs.report_semaphore.query.short_query != 0) {
            const GPUVAddr sequence_address{regs.report_semaphore.Address()};
            const u32 payload = regs.report_semaphore.payload;
            std::function<void()> operation([this, sequence_address, payload] {
                memory_manager.Write<u32>(sequence_address, payload);
            });
            rasterizer->SignalFence(std::move(operation));
        } else {
            struct LongQueryResult {
                u64_le value;
                u64_le timestamp;
            };
            const GPUVAddr sequence_address{regs.report_semaphore.Address()};
            const u32 payload = regs.report_semaphore.payload;
            std::function<void()> operation([this, sequence_address, payload] {
                memory_manager.Write<u64>(sequence_address + sizeof(u64), system.GPU().GetTicks());
                memory_manager.Write<u64>(sequence_address, payload);
            });
            rasterizer->SyncOperation(std::move(operation));
        }
        break;
    case Regs::ReportSemaphore::Operation::Acquire:
        // TODO(Blinkhawk): Under this operation, the GPU waits for the CPU to write a value that
        // matches the current payload.
        UNIMPLEMENTED_MSG("Unimplemented query operation ACQUIRE");
        break;
    case Regs::ReportSemaphore::Operation::ReportOnly:
        if (const std::optional<u64> result = GetQueryResult()) {
            // If the query returns an empty optional it means it's cached and deferred.
            // In this case we have a non-empty result, so we stamp it immediately.
            StampQueryResult(*result, regs.report_semaphore.query.short_query == 0);
        }
        break;
    case Regs::ReportSemaphore::Operation::Trap:
        UNIMPLEMENTED_MSG("Unimplemented query operation TRAP");
        break;
    default:
        UNIMPLEMENTED_MSG("Unknown query operation");
        break;
    }
}

void Maxwell3D::ProcessQueryCondition() {
    const GPUVAddr condition_address{regs.render_enable.Address()};
    switch (regs.render_enable.mode) {
    case Regs::RenderEnable::Mode::True: {
        execute_on = true;
        break;
    }
    case Regs::RenderEnable::Mode::False: {
        execute_on = false;
        break;
    }
    case Regs::RenderEnable::Mode::Conditional: {
        Regs::ReportSemaphore::Compare cmp;
        memory_manager.ReadBlock(condition_address, &cmp, sizeof(cmp));
        execute_on = cmp.initial_sequence != 0U && cmp.initial_mode != 0U;
        break;
    }
    case Regs::RenderEnable::Mode::IfEqual: {
        Regs::ReportSemaphore::Compare cmp;
        memory_manager.ReadBlock(condition_address, &cmp, sizeof(cmp));
        execute_on =
            cmp.initial_sequence == cmp.current_sequence && cmp.initial_mode == cmp.current_mode;
        break;
    }
    case Regs::RenderEnable::Mode::IfNotEqual: {
        Regs::ReportSemaphore::Compare cmp;
        memory_manager.ReadBlock(condition_address, &cmp, sizeof(cmp));
        execute_on =
            cmp.initial_sequence != cmp.current_sequence || cmp.initial_mode != cmp.current_mode;
        break;
    }
    default: {
        UNIMPLEMENTED_MSG("Uninplemented Condition Mode!");
        execute_on = true;
        break;
    }
    }
}

void Maxwell3D::ProcessCounterReset() {
    switch (regs.clear_report_value) {
    case Regs::ClearReport::ZPassPixelCount:
        rasterizer->ResetCounter(QueryType::SamplesPassed);
        break;
    default:
        LOG_DEBUG(Render_OpenGL, "Unimplemented counter reset={}", regs.clear_report_value);
        break;
    }
}

void Maxwell3D::ProcessSyncPoint() {
    const u32 sync_point = regs.sync_info.sync_point.Value();
    const auto condition = regs.sync_info.condition.Value();
    [[maybe_unused]] const u32 cache_flush = regs.sync_info.clean_l2.Value();
    if (condition == Regs::SyncInfo::Condition::RopWritesDone) {
        rasterizer->SignalSyncPoint(sync_point);
    }
}

std::optional<u64> Maxwell3D::GetQueryResult() {
    switch (regs.report_semaphore.query.report) {
    case Regs::ReportSemaphore::Report::Payload:
        return regs.report_semaphore.payload;
    case Regs::ReportSemaphore::Report::ZPassPixelCount64:
        // Deferred.
        rasterizer->Query(regs.report_semaphore.Address(), QueryType::SamplesPassed,
                          system.GPU().GetTicks());
        return std::nullopt;
    default:
        LOG_DEBUG(HW_GPU, "Unimplemented query report type {}",
                  regs.report_semaphore.query.report.Value());
        return 1;
    }
}

void Maxwell3D::ProcessCBBind(size_t stage_index) {
    // Bind the buffer currently in CB_ADDRESS to the specified index in the desired shader stage.
    const auto& bind_data = regs.bind_groups[stage_index];
    auto& buffer = state.shader_stages[stage_index].const_buffers[bind_data.shader_slot];
    buffer.enabled = bind_data.valid.Value() != 0;
    buffer.address = regs.const_buffer.Address();
    buffer.size = regs.const_buffer.size;

    const bool is_enabled = bind_data.valid.Value() != 0;
    if (!is_enabled) {
        rasterizer->DisableGraphicsUniformBuffer(stage_index, bind_data.shader_slot);
        return;
    }
    const GPUVAddr gpu_addr = regs.const_buffer.Address();
    const u32 size = regs.const_buffer.size;
    rasterizer->BindGraphicsUniformBuffer(stage_index, bind_data.shader_slot, gpu_addr, size);
}

void Maxwell3D::ProcessCBMultiData(const u32* start_base, u32 amount) {
    // Write the input value to the current const buffer at the current position.
    const GPUVAddr buffer_address = regs.const_buffer.Address();
    ASSERT(buffer_address != 0);

    // Don't allow writing past the end of the buffer.
    ASSERT(regs.const_buffer.offset <= regs.const_buffer.size);

    const GPUVAddr address{buffer_address + regs.const_buffer.offset};
    const size_t copy_size = amount * sizeof(u32);
    memory_manager.WriteBlock(address, start_base, copy_size);

    // Increment the current buffer position.
    regs.const_buffer.offset += static_cast<u32>(copy_size);
}

void Maxwell3D::ProcessCBData(u32 value) {
    ProcessCBMultiData(&value, 1);
}

Texture::TICEntry Maxwell3D::GetTICEntry(u32 tic_index) const {
    const GPUVAddr tic_address_gpu{regs.tex_header.Address() +
                                   tic_index * sizeof(Texture::TICEntry)};

    Texture::TICEntry tic_entry;
    memory_manager.ReadBlockUnsafe(tic_address_gpu, &tic_entry, sizeof(Texture::TICEntry));

    return tic_entry;
}

Texture::TSCEntry Maxwell3D::GetTSCEntry(u32 tsc_index) const {
    const GPUVAddr tsc_address_gpu{regs.tex_sampler.Address() +
                                   tsc_index * sizeof(Texture::TSCEntry)};

    Texture::TSCEntry tsc_entry;
    memory_manager.ReadBlockUnsafe(tsc_address_gpu, &tsc_entry, sizeof(Texture::TSCEntry));
    return tsc_entry;
}

u32 Maxwell3D::GetRegisterValue(u32 method) const {
    ASSERT_MSG(method < Regs::NUM_REGS, "Invalid Maxwell3D register");
    return regs.reg_array[method];
}

void Maxwell3D::ProcessClearBuffers() {
    rasterizer->Clear();
}

} // namespace Tegra::Engines
