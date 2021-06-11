// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <stack>
#include <vector>
#include "audio_core/common.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/swap.h"

namespace AudioCore {
class BehaviorInfo;

class EdgeMatrix {
public:
    EdgeMatrix();
    ~EdgeMatrix();

    void Initialize(std::size_t _node_count);
    bool Connected(s32 a, s32 b);
    void Connect(s32 a, s32 b);
    void Disconnect(s32 a, s32 b);
    void RemoveEdges(s32 edge);
    std::size_t GetNodeCount() const;

private:
    void SetState(s32 a, s32 b, bool state);
    bool GetState(s32 a, s32 b);

    bool InRange(s32 a, s32 b) const;
    std::vector<bool> edge_matrix{};
    std::size_t node_count{};
};

class NodeStates {
public:
    enum class State {
        NoState = 0,
        InFound = 1,
        InCompleted = 2,
    };

    // Looks to be a fixed size stack. Placed within the NodeStates class based on symbols
    class Stack {
    public:
        Stack();
        ~Stack();

        void Reset(std::size_t size);
        void push(s32 val);
        std::size_t Count() const;
        s32 top() const;
        s32 pop();

    private:
        std::vector<s32> stack{};
        std::size_t stack_size{};
        std::size_t stack_pos{};
    };
    NodeStates();
    ~NodeStates();

    void Initialize(std::size_t node_count_);
    bool Tsort(EdgeMatrix& edge_matrix);
    std::size_t GetIndexPos() const;
    const std::vector<s32>& GetIndexList() const;

private:
    void PushTsortResult(s32 index);
    bool DepthFirstSearch(EdgeMatrix& edge_matrix);
    void ResetState();
    void UpdateState(State state, std::size_t i);
    State GetState(std::size_t i);

    std::size_t node_count{};
    std::vector<bool> was_node_found{};
    std::vector<bool> was_node_completed{};
    std::size_t index_pos{};
    std::vector<s32> index_list{};
    Stack index_stack{};
};

enum class SplitterMagic : u32_le {
    SplitterHeader = Common::MakeMagic('S', 'N', 'D', 'H'),
    DataHeader = Common::MakeMagic('S', 'N', 'D', 'D'),
    InfoHeader = Common::MakeMagic('S', 'N', 'D', 'I'),
};

class SplitterInfo {
public:
    struct InHeader {
        SplitterMagic magic{};
        s32_le info_count{};
        s32_le data_count{};
        INSERT_PADDING_WORDS(5);
    };
    static_assert(sizeof(InHeader) == 0x20, "SplitterInfo::InHeader is an invalid size");

    struct InInfoPrams {
        SplitterMagic magic{};
        s32_le send_id{};
        s32_le sample_rate{};
        s32_le length{};
        s32_le resource_id_base{};
    };
    static_assert(sizeof(InInfoPrams) == 0x14, "SplitterInfo::InInfoPrams is an invalid size");

    struct InDestinationParams {
        SplitterMagic magic{};
        s32_le splitter_id{};
        std::array<float_le, AudioCommon::MAX_MIX_BUFFERS> mix_volumes{};
        s32_le mix_id{};
        bool in_use{};
        INSERT_PADDING_BYTES(3);
    };
    static_assert(sizeof(InDestinationParams) == 0x70,
                  "SplitterInfo::InDestinationParams is an invalid size");
};

class ServerSplitterDestinationData {
public:
    explicit ServerSplitterDestinationData(s32 id_);
    ~ServerSplitterDestinationData();

    void Update(SplitterInfo::InDestinationParams& header);

    ServerSplitterDestinationData* GetNextDestination();
    const ServerSplitterDestinationData* GetNextDestination() const;
    void SetNextDestination(ServerSplitterDestinationData* dest);
    bool ValidMixId() const;
    s32 GetMixId() const;
    bool IsConfigured() const;
    float GetMixVolume(std::size_t i) const;
    const std::array<float, AudioCommon::MAX_MIX_BUFFERS>& CurrentMixVolumes() const;
    const std::array<float, AudioCommon::MAX_MIX_BUFFERS>& LastMixVolumes() const;
    void MarkDirty();
    void UpdateInternalState();

private:
    bool needs_update{};
    bool in_use{};
    s32 id{};
    s32 mix_id{};
    std::array<float, AudioCommon::MAX_MIX_BUFFERS> current_mix_volumes{};
    std::array<float, AudioCommon::MAX_MIX_BUFFERS> last_mix_volumes{};
    ServerSplitterDestinationData* next = nullptr;
};

class ServerSplitterInfo {
public:
    explicit ServerSplitterInfo(s32 id_);
    ~ServerSplitterInfo();

    void InitializeInfos();
    void ClearNewConnectionFlag();
    std::size_t Update(SplitterInfo::InInfoPrams& header);

    ServerSplitterDestinationData* GetHead();
    const ServerSplitterDestinationData* GetHead() const;
    ServerSplitterDestinationData* GetData(std::size_t depth);
    const ServerSplitterDestinationData* GetData(std::size_t depth) const;

    bool HasNewConnection() const;
    s32 GetLength() const;

    void SetHead(ServerSplitterDestinationData* new_head);
    void SetHeadDepth(s32 length);

private:
    s32 sample_rate{};
    s32 id{};
    s32 send_length{};
    ServerSplitterDestinationData* head = nullptr;
    bool new_connection{};
};

class SplitterContext {
public:
    SplitterContext();
    ~SplitterContext();

    void Initialize(BehaviorInfo& behavior_info, std::size_t splitter_count,
                    std::size_t data_count);

    bool Update(const std::vector<u8>& input, std::size_t& input_offset, std::size_t& bytes_read);
    bool UsingSplitter() const;

    ServerSplitterInfo& GetInfo(std::size_t i);
    const ServerSplitterInfo& GetInfo(std::size_t i) const;
    ServerSplitterDestinationData& GetData(std::size_t i);
    const ServerSplitterDestinationData& GetData(std::size_t i) const;
    ServerSplitterDestinationData* GetDestinationData(std::size_t info, std::size_t data);
    const ServerSplitterDestinationData* GetDestinationData(std::size_t info,
                                                            std::size_t data) const;
    void UpdateInternalState();

    std::size_t GetInfoCount() const;
    std::size_t GetDataCount() const;

private:
    void Setup(std::size_t info_count, std::size_t data_count, bool is_splitter_bug_fixed);
    bool UpdateInfo(const std::vector<u8>& input, std::size_t& input_offset,
                    std::size_t& bytes_read, s32 in_splitter_count);
    bool UpdateData(const std::vector<u8>& input, std::size_t& input_offset,
                    std::size_t& bytes_read, s32 in_data_count);
    bool RecomposeDestination(ServerSplitterInfo& info, SplitterInfo::InInfoPrams& header,
                              const std::vector<u8>& input, const std::size_t& input_offset);

    std::vector<ServerSplitterInfo> infos{};
    std::vector<ServerSplitterDestinationData> datas{};

    std::size_t info_count{};
    std::size_t data_count{};
    bool bug_fixed{};
};
} // namespace AudioCore
