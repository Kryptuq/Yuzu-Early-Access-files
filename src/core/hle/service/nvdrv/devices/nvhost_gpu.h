// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <vector>
#include "common/bit_field.h"
#include "common/common_types.h"
#include "common/swap.h"
#include "core/hle/service/nvdrv/devices/nvdevice.h"
#include "core/hle/service/nvdrv/nvdata.h"
#include "video_core/dma_pusher.h"

namespace Service::Nvidia {
class SyncpointManager;
}

namespace Service::Nvidia::Devices {

class nvmap;
class nvhost_gpu final : public nvdevice {
public:
    explicit nvhost_gpu(Core::System& system_, std::shared_ptr<nvmap> nvmap_dev_,
                        SyncpointManager& syncpoint_manager_);
    ~nvhost_gpu() override;

    NvResult Ioctl1(DeviceFD fd, Ioctl command, const std::vector<u8>& input,
                    std::vector<u8>& output) override;
    NvResult Ioctl2(DeviceFD fd, Ioctl command, const std::vector<u8>& input,
                    const std::vector<u8>& inline_input, std::vector<u8>& output) override;
    NvResult Ioctl3(DeviceFD fd, Ioctl command, const std::vector<u8>& input,
                    std::vector<u8>& output, std::vector<u8>& inline_output) override;

    void OnOpen(DeviceFD fd) override;
    void OnClose(DeviceFD fd) override;

private:
    enum class CtxObjects : u32_le {
        Ctx2D = 0x902D,
        Ctx3D = 0xB197,
        CtxCompute = 0xB1C0,
        CtxKepler = 0xA140,
        CtxDMA = 0xB0B5,
        CtxChannelGPFIFO = 0xB06F,
    };

    struct IoctlSetNvmapFD {
        s32_le nvmap_fd{};
    };
    static_assert(sizeof(IoctlSetNvmapFD) == 4, "IoctlSetNvmapFD is incorrect size");

    struct IoctlChannelSetTimeout {
        u32_le timeout{};
    };
    static_assert(sizeof(IoctlChannelSetTimeout) == 4, "IoctlChannelSetTimeout is incorrect size");

    struct IoctlAllocGPFIFO {
        u32_le num_entries{};
        u32_le flags{};
    };
    static_assert(sizeof(IoctlAllocGPFIFO) == 8, "IoctlAllocGPFIFO is incorrect size");

    struct IoctlClientData {
        u64_le data{};
    };
    static_assert(sizeof(IoctlClientData) == 8, "IoctlClientData is incorrect size");

    struct IoctlZCullBind {
        u64_le gpu_va{};
        u32_le mode{}; // 0=global, 1=no_ctxsw, 2=separate_buffer, 3=part_of_regular_buf
        INSERT_PADDING_WORDS(1);
    };
    static_assert(sizeof(IoctlZCullBind) == 16, "IoctlZCullBind is incorrect size");

    struct IoctlSetErrorNotifier {
        u64_le offset{};
        u64_le size{};
        u32_le mem{}; // nvmap object handle
        INSERT_PADDING_WORDS(1);
    };
    static_assert(sizeof(IoctlSetErrorNotifier) == 24, "IoctlSetErrorNotifier is incorrect size");

    struct IoctlChannelSetPriority {
        u32_le priority{};
    };
    static_assert(sizeof(IoctlChannelSetPriority) == 4,
                  "IoctlChannelSetPriority is incorrect size");

    struct IoctlSetTimeslice {
        u32_le timeslice{};
    };
    static_assert(sizeof(IoctlSetTimeslice) == 4, "IoctlSetTimeslice is incorrect size");

    struct IoctlEventIdControl {
        u32_le cmd{}; // 0=disable, 1=enable, 2=clear
        u32_le id{};
    };
    static_assert(sizeof(IoctlEventIdControl) == 8, "IoctlEventIdControl is incorrect size");

    struct IoctlGetErrorNotification {
        u64_le timestamp{};
        u32_le info32{};
        u16_le info16{};
        u16_le status{}; // always 0xFFFF
    };
    static_assert(sizeof(IoctlGetErrorNotification) == 16,
                  "IoctlGetErrorNotification is incorrect size");

    static_assert(sizeof(Fence) == 8, "Fence is incorrect size");

    struct IoctlAllocGpfifoEx {
        u32_le num_entries{};
        u32_le flags{};
        u32_le unk0{};
        u32_le unk1{};
        u32_le unk2{};
        u32_le unk3{};
        u32_le unk4{};
        u32_le unk5{};
    };
    static_assert(sizeof(IoctlAllocGpfifoEx) == 32, "IoctlAllocGpfifoEx is incorrect size");

    struct IoctlAllocGpfifoEx2 {
        u32_le num_entries{}; // in
        u32_le flags{};       // in
        u32_le unk0{};        // in (1 works)
        Fence fence_out{};    // out
        u32_le unk1{};        // in
        u32_le unk2{};        // in
        u32_le unk3{};        // in
    };
    static_assert(sizeof(IoctlAllocGpfifoEx2) == 32, "IoctlAllocGpfifoEx2 is incorrect size");

    struct IoctlAllocObjCtx {
        u32_le class_num{}; // 0x902D=2d, 0xB197=3d, 0xB1C0=compute, 0xA140=kepler, 0xB0B5=DMA,
                            // 0xB06F=channel_gpfifo
        u32_le flags{};
        u64_le obj_id{}; // (ignored) used for FREE_OBJ_CTX ioctl, which is not supported
    };
    static_assert(sizeof(IoctlAllocObjCtx) == 16, "IoctlAllocObjCtx is incorrect size");

    struct IoctlSubmitGpfifo {
        u64_le address{};     // pointer to gpfifo entry structs
        u32_le num_entries{}; // number of fence objects being submitted
        union {
            u32_le raw;
            BitField<0, 1, u32_le> add_wait;      // append a wait sync_point to the list
            BitField<1, 1, u32_le> add_increment; // append an increment to the list
            BitField<2, 1, u32_le> new_hw_format; // mostly ignored
            BitField<4, 1, u32_le> suppress_wfi;  // suppress wait for interrupt
            BitField<8, 1, u32_le> increment;     // increment the returned fence
        } flags;
        Fence fence_out{}; // returned new fence object for others to wait on

        u32 AddIncrementValue() const {
            return flags.add_increment.Value() << 1;
        }
    };
    static_assert(sizeof(IoctlSubmitGpfifo) == 16 + sizeof(Fence),
                  "IoctlSubmitGpfifo is incorrect size");

    struct IoctlGetWaitbase {
        u32 unknown{}; // seems to be ignored? Nintendo added this
        u32 value{};
    };
    static_assert(sizeof(IoctlGetWaitbase) == 8, "IoctlGetWaitbase is incorrect size");

    s32_le nvmap_fd{};
    u64_le user_data{};
    IoctlZCullBind zcull_params{};
    u32_le channel_priority{};
    u32_le channel_timeslice{};

    NvResult SetNVMAPfd(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult SetClientData(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult GetClientData(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult ZCullBind(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult SetErrorNotifier(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult SetChannelPriority(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult AllocGPFIFOEx2(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult AllocateObjectContext(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult SubmitGPFIFOImpl(IoctlSubmitGpfifo& params, std::vector<u8>& output,
                              Tegra::CommandList&& entries);
    NvResult SubmitGPFIFOBase(const std::vector<u8>& input, std::vector<u8>& output,
                              bool kickoff = false);
    NvResult SubmitGPFIFOBase(const std::vector<u8>& input, const std::vector<u8>& input_inline,
                              std::vector<u8>& output);
    NvResult GetWaitbase(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult ChannelSetTimeout(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult ChannelSetTimeslice(const std::vector<u8>& input, std::vector<u8>& output);

    std::shared_ptr<nvmap> nvmap_dev;
    SyncpointManager& syncpoint_manager;
    Fence channel_fence;
};

} // namespace Service::Nvidia::Devices
