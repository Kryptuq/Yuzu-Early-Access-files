// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <list>
#include <map>
#include <optional>
#include <set>
#include <tuple>
#include <vector>

#include "common/common_types.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/engines/shader_bytecode.h"
#include "video_core/engines/shader_header.h"
#include "video_core/shader/ast.h"
#include "video_core/shader/compiler_settings.h"
#include "video_core/shader/memory_util.h"
#include "video_core/shader/node.h"
#include "video_core/shader/registry.h"

namespace VideoCommon::Shader {

struct ShaderBlock;

constexpr u32 MAX_PROGRAM_LENGTH = 0x1000;

struct ConstBuffer {
    constexpr explicit ConstBuffer(u32 max_offset_, bool is_indirect_)
        : max_offset{max_offset_}, is_indirect{is_indirect_} {}

    constexpr ConstBuffer() = default;

    void MarkAsUsed(u64 offset) {
        max_offset = std::max(max_offset, static_cast<u32>(offset));
    }

    void MarkAsUsedIndirect() {
        is_indirect = true;
    }

    bool IsIndirect() const {
        return is_indirect;
    }

    u32 GetSize() const {
        return max_offset + static_cast<u32>(sizeof(float));
    }

    u32 GetMaxOffset() const {
        return max_offset;
    }

private:
    u32 max_offset = 0;
    bool is_indirect = false;
};

struct GlobalMemoryUsage {
    bool is_read{};
    bool is_written{};
};

class ShaderIR final {
public:
    explicit ShaderIR(const ProgramCode& program_code_, u32 main_offset_,
                      CompilerSettings settings_, Registry& registry_);
    ~ShaderIR();

    const std::map<u32, NodeBlock>& GetBasicBlocks() const {
        return basic_blocks;
    }

    const std::set<u32>& GetRegisters() const {
        return used_registers;
    }

    const std::set<Tegra::Shader::Pred>& GetPredicates() const {
        return used_predicates;
    }

    const std::set<Tegra::Shader::Attribute::Index>& GetInputAttributes() const {
        return used_input_attributes;
    }

    const std::set<Tegra::Shader::Attribute::Index>& GetOutputAttributes() const {
        return used_output_attributes;
    }

    const std::map<u32, ConstBuffer>& GetConstantBuffers() const {
        return used_cbufs;
    }

    const std::list<SamplerEntry>& GetSamplers() const {
        return used_samplers;
    }

    const std::list<ImageEntry>& GetImages() const {
        return used_images;
    }

    const std::array<bool, Tegra::Engines::Maxwell3D::Regs::NumClipDistances>& GetClipDistances()
        const {
        return used_clip_distances;
    }

    const std::map<GlobalMemoryBase, GlobalMemoryUsage>& GetGlobalMemory() const {
        return used_global_memory;
    }

    std::size_t GetLength() const {
        return static_cast<std::size_t>(coverage_end * sizeof(u64));
    }

    bool UsesLayer() const {
        return uses_layer;
    }

    bool UsesViewportIndex() const {
        return uses_viewport_index;
    }

    bool UsesPointSize() const {
        return uses_point_size;
    }

    bool UsesInstanceId() const {
        return uses_instance_id;
    }

    bool UsesVertexId() const {
        return uses_vertex_id;
    }

    bool UsesLegacyVaryings() const {
        return uses_legacy_varyings;
    }

    bool UsesYNegate() const {
        return uses_y_negate;
    }

    bool UsesWarps() const {
        return uses_warps;
    }

    bool HasPhysicalAttributes() const {
        return uses_physical_attributes;
    }

    const Tegra::Shader::Header& GetHeader() const {
        return header;
    }

    bool IsFlowStackDisabled() const {
        return disable_flow_stack;
    }

    bool IsDecompiled() const {
        return decompiled;
    }

    const ASTManager& GetASTManager() const {
        return program_manager;
    }

    ASTNode GetASTProgram() const {
        return program_manager.GetProgram();
    }

    u32 GetASTNumVariables() const {
        return program_manager.GetVariables();
    }

    u32 ConvertAddressToNvidiaSpace(u32 address) const {
        return (address - main_offset) * static_cast<u32>(sizeof(Tegra::Shader::Instruction));
    }

    /// Returns a condition code evaluated from internal flags
    Node GetConditionCode(Tegra::Shader::ConditionCode cc) const;

    const Node& GetAmendNode(std::size_t index) const {
        return amend_code[index];
    }

    u32 GetNumCustomVariables() const {
        return num_custom_variables;
    }

private:
    friend class ASTDecoder;

    struct SamplerInfo {
        std::optional<Tegra::Shader::TextureType> type;
        std::optional<bool> is_array;
        std::optional<bool> is_shadow;
        std::optional<bool> is_buffer;

        constexpr bool IsComplete() const noexcept {
            return type && is_array && is_shadow && is_buffer;
        }
    };

    void Decode();
    void PostDecode();

    NodeBlock DecodeRange(u32 begin, u32 end);
    void DecodeRangeInner(NodeBlock& bb, u32 begin, u32 end);
    void InsertControlFlow(NodeBlock& bb, const ShaderBlock& block);

    /**
     * Decodes a single instruction from Tegra to IR.
     * @param bb Basic block where the nodes will be written to.
     * @param pc Program counter. Offset to decode.
     * @return Next address to decode.
     */
    u32 DecodeInstr(NodeBlock& bb, u32 pc);

    u32 DecodeArithmetic(NodeBlock& bb, u32 pc);
    u32 DecodeArithmeticImmediate(NodeBlock& bb, u32 pc);
    u32 DecodeBfe(NodeBlock& bb, u32 pc);
    u32 DecodeBfi(NodeBlock& bb, u32 pc);
    u32 DecodeShift(NodeBlock& bb, u32 pc);
    u32 DecodeArithmeticInteger(NodeBlock& bb, u32 pc);
    u32 DecodeArithmeticIntegerImmediate(NodeBlock& bb, u32 pc);
    u32 DecodeArithmeticHalf(NodeBlock& bb, u32 pc);
    u32 DecodeArithmeticHalfImmediate(NodeBlock& bb, u32 pc);
    u32 DecodeFfma(NodeBlock& bb, u32 pc);
    u32 DecodeHfma2(NodeBlock& bb, u32 pc);
    u32 DecodeConversion(NodeBlock& bb, u32 pc);
    u32 DecodeWarp(NodeBlock& bb, u32 pc);
    u32 DecodeMemory(NodeBlock& bb, u32 pc);
    u32 DecodeTexture(NodeBlock& bb, u32 pc);
    u32 DecodeImage(NodeBlock& bb, u32 pc);
    u32 DecodeFloatSetPredicate(NodeBlock& bb, u32 pc);
    u32 DecodeIntegerSetPredicate(NodeBlock& bb, u32 pc);
    u32 DecodeHalfSetPredicate(NodeBlock& bb, u32 pc);
    u32 DecodePredicateSetRegister(NodeBlock& bb, u32 pc);
    u32 DecodePredicateSetPredicate(NodeBlock& bb, u32 pc);
    u32 DecodeRegisterSetPredicate(NodeBlock& bb, u32 pc);
    u32 DecodeFloatSet(NodeBlock& bb, u32 pc);
    u32 DecodeIntegerSet(NodeBlock& bb, u32 pc);
    u32 DecodeHalfSet(NodeBlock& bb, u32 pc);
    u32 DecodeVideo(NodeBlock& bb, u32 pc);
    u32 DecodeXmad(NodeBlock& bb, u32 pc);
    u32 DecodeOther(NodeBlock& bb, u32 pc);

    /// Generates a node for a passed register.
    Node GetRegister(Tegra::Shader::Register reg);
    /// Generates a node for a custom variable
    Node GetCustomVariable(u32 id);
    /// Generates a node representing a 19-bit immediate value
    Node GetImmediate19(Tegra::Shader::Instruction instr);
    /// Generates a node representing a 32-bit immediate value
    Node GetImmediate32(Tegra::Shader::Instruction instr);
    /// Generates a node representing a constant buffer
    Node GetConstBuffer(u64 index, u64 offset);
    /// Generates a node representing a constant buffer with a variadic offset
    Node GetConstBufferIndirect(u64 index, u64 offset, Node node);
    /// Generates a node for a passed predicate. It can be optionally negated
    Node GetPredicate(u64 pred, bool negated = false);
    /// Generates a predicate node for an immediate true or false value
    Node GetPredicate(bool immediate);
    /// Generates a node representing an input attribute. Keeps track of used attributes.
    Node GetInputAttribute(Tegra::Shader::Attribute::Index index, u64 element, Node buffer = {});
    /// Generates a node representing a physical input attribute.
    Node GetPhysicalInputAttribute(Tegra::Shader::Register physical_address, Node buffer = {});
    /// Generates a node representing an output attribute. Keeps track of used attributes.
    Node GetOutputAttribute(Tegra::Shader::Attribute::Index index, u64 element, Node buffer);
    /// Generates a node representing an internal flag
    Node GetInternalFlag(InternalFlag flag, bool negated = false) const;
    /// Generates a node representing a local memory address
    Node GetLocalMemory(Node address);
    /// Generates a node representing a shared memory address
    Node GetSharedMemory(Node address);
    /// Generates a temporary, internally it uses a post-RZ register
    Node GetTemporary(u32 id);

    /// Sets a register. src value must be a number-evaluated node.
    void SetRegister(NodeBlock& bb, Tegra::Shader::Register dest, Node src);
    /// Sets a predicate. src value must be a bool-evaluated node
    void SetPredicate(NodeBlock& bb, u64 dest, Node src);
    /// Sets an internal flag. src value must be a bool-evaluated node
    void SetInternalFlag(NodeBlock& bb, InternalFlag flag, Node value);
    /// Sets a local memory address with a value.
    void SetLocalMemory(NodeBlock& bb, Node address, Node value);
    /// Sets a shared memory address with a value.
    void SetSharedMemory(NodeBlock& bb, Node address, Node value);
    /// Sets a temporary. Internally it uses a post-RZ register
    void SetTemporary(NodeBlock& bb, u32 id, Node value);

    /// Sets internal flags from a float
    void SetInternalFlagsFromFloat(NodeBlock& bb, Node value, bool sets_cc = true);
    /// Sets internal flags from an integer
    void SetInternalFlagsFromInteger(NodeBlock& bb, Node value, bool sets_cc = true);

    /// Conditionally absolute/negated float. Absolute is applied first
    Node GetOperandAbsNegFloat(Node value, bool absolute, bool negate);
    /// Conditionally saturates a float
    Node GetSaturatedFloat(Node value, bool saturate = true);

    /// Converts an integer to different sizes.
    Node ConvertIntegerSize(Node value, Tegra::Shader::Register::Size size, bool is_signed);
    /// Conditionally absolute/negated integer. Absolute is applied first
    Node GetOperandAbsNegInteger(Node value, bool absolute, bool negate, bool is_signed);

    /// Unpacks a half immediate from an instruction
    Node UnpackHalfImmediate(Tegra::Shader::Instruction instr, bool has_negation);
    /// Unpacks a binary value into a half float pair with a type format
    Node UnpackHalfFloat(Node value, Tegra::Shader::HalfType type);
    /// Merges a half pair into another value
    Node HalfMerge(Node dest, Node src, Tegra::Shader::HalfMerge merge);
    /// Conditionally absolute/negated half float pair. Absolute is applied first
    Node GetOperandAbsNegHalf(Node value, bool absolute, bool negate);
    /// Conditionally saturates a half float pair
    Node GetSaturatedHalfFloat(Node value, bool saturate = true);

    /// Get image component value by type and size
    std::pair<Node, bool> GetComponentValue(Tegra::Texture::ComponentType component_type,
                                            u32 component_size, Node original_value);

    /// Returns a predicate comparing two floats
    Node GetPredicateComparisonFloat(Tegra::Shader::PredCondition condition, Node op_a, Node op_b);
    /// Returns a predicate comparing two integers
    Node GetPredicateComparisonInteger(Tegra::Shader::PredCondition condition, bool is_signed,
                                       Node op_a, Node op_b);
    /// Returns a predicate comparing two half floats. meta consumes how both pairs will be compared
    Node GetPredicateComparisonHalf(Tegra::Shader::PredCondition condition, Node op_a, Node op_b);

    /// Returns a predicate combiner operation
    OperationCode GetPredicateCombiner(Tegra::Shader::PredOperation operation);

    /// Queries the missing sampler info from the execution context.
    SamplerInfo GetSamplerInfo(SamplerInfo info,
                               std::optional<Tegra::Engines::SamplerDescriptor> sampler);

    /// Accesses a texture sampler.
    std::optional<SamplerEntry> GetSampler(Tegra::Shader::Sampler sampler, SamplerInfo info);

    /// Accesses a texture sampler for a bindless texture.
    std::optional<SamplerEntry> GetBindlessSampler(Tegra::Shader::Register reg, SamplerInfo info,
                                                   Node& index_var);

    /// Accesses an image.
    ImageEntry& GetImage(Tegra::Shader::Image image, Tegra::Shader::ImageType type);

    /// Access a bindless image sampler.
    ImageEntry& GetBindlessImage(Tegra::Shader::Register reg, Tegra::Shader::ImageType type);

    /// Extracts a sequence of bits from a node
    Node BitfieldExtract(Node value, u32 offset, u32 bits);

    /// Inserts a sequence of bits from a node
    Node BitfieldInsert(Node base, Node insert, u32 offset, u32 bits);

    /// Marks the usage of a input or output attribute.
    void MarkAttributeUsage(Tegra::Shader::Attribute::Index index, u64 element);

    /// Decodes VMNMX instruction and inserts its code into the passed basic block.
    void DecodeVMNMX(NodeBlock& bb, Tegra::Shader::Instruction instr);

    void WriteTexInstructionFloat(NodeBlock& bb, Tegra::Shader::Instruction instr,
                                  const Node4& components);

    void WriteTexsInstructionFloat(NodeBlock& bb, Tegra::Shader::Instruction instr,
                                   const Node4& components, bool ignore_mask = false);
    void WriteTexsInstructionHalfFloat(NodeBlock& bb, Tegra::Shader::Instruction instr,
                                       const Node4& components, bool ignore_mask = false);

    Node4 GetTexCode(Tegra::Shader::Instruction instr, Tegra::Shader::TextureType texture_type,
                     Tegra::Shader::TextureProcessMode process_mode, bool depth_compare,
                     bool is_array, bool is_aoffi,
                     std::optional<Tegra::Shader::Register> bindless_reg);

    Node4 GetTexsCode(Tegra::Shader::Instruction instr, Tegra::Shader::TextureType texture_type,
                      Tegra::Shader::TextureProcessMode process_mode, bool depth_compare,
                      bool is_array);

    Node4 GetTld4Code(Tegra::Shader::Instruction instr, Tegra::Shader::TextureType texture_type,
                      bool depth_compare, bool is_array, bool is_aoffi, bool is_ptp,
                      bool is_bindless);

    Node4 GetTldCode(Tegra::Shader::Instruction instr);

    Node4 GetTldsCode(Tegra::Shader::Instruction instr, Tegra::Shader::TextureType texture_type,
                      bool is_array);

    std::tuple<std::size_t, std::size_t> ValidateAndGetCoordinateElement(
        Tegra::Shader::TextureType texture_type, bool depth_compare, bool is_array,
        bool lod_bias_enabled, std::size_t max_coords, std::size_t max_inputs);

    std::vector<Node> GetAoffiCoordinates(Node aoffi_reg, std::size_t coord_count, bool is_tld4);

    std::vector<Node> GetPtpCoordinates(std::array<Node, 2> ptp_regs);

    Node4 GetTextureCode(Tegra::Shader::Instruction instr, Tegra::Shader::TextureType texture_type,
                         Tegra::Shader::TextureProcessMode process_mode, std::vector<Node> coords,
                         Node array, Node depth_compare, u32 bias_offset, std::vector<Node> aoffi,
                         std::optional<Tegra::Shader::Register> bindless_reg);

    Node GetVideoOperand(Node op, bool is_chunk, bool is_signed, Tegra::Shader::VideoType type,
                         u64 byte_height);

    void WriteLogicOperation(NodeBlock& bb, Tegra::Shader::Register dest,
                             Tegra::Shader::LogicOperation logic_op, Node op_a, Node op_b,
                             Tegra::Shader::PredicateResultMode predicate_mode,
                             Tegra::Shader::Pred predicate, bool sets_cc);
    void WriteLop3Instruction(NodeBlock& bb, Tegra::Shader::Register dest, Node op_a, Node op_b,
                              Node op_c, Node imm_lut, bool sets_cc);

    std::tuple<Node, u32, u32> TrackCbuf(Node tracked, const NodeBlock& code, s64 cursor) const;

    std::pair<Node, TrackSampler> TrackBindlessSampler(Node tracked, const NodeBlock& code,
                                                       s64 cursor);

    std::pair<Node, TrackSampler> HandleBindlessIndirectRead(const CbufNode& cbuf,
                                                             const OperationNode& operation,
                                                             Node gpr, Node base_offset,
                                                             Node tracked, const NodeBlock& code,
                                                             s64 cursor);

    std::optional<u32> TrackImmediate(Node tracked, const NodeBlock& code, s64 cursor) const;

    std::pair<Node, s64> TrackRegister(const GprNode* tracked, const NodeBlock& code,
                                       s64 cursor) const;

    std::tuple<Node, Node, GlobalMemoryBase> TrackGlobalMemory(NodeBlock& bb,
                                                               Tegra::Shader::Instruction instr,
                                                               bool is_read, bool is_write);

    /// Register new amending code and obtain the reference id.
    std::size_t DeclareAmend(Node new_amend);

    u32 NewCustomVariable();

    const ProgramCode& program_code;
    const u32 main_offset;
    const CompilerSettings settings;
    Registry& registry;

    bool decompiled{};
    bool disable_flow_stack{};

    u32 coverage_begin{};
    u32 coverage_end{};

    std::map<u32, NodeBlock> basic_blocks;
    NodeBlock global_code;
    ASTManager program_manager{true, true};
    std::vector<Node> amend_code;
    u32 num_custom_variables{};

    std::set<u32> used_registers;
    std::set<Tegra::Shader::Pred> used_predicates;
    std::set<Tegra::Shader::Attribute::Index> used_input_attributes;
    std::set<Tegra::Shader::Attribute::Index> used_output_attributes;
    std::map<u32, ConstBuffer> used_cbufs;
    std::list<SamplerEntry> used_samplers;
    std::list<ImageEntry> used_images;
    std::array<bool, Tegra::Engines::Maxwell3D::Regs::NumClipDistances> used_clip_distances{};
    std::map<GlobalMemoryBase, GlobalMemoryUsage> used_global_memory;
    bool uses_layer{};
    bool uses_viewport_index{};
    bool uses_point_size{};
    bool uses_physical_attributes{}; // Shader uses AL2P or physical attribute read/writes
    bool uses_instance_id{};
    bool uses_vertex_id{};
    bool uses_legacy_varyings{};
    bool uses_y_negate{};
    bool uses_warps{};
    bool uses_indexed_samplers{};

    Tegra::Shader::Header header;
};

} // namespace VideoCommon::Shader
