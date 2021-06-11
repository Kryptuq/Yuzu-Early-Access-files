// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <bitset>
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "video_core/engines/kepler_compute.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/engines/shader_type.h"
#include "video_core/memory_manager.h"
#include "video_core/rasterizer_interface.h"
#include "video_core/renderer_base.h"
#include "video_core/textures/decoders.h"

namespace Tegra::Engines {

KeplerCompute::KeplerCompute(Core::System& system_, MemoryManager& memory_manager_)
    : system{system_}, memory_manager{memory_manager_}, upload_state{memory_manager, regs.upload} {}

KeplerCompute::~KeplerCompute() = default;

void KeplerCompute::BindRasterizer(VideoCore::RasterizerInterface* rasterizer_) {
    rasterizer = rasterizer_;
}

void KeplerCompute::CallMethod(u32 method, u32 method_argument, bool is_last_call) {
    ASSERT_MSG(method < Regs::NUM_REGS,
               "Invalid KeplerCompute register, increase the size of the Regs structure");

    regs.reg_array[method] = method_argument;

    switch (method) {
    case KEPLER_COMPUTE_REG_INDEX(exec_upload): {
        upload_state.ProcessExec(regs.exec_upload.linear != 0);
        break;
    }
    case KEPLER_COMPUTE_REG_INDEX(data_upload): {
        upload_state.ProcessData(method_argument, is_last_call);
        if (is_last_call) {
        }
        break;
    }
    case KEPLER_COMPUTE_REG_INDEX(launch):
        ProcessLaunch();
        break;
    default:
        break;
    }
}

void KeplerCompute::CallMultiMethod(u32 method, const u32* base_start, u32 amount,
                                    u32 methods_pending) {
    for (std::size_t i = 0; i < amount; i++) {
        CallMethod(method, base_start[i], methods_pending - static_cast<u32>(i) <= 1);
    }
}

u32 KeplerCompute::AccessConstBuffer32(ShaderType stage, u64 const_buffer, u64 offset) const {
    ASSERT(stage == ShaderType::Compute);
    const auto& buffer = launch_description.const_buffer_config[const_buffer];
    u32 result;
    std::memcpy(&result, memory_manager.GetPointer(buffer.Address() + offset), sizeof(u32));
    return result;
}

SamplerDescriptor KeplerCompute::AccessBoundSampler(ShaderType stage, u64 offset) const {
    return AccessBindlessSampler(stage, regs.tex_cb_index, offset * sizeof(Texture::TextureHandle));
}

SamplerDescriptor KeplerCompute::AccessBindlessSampler(ShaderType stage, u64 const_buffer,
                                                       u64 offset) const {
    ASSERT(stage == ShaderType::Compute);
    const auto& tex_info_buffer = launch_description.const_buffer_config[const_buffer];
    const GPUVAddr tex_info_address = tex_info_buffer.Address() + offset;
    return AccessSampler(memory_manager.Read<u32>(tex_info_address));
}

SamplerDescriptor KeplerCompute::AccessSampler(u32 handle) const {
    const Texture::TextureHandle tex_handle{handle};
    const Texture::TICEntry tic = GetTICEntry(tex_handle.tic_id);
    const Texture::TSCEntry tsc = GetTSCEntry(tex_handle.tsc_id);

    SamplerDescriptor result = SamplerDescriptor::FromTIC(tic);
    result.is_shadow.Assign(tsc.depth_compare_enabled.Value());
    return result;
}

VideoCore::GuestDriverProfile& KeplerCompute::AccessGuestDriverProfile() {
    return rasterizer->AccessGuestDriverProfile();
}

const VideoCore::GuestDriverProfile& KeplerCompute::AccessGuestDriverProfile() const {
    return rasterizer->AccessGuestDriverProfile();
}

void KeplerCompute::ProcessLaunch() {
    const GPUVAddr launch_desc_loc = regs.launch_desc_loc.Address();
    memory_manager.ReadBlockUnsafe(launch_desc_loc, &launch_description,
                                   LaunchParams::NUM_LAUNCH_PARAMETERS * sizeof(u32));

    const GPUVAddr code_addr = regs.code_loc.Address() + launch_description.program_start;
    LOG_TRACE(HW_GPU, "Compute invocation launched at address 0x{:016x}", code_addr);

    rasterizer->DispatchCompute(code_addr);
}

Texture::TICEntry KeplerCompute::GetTICEntry(u32 tic_index) const {
    const GPUVAddr tic_address_gpu{regs.tic.Address() + tic_index * sizeof(Texture::TICEntry)};

    Texture::TICEntry tic_entry;
    memory_manager.ReadBlockUnsafe(tic_address_gpu, &tic_entry, sizeof(Texture::TICEntry));

    return tic_entry;
}

Texture::TSCEntry KeplerCompute::GetTSCEntry(u32 tsc_index) const {
    const GPUVAddr tsc_address_gpu{regs.tsc.Address() + tsc_index * sizeof(Texture::TSCEntry)};

    Texture::TSCEntry tsc_entry;
    memory_manager.ReadBlockUnsafe(tsc_address_gpu, &tsc_entry, sizeof(Texture::TSCEntry));
    return tsc_entry;
}

} // namespace Tegra::Engines
