// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/behavior_info.h"
#include "audio_core/splitter_context.h"
#include "common/alignment.h"
#include "common/assert.h"
#include "common/logging/log.h"

namespace AudioCore {

ServerSplitterDestinationData::ServerSplitterDestinationData(s32 id_) : id{id_} {}
ServerSplitterDestinationData::~ServerSplitterDestinationData() = default;

void ServerSplitterDestinationData::Update(SplitterInfo::InDestinationParams& header) {
    // Log error as these are not actually failure states
    if (header.magic != SplitterMagic::DataHeader) {
        LOG_ERROR(Audio, "Splitter destination header is invalid!");
        return;
    }

    // Incorrect splitter id
    if (header.splitter_id != id) {
        LOG_ERROR(Audio, "Splitter destination ids do not match!");
        return;
    }

    mix_id = header.mix_id;
    // Copy our mix volumes
    std::copy(header.mix_volumes.begin(), header.mix_volumes.end(), current_mix_volumes.begin());
    if (!in_use && header.in_use) {
        // Update mix volumes
        std::copy(current_mix_volumes.begin(), current_mix_volumes.end(), last_mix_volumes.begin());
        needs_update = false;
    }
    in_use = header.in_use;
}

ServerSplitterDestinationData* ServerSplitterDestinationData::GetNextDestination() {
    return next;
}

const ServerSplitterDestinationData* ServerSplitterDestinationData::GetNextDestination() const {
    return next;
}

void ServerSplitterDestinationData::SetNextDestination(ServerSplitterDestinationData* dest) {
    next = dest;
}

bool ServerSplitterDestinationData::ValidMixId() const {
    return GetMixId() != AudioCommon::NO_MIX;
}

s32 ServerSplitterDestinationData::GetMixId() const {
    return mix_id;
}

bool ServerSplitterDestinationData::IsConfigured() const {
    return in_use && ValidMixId();
}

float ServerSplitterDestinationData::GetMixVolume(std::size_t i) const {
    ASSERT(i < AudioCommon::MAX_MIX_BUFFERS);
    return current_mix_volumes.at(i);
}

const std::array<float, AudioCommon::MAX_MIX_BUFFERS>&
ServerSplitterDestinationData::CurrentMixVolumes() const {
    return current_mix_volumes;
}

const std::array<float, AudioCommon::MAX_MIX_BUFFERS>&
ServerSplitterDestinationData::LastMixVolumes() const {
    return last_mix_volumes;
}

void ServerSplitterDestinationData::MarkDirty() {
    needs_update = true;
}

void ServerSplitterDestinationData::UpdateInternalState() {
    if (in_use && needs_update) {
        std::copy(current_mix_volumes.begin(), current_mix_volumes.end(), last_mix_volumes.begin());
    }
    needs_update = false;
}

ServerSplitterInfo::ServerSplitterInfo(s32 id_) : id(id_) {}
ServerSplitterInfo::~ServerSplitterInfo() = default;

void ServerSplitterInfo::InitializeInfos() {
    send_length = 0;
    head = nullptr;
    new_connection = true;
}

void ServerSplitterInfo::ClearNewConnectionFlag() {
    new_connection = false;
}

std::size_t ServerSplitterInfo::Update(SplitterInfo::InInfoPrams& header) {
    if (header.send_id != id) {
        return 0;
    }

    sample_rate = header.sample_rate;
    new_connection = true;
    // We need to update the size here due to the splitter bug being present and providing an
    // incorrect size. We're suppose to also update the header here but we just ignore and continue
    return (sizeof(s32_le) * (header.length - 1)) + (sizeof(s32_le) * 3);
}

ServerSplitterDestinationData* ServerSplitterInfo::GetHead() {
    return head;
}

const ServerSplitterDestinationData* ServerSplitterInfo::GetHead() const {
    return head;
}

ServerSplitterDestinationData* ServerSplitterInfo::GetData(std::size_t depth) {
    auto* current_head = head;
    for (std::size_t i = 0; i < depth; i++) {
        if (current_head == nullptr) {
            return nullptr;
        }
        current_head = current_head->GetNextDestination();
    }
    return current_head;
}

const ServerSplitterDestinationData* ServerSplitterInfo::GetData(std::size_t depth) const {
    auto* current_head = head;
    for (std::size_t i = 0; i < depth; i++) {
        if (current_head == nullptr) {
            return nullptr;
        }
        current_head = current_head->GetNextDestination();
    }
    return current_head;
}

bool ServerSplitterInfo::HasNewConnection() const {
    return new_connection;
}

s32 ServerSplitterInfo::GetLength() const {
    return send_length;
}

void ServerSplitterInfo::SetHead(ServerSplitterDestinationData* new_head) {
    head = new_head;
}

void ServerSplitterInfo::SetHeadDepth(s32 length) {
    send_length = length;
}

SplitterContext::SplitterContext() = default;
SplitterContext::~SplitterContext() = default;

void SplitterContext::Initialize(BehaviorInfo& behavior_info, std::size_t _info_count,
                                 std::size_t _data_count) {
    if (!behavior_info.IsSplitterSupported() || _data_count == 0 || _info_count == 0) {
        Setup(0, 0, false);
        return;
    }
    // Only initialize if we're using splitters
    Setup(_info_count, _data_count, behavior_info.IsSplitterBugFixed());
}

bool SplitterContext::Update(const std::vector<u8>& input, std::size_t& input_offset,
                             std::size_t& bytes_read) {
    const auto UpdateOffsets = [&](std::size_t read) {
        input_offset += read;
        bytes_read += read;
    };

    if (info_count == 0 || data_count == 0) {
        bytes_read = 0;
        return true;
    }

    if (!AudioCommon::CanConsumeBuffer(input.size(), input_offset,
                                       sizeof(SplitterInfo::InHeader))) {
        LOG_ERROR(Audio, "Buffer is an invalid size!");
        return false;
    }
    SplitterInfo::InHeader header{};
    std::memcpy(&header, input.data() + input_offset, sizeof(SplitterInfo::InHeader));
    UpdateOffsets(sizeof(SplitterInfo::InHeader));

    if (header.magic != SplitterMagic::SplitterHeader) {
        LOG_ERROR(Audio, "Invalid header magic! Expecting {:X} but got {:X}",
                  SplitterMagic::SplitterHeader, header.magic);
        return false;
    }

    // Clear all connections
    for (auto& info : infos) {
        info.ClearNewConnectionFlag();
    }

    UpdateInfo(input, input_offset, bytes_read, header.info_count);
    UpdateData(input, input_offset, bytes_read, header.data_count);
    const auto aligned_bytes_read = Common::AlignUp(bytes_read, 16);
    input_offset += aligned_bytes_read - bytes_read;
    bytes_read = aligned_bytes_read;
    return true;
}

bool SplitterContext::UsingSplitter() const {
    return info_count > 0 && data_count > 0;
}

ServerSplitterInfo& SplitterContext::GetInfo(std::size_t i) {
    ASSERT(i < info_count);
    return infos.at(i);
}

const ServerSplitterInfo& SplitterContext::GetInfo(std::size_t i) const {
    ASSERT(i < info_count);
    return infos.at(i);
}

ServerSplitterDestinationData& SplitterContext::GetData(std::size_t i) {
    ASSERT(i < data_count);
    return datas.at(i);
}

const ServerSplitterDestinationData& SplitterContext::GetData(std::size_t i) const {
    ASSERT(i < data_count);
    return datas.at(i);
}

ServerSplitterDestinationData* SplitterContext::GetDestinationData(std::size_t info,
                                                                   std::size_t data) {
    ASSERT(info < info_count);
    auto& cur_info = GetInfo(info);
    return cur_info.GetData(data);
}

const ServerSplitterDestinationData* SplitterContext::GetDestinationData(std::size_t info,
                                                                         std::size_t data) const {
    ASSERT(info < info_count);
    const auto& cur_info = GetInfo(info);
    return cur_info.GetData(data);
}

void SplitterContext::UpdateInternalState() {
    if (data_count == 0) {
        return;
    }

    for (auto& data : datas) {
        data.UpdateInternalState();
    }
}

std::size_t SplitterContext::GetInfoCount() const {
    return info_count;
}

std::size_t SplitterContext::GetDataCount() const {
    return data_count;
}

void SplitterContext::Setup(std::size_t info_count_, std::size_t data_count_,
                            bool is_splitter_bug_fixed) {

    info_count = info_count_;
    data_count = data_count_;

    for (std::size_t i = 0; i < info_count; i++) {
        auto& splitter = infos.emplace_back(static_cast<s32>(i));
        splitter.InitializeInfos();
    }
    for (std::size_t i = 0; i < data_count; i++) {
        datas.emplace_back(static_cast<s32>(i));
    }

    bug_fixed = is_splitter_bug_fixed;
}

bool SplitterContext::UpdateInfo(const std::vector<u8>& input, std::size_t& input_offset,
                                 std::size_t& bytes_read, s32 in_splitter_count) {
    const auto UpdateOffsets = [&](std::size_t read) {
        input_offset += read;
        bytes_read += read;
    };

    for (s32 i = 0; i < in_splitter_count; i++) {
        if (!AudioCommon::CanConsumeBuffer(input.size(), input_offset,
                                           sizeof(SplitterInfo::InInfoPrams))) {
            LOG_ERROR(Audio, "Buffer is an invalid size!");
            return false;
        }
        SplitterInfo::InInfoPrams header{};
        std::memcpy(&header, input.data() + input_offset, sizeof(SplitterInfo::InInfoPrams));

        // Logged as warning as these don't actually cause a bailout for some reason
        if (header.magic != SplitterMagic::InfoHeader) {
            LOG_ERROR(Audio, "Bad splitter data header");
            break;
        }

        if (header.send_id < 0 || static_cast<std::size_t>(header.send_id) > info_count) {
            LOG_ERROR(Audio, "Bad splitter data id");
            break;
        }

        UpdateOffsets(sizeof(SplitterInfo::InInfoPrams));
        auto& info = GetInfo(header.send_id);
        if (!RecomposeDestination(info, header, input, input_offset)) {
            LOG_ERROR(Audio, "Failed to recompose destination for splitter!");
            return false;
        }
        const std::size_t read = info.Update(header);
        bytes_read += read;
        input_offset += read;
    }
    return true;
}

bool SplitterContext::UpdateData(const std::vector<u8>& input, std::size_t& input_offset,
                                 std::size_t& bytes_read, s32 in_data_count) {
    const auto UpdateOffsets = [&](std::size_t read) {
        input_offset += read;
        bytes_read += read;
    };

    for (s32 i = 0; i < in_data_count; i++) {
        if (!AudioCommon::CanConsumeBuffer(input.size(), input_offset,
                                           sizeof(SplitterInfo::InDestinationParams))) {
            LOG_ERROR(Audio, "Buffer is an invalid size!");
            return false;
        }
        SplitterInfo::InDestinationParams header{};
        std::memcpy(&header, input.data() + input_offset,
                    sizeof(SplitterInfo::InDestinationParams));
        UpdateOffsets(sizeof(SplitterInfo::InDestinationParams));

        // Logged as warning as these don't actually cause a bailout for some reason
        if (header.magic != SplitterMagic::DataHeader) {
            LOG_ERROR(Audio, "Bad splitter data header");
            break;
        }

        if (header.splitter_id < 0 || static_cast<std::size_t>(header.splitter_id) > data_count) {
            LOG_ERROR(Audio, "Bad splitter data id");
            break;
        }
        GetData(header.splitter_id).Update(header);
    }
    return true;
}

bool SplitterContext::RecomposeDestination(ServerSplitterInfo& info,
                                           SplitterInfo::InInfoPrams& header,
                                           const std::vector<u8>& input,
                                           const std::size_t& input_offset) {
    // Clear our current destinations
    auto* current_head = info.GetHead();
    while (current_head != nullptr) {
        auto* next_head = current_head->GetNextDestination();
        current_head->SetNextDestination(nullptr);
        current_head = next_head;
    }
    info.SetHead(nullptr);

    s32 size = header.length;
    // If the splitter bug is present, calculate fixed size
    if (!bug_fixed) {
        if (info_count > 0) {
            const auto factor = data_count / info_count;
            size = std::min(header.length, static_cast<s32>(factor));
        } else {
            size = 0;
        }
    }

    if (size < 1) {
        LOG_ERROR(Audio, "Invalid splitter info size! size={:X}", size);
        return true;
    }

    auto* start_head = &GetData(header.resource_id_base);
    current_head = start_head;
    std::vector<s32_le> resource_ids(size - 1);
    if (!AudioCommon::CanConsumeBuffer(input.size(), input_offset,
                                       resource_ids.size() * sizeof(s32_le))) {
        LOG_ERROR(Audio, "Buffer is an invalid size!");
        return false;
    }
    std::memcpy(resource_ids.data(), input.data() + input_offset,
                resource_ids.size() * sizeof(s32_le));

    for (auto resource_id : resource_ids) {
        auto* head = &GetData(resource_id);
        current_head->SetNextDestination(head);
        current_head = head;
    }

    info.SetHead(start_head);
    info.SetHeadDepth(size);

    return true;
}

NodeStates::NodeStates() = default;
NodeStates::~NodeStates() = default;

void NodeStates::Initialize(std::size_t node_count_) {
    // Setup our work parameters
    node_count = node_count_;
    was_node_found.resize(node_count);
    was_node_completed.resize(node_count);
    index_list.resize(node_count);
    index_stack.Reset(node_count * node_count);
}

bool NodeStates::Tsort(EdgeMatrix& edge_matrix) {
    return DepthFirstSearch(edge_matrix);
}

std::size_t NodeStates::GetIndexPos() const {
    return index_pos;
}

const std::vector<s32>& NodeStates::GetIndexList() const {
    return index_list;
}

void NodeStates::PushTsortResult(s32 index) {
    ASSERT(index < static_cast<s32>(node_count));
    index_list[index_pos++] = index;
}

bool NodeStates::DepthFirstSearch(EdgeMatrix& edge_matrix) {
    ResetState();
    for (std::size_t i = 0; i < node_count; i++) {
        const auto node_id = static_cast<s32>(i);

        // If we don't have a state, send to our index stack for work
        if (GetState(i) == NodeStates::State::NoState) {
            index_stack.push(node_id);
        }

        // While we have work to do in our stack
        while (index_stack.Count() > 0) {
            // Get the current node
            const auto current_stack_index = index_stack.top();
            // Check if we've seen the node yet
            const auto index_state = GetState(current_stack_index);
            if (index_state == NodeStates::State::NoState) {
                // Mark the node as seen
                UpdateState(NodeStates::State::InFound, current_stack_index);
            } else if (index_state == NodeStates::State::InFound) {
                // We've seen this node before, mark it as completed
                UpdateState(NodeStates::State::InCompleted, current_stack_index);
                // Update our index list
                PushTsortResult(current_stack_index);
                // Pop the stack
                index_stack.pop();
                continue;
            } else if (index_state == NodeStates::State::InCompleted) {
                // If our node is already sorted, clear it
                index_stack.pop();
                continue;
            }

            const auto edge_node_count = edge_matrix.GetNodeCount();
            for (s32 j = 0; j < static_cast<s32>(edge_node_count); j++) {
                // Check if our node is connected to our edge matrix
                if (!edge_matrix.Connected(current_stack_index, j)) {
                    continue;
                }

                // Check if our node exists
                const auto node_state = GetState(j);
                if (node_state == NodeStates::State::NoState) {
                    // Add more work
                    index_stack.push(j);
                } else if (node_state == NodeStates::State::InFound) {
                    UNREACHABLE_MSG("Node start marked as found");
                    ResetState();
                    return false;
                }
            }
        }
    }
    return true;
}

void NodeStates::ResetState() {
    // Reset to the start of our index stack
    index_pos = 0;
    for (std::size_t i = 0; i < node_count; i++) {
        // Mark all nodes as not found
        was_node_found[i] = false;
        // Mark all nodes as uncompleted
        was_node_completed[i] = false;
        // Mark all indexes as invalid
        index_list[i] = -1;
    }
}

void NodeStates::UpdateState(NodeStates::State state, std::size_t i) {
    switch (state) {
    case NodeStates::State::NoState:
        was_node_found[i] = false;
        was_node_completed[i] = false;
        break;
    case NodeStates::State::InFound:
        was_node_found[i] = true;
        was_node_completed[i] = false;
        break;
    case NodeStates::State::InCompleted:
        was_node_found[i] = false;
        was_node_completed[i] = true;
        break;
    }
}

NodeStates::State NodeStates::GetState(std::size_t i) {
    ASSERT(i < node_count);
    if (was_node_found[i]) {
        // If our node exists in our found list
        return NodeStates::State::InFound;
    } else if (was_node_completed[i]) {
        // If node is in the completed list
        return NodeStates::State::InCompleted;
    } else {
        // If in neither
        return NodeStates::State::NoState;
    }
}

NodeStates::Stack::Stack() = default;
NodeStates::Stack::~Stack() = default;

void NodeStates::Stack::Reset(std::size_t size) {
    // Mark our stack as empty
    stack.resize(size);
    stack_size = size;
    stack_pos = 0;
    std::fill(stack.begin(), stack.end(), 0);
}

void NodeStates::Stack::push(s32 val) {
    ASSERT(stack_pos < stack_size);
    stack[stack_pos++] = val;
}

std::size_t NodeStates::Stack::Count() const {
    return stack_pos;
}

s32 NodeStates::Stack::top() const {
    ASSERT(stack_pos > 0);
    return stack[stack_pos - 1];
}

s32 NodeStates::Stack::pop() {
    ASSERT(stack_pos > 0);
    stack_pos--;
    return stack[stack_pos];
}

EdgeMatrix::EdgeMatrix() = default;
EdgeMatrix::~EdgeMatrix() = default;

void EdgeMatrix::Initialize(std::size_t _node_count) {
    node_count = _node_count;
    edge_matrix.resize(node_count * node_count);
}

bool EdgeMatrix::Connected(s32 a, s32 b) {
    return GetState(a, b);
}

void EdgeMatrix::Connect(s32 a, s32 b) {
    SetState(a, b, true);
}

void EdgeMatrix::Disconnect(s32 a, s32 b) {
    SetState(a, b, false);
}

void EdgeMatrix::RemoveEdges(s32 edge) {
    for (std::size_t i = 0; i < node_count; i++) {
        SetState(edge, static_cast<s32>(i), false);
    }
}

std::size_t EdgeMatrix::GetNodeCount() const {
    return node_count;
}

void EdgeMatrix::SetState(s32 a, s32 b, bool state) {
    ASSERT(InRange(a, b));
    edge_matrix.at(a * node_count + b) = state;
}

bool EdgeMatrix::GetState(s32 a, s32 b) {
    ASSERT(InRange(a, b));
    return edge_matrix.at(a * node_count + b);
}

bool EdgeMatrix::InRange(s32 a, s32 b) const {
    const std::size_t pos = a * node_count + b;
    return pos < (node_count * node_count);
}

} // namespace AudioCore
