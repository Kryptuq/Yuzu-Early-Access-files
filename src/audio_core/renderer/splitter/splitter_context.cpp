// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/common/audio_renderer_parameter.h"
#include "audio_core/common/workbuffer_allocator.h"
#include "audio_core/renderer/behavior/behavior_info.h"
#include "audio_core/renderer/splitter/splitter_context.h"
#include "common/alignment.h"

namespace AudioCore::AudioRenderer {

SplitterDestinationData* SplitterContext::GetDesintationData(const s32 splitter_id,
                                                             const s32 destination_id) {
    return splitter_infos[splitter_id].GetData(destination_id);
}

SplitterInfo& SplitterContext::GetInfo(const s32 splitter_id) {
    return splitter_infos[splitter_id];
}

u32 SplitterContext::GetDataCount() const {
    return destinations_count;
}

u32 SplitterContext::GetInfoCount() const {
    return info_count;
}

SplitterDestinationData& SplitterContext::GetData(const u32 index) {
    return splitter_destinations[index];
}

void SplitterContext::Setup(std::span<SplitterInfo> splitter_infos_, const u32 splitter_info_count_,
                            SplitterDestinationData* splitter_destinations_,
                            const u32 destination_count_, const bool splitter_bug_fixed_) {
    splitter_infos = splitter_infos_;
    info_count = splitter_info_count_;
    splitter_destinations = splitter_destinations_;
    destinations_count = destination_count_;
    splitter_bug_fixed = splitter_bug_fixed_;
}

bool SplitterContext::UsingSplitter() const {
    return splitter_infos.size() > 0 && info_count > 0 && splitter_destinations != nullptr &&
           destinations_count > 0;
}

void SplitterContext::ClearAllNewConnectionFlag() {
    for (s32 i = 0; i < info_count; i++) {
        splitter_infos[i].SetNewConnectionFlag();
    }
}

bool SplitterContext::Initialize(const BehaviorInfo& behavior,
                                 const AudioRendererParameterInternal& params,
                                 WorkbufferAllocator& allocator) {
    if (behavior.IsSplitterSupported() && params.splitter_infos > 0 &&
        params.splitter_destinations > 0) {
        splitter_infos = allocator.Allocate<SplitterInfo>(params.splitter_infos, 0x10);

        for (u32 i = 0; i < params.splitter_infos; i++) {
            std::construct_at<SplitterInfo>(&splitter_infos[i], static_cast<s32>(i));
        }

        if (splitter_infos.size() == 0) {
            splitter_infos = {};
            return false;
        }

        splitter_destinations =
            allocator.Allocate<SplitterDestinationData>(params.splitter_destinations, 0x10).data();

        for (s32 i = 0; i < params.splitter_destinations; i++) {
            std::construct_at<SplitterDestinationData>(&splitter_destinations[i], i);
        }

        if (params.splitter_destinations <= 0) {
            splitter_infos = {};
            splitter_destinations = nullptr;
            return false;
        }

        Setup(splitter_infos, params.splitter_infos, splitter_destinations,
              params.splitter_destinations, behavior.IsSplitterBugFixed());
    }
    return true;
}

bool SplitterContext::Update(const u8* input, u32& consumed_size) {
    auto in_params{reinterpret_cast<const InParameterHeader*>(input)};

    if (destinations_count == 0 || info_count == 0) {
        consumed_size = 0;
        return true;
    }

    if (in_params->magic != GetSplitterInParamHeaderMagic()) {
        consumed_size = 0;
        return false;
    }

    for (auto& splitter_info : splitter_infos) {
        splitter_info.ClearNewConnectionFlag();
    }

    u32 offset{sizeof(InParameterHeader)};
    offset = UpdateInfo(input, offset, in_params->info_count);
    offset = UpdateData(input, offset, in_params->destination_count);

    consumed_size = Common::AlignUp(offset, 0x10);
    return true;
}

u32 SplitterContext::UpdateInfo(const u8* input, u32 offset, const u32 splitter_count) {
    for (u32 i = 0; i < splitter_count; i++) {
        auto info_header{reinterpret_cast<const SplitterInfo::InParameter*>(input + offset)};

        if (info_header->magic != GetSplitterInfoMagic()) {
            continue;
        }

        if (info_header->id < 0 || info_header->id > info_count) {
            break;
        }

        auto& info{splitter_infos[info_header->id]};
        RecomposeDestination(info, info_header);

        offset += info.Update(info_header);
    }

    return offset;
}

u32 SplitterContext::UpdateData(const u8* input, u32 offset, const u32 count) {
    for (u32 i = 0; i < count; i++) {
        auto data_header{
            reinterpret_cast<const SplitterDestinationData::InParameter*>(input + offset)};

        if (data_header->magic != GetSplitterSendDataMagic()) {
            continue;
        }

        if (data_header->id < 0 || data_header->id > destinations_count) {
            continue;
        }

        splitter_destinations[data_header->id].Update(*data_header);
        offset += sizeof(SplitterDestinationData::InParameter);
    }

    return offset;
}

void SplitterContext::UpdateInternalState() {
    for (s32 i = 0; i < info_count; i++) {
        splitter_infos[i].UpdateInternalState();
    }
}

void SplitterContext::RecomposeDestination(SplitterInfo& out_info,
                                           const SplitterInfo::InParameter* info_header) {
    auto destination{out_info.GetData(0)};
    while (destination != nullptr) {
        auto dest{destination->GetNext()};
        destination->SetNext(nullptr);
        destination = dest;
    }
    out_info.SetDestinations(nullptr);

    auto dest_count{info_header->destination_count};
    if (!splitter_bug_fixed) {
        dest_count = std::min(dest_count, GetDestCountPerInfoForCompat());
    }

    if (dest_count == 0) {
        return;
    }

    std::span<const u32> destination_ids{reinterpret_cast<const u32*>(&info_header[1]), dest_count};

    auto head{&splitter_destinations[destination_ids[0]]};
    auto current_destination{head};
    for (u32 i = 1; i < dest_count; i++) {
        auto next_destination{&splitter_destinations[destination_ids[i]]};
        current_destination->SetNext(next_destination);
        current_destination = next_destination;
    }

    out_info.SetDestinations(head);
    out_info.SetDestinationCount(dest_count);
}

u32 SplitterContext::GetDestCountPerInfoForCompat() const {
    if (info_count <= 0) {
        return 0;
    }
    return static_cast<u32>(destinations_count / info_count);
}

u64 SplitterContext::CalcWorkBufferSize(const BehaviorInfo& behavior,
                                        const AudioRendererParameterInternal& params) {
    u64 size{0};
    if (!behavior.IsSplitterSupported()) {
        return size;
    }

    size += params.splitter_destinations * sizeof(SplitterDestinationData) +
            params.splitter_infos * sizeof(SplitterInfo);

    if (behavior.IsSplitterBugFixed()) {
        size += Common::AlignUp(params.splitter_destinations * sizeof(u32), 0x10);
    }
    return size;
}

} // namespace AudioCore::AudioRenderer
