// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include <fmt/format.h>

#include <sirit/sirit.h>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/common_types.h"
#include "common/logging/log.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/engines/shader_bytecode.h"
#include "video_core/engines/shader_header.h"
#include "video_core/engines/shader_type.h"
#include "video_core/renderer_vulkan/vk_shader_decompiler.h"
#include "video_core/shader/node.h"
#include "video_core/shader/shader_ir.h"
#include "video_core/shader/transform_feedback.h"
#include "video_core/vulkan_common/vulkan_device.h"

namespace Vulkan {

namespace {

using Sirit::Id;
using Tegra::Engines::ShaderType;
using Tegra::Shader::Attribute;
using Tegra::Shader::PixelImap;
using Tegra::Shader::Register;
using namespace VideoCommon::Shader;

using Maxwell = Tegra::Engines::Maxwell3D::Regs;
using Operation = const OperationNode&;

class ASTDecompiler;
class ExprDecompiler;

// TODO(Rodrigo): Use rasterizer's value
constexpr u32 MaxConstBufferFloats = 0x4000;
constexpr u32 MaxConstBufferElements = MaxConstBufferFloats / 4;

constexpr u32 NumInputPatches = 32; // This value seems to be the standard

enum class Type { Void, Bool, Bool2, Float, Int, Uint, HalfFloat };

class Expression final {
public:
    Expression(Id id_, Type type_) : id{id_}, type{type_} {
        ASSERT(type_ != Type::Void);
    }
    Expression() : type{Type::Void} {}

    Id id{};
    Type type{};
};
static_assert(std::is_standard_layout_v<Expression>);

struct TexelBuffer {
    Id image_type{};
    Id image{};
};

struct SampledImage {
    Id image_type{};
    Id sampler_type{};
    Id sampler_pointer_type{};
    Id variable{};
};

struct StorageImage {
    Id image_type{};
    Id image{};
};

struct AttributeType {
    Type type;
    Id scalar;
    Id vector;
};

struct VertexIndices {
    std::optional<u32> position;
    std::optional<u32> layer;
    std::optional<u32> viewport;
    std::optional<u32> point_size;
    std::optional<u32> clip_distances;
};

struct GenericVaryingDescription {
    Id id = nullptr;
    u32 first_element = 0;
    bool is_scalar = false;
};

spv::Dim GetSamplerDim(const SamplerEntry& sampler) {
    ASSERT(!sampler.is_buffer);
    switch (sampler.type) {
    case Tegra::Shader::TextureType::Texture1D:
        return spv::Dim::Dim1D;
    case Tegra::Shader::TextureType::Texture2D:
        return spv::Dim::Dim2D;
    case Tegra::Shader::TextureType::Texture3D:
        return spv::Dim::Dim3D;
    case Tegra::Shader::TextureType::TextureCube:
        return spv::Dim::Cube;
    default:
        UNIMPLEMENTED_MSG("Unimplemented sampler type={}", sampler.type);
        return spv::Dim::Dim2D;
    }
}

std::pair<spv::Dim, bool> GetImageDim(const ImageEntry& image) {
    switch (image.type) {
    case Tegra::Shader::ImageType::Texture1D:
        return {spv::Dim::Dim1D, false};
    case Tegra::Shader::ImageType::TextureBuffer:
        return {spv::Dim::Buffer, false};
    case Tegra::Shader::ImageType::Texture1DArray:
        return {spv::Dim::Dim1D, true};
    case Tegra::Shader::ImageType::Texture2D:
        return {spv::Dim::Dim2D, false};
    case Tegra::Shader::ImageType::Texture2DArray:
        return {spv::Dim::Dim2D, true};
    case Tegra::Shader::ImageType::Texture3D:
        return {spv::Dim::Dim3D, false};
    default:
        UNIMPLEMENTED_MSG("Unimplemented image type={}", image.type);
        return {spv::Dim::Dim2D, false};
    }
}

/// Returns the number of vertices present in a primitive topology.
u32 GetNumPrimitiveTopologyVertices(Maxwell::PrimitiveTopology primitive_topology) {
    switch (primitive_topology) {
    case Maxwell::PrimitiveTopology::Points:
        return 1;
    case Maxwell::PrimitiveTopology::Lines:
    case Maxwell::PrimitiveTopology::LineLoop:
    case Maxwell::PrimitiveTopology::LineStrip:
        return 2;
    case Maxwell::PrimitiveTopology::Triangles:
    case Maxwell::PrimitiveTopology::TriangleStrip:
    case Maxwell::PrimitiveTopology::TriangleFan:
        return 3;
    case Maxwell::PrimitiveTopology::LinesAdjacency:
    case Maxwell::PrimitiveTopology::LineStripAdjacency:
        return 4;
    case Maxwell::PrimitiveTopology::TrianglesAdjacency:
    case Maxwell::PrimitiveTopology::TriangleStripAdjacency:
        return 6;
    case Maxwell::PrimitiveTopology::Quads:
        UNIMPLEMENTED_MSG("Quads");
        return 3;
    case Maxwell::PrimitiveTopology::QuadStrip:
        UNIMPLEMENTED_MSG("QuadStrip");
        return 3;
    case Maxwell::PrimitiveTopology::Polygon:
        UNIMPLEMENTED_MSG("Polygon");
        return 3;
    case Maxwell::PrimitiveTopology::Patches:
        UNIMPLEMENTED_MSG("Patches");
        return 3;
    default:
        UNREACHABLE();
        return 3;
    }
}

spv::ExecutionMode GetExecutionMode(Maxwell::TessellationPrimitive primitive) {
    switch (primitive) {
    case Maxwell::TessellationPrimitive::Isolines:
        return spv::ExecutionMode::Isolines;
    case Maxwell::TessellationPrimitive::Triangles:
        return spv::ExecutionMode::Triangles;
    case Maxwell::TessellationPrimitive::Quads:
        return spv::ExecutionMode::Quads;
    }
    UNREACHABLE();
    return spv::ExecutionMode::Triangles;
}

spv::ExecutionMode GetExecutionMode(Maxwell::TessellationSpacing spacing) {
    switch (spacing) {
    case Maxwell::TessellationSpacing::Equal:
        return spv::ExecutionMode::SpacingEqual;
    case Maxwell::TessellationSpacing::FractionalOdd:
        return spv::ExecutionMode::SpacingFractionalOdd;
    case Maxwell::TessellationSpacing::FractionalEven:
        return spv::ExecutionMode::SpacingFractionalEven;
    }
    UNREACHABLE();
    return spv::ExecutionMode::SpacingEqual;
}

spv::ExecutionMode GetExecutionMode(Maxwell::PrimitiveTopology input_topology) {
    switch (input_topology) {
    case Maxwell::PrimitiveTopology::Points:
        return spv::ExecutionMode::InputPoints;
    case Maxwell::PrimitiveTopology::Lines:
    case Maxwell::PrimitiveTopology::LineLoop:
    case Maxwell::PrimitiveTopology::LineStrip:
        return spv::ExecutionMode::InputLines;
    case Maxwell::PrimitiveTopology::Triangles:
    case Maxwell::PrimitiveTopology::TriangleStrip:
    case Maxwell::PrimitiveTopology::TriangleFan:
        return spv::ExecutionMode::Triangles;
    case Maxwell::PrimitiveTopology::LinesAdjacency:
    case Maxwell::PrimitiveTopology::LineStripAdjacency:
        return spv::ExecutionMode::InputLinesAdjacency;
    case Maxwell::PrimitiveTopology::TrianglesAdjacency:
    case Maxwell::PrimitiveTopology::TriangleStripAdjacency:
        return spv::ExecutionMode::InputTrianglesAdjacency;
    case Maxwell::PrimitiveTopology::Quads:
        UNIMPLEMENTED_MSG("Quads");
        return spv::ExecutionMode::Triangles;
    case Maxwell::PrimitiveTopology::QuadStrip:
        UNIMPLEMENTED_MSG("QuadStrip");
        return spv::ExecutionMode::Triangles;
    case Maxwell::PrimitiveTopology::Polygon:
        UNIMPLEMENTED_MSG("Polygon");
        return spv::ExecutionMode::Triangles;
    case Maxwell::PrimitiveTopology::Patches:
        UNIMPLEMENTED_MSG("Patches");
        return spv::ExecutionMode::Triangles;
    }
    UNREACHABLE();
    return spv::ExecutionMode::Triangles;
}

spv::ExecutionMode GetExecutionMode(Tegra::Shader::OutputTopology output_topology) {
    switch (output_topology) {
    case Tegra::Shader::OutputTopology::PointList:
        return spv::ExecutionMode::OutputPoints;
    case Tegra::Shader::OutputTopology::LineStrip:
        return spv::ExecutionMode::OutputLineStrip;
    case Tegra::Shader::OutputTopology::TriangleStrip:
        return spv::ExecutionMode::OutputTriangleStrip;
    default:
        UNREACHABLE();
        return spv::ExecutionMode::OutputPoints;
    }
}

/// Returns true if an attribute index is one of the 32 generic attributes
constexpr bool IsGenericAttribute(Attribute::Index attribute) {
    return attribute >= Attribute::Index::Attribute_0 &&
           attribute <= Attribute::Index::Attribute_31;
}

/// Returns the location of a generic attribute
u32 GetGenericAttributeLocation(Attribute::Index attribute) {
    ASSERT(IsGenericAttribute(attribute));
    return static_cast<u32>(attribute) - static_cast<u32>(Attribute::Index::Attribute_0);
}

/// Returns true if an object has to be treated as precise
bool IsPrecise(Operation operand) {
    const auto& meta{operand.GetMeta()};
    if (std::holds_alternative<MetaArithmetic>(meta)) {
        return std::get<MetaArithmetic>(meta).precise;
    }
    return false;
}

class SPIRVDecompiler final : public Sirit::Module {
public:
    explicit SPIRVDecompiler(const Device& device_, const ShaderIR& ir_, ShaderType stage_,
                             const Registry& registry_, const Specialization& specialization_)
        : Module(0x00010300), device{device_}, ir{ir_}, stage{stage_}, header{ir_.GetHeader()},
          registry{registry_}, specialization{specialization_} {
        if (stage_ != ShaderType::Compute) {
            transform_feedback = BuildTransformFeedback(registry_.GetGraphicsInfo());
        }

        AddCapability(spv::Capability::Shader);
        AddCapability(spv::Capability::UniformAndStorageBuffer16BitAccess);
        AddCapability(spv::Capability::ImageQuery);
        AddCapability(spv::Capability::Image1D);
        AddCapability(spv::Capability::ImageBuffer);
        AddCapability(spv::Capability::ImageGatherExtended);
        AddCapability(spv::Capability::SampledBuffer);
        AddCapability(spv::Capability::StorageImageWriteWithoutFormat);
        AddCapability(spv::Capability::DrawParameters);
        AddCapability(spv::Capability::SubgroupBallotKHR);
        AddCapability(spv::Capability::SubgroupVoteKHR);
        AddExtension("SPV_KHR_16bit_storage");
        AddExtension("SPV_KHR_shader_ballot");
        AddExtension("SPV_KHR_subgroup_vote");
        AddExtension("SPV_KHR_storage_buffer_storage_class");
        AddExtension("SPV_KHR_variable_pointers");
        AddExtension("SPV_KHR_shader_draw_parameters");

        if (!transform_feedback.empty()) {
            if (device.IsExtTransformFeedbackSupported()) {
                AddCapability(spv::Capability::TransformFeedback);
            } else {
                LOG_ERROR(Render_Vulkan, "Shader requires transform feedbacks but these are not "
                                         "supported on this device");
            }
        }
        if (ir.UsesLayer() || ir.UsesViewportIndex()) {
            if (ir.UsesViewportIndex()) {
                AddCapability(spv::Capability::MultiViewport);
            }
            if (stage != ShaderType::Geometry && device.IsExtShaderViewportIndexLayerSupported()) {
                AddExtension("SPV_EXT_shader_viewport_index_layer");
                AddCapability(spv::Capability::ShaderViewportIndexLayerEXT);
            }
        }
        if (device.IsFormatlessImageLoadSupported()) {
            AddCapability(spv::Capability::StorageImageReadWithoutFormat);
        }
        if (device.IsFloat16Supported()) {
            AddCapability(spv::Capability::Float16);
        }
        t_scalar_half = Name(TypeFloat(device_.IsFloat16Supported() ? 16 : 32), "scalar_half");
        t_half = Name(TypeVector(t_scalar_half, 2), "half");

        const Id main = Decompile();

        switch (stage) {
        case ShaderType::Vertex:
            AddEntryPoint(spv::ExecutionModel::Vertex, main, "main", interfaces);
            break;
        case ShaderType::TesselationControl:
            AddCapability(spv::Capability::Tessellation);
            AddEntryPoint(spv::ExecutionModel::TessellationControl, main, "main", interfaces);
            AddExecutionMode(main, spv::ExecutionMode::OutputVertices,
                             header.common2.threads_per_input_primitive);
            break;
        case ShaderType::TesselationEval: {
            const auto& info = registry.GetGraphicsInfo();
            AddCapability(spv::Capability::Tessellation);
            AddEntryPoint(spv::ExecutionModel::TessellationEvaluation, main, "main", interfaces);
            AddExecutionMode(main, GetExecutionMode(info.tessellation_primitive));
            AddExecutionMode(main, GetExecutionMode(info.tessellation_spacing));
            AddExecutionMode(main, info.tessellation_clockwise
                                       ? spv::ExecutionMode::VertexOrderCw
                                       : spv::ExecutionMode::VertexOrderCcw);
            break;
        }
        case ShaderType::Geometry: {
            const auto& info = registry.GetGraphicsInfo();
            AddCapability(spv::Capability::Geometry);
            AddEntryPoint(spv::ExecutionModel::Geometry, main, "main", interfaces);
            AddExecutionMode(main, GetExecutionMode(info.primitive_topology));
            AddExecutionMode(main, GetExecutionMode(header.common3.output_topology));
            AddExecutionMode(main, spv::ExecutionMode::OutputVertices,
                             header.common4.max_output_vertices);
            // TODO(Rodrigo): Where can we get this info from?
            AddExecutionMode(main, spv::ExecutionMode::Invocations, 1U);
            break;
        }
        case ShaderType::Fragment:
            AddEntryPoint(spv::ExecutionModel::Fragment, main, "main", interfaces);
            AddExecutionMode(main, spv::ExecutionMode::OriginUpperLeft);
            if (header.ps.omap.depth) {
                AddExecutionMode(main, spv::ExecutionMode::DepthReplacing);
            }
            if (specialization.early_fragment_tests) {
                AddExecutionMode(main, spv::ExecutionMode::EarlyFragmentTests);
            }
            break;
        case ShaderType::Compute:
            const auto workgroup_size = specialization.workgroup_size;
            AddExecutionMode(main, spv::ExecutionMode::LocalSize, workgroup_size[0],
                             workgroup_size[1], workgroup_size[2]);
            AddEntryPoint(spv::ExecutionModel::GLCompute, main, "main", interfaces);
            break;
        }
    }

private:
    Id Decompile() {
        DeclareCommon();
        DeclareVertex();
        DeclareTessControl();
        DeclareTessEval();
        DeclareGeometry();
        DeclareFragment();
        DeclareCompute();
        DeclareRegisters();
        DeclareCustomVariables();
        DeclarePredicates();
        DeclareLocalMemory();
        DeclareSharedMemory();
        DeclareInternalFlags();
        DeclareInputAttributes();
        DeclareOutputAttributes();

        u32 binding = specialization.base_binding;
        binding = DeclareConstantBuffers(binding);
        binding = DeclareGlobalBuffers(binding);
        binding = DeclareUniformTexels(binding);
        binding = DeclareSamplers(binding);
        binding = DeclareStorageTexels(binding);
        binding = DeclareImages(binding);

        const Id main = OpFunction(t_void, {}, TypeFunction(t_void));
        AddLabel();

        if (ir.IsDecompiled()) {
            DeclareFlowVariables();
            DecompileAST();
        } else {
            AllocateLabels();
            DecompileBranchMode();
        }

        OpReturn();
        OpFunctionEnd();

        return main;
    }

    void DefinePrologue() {
        if (stage == ShaderType::Vertex) {
            // Clear Position to avoid reading trash on the Z conversion.
            const auto position_index = out_indices.position.value();
            const Id position = AccessElement(t_out_float4, out_vertex, position_index);
            OpStore(position, v_varying_default);

            if (specialization.point_size) {
                const u32 point_size_index = out_indices.point_size.value();
                const Id out_point_size = AccessElement(t_out_float, out_vertex, point_size_index);
                OpStore(out_point_size, Constant(t_float, *specialization.point_size));
            }
        }
    }

    void DecompileAST();

    void DecompileBranchMode() {
        const u32 first_address = ir.GetBasicBlocks().begin()->first;
        const Id loop_label = OpLabel("loop");
        const Id merge_label = OpLabel("merge");
        const Id dummy_label = OpLabel();
        const Id jump_label = OpLabel();
        continue_label = OpLabel("continue");

        std::vector<Sirit::Literal> literals;
        std::vector<Id> branch_labels;
        for (const auto& [literal, label] : labels) {
            literals.push_back(literal);
            branch_labels.push_back(label);
        }

        jmp_to = OpVariable(TypePointer(spv::StorageClass::Function, t_uint),
                            spv::StorageClass::Function, Constant(t_uint, first_address));
        AddLocalVariable(jmp_to);

        std::tie(ssy_flow_stack, ssy_flow_stack_top) = CreateFlowStack();
        std::tie(pbk_flow_stack, pbk_flow_stack_top) = CreateFlowStack();

        Name(jmp_to, "jmp_to");
        Name(ssy_flow_stack, "ssy_flow_stack");
        Name(ssy_flow_stack_top, "ssy_flow_stack_top");
        Name(pbk_flow_stack, "pbk_flow_stack");
        Name(pbk_flow_stack_top, "pbk_flow_stack_top");

        DefinePrologue();

        OpBranch(loop_label);
        AddLabel(loop_label);
        OpLoopMerge(merge_label, continue_label, spv::LoopControlMask::MaskNone);
        OpBranch(dummy_label);

        AddLabel(dummy_label);
        const Id default_branch = OpLabel();
        const Id jmp_to_load = OpLoad(t_uint, jmp_to);
        OpSelectionMerge(jump_label, spv::SelectionControlMask::MaskNone);
        OpSwitch(jmp_to_load, default_branch, literals, branch_labels);

        AddLabel(default_branch);
        OpReturn();

        for (const auto& [address, bb] : ir.GetBasicBlocks()) {
            AddLabel(labels.at(address));

            VisitBasicBlock(bb);

            const auto next_it = labels.lower_bound(address + 1);
            const Id next_label = next_it != labels.end() ? next_it->second : default_branch;
            OpBranch(next_label);
        }

        AddLabel(jump_label);
        OpBranch(continue_label);
        AddLabel(continue_label);
        OpBranch(loop_label);
        AddLabel(merge_label);
    }

private:
    friend class ASTDecompiler;
    friend class ExprDecompiler;

    static constexpr auto INTERNAL_FLAGS_COUNT = static_cast<std::size_t>(InternalFlag::Amount);

    void AllocateLabels() {
        for (const auto& pair : ir.GetBasicBlocks()) {
            const u32 address = pair.first;
            labels.emplace(address, OpLabel(fmt::format("label_0x{:x}", address)));
        }
    }

    void DeclareCommon() {
        thread_id =
            DeclareInputBuiltIn(spv::BuiltIn::SubgroupLocalInvocationId, t_in_uint, "thread_id");
        thread_masks[0] =
            DeclareInputBuiltIn(spv::BuiltIn::SubgroupEqMask, t_in_uint4, "thread_eq_mask");
        thread_masks[1] =
            DeclareInputBuiltIn(spv::BuiltIn::SubgroupGeMask, t_in_uint4, "thread_ge_mask");
        thread_masks[2] =
            DeclareInputBuiltIn(spv::BuiltIn::SubgroupGtMask, t_in_uint4, "thread_gt_mask");
        thread_masks[3] =
            DeclareInputBuiltIn(spv::BuiltIn::SubgroupLeMask, t_in_uint4, "thread_le_mask");
        thread_masks[4] =
            DeclareInputBuiltIn(spv::BuiltIn::SubgroupLtMask, t_in_uint4, "thread_lt_mask");
    }

    void DeclareVertex() {
        if (stage != ShaderType::Vertex) {
            return;
        }
        Id out_vertex_struct;
        std::tie(out_vertex_struct, out_indices) = DeclareVertexStruct();
        const Id vertex_ptr = TypePointer(spv::StorageClass::Output, out_vertex_struct);
        out_vertex = OpVariable(vertex_ptr, spv::StorageClass::Output);
        interfaces.push_back(AddGlobalVariable(Name(out_vertex, "out_vertex")));

        // Declare input attributes
        vertex_index = DeclareInputBuiltIn(spv::BuiltIn::VertexIndex, t_in_int, "vertex_index");
        instance_index =
            DeclareInputBuiltIn(spv::BuiltIn::InstanceIndex, t_in_int, "instance_index");
        base_vertex = DeclareInputBuiltIn(spv::BuiltIn::BaseVertex, t_in_int, "base_vertex");
        base_instance = DeclareInputBuiltIn(spv::BuiltIn::BaseInstance, t_in_int, "base_instance");
    }

    void DeclareTessControl() {
        if (stage != ShaderType::TesselationControl) {
            return;
        }
        DeclareInputVertexArray(NumInputPatches);
        DeclareOutputVertexArray(header.common2.threads_per_input_primitive);

        tess_level_outer = DeclareBuiltIn(
            spv::BuiltIn::TessLevelOuter, spv::StorageClass::Output,
            TypePointer(spv::StorageClass::Output, TypeArray(t_float, Constant(t_uint, 4U))),
            "tess_level_outer");
        Decorate(tess_level_outer, spv::Decoration::Patch);

        tess_level_inner = DeclareBuiltIn(
            spv::BuiltIn::TessLevelInner, spv::StorageClass::Output,
            TypePointer(spv::StorageClass::Output, TypeArray(t_float, Constant(t_uint, 2U))),
            "tess_level_inner");
        Decorate(tess_level_inner, spv::Decoration::Patch);

        invocation_id = DeclareInputBuiltIn(spv::BuiltIn::InvocationId, t_in_int, "invocation_id");
    }

    void DeclareTessEval() {
        if (stage != ShaderType::TesselationEval) {
            return;
        }
        DeclareInputVertexArray(NumInputPatches);
        DeclareOutputVertex();

        tess_coord = DeclareInputBuiltIn(spv::BuiltIn::TessCoord, t_in_float3, "tess_coord");
    }

    void DeclareGeometry() {
        if (stage != ShaderType::Geometry) {
            return;
        }
        const auto& info = registry.GetGraphicsInfo();
        const u32 num_input = GetNumPrimitiveTopologyVertices(info.primitive_topology);
        DeclareInputVertexArray(num_input);
        DeclareOutputVertex();
    }

    void DeclareFragment() {
        if (stage != ShaderType::Fragment) {
            return;
        }

        for (u32 rt = 0; rt < static_cast<u32>(std::size(frag_colors)); ++rt) {
            if (!IsRenderTargetEnabled(rt)) {
                continue;
            }
            const Id id = AddGlobalVariable(OpVariable(t_out_float4, spv::StorageClass::Output));
            Name(id, fmt::format("frag_color{}", rt));
            Decorate(id, spv::Decoration::Location, rt);

            frag_colors[rt] = id;
            interfaces.push_back(id);
        }

        if (header.ps.omap.depth) {
            frag_depth = AddGlobalVariable(OpVariable(t_out_float, spv::StorageClass::Output));
            Name(frag_depth, "frag_depth");
            Decorate(frag_depth, spv::Decoration::BuiltIn,
                     static_cast<u32>(spv::BuiltIn::FragDepth));

            interfaces.push_back(frag_depth);
        }

        frag_coord = DeclareInputBuiltIn(spv::BuiltIn::FragCoord, t_in_float4, "frag_coord");
        front_facing = DeclareInputBuiltIn(spv::BuiltIn::FrontFacing, t_in_bool, "front_facing");
        point_coord = DeclareInputBuiltIn(spv::BuiltIn::PointCoord, t_in_float2, "point_coord");
    }

    void DeclareCompute() {
        if (stage != ShaderType::Compute) {
            return;
        }

        workgroup_id = DeclareInputBuiltIn(spv::BuiltIn::WorkgroupId, t_in_uint3, "workgroup_id");
        local_invocation_id =
            DeclareInputBuiltIn(spv::BuiltIn::LocalInvocationId, t_in_uint3, "local_invocation_id");
    }

    void DeclareRegisters() {
        for (const u32 gpr : ir.GetRegisters()) {
            const Id id = OpVariable(t_prv_float, spv::StorageClass::Private, v_float_zero);
            Name(id, fmt::format("gpr_{}", gpr));
            registers.emplace(gpr, AddGlobalVariable(id));
        }
    }

    void DeclareCustomVariables() {
        const u32 num_custom_variables = ir.GetNumCustomVariables();
        for (u32 i = 0; i < num_custom_variables; ++i) {
            const Id id = OpVariable(t_prv_float, spv::StorageClass::Private, v_float_zero);
            Name(id, fmt::format("custom_var_{}", i));
            custom_variables.emplace(i, AddGlobalVariable(id));
        }
    }

    void DeclarePredicates() {
        for (const auto pred : ir.GetPredicates()) {
            const Id id = OpVariable(t_prv_bool, spv::StorageClass::Private, v_false);
            Name(id, fmt::format("pred_{}", static_cast<u32>(pred)));
            predicates.emplace(pred, AddGlobalVariable(id));
        }
    }

    void DeclareFlowVariables() {
        for (u32 i = 0; i < ir.GetASTNumVariables(); i++) {
            const Id id = OpVariable(t_prv_bool, spv::StorageClass::Private, v_false);
            Name(id, fmt::format("flow_var_{}", static_cast<u32>(i)));
            flow_variables.emplace(i, AddGlobalVariable(id));
        }
    }

    void DeclareLocalMemory() {
        // TODO(Rodrigo): Unstub kernel local memory size and pass it from a register at
        // specialization time.
        const u64 lmem_size = stage == ShaderType::Compute ? 0x400 : header.GetLocalMemorySize();
        if (lmem_size == 0) {
            return;
        }
        const auto element_count = static_cast<u32>(Common::AlignUp(lmem_size, 4) / 4);
        const Id type_array = TypeArray(t_float, Constant(t_uint, element_count));
        const Id type_pointer = TypePointer(spv::StorageClass::Private, type_array);
        Name(type_pointer, "LocalMemory");

        local_memory =
            OpVariable(type_pointer, spv::StorageClass::Private, ConstantNull(type_array));
        AddGlobalVariable(Name(local_memory, "local_memory"));
    }

    void DeclareSharedMemory() {
        if (stage != ShaderType::Compute) {
            return;
        }
        t_smem_uint = TypePointer(spv::StorageClass::Workgroup, t_uint);

        u32 smem_size = specialization.shared_memory_size * 4;
        if (smem_size == 0) {
            // Avoid declaring an empty array.
            return;
        }
        const u32 limit = device.GetMaxComputeSharedMemorySize();
        if (smem_size > limit) {
            LOG_ERROR(Render_Vulkan, "Shared memory size {} is clamped to host's limit {}",
                      smem_size, limit);
            smem_size = limit;
        }

        const Id type_array = TypeArray(t_uint, Constant(t_uint, smem_size / 4));
        const Id type_pointer = TypePointer(spv::StorageClass::Workgroup, type_array);
        Name(type_pointer, "SharedMemory");

        shared_memory = OpVariable(type_pointer, spv::StorageClass::Workgroup);
        AddGlobalVariable(Name(shared_memory, "shared_memory"));
    }

    void DeclareInternalFlags() {
        static constexpr std::array names{"zero", "sign", "carry", "overflow"};

        for (std::size_t flag = 0; flag < INTERNAL_FLAGS_COUNT; ++flag) {
            const Id id = OpVariable(t_prv_bool, spv::StorageClass::Private, v_false);
            internal_flags[flag] = AddGlobalVariable(Name(id, names[flag]));
        }
    }

    void DeclareInputVertexArray(u32 length) {
        constexpr auto storage = spv::StorageClass::Input;
        std::tie(in_indices, in_vertex) = DeclareVertexArray(storage, "in_indices", length);
    }

    void DeclareOutputVertexArray(u32 length) {
        constexpr auto storage = spv::StorageClass::Output;
        std::tie(out_indices, out_vertex) = DeclareVertexArray(storage, "out_indices", length);
    }

    std::tuple<VertexIndices, Id> DeclareVertexArray(spv::StorageClass storage_class,
                                                     std::string name, u32 length) {
        const auto [struct_id, indices] = DeclareVertexStruct();
        const Id vertex_array = TypeArray(struct_id, Constant(t_uint, length));
        const Id vertex_ptr = TypePointer(storage_class, vertex_array);
        const Id vertex = OpVariable(vertex_ptr, storage_class);
        AddGlobalVariable(Name(vertex, std::move(name)));
        interfaces.push_back(vertex);
        return {indices, vertex};
    }

    void DeclareOutputVertex() {
        Id out_vertex_struct;
        std::tie(out_vertex_struct, out_indices) = DeclareVertexStruct();
        const Id out_vertex_ptr = TypePointer(spv::StorageClass::Output, out_vertex_struct);
        out_vertex = OpVariable(out_vertex_ptr, spv::StorageClass::Output);
        interfaces.push_back(AddGlobalVariable(Name(out_vertex, "out_vertex")));
    }

    void DeclareInputAttributes() {
        for (const auto index : ir.GetInputAttributes()) {
            if (!IsGenericAttribute(index)) {
                continue;
            }
            const u32 location = GetGenericAttributeLocation(index);
            if (!IsAttributeEnabled(location)) {
                continue;
            }
            const auto type_descriptor = GetAttributeType(location);
            Id type;
            if (IsInputAttributeArray()) {
                type = GetTypeVectorDefinitionLut(type_descriptor.type).at(3);
                type = TypeArray(type, Constant(t_uint, GetNumInputVertices()));
                type = TypePointer(spv::StorageClass::Input, type);
            } else {
                type = type_descriptor.vector;
            }
            const Id id = OpVariable(type, spv::StorageClass::Input);
            AddGlobalVariable(Name(id, fmt::format("in_attr{}", location)));
            input_attributes.emplace(index, id);
            interfaces.push_back(id);

            Decorate(id, spv::Decoration::Location, location);

            if (stage != ShaderType::Fragment) {
                continue;
            }
            switch (header.ps.GetPixelImap(location)) {
            case PixelImap::Constant:
                Decorate(id, spv::Decoration::Flat);
                break;
            case PixelImap::Perspective:
                // Default
                break;
            case PixelImap::ScreenLinear:
                Decorate(id, spv::Decoration::NoPerspective);
                break;
            default:
                UNREACHABLE_MSG("Unused attribute being fetched");
            }
        }
    }

    void DeclareOutputAttributes() {
        if (stage == ShaderType::Compute || stage == ShaderType::Fragment) {
            return;
        }

        UNIMPLEMENTED_IF(registry.GetGraphicsInfo().tfb_enabled && stage != ShaderType::Vertex);
        for (const auto index : ir.GetOutputAttributes()) {
            if (!IsGenericAttribute(index)) {
                continue;
            }
            DeclareOutputAttribute(index);
        }
    }

    void DeclareOutputAttribute(Attribute::Index index) {
        static constexpr std::string_view swizzle = "xyzw";

        const u32 location = GetGenericAttributeLocation(index);
        u8 element = 0;
        while (element < 4) {
            const std::size_t remainder = 4 - element;

            std::size_t num_components = remainder;
            const std::optional tfb = GetTransformFeedbackInfo(index, element);
            if (tfb) {
                num_components = tfb->components;
            }

            Id type = GetTypeVectorDefinitionLut(Type::Float).at(num_components - 1);
            Id varying_default = v_varying_default;
            if (IsOutputAttributeArray()) {
                const u32 num = GetNumOutputVertices();
                type = TypeArray(type, Constant(t_uint, num));
                if (device.GetDriverID() != VK_DRIVER_ID_INTEL_PROPRIETARY_WINDOWS_KHR) {
                    // Intel's proprietary driver fails to setup defaults for arrayed output
                    // attributes.
                    varying_default = ConstantComposite(type, std::vector(num, varying_default));
                }
            }
            type = TypePointer(spv::StorageClass::Output, type);

            std::string name = fmt::format("out_attr{}", location);
            if (num_components < 4 || element > 0) {
                name = fmt::format("{}_{}", name, swizzle.substr(element, num_components));
            }

            const Id id = OpVariable(type, spv::StorageClass::Output, varying_default);
            Name(AddGlobalVariable(id), name);

            GenericVaryingDescription description;
            description.id = id;
            description.first_element = element;
            description.is_scalar = num_components == 1;
            for (u32 i = 0; i < num_components; ++i) {
                const u8 offset = static_cast<u8>(static_cast<u32>(index) * 4 + element + i);
                output_attributes.emplace(offset, description);
            }
            interfaces.push_back(id);

            Decorate(id, spv::Decoration::Location, location);
            if (element > 0) {
                Decorate(id, spv::Decoration::Component, static_cast<u32>(element));
            }
            if (tfb && device.IsExtTransformFeedbackSupported()) {
                Decorate(id, spv::Decoration::XfbBuffer, static_cast<u32>(tfb->buffer));
                Decorate(id, spv::Decoration::XfbStride, static_cast<u32>(tfb->stride));
                Decorate(id, spv::Decoration::Offset, static_cast<u32>(tfb->offset));
            }

            element = static_cast<u8>(static_cast<std::size_t>(element) + num_components);
        }
    }

    std::optional<VaryingTFB> GetTransformFeedbackInfo(Attribute::Index index, u8 element = 0) {
        const u8 location = static_cast<u8>(static_cast<u32>(index) * 4 + element);
        const auto it = transform_feedback.find(location);
        if (it == transform_feedback.end()) {
            return {};
        }
        return it->second;
    }

    u32 DeclareConstantBuffers(u32 binding) {
        for (const auto& [index, size] : ir.GetConstantBuffers()) {
            const Id type = device.IsKhrUniformBufferStandardLayoutSupported() ? t_cbuf_scalar_ubo
                                                                               : t_cbuf_std140_ubo;
            const Id id = OpVariable(type, spv::StorageClass::Uniform);
            AddGlobalVariable(Name(id, fmt::format("cbuf_{}", index)));

            Decorate(id, spv::Decoration::Binding, binding++);
            Decorate(id, spv::Decoration::DescriptorSet, DESCRIPTOR_SET);
            constant_buffers.emplace(index, id);
        }
        return binding;
    }

    u32 DeclareGlobalBuffers(u32 binding) {
        for (const auto& [base, usage] : ir.GetGlobalMemory()) {
            const Id id = OpVariable(t_gmem_ssbo, spv::StorageClass::StorageBuffer);
            AddGlobalVariable(
                Name(id, fmt::format("gmem_{}_{}", base.cbuf_index, base.cbuf_offset)));

            Decorate(id, spv::Decoration::Binding, binding++);
            Decorate(id, spv::Decoration::DescriptorSet, DESCRIPTOR_SET);
            global_buffers.emplace(base, id);
        }
        return binding;
    }

    u32 DeclareUniformTexels(u32 binding) {
        for (const auto& sampler : ir.GetSamplers()) {
            if (!sampler.is_buffer) {
                continue;
            }
            ASSERT(!sampler.is_array);
            ASSERT(!sampler.is_shadow);

            constexpr auto dim = spv::Dim::Buffer;
            constexpr int depth = 0;
            constexpr int arrayed = 0;
            constexpr bool ms = false;
            constexpr int sampled = 1;
            constexpr auto format = spv::ImageFormat::Unknown;
            const Id image_type = TypeImage(t_float, dim, depth, arrayed, ms, sampled, format);
            const Id pointer_type = TypePointer(spv::StorageClass::UniformConstant, image_type);
            const Id id = OpVariable(pointer_type, spv::StorageClass::UniformConstant);
            AddGlobalVariable(Name(id, fmt::format("sampler_{}", sampler.index)));
            Decorate(id, spv::Decoration::Binding, binding++);
            Decorate(id, spv::Decoration::DescriptorSet, DESCRIPTOR_SET);

            uniform_texels.emplace(sampler.index, TexelBuffer{image_type, id});
        }
        return binding;
    }

    u32 DeclareSamplers(u32 binding) {
        for (const auto& sampler : ir.GetSamplers()) {
            if (sampler.is_buffer) {
                continue;
            }
            const auto dim = GetSamplerDim(sampler);
            const int depth = sampler.is_shadow ? 1 : 0;
            const int arrayed = sampler.is_array ? 1 : 0;
            constexpr bool ms = false;
            constexpr int sampled = 1;
            constexpr auto format = spv::ImageFormat::Unknown;
            const Id image_type = TypeImage(t_float, dim, depth, arrayed, ms, sampled, format);
            const Id sampler_type = TypeSampledImage(image_type);
            const Id sampler_pointer_type =
                TypePointer(spv::StorageClass::UniformConstant, sampler_type);
            const Id type = sampler.is_indexed
                                ? TypeArray(sampler_type, Constant(t_uint, sampler.size))
                                : sampler_type;
            const Id pointer_type = TypePointer(spv::StorageClass::UniformConstant, type);
            const Id id = OpVariable(pointer_type, spv::StorageClass::UniformConstant);
            AddGlobalVariable(Name(id, fmt::format("sampler_{}", sampler.index)));
            Decorate(id, spv::Decoration::Binding, binding++);
            Decorate(id, spv::Decoration::DescriptorSet, DESCRIPTOR_SET);

            sampled_images.emplace(
                sampler.index, SampledImage{image_type, sampler_type, sampler_pointer_type, id});
        }
        return binding;
    }

    u32 DeclareStorageTexels(u32 binding) {
        for (const auto& image : ir.GetImages()) {
            if (image.type != Tegra::Shader::ImageType::TextureBuffer) {
                continue;
            }
            DeclareImage(image, binding);
        }
        return binding;
    }

    u32 DeclareImages(u32 binding) {
        for (const auto& image : ir.GetImages()) {
            if (image.type == Tegra::Shader::ImageType::TextureBuffer) {
                continue;
            }
            DeclareImage(image, binding);
        }
        return binding;
    }

    void DeclareImage(const ImageEntry& image, u32& binding) {
        const auto [dim, arrayed] = GetImageDim(image);
        constexpr int depth = 0;
        constexpr bool ms = false;
        constexpr int sampled = 2; // This won't be accessed with a sampler
        const auto format = image.is_atomic ? spv::ImageFormat::R32ui : spv::ImageFormat::Unknown;
        const Id image_type = TypeImage(t_uint, dim, depth, arrayed, ms, sampled, format, {});
        const Id pointer_type = TypePointer(spv::StorageClass::UniformConstant, image_type);
        const Id id = OpVariable(pointer_type, spv::StorageClass::UniformConstant);
        AddGlobalVariable(Name(id, fmt::format("image_{}", image.index)));

        Decorate(id, spv::Decoration::Binding, binding++);
        Decorate(id, spv::Decoration::DescriptorSet, DESCRIPTOR_SET);
        if (image.is_read && !image.is_written) {
            Decorate(id, spv::Decoration::NonWritable);
        } else if (image.is_written && !image.is_read) {
            Decorate(id, spv::Decoration::NonReadable);
        }

        images.emplace(image.index, StorageImage{image_type, id});
    }

    bool IsRenderTargetEnabled(u32 rt) const {
        for (u32 component = 0; component < 4; ++component) {
            if (header.ps.IsColorComponentOutputEnabled(rt, component)) {
                return true;
            }
        }
        return false;
    }

    bool IsInputAttributeArray() const {
        return stage == ShaderType::TesselationControl || stage == ShaderType::TesselationEval ||
               stage == ShaderType::Geometry;
    }

    bool IsOutputAttributeArray() const {
        return stage == ShaderType::TesselationControl;
    }

    bool IsAttributeEnabled(u32 location) const {
        return stage != ShaderType::Vertex || specialization.enabled_attributes[location];
    }

    u32 GetNumInputVertices() const {
        switch (stage) {
        case ShaderType::Geometry:
            return GetNumPrimitiveTopologyVertices(registry.GetGraphicsInfo().primitive_topology);
        case ShaderType::TesselationControl:
        case ShaderType::TesselationEval:
            return NumInputPatches;
        default:
            UNREACHABLE();
            return 1;
        }
    }

    u32 GetNumOutputVertices() const {
        switch (stage) {
        case ShaderType::TesselationControl:
            return header.common2.threads_per_input_primitive;
        default:
            UNREACHABLE();
            return 1;
        }
    }

    std::tuple<Id, VertexIndices> DeclareVertexStruct() {
        struct BuiltIn {
            Id type;
            spv::BuiltIn builtin;
            const char* name;
        };
        std::vector<BuiltIn> members;
        members.reserve(4);

        const auto AddBuiltIn = [&](Id type, spv::BuiltIn builtin, const char* name) {
            const auto index = static_cast<u32>(members.size());
            members.push_back(BuiltIn{type, builtin, name});
            return index;
        };

        VertexIndices indices;
        indices.position = AddBuiltIn(t_float4, spv::BuiltIn::Position, "position");

        if (ir.UsesLayer()) {
            if (stage != ShaderType::Vertex || device.IsExtShaderViewportIndexLayerSupported()) {
                indices.layer = AddBuiltIn(t_int, spv::BuiltIn::Layer, "layer");
            } else {
                LOG_ERROR(
                    Render_Vulkan,
                    "Shader requires Layer but it's not supported on this stage with this device.");
            }
        }

        if (ir.UsesViewportIndex()) {
            if (stage != ShaderType::Vertex || device.IsExtShaderViewportIndexLayerSupported()) {
                indices.viewport = AddBuiltIn(t_int, spv::BuiltIn::ViewportIndex, "viewport_index");
            } else {
                LOG_ERROR(Render_Vulkan, "Shader requires ViewportIndex but it's not supported on "
                                         "this stage with this device.");
            }
        }

        if (ir.UsesPointSize() || specialization.point_size) {
            indices.point_size = AddBuiltIn(t_float, spv::BuiltIn::PointSize, "point_size");
        }

        const auto& ir_output_attributes = ir.GetOutputAttributes();
        const bool declare_clip_distances = std::any_of(
            ir_output_attributes.begin(), ir_output_attributes.end(), [](const auto& index) {
                return index == Attribute::Index::ClipDistances0123 ||
                       index == Attribute::Index::ClipDistances4567;
            });
        if (declare_clip_distances) {
            indices.clip_distances = AddBuiltIn(TypeArray(t_float, Constant(t_uint, 8)),
                                                spv::BuiltIn::ClipDistance, "clip_distances");
        }

        std::vector<Id> member_types;
        member_types.reserve(members.size());
        for (std::size_t i = 0; i < members.size(); ++i) {
            member_types.push_back(members[i].type);
        }
        const Id per_vertex_struct = Name(TypeStruct(member_types), "PerVertex");
        Decorate(per_vertex_struct, spv::Decoration::Block);

        for (std::size_t index = 0; index < members.size(); ++index) {
            const auto& member = members[index];
            MemberName(per_vertex_struct, static_cast<u32>(index), member.name);
            MemberDecorate(per_vertex_struct, static_cast<u32>(index), spv::Decoration::BuiltIn,
                           static_cast<u32>(member.builtin));
        }

        return {per_vertex_struct, indices};
    }

    void VisitBasicBlock(const NodeBlock& bb) {
        for (const auto& node : bb) {
            Visit(node);
        }
    }

    Expression Visit(const Node& node) {
        if (const auto operation = std::get_if<OperationNode>(&*node)) {
            if (const auto amend_index = operation->GetAmendIndex()) {
                [[maybe_unused]] const Type type = Visit(ir.GetAmendNode(*amend_index)).type;
                ASSERT(type == Type::Void);
            }
            const auto operation_index = static_cast<std::size_t>(operation->GetCode());
            const auto decompiler = operation_decompilers[operation_index];
            if (decompiler == nullptr) {
                UNREACHABLE_MSG("Operation decompiler {} not defined", operation_index);
            }
            return (this->*decompiler)(*operation);
        }

        if (const auto gpr = std::get_if<GprNode>(&*node)) {
            const u32 index = gpr->GetIndex();
            if (index == Register::ZeroIndex) {
                return {v_float_zero, Type::Float};
            }
            return {OpLoad(t_float, registers.at(index)), Type::Float};
        }

        if (const auto cv = std::get_if<CustomVarNode>(&*node)) {
            const u32 index = cv->GetIndex();
            return {OpLoad(t_float, custom_variables.at(index)), Type::Float};
        }

        if (const auto immediate = std::get_if<ImmediateNode>(&*node)) {
            return {Constant(t_uint, immediate->GetValue()), Type::Uint};
        }

        if (const auto predicate = std::get_if<PredicateNode>(&*node)) {
            const auto value = [&]() -> Id {
                switch (const auto index = predicate->GetIndex(); index) {
                case Tegra::Shader::Pred::UnusedIndex:
                    return v_true;
                case Tegra::Shader::Pred::NeverExecute:
                    return v_false;
                default:
                    return OpLoad(t_bool, predicates.at(index));
                }
            }();
            if (predicate->IsNegated()) {
                return {OpLogicalNot(t_bool, value), Type::Bool};
            }
            return {value, Type::Bool};
        }

        if (const auto abuf = std::get_if<AbufNode>(&*node)) {
            const auto attribute = abuf->GetIndex();
            const u32 element = abuf->GetElement();
            const auto& buffer = abuf->GetBuffer();

            const auto ArrayPass = [&](Id pointer_type, Id composite, std::vector<u32> indices) {
                std::vector<Id> members;
                members.reserve(std::size(indices) + 1);

                if (buffer && IsInputAttributeArray()) {
                    members.push_back(AsUint(Visit(buffer)));
                }
                for (const u32 index : indices) {
                    members.push_back(Constant(t_uint, index));
                }
                return OpAccessChain(pointer_type, composite, members);
            };

            switch (attribute) {
            case Attribute::Index::Position: {
                if (stage == ShaderType::Fragment) {
                    return {OpLoad(t_float, AccessElement(t_in_float, frag_coord, element)),
                            Type::Float};
                }
                const std::vector elements = {in_indices.position.value(), element};
                return {OpLoad(t_float, ArrayPass(t_in_float, in_vertex, elements)), Type::Float};
            }
            case Attribute::Index::PointCoord: {
                switch (element) {
                case 0:
                case 1:
                    return {OpCompositeExtract(t_float, OpLoad(t_float2, point_coord), element),
                            Type::Float};
                }
                UNIMPLEMENTED_MSG("Unimplemented point coord element={}", element);
                return {v_float_zero, Type::Float};
            }
            case Attribute::Index::TessCoordInstanceIDVertexID:
                // TODO(Subv): Find out what the values are for the first two elements when inside a
                // vertex shader, and what's the value of the fourth element when inside a Tess Eval
                // shader.
                switch (element) {
                case 0:
                case 1:
                    return {OpLoad(t_float, AccessElement(t_in_float, tess_coord, element)),
                            Type::Float};
                case 2:
                    return {
                        OpISub(t_int, OpLoad(t_int, instance_index), OpLoad(t_int, base_instance)),
                        Type::Int};
                case 3:
                    return {OpISub(t_int, OpLoad(t_int, vertex_index), OpLoad(t_int, base_vertex)),
                            Type::Int};
                }
                UNIMPLEMENTED_MSG("Unmanaged TessCoordInstanceIDVertexID element={}", element);
                return {Constant(t_uint, 0U), Type::Uint};
            case Attribute::Index::FrontFacing:
                // TODO(Subv): Find out what the values are for the other elements.
                ASSERT(stage == ShaderType::Fragment);
                if (element == 3) {
                    const Id is_front_facing = OpLoad(t_bool, front_facing);
                    const Id true_value = Constant(t_int, static_cast<s32>(-1));
                    const Id false_value = Constant(t_int, 0);
                    return {OpSelect(t_int, is_front_facing, true_value, false_value), Type::Int};
                }
                UNIMPLEMENTED_MSG("Unmanaged FrontFacing element={}", element);
                return {v_float_zero, Type::Float};
            default:
                if (!IsGenericAttribute(attribute)) {
                    break;
                }
                const u32 location = GetGenericAttributeLocation(attribute);
                if (!IsAttributeEnabled(location)) {
                    // Disabled attributes (also known as constant attributes) always return zero.
                    return {v_float_zero, Type::Float};
                }
                const auto type_descriptor = GetAttributeType(location);
                const Type type = type_descriptor.type;
                const Id attribute_id = input_attributes.at(attribute);
                const std::vector elements = {element};
                const Id pointer = ArrayPass(type_descriptor.scalar, attribute_id, elements);
                return {OpLoad(GetTypeDefinition(type), pointer), type};
            }
            UNIMPLEMENTED_MSG("Unhandled input attribute: {}", attribute);
            return {v_float_zero, Type::Float};
        }

        if (const auto cbuf = std::get_if<CbufNode>(&*node)) {
            const Node& offset = cbuf->GetOffset();
            const Id buffer_id = constant_buffers.at(cbuf->GetIndex());

            Id pointer{};
            if (device.IsKhrUniformBufferStandardLayoutSupported()) {
                const Id buffer_offset =
                    OpShiftRightLogical(t_uint, AsUint(Visit(offset)), Constant(t_uint, 2U));
                pointer =
                    OpAccessChain(t_cbuf_float, buffer_id, Constant(t_uint, 0U), buffer_offset);
            } else {
                Id buffer_index{};
                Id buffer_element{};
                if (const auto immediate = std::get_if<ImmediateNode>(&*offset)) {
                    // Direct access
                    const u32 offset_imm = immediate->GetValue();
                    ASSERT(offset_imm % 4 == 0);
                    buffer_index = Constant(t_uint, offset_imm / 16);
                    buffer_element = Constant(t_uint, (offset_imm / 4) % 4);
                } else if (std::holds_alternative<OperationNode>(*offset)) {
                    // Indirect access
                    const Id offset_id = AsUint(Visit(offset));
                    const Id unsafe_offset = OpUDiv(t_uint, offset_id, Constant(t_uint, 4));
                    const Id final_offset =
                        OpUMod(t_uint, unsafe_offset, Constant(t_uint, MaxConstBufferElements - 1));
                    buffer_index = OpUDiv(t_uint, final_offset, Constant(t_uint, 4));
                    buffer_element = OpUMod(t_uint, final_offset, Constant(t_uint, 4));
                } else {
                    UNREACHABLE_MSG("Unmanaged offset node type");
                }
                pointer = OpAccessChain(t_cbuf_float, buffer_id, v_uint_zero, buffer_index,
                                        buffer_element);
            }
            return {OpLoad(t_float, pointer), Type::Float};
        }

        if (const auto gmem = std::get_if<GmemNode>(&*node)) {
            return {OpLoad(t_uint, GetGlobalMemoryPointer(*gmem)), Type::Uint};
        }

        if (const auto lmem = std::get_if<LmemNode>(&*node)) {
            Id address = AsUint(Visit(lmem->GetAddress()));
            address = OpShiftRightLogical(t_uint, address, Constant(t_uint, 2U));
            const Id pointer = OpAccessChain(t_prv_float, local_memory, address);
            return {OpLoad(t_float, pointer), Type::Float};
        }

        if (const auto smem = std::get_if<SmemNode>(&*node)) {
            return {OpLoad(t_uint, GetSharedMemoryPointer(*smem)), Type::Uint};
        }

        if (const auto internal_flag = std::get_if<InternalFlagNode>(&*node)) {
            const Id flag = internal_flags.at(static_cast<std::size_t>(internal_flag->GetFlag()));
            return {OpLoad(t_bool, flag), Type::Bool};
        }

        if (const auto conditional = std::get_if<ConditionalNode>(&*node)) {
            if (const auto amend_index = conditional->GetAmendIndex()) {
                [[maybe_unused]] const Type type = Visit(ir.GetAmendNode(*amend_index)).type;
                ASSERT(type == Type::Void);
            }
            // It's invalid to call conditional on nested nodes, use an operation instead
            const Id true_label = OpLabel();
            const Id skip_label = OpLabel();
            const Id condition = AsBool(Visit(conditional->GetCondition()));
            OpSelectionMerge(skip_label, spv::SelectionControlMask::MaskNone);
            OpBranchConditional(condition, true_label, skip_label);
            AddLabel(true_label);

            conditional_branch_set = true;
            inside_branch = false;
            VisitBasicBlock(conditional->GetCode());
            conditional_branch_set = false;
            if (!inside_branch) {
                OpBranch(skip_label);
            } else {
                inside_branch = false;
            }
            AddLabel(skip_label);
            return {};
        }

        if (const auto comment = std::get_if<CommentNode>(&*node)) {
            if (device.HasDebuggingToolAttached()) {
                // We should insert comments with OpString instead of using named variables
                Name(OpUndef(t_int), comment->GetText());
            }
            return {};
        }

        UNREACHABLE();
        return {};
    }

    template <Id (Module::*func)(Id, Id), Type result_type, Type type_a = result_type>
    Expression Unary(Operation operation) {
        const Id type_def = GetTypeDefinition(result_type);
        const Id op_a = As(Visit(operation[0]), type_a);

        const Id value = (this->*func)(type_def, op_a);
        if (IsPrecise(operation)) {
            Decorate(value, spv::Decoration::NoContraction);
        }
        return {value, result_type};
    }

    template <Id (Module::*func)(Id, Id, Id), Type result_type, Type type_a = result_type,
              Type type_b = type_a>
    Expression Binary(Operation operation) {
        const Id type_def = GetTypeDefinition(result_type);
        const Id op_a = As(Visit(operation[0]), type_a);
        const Id op_b = As(Visit(operation[1]), type_b);

        const Id value = (this->*func)(type_def, op_a, op_b);
        if (IsPrecise(operation)) {
            Decorate(value, spv::Decoration::NoContraction);
        }
        return {value, result_type};
    }

    template <Id (Module::*func)(Id, Id, Id, Id), Type result_type, Type type_a = result_type,
              Type type_b = type_a, Type type_c = type_b>
    Expression Ternary(Operation operation) {
        const Id type_def = GetTypeDefinition(result_type);
        const Id op_a = As(Visit(operation[0]), type_a);
        const Id op_b = As(Visit(operation[1]), type_b);
        const Id op_c = As(Visit(operation[2]), type_c);

        const Id value = (this->*func)(type_def, op_a, op_b, op_c);
        if (IsPrecise(operation)) {
            Decorate(value, spv::Decoration::NoContraction);
        }
        return {value, result_type};
    }

    template <Id (Module::*func)(Id, Id, Id, Id, Id), Type result_type, Type type_a = result_type,
              Type type_b = type_a, Type type_c = type_b, Type type_d = type_c>
    Expression Quaternary(Operation operation) {
        const Id type_def = GetTypeDefinition(result_type);
        const Id op_a = As(Visit(operation[0]), type_a);
        const Id op_b = As(Visit(operation[1]), type_b);
        const Id op_c = As(Visit(operation[2]), type_c);
        const Id op_d = As(Visit(operation[3]), type_d);

        const Id value = (this->*func)(type_def, op_a, op_b, op_c, op_d);
        if (IsPrecise(operation)) {
            Decorate(value, spv::Decoration::NoContraction);
        }
        return {value, result_type};
    }

    Expression Assign(Operation operation) {
        const Node& dest = operation[0];
        const Node& src = operation[1];

        Expression target{};
        if (const auto gpr = std::get_if<GprNode>(&*dest)) {
            if (gpr->GetIndex() == Register::ZeroIndex) {
                // Writing to Register::ZeroIndex is a no op but we still have to visit its source
                // because it might have side effects.
                Visit(src);
                return {};
            }
            target = {registers.at(gpr->GetIndex()), Type::Float};

        } else if (const auto abuf = std::get_if<AbufNode>(&*dest)) {
            const auto& buffer = abuf->GetBuffer();
            const auto ArrayPass = [&](Id pointer_type, Id composite, std::vector<u32> indices) {
                std::vector<Id> members;
                members.reserve(std::size(indices) + 1);

                if (buffer && IsOutputAttributeArray()) {
                    members.push_back(AsUint(Visit(buffer)));
                }
                for (const u32 index : indices) {
                    members.push_back(Constant(t_uint, index));
                }
                return OpAccessChain(pointer_type, composite, members);
            };

            target = [&]() -> Expression {
                const u32 element = abuf->GetElement();
                switch (const auto attribute = abuf->GetIndex(); attribute) {
                case Attribute::Index::Position: {
                    const u32 index = out_indices.position.value();
                    return {ArrayPass(t_out_float, out_vertex, {index, element}), Type::Float};
                }
                case Attribute::Index::LayerViewportPointSize:
                    switch (element) {
                    case 1: {
                        if (!out_indices.layer) {
                            return {};
                        }
                        const u32 index = out_indices.layer.value();
                        return {AccessElement(t_out_int, out_vertex, index), Type::Int};
                    }
                    case 2: {
                        if (!out_indices.viewport) {
                            return {};
                        }
                        const u32 index = out_indices.viewport.value();
                        return {AccessElement(t_out_int, out_vertex, index), Type::Int};
                    }
                    case 3: {
                        const auto index = out_indices.point_size.value();
                        return {AccessElement(t_out_float, out_vertex, index), Type::Float};
                    }
                    default:
                        UNIMPLEMENTED_MSG("LayerViewportPoint element={}", abuf->GetElement());
                        return {};
                    }
                case Attribute::Index::ClipDistances0123: {
                    const u32 index = out_indices.clip_distances.value();
                    return {AccessElement(t_out_float, out_vertex, index, element), Type::Float};
                }
                case Attribute::Index::ClipDistances4567: {
                    const u32 index = out_indices.clip_distances.value();
                    return {AccessElement(t_out_float, out_vertex, index, element + 4),
                            Type::Float};
                }
                default:
                    if (IsGenericAttribute(attribute)) {
                        const u8 offset = static_cast<u8>(static_cast<u8>(attribute) * 4 + element);
                        const GenericVaryingDescription description = output_attributes.at(offset);
                        const Id composite = description.id;
                        std::vector<u32> indices;
                        if (!description.is_scalar) {
                            indices.push_back(element - description.first_element);
                        }
                        return {ArrayPass(t_out_float, composite, indices), Type::Float};
                    }
                    UNIMPLEMENTED_MSG("Unhandled output attribute: {}",
                                      static_cast<u32>(attribute));
                    return {};
                }
            }();

        } else if (const auto patch = std::get_if<PatchNode>(&*dest)) {
            target = [&]() -> Expression {
                const u32 offset = patch->GetOffset();
                switch (offset) {
                case 0:
                case 1:
                case 2:
                case 3:
                    return {AccessElement(t_out_float, tess_level_outer, offset % 4), Type::Float};
                case 4:
                case 5:
                    return {AccessElement(t_out_float, tess_level_inner, offset % 4), Type::Float};
                }
                UNIMPLEMENTED_MSG("Unhandled patch output offset: {}", offset);
                return {};
            }();

        } else if (const auto lmem = std::get_if<LmemNode>(&*dest)) {
            Id address = AsUint(Visit(lmem->GetAddress()));
            address = OpUDiv(t_uint, address, Constant(t_uint, 4));
            target = {OpAccessChain(t_prv_float, local_memory, address), Type::Float};

        } else if (const auto smem = std::get_if<SmemNode>(&*dest)) {
            target = {GetSharedMemoryPointer(*smem), Type::Uint};

        } else if (const auto gmem = std::get_if<GmemNode>(&*dest)) {
            target = {GetGlobalMemoryPointer(*gmem), Type::Uint};

        } else if (const auto cv = std::get_if<CustomVarNode>(&*dest)) {
            target = {custom_variables.at(cv->GetIndex()), Type::Float};

        } else {
            UNIMPLEMENTED();
        }

        if (!target.id) {
            // On failure we return a nullptr target.id, skip these stores.
            return {};
        }

        OpStore(target.id, As(Visit(src), target.type));
        return {};
    }

    template <u32 offset>
    Expression FCastHalf(Operation operation) {
        const Id value = AsHalfFloat(Visit(operation[0]));
        return {GetFloatFromHalfScalar(OpCompositeExtract(t_scalar_half, value, offset)),
                Type::Float};
    }

    Expression FSwizzleAdd(Operation operation) {
        const Id minus = Constant(t_float, -1.0f);
        const Id plus = v_float_one;
        const Id zero = v_float_zero;
        const Id lut_a = ConstantComposite(t_float4, minus, plus, minus, zero);
        const Id lut_b = ConstantComposite(t_float4, minus, minus, plus, minus);

        Id mask = OpLoad(t_uint, thread_id);
        mask = OpBitwiseAnd(t_uint, mask, Constant(t_uint, 3));
        mask = OpShiftLeftLogical(t_uint, mask, Constant(t_uint, 1));
        mask = OpShiftRightLogical(t_uint, AsUint(Visit(operation[2])), mask);
        mask = OpBitwiseAnd(t_uint, mask, Constant(t_uint, 3));

        const Id modifier_a = OpVectorExtractDynamic(t_float, lut_a, mask);
        const Id modifier_b = OpVectorExtractDynamic(t_float, lut_b, mask);

        const Id op_a = OpFMul(t_float, AsFloat(Visit(operation[0])), modifier_a);
        const Id op_b = OpFMul(t_float, AsFloat(Visit(operation[1])), modifier_b);
        return {OpFAdd(t_float, op_a, op_b), Type::Float};
    }

    Expression HNegate(Operation operation) {
        const bool is_f16 = device.IsFloat16Supported();
        const Id minus_one = Constant(t_scalar_half, is_f16 ? 0xbc00 : 0xbf800000);
        const Id one = Constant(t_scalar_half, is_f16 ? 0x3c00 : 0x3f800000);
        const auto GetNegate = [&](std::size_t index) {
            return OpSelect(t_scalar_half, AsBool(Visit(operation[index])), minus_one, one);
        };
        const Id negation = OpCompositeConstruct(t_half, GetNegate(1), GetNegate(2));
        return {OpFMul(t_half, AsHalfFloat(Visit(operation[0])), negation), Type::HalfFloat};
    }

    Expression HClamp(Operation operation) {
        const auto Pack = [&](std::size_t index) {
            const Id scalar = GetHalfScalarFromFloat(AsFloat(Visit(operation[index])));
            return OpCompositeConstruct(t_half, scalar, scalar);
        };
        const Id value = AsHalfFloat(Visit(operation[0]));
        const Id min = Pack(1);
        const Id max = Pack(2);

        const Id clamped = OpFClamp(t_half, value, min, max);
        if (IsPrecise(operation)) {
            Decorate(clamped, spv::Decoration::NoContraction);
        }
        return {clamped, Type::HalfFloat};
    }

    Expression HCastFloat(Operation operation) {
        const Id value = GetHalfScalarFromFloat(AsFloat(Visit(operation[0])));
        return {OpCompositeConstruct(t_half, value, Constant(t_scalar_half, 0)), Type::HalfFloat};
    }

    Expression HUnpack(Operation operation) {
        Expression operand = Visit(operation[0]);
        const auto type = std::get<Tegra::Shader::HalfType>(operation.GetMeta());
        if (type == Tegra::Shader::HalfType::H0_H1) {
            return operand;
        }
        const auto value = [&] {
            switch (std::get<Tegra::Shader::HalfType>(operation.GetMeta())) {
            case Tegra::Shader::HalfType::F32:
                return GetHalfScalarFromFloat(AsFloat(operand));
            case Tegra::Shader::HalfType::H0_H0:
                return OpCompositeExtract(t_scalar_half, AsHalfFloat(operand), 0);
            case Tegra::Shader::HalfType::H1_H1:
                return OpCompositeExtract(t_scalar_half, AsHalfFloat(operand), 1);
            default:
                UNREACHABLE();
                return ConstantNull(t_half);
            }
        }();
        return {OpCompositeConstruct(t_half, value, value), Type::HalfFloat};
    }

    Expression HMergeF32(Operation operation) {
        const Id value = AsHalfFloat(Visit(operation[0]));
        return {GetFloatFromHalfScalar(OpCompositeExtract(t_scalar_half, value, 0)), Type::Float};
    }

    template <u32 offset>
    Expression HMergeHN(Operation operation) {
        const Id target = AsHalfFloat(Visit(operation[0]));
        const Id source = AsHalfFloat(Visit(operation[1]));
        const Id object = OpCompositeExtract(t_scalar_half, source, offset);
        return {OpCompositeInsert(t_half, object, target, offset), Type::HalfFloat};
    }

    Expression HPack2(Operation operation) {
        const Id low = GetHalfScalarFromFloat(AsFloat(Visit(operation[0])));
        const Id high = GetHalfScalarFromFloat(AsFloat(Visit(operation[1])));
        return {OpCompositeConstruct(t_half, low, high), Type::HalfFloat};
    }

    Expression LogicalAddCarry(Operation operation) {
        const Id op_a = AsUint(Visit(operation[0]));
        const Id op_b = AsUint(Visit(operation[1]));

        const Id result = OpIAddCarry(TypeStruct({t_uint, t_uint}), op_a, op_b);
        const Id carry = OpCompositeExtract(t_uint, result, 1);
        return {OpINotEqual(t_bool, carry, v_uint_zero), Type::Bool};
    }

    Expression LogicalAssign(Operation operation) {
        const Node& dest = operation[0];
        const Node& src = operation[1];

        Id target{};
        if (const auto pred = std::get_if<PredicateNode>(&*dest)) {
            ASSERT_MSG(!pred->IsNegated(), "Negating logical assignment");

            const auto index = pred->GetIndex();
            switch (index) {
            case Tegra::Shader::Pred::NeverExecute:
            case Tegra::Shader::Pred::UnusedIndex:
                // Writing to these predicates is a no-op
                return {};
            }
            target = predicates.at(index);

        } else if (const auto flag = std::get_if<InternalFlagNode>(&*dest)) {
            target = internal_flags.at(static_cast<u32>(flag->GetFlag()));
        }

        OpStore(target, AsBool(Visit(src)));
        return {};
    }

    Expression LogicalFOrdered(Operation operation) {
        // Emulate SPIR-V's OpOrdered
        const Id op_a = AsFloat(Visit(operation[0]));
        const Id op_b = AsFloat(Visit(operation[1]));
        const Id is_num_a = OpFOrdEqual(t_bool, op_a, op_a);
        const Id is_num_b = OpFOrdEqual(t_bool, op_b, op_b);
        return {OpLogicalAnd(t_bool, is_num_a, is_num_b), Type::Bool};
    }

    Expression LogicalFUnordered(Operation operation) {
        // Emulate SPIR-V's OpUnordered
        const Id op_a = AsFloat(Visit(operation[0]));
        const Id op_b = AsFloat(Visit(operation[1]));
        const Id is_nan_a = OpIsNan(t_bool, op_a);
        const Id is_nan_b = OpIsNan(t_bool, op_b);
        return {OpLogicalOr(t_bool, is_nan_a, is_nan_b), Type::Bool};
    }

    Id GetTextureSampler(Operation operation) {
        const auto& meta = std::get<MetaTexture>(operation.GetMeta());
        ASSERT(!meta.sampler.is_buffer);

        const auto& entry = sampled_images.at(meta.sampler.index);
        Id sampler = entry.variable;
        if (meta.sampler.is_indexed) {
            const Id index = AsInt(Visit(meta.index));
            sampler = OpAccessChain(entry.sampler_pointer_type, sampler, index);
        }
        return OpLoad(entry.sampler_type, sampler);
    }

    Id GetTextureImage(Operation operation) {
        const auto& meta = std::get<MetaTexture>(operation.GetMeta());
        const u32 index = meta.sampler.index;
        if (meta.sampler.is_buffer) {
            const auto& entry = uniform_texels.at(index);
            return OpLoad(entry.image_type, entry.image);
        } else {
            const auto& entry = sampled_images.at(index);
            return OpImage(entry.image_type, GetTextureSampler(operation));
        }
    }

    Id GetImage(Operation operation) {
        const auto& meta = std::get<MetaImage>(operation.GetMeta());
        const auto entry = images.at(meta.image.index);
        return OpLoad(entry.image_type, entry.image);
    }

    Id AssembleVector(const std::vector<Id>& coords, Type type) {
        const Id coords_type = GetTypeVectorDefinitionLut(type).at(coords.size() - 1);
        return coords.size() == 1 ? coords[0] : OpCompositeConstruct(coords_type, coords);
    }

    Id GetCoordinates(Operation operation, Type type) {
        std::vector<Id> coords;
        for (std::size_t i = 0; i < operation.GetOperandsCount(); ++i) {
            coords.push_back(As(Visit(operation[i]), type));
        }
        if (const auto meta = std::get_if<MetaTexture>(&operation.GetMeta())) {
            // Add array coordinate for textures
            if (meta->sampler.is_array) {
                Id array = AsInt(Visit(meta->array));
                if (type == Type::Float) {
                    array = OpConvertSToF(t_float, array);
                }
                coords.push_back(array);
            }
        }
        return AssembleVector(coords, type);
    }

    Id GetOffsetCoordinates(Operation operation) {
        const auto& meta = std::get<MetaTexture>(operation.GetMeta());
        std::vector<Id> coords;
        coords.reserve(meta.aoffi.size());
        for (const auto& coord : meta.aoffi) {
            coords.push_back(AsInt(Visit(coord)));
        }
        return AssembleVector(coords, Type::Int);
    }

    std::pair<Id, Id> GetDerivatives(Operation operation) {
        const auto& meta = std::get<MetaTexture>(operation.GetMeta());
        const auto& derivatives = meta.derivates;
        ASSERT(derivatives.size() % 2 == 0);

        const std::size_t components = derivatives.size() / 2;
        std::vector<Id> dx, dy;
        dx.reserve(components);
        dy.reserve(components);
        for (std::size_t index = 0; index < components; ++index) {
            dx.push_back(AsFloat(Visit(derivatives.at(index * 2 + 0))));
            dy.push_back(AsFloat(Visit(derivatives.at(index * 2 + 1))));
        }
        return {AssembleVector(dx, Type::Float), AssembleVector(dy, Type::Float)};
    }

    Expression GetTextureElement(Operation operation, Id sample_value, Type type) {
        const auto& meta = std::get<MetaTexture>(operation.GetMeta());
        const auto type_def = GetTypeDefinition(type);
        return {OpCompositeExtract(type_def, sample_value, meta.element), type};
    }

    Expression Texture(Operation operation) {
        const auto& meta = std::get<MetaTexture>(operation.GetMeta());

        const bool can_implicit = stage == ShaderType::Fragment;
        const Id sampler = GetTextureSampler(operation);
        const Id coords = GetCoordinates(operation, Type::Float);

        std::vector<Id> operands;
        spv::ImageOperandsMask mask{};
        if (meta.bias) {
            mask = mask | spv::ImageOperandsMask::Bias;
            operands.push_back(AsFloat(Visit(meta.bias)));
        }

        if (!can_implicit) {
            mask = mask | spv::ImageOperandsMask::Lod;
            operands.push_back(v_float_zero);
        }

        if (!meta.aoffi.empty()) {
            mask = mask | spv::ImageOperandsMask::Offset;
            operands.push_back(GetOffsetCoordinates(operation));
        }

        if (meta.depth_compare) {
            // Depth sampling
            UNIMPLEMENTED_IF(meta.bias);
            const Id dref = AsFloat(Visit(meta.depth_compare));
            if (can_implicit) {
                return {
                    OpImageSampleDrefImplicitLod(t_float, sampler, coords, dref, mask, operands),
                    Type::Float};
            } else {
                return {
                    OpImageSampleDrefExplicitLod(t_float, sampler, coords, dref, mask, operands),
                    Type::Float};
            }
        }

        Id texture;
        if (can_implicit) {
            texture = OpImageSampleImplicitLod(t_float4, sampler, coords, mask, operands);
        } else {
            texture = OpImageSampleExplicitLod(t_float4, sampler, coords, mask, operands);
        }
        return GetTextureElement(operation, texture, Type::Float);
    }

    Expression TextureLod(Operation operation) {
        const auto& meta = std::get<MetaTexture>(operation.GetMeta());

        const Id sampler = GetTextureSampler(operation);
        const Id coords = GetCoordinates(operation, Type::Float);
        const Id lod = AsFloat(Visit(meta.lod));

        spv::ImageOperandsMask mask = spv::ImageOperandsMask::Lod;
        std::vector<Id> operands{lod};

        if (!meta.aoffi.empty()) {
            mask = mask | spv::ImageOperandsMask::Offset;
            operands.push_back(GetOffsetCoordinates(operation));
        }

        if (meta.sampler.is_shadow) {
            const Id dref = AsFloat(Visit(meta.depth_compare));
            return {OpImageSampleDrefExplicitLod(t_float, sampler, coords, dref, mask, operands),
                    Type::Float};
        }
        const Id texture = OpImageSampleExplicitLod(t_float4, sampler, coords, mask, operands);
        return GetTextureElement(operation, texture, Type::Float);
    }

    Expression TextureGather(Operation operation) {
        const auto& meta = std::get<MetaTexture>(operation.GetMeta());

        const Id coords = GetCoordinates(operation, Type::Float);

        spv::ImageOperandsMask mask = spv::ImageOperandsMask::MaskNone;
        std::vector<Id> operands;
        Id texture{};

        if (!meta.aoffi.empty()) {
            mask = mask | spv::ImageOperandsMask::Offset;
            operands.push_back(GetOffsetCoordinates(operation));
        }

        if (meta.sampler.is_shadow) {
            texture = OpImageDrefGather(t_float4, GetTextureSampler(operation), coords,
                                        AsFloat(Visit(meta.depth_compare)), mask, operands);
        } else {
            u32 component_value = 0;
            if (meta.component) {
                const auto component = std::get_if<ImmediateNode>(&*meta.component);
                ASSERT_MSG(component, "Component is not an immediate value");
                component_value = component->GetValue();
            }
            texture = OpImageGather(t_float4, GetTextureSampler(operation), coords,
                                    Constant(t_uint, component_value), mask, operands);
        }
        return GetTextureElement(operation, texture, Type::Float);
    }

    Expression TextureQueryDimensions(Operation operation) {
        const auto& meta = std::get<MetaTexture>(operation.GetMeta());
        UNIMPLEMENTED_IF(!meta.aoffi.empty());
        UNIMPLEMENTED_IF(meta.depth_compare);

        const auto image_id = GetTextureImage(operation);
        if (meta.element == 3) {
            return {OpImageQueryLevels(t_int, image_id), Type::Int};
        }

        const Id lod = AsUint(Visit(operation[0]));
        const std::size_t coords_count = [&meta] {
            switch (const auto type = meta.sampler.type) {
            case Tegra::Shader::TextureType::Texture1D:
                return 1;
            case Tegra::Shader::TextureType::Texture2D:
            case Tegra::Shader::TextureType::TextureCube:
                return 2;
            case Tegra::Shader::TextureType::Texture3D:
                return 3;
            default:
                UNREACHABLE_MSG("Invalid texture type={}", type);
                return 2;
            }
        }();

        if (meta.element >= coords_count) {
            return {v_float_zero, Type::Float};
        }

        const std::array<Id, 3> types = {t_int, t_int2, t_int3};
        const Id sizes = OpImageQuerySizeLod(types.at(coords_count - 1), image_id, lod);
        const Id size = OpCompositeExtract(t_int, sizes, meta.element);
        return {size, Type::Int};
    }

    Expression TextureQueryLod(Operation operation) {
        const auto& meta = std::get<MetaTexture>(operation.GetMeta());
        UNIMPLEMENTED_IF(!meta.aoffi.empty());
        UNIMPLEMENTED_IF(meta.depth_compare);

        if (meta.element >= 2) {
            UNREACHABLE_MSG("Invalid element");
            return {v_float_zero, Type::Float};
        }
        const auto sampler_id = GetTextureSampler(operation);

        const Id multiplier = Constant(t_float, 256.0f);
        const Id multipliers = ConstantComposite(t_float2, multiplier, multiplier);

        const Id coords = GetCoordinates(operation, Type::Float);
        Id size = OpImageQueryLod(t_float2, sampler_id, coords);
        size = OpFMul(t_float2, size, multipliers);
        size = OpConvertFToS(t_int2, size);
        return GetTextureElement(operation, size, Type::Int);
    }

    Expression TexelFetch(Operation operation) {
        const auto& meta = std::get<MetaTexture>(operation.GetMeta());
        UNIMPLEMENTED_IF(meta.depth_compare);

        const Id image = GetTextureImage(operation);
        const Id coords = GetCoordinates(operation, Type::Int);

        spv::ImageOperandsMask mask = spv::ImageOperandsMask::MaskNone;
        std::vector<Id> operands;
        Id fetch;

        if (meta.lod && !meta.sampler.is_buffer) {
            mask = mask | spv::ImageOperandsMask::Lod;
            operands.push_back(AsInt(Visit(meta.lod)));
        }

        if (!meta.aoffi.empty()) {
            mask = mask | spv::ImageOperandsMask::Offset;
            operands.push_back(GetOffsetCoordinates(operation));
        }

        fetch = OpImageFetch(t_float4, image, coords, mask, operands);
        return GetTextureElement(operation, fetch, Type::Float);
    }

    Expression TextureGradient(Operation operation) {
        const auto& meta = std::get<MetaTexture>(operation.GetMeta());
        UNIMPLEMENTED_IF(!meta.aoffi.empty());

        const Id sampler = GetTextureSampler(operation);
        const Id coords = GetCoordinates(operation, Type::Float);
        const auto [dx, dy] = GetDerivatives(operation);
        const std::vector grad = {dx, dy};

        static constexpr auto mask = spv::ImageOperandsMask::Grad;
        const Id texture = OpImageSampleExplicitLod(t_float4, sampler, coords, mask, grad);
        return GetTextureElement(operation, texture, Type::Float);
    }

    Expression ImageLoad(Operation operation) {
        if (!device.IsFormatlessImageLoadSupported()) {
            return {v_float_zero, Type::Float};
        }

        const auto& meta{std::get<MetaImage>(operation.GetMeta())};

        const Id coords = GetCoordinates(operation, Type::Int);
        const Id texel = OpImageRead(t_uint4, GetImage(operation), coords);

        return {OpCompositeExtract(t_uint, texel, meta.element), Type::Uint};
    }

    Expression ImageStore(Operation operation) {
        const auto meta{std::get<MetaImage>(operation.GetMeta())};
        std::vector<Id> colors;
        for (const auto& value : meta.values) {
            colors.push_back(AsUint(Visit(value)));
        }

        const Id coords = GetCoordinates(operation, Type::Int);
        const Id texel = OpCompositeConstruct(t_uint4, colors);

        OpImageWrite(GetImage(operation), coords, texel, {});
        return {};
    }

    template <Id (Module::*func)(Id, Id, Id, Id, Id)>
    Expression AtomicImage(Operation operation) {
        const auto& meta{std::get<MetaImage>(operation.GetMeta())};
        ASSERT(meta.values.size() == 1);

        const Id coordinate = GetCoordinates(operation, Type::Int);
        const Id image = images.at(meta.image.index).image;
        const Id sample = v_uint_zero;
        const Id pointer = OpImageTexelPointer(t_image_uint, image, coordinate, sample);

        const Id scope = Constant(t_uint, static_cast<u32>(spv::Scope::Device));
        const Id semantics = v_uint_zero;
        const Id value = AsUint(Visit(meta.values[0]));
        return {(this->*func)(t_uint, pointer, scope, semantics, value), Type::Uint};
    }

    template <Id (Module::*func)(Id, Id, Id, Id, Id)>
    Expression Atomic(Operation operation) {
        Id pointer;
        if (const auto smem = std::get_if<SmemNode>(&*operation[0])) {
            pointer = GetSharedMemoryPointer(*smem);
        } else if (const auto gmem = std::get_if<GmemNode>(&*operation[0])) {
            pointer = GetGlobalMemoryPointer(*gmem);
        } else {
            UNREACHABLE();
            return {v_float_zero, Type::Float};
        }
        const Id scope = Constant(t_uint, static_cast<u32>(spv::Scope::Device));
        const Id semantics = v_uint_zero;
        const Id value = AsUint(Visit(operation[1]));

        return {(this->*func)(t_uint, pointer, scope, semantics, value), Type::Uint};
    }

    template <Id (Module::*func)(Id, Id, Id, Id, Id)>
    Expression Reduce(Operation operation) {
        Atomic<func>(operation);
        return {};
    }

    Expression Branch(Operation operation) {
        const auto& target = std::get<ImmediateNode>(*operation[0]);
        OpStore(jmp_to, Constant(t_uint, target.GetValue()));
        OpBranch(continue_label);
        inside_branch = true;
        if (!conditional_branch_set) {
            AddLabel();
        }
        return {};
    }

    Expression BranchIndirect(Operation operation) {
        const Id op_a = AsUint(Visit(operation[0]));

        OpStore(jmp_to, op_a);
        OpBranch(continue_label);
        inside_branch = true;
        if (!conditional_branch_set) {
            AddLabel();
        }
        return {};
    }

    Expression PushFlowStack(Operation operation) {
        const auto& target = std::get<ImmediateNode>(*operation[0]);
        const auto [flow_stack, flow_stack_top] = GetFlowStack(operation);
        const Id current = OpLoad(t_uint, flow_stack_top);
        const Id next = OpIAdd(t_uint, current, Constant(t_uint, 1));
        const Id access = OpAccessChain(t_func_uint, flow_stack, current);

        OpStore(access, Constant(t_uint, target.GetValue()));
        OpStore(flow_stack_top, next);
        return {};
    }

    Expression PopFlowStack(Operation operation) {
        const auto [flow_stack, flow_stack_top] = GetFlowStack(operation);
        const Id current = OpLoad(t_uint, flow_stack_top);
        const Id previous = OpISub(t_uint, current, Constant(t_uint, 1));
        const Id access = OpAccessChain(t_func_uint, flow_stack, previous);
        const Id target = OpLoad(t_uint, access);

        OpStore(flow_stack_top, previous);
        OpStore(jmp_to, target);
        OpBranch(continue_label);
        inside_branch = true;
        if (!conditional_branch_set) {
            AddLabel();
        }
        return {};
    }

    Id MaxwellToSpirvComparison(Maxwell::ComparisonOp compare_op, Id operand_1, Id operand_2) {
        using Compare = Maxwell::ComparisonOp;
        switch (compare_op) {
        case Compare::NeverOld:
            return v_false; // Never let the test pass
        case Compare::LessOld:
            return OpFOrdLessThan(t_bool, operand_1, operand_2);
        case Compare::EqualOld:
            return OpFOrdEqual(t_bool, operand_1, operand_2);
        case Compare::LessEqualOld:
            return OpFOrdLessThanEqual(t_bool, operand_1, operand_2);
        case Compare::GreaterOld:
            return OpFOrdGreaterThan(t_bool, operand_1, operand_2);
        case Compare::NotEqualOld:
            return OpFOrdNotEqual(t_bool, operand_1, operand_2);
        case Compare::GreaterEqualOld:
            return OpFOrdGreaterThanEqual(t_bool, operand_1, operand_2);
        default:
            UNREACHABLE();
            return v_true;
        }
    }

    void AlphaTest(Id pointer) {
        if (specialization.alpha_test_func == Maxwell::ComparisonOp::AlwaysOld) {
            return;
        }
        const Id true_label = OpLabel();
        const Id discard_label = OpLabel();
        const Id alpha_reference = Constant(t_float, specialization.alpha_test_ref);
        const Id alpha_value = OpLoad(t_float, pointer);
        const Id condition =
            MaxwellToSpirvComparison(specialization.alpha_test_func, alpha_value, alpha_reference);

        OpBranchConditional(condition, true_label, discard_label);
        AddLabel(discard_label);
        OpKill();
        AddLabel(true_label);
    }

    void PreExit() {
        if (stage == ShaderType::Vertex && specialization.ndc_minus_one_to_one) {
            const u32 position_index = out_indices.position.value();
            const Id z_pointer = AccessElement(t_out_float, out_vertex, position_index, 2U);
            const Id w_pointer = AccessElement(t_out_float, out_vertex, position_index, 3U);
            Id depth = OpLoad(t_float, z_pointer);
            depth = OpFAdd(t_float, depth, OpLoad(t_float, w_pointer));
            depth = OpFMul(t_float, depth, Constant(t_float, 0.5f));
            OpStore(z_pointer, depth);
        }
        if (stage == ShaderType::Fragment) {
            const auto SafeGetRegister = [this](u32 reg) {
                if (const auto it = registers.find(reg); it != registers.end()) {
                    return OpLoad(t_float, it->second);
                }
                return v_float_zero;
            };

            UNIMPLEMENTED_IF_MSG(header.ps.omap.sample_mask != 0,
                                 "Sample mask write is unimplemented");

            // Write the color outputs using the data in the shader registers, disabled
            // rendertargets/components are skipped in the register assignment.
            u32 current_reg = 0;
            for (u32 rt = 0; rt < Maxwell::NumRenderTargets; ++rt) {
                // TODO(Subv): Figure out how dual-source blending is configured in the Switch.
                for (u32 component = 0; component < 4; ++component) {
                    if (!header.ps.IsColorComponentOutputEnabled(rt, component)) {
                        continue;
                    }
                    const Id pointer = AccessElement(t_out_float, frag_colors[rt], component);
                    OpStore(pointer, SafeGetRegister(current_reg));
                    if (rt == 0 && component == 3) {
                        AlphaTest(pointer);
                    }
                    ++current_reg;
                }
            }
            if (header.ps.omap.depth) {
                // The depth output is always 2 registers after the last color output, and
                // current_reg already contains one past the last color register.
                OpStore(frag_depth, SafeGetRegister(current_reg + 1));
            }
        }
    }

    Expression Exit(Operation operation) {
        PreExit();
        inside_branch = true;
        if (conditional_branch_set) {
            OpReturn();
        } else {
            const Id dummy = OpLabel();
            OpBranch(dummy);
            AddLabel(dummy);
            OpReturn();
            AddLabel();
        }
        return {};
    }

    Expression Discard(Operation operation) {
        inside_branch = true;
        if (conditional_branch_set) {
            OpKill();
        } else {
            const Id dummy = OpLabel();
            OpBranch(dummy);
            AddLabel(dummy);
            OpKill();
            AddLabel();
        }
        return {};
    }

    Expression EmitVertex(Operation) {
        OpEmitVertex();
        return {};
    }

    Expression EndPrimitive(Operation operation) {
        OpEndPrimitive();
        return {};
    }

    Expression InvocationId(Operation) {
        return {OpLoad(t_int, invocation_id), Type::Int};
    }

    Expression YNegate(Operation) {
        LOG_WARNING(Render_Vulkan, "(STUBBED)");
        return {Constant(t_float, 1.0f), Type::Float};
    }

    template <u32 element>
    Expression LocalInvocationId(Operation) {
        const Id id = OpLoad(t_uint3, local_invocation_id);
        return {OpCompositeExtract(t_uint, id, element), Type::Uint};
    }

    template <u32 element>
    Expression WorkGroupId(Operation operation) {
        const Id id = OpLoad(t_uint3, workgroup_id);
        return {OpCompositeExtract(t_uint, id, element), Type::Uint};
    }

    Expression BallotThread(Operation operation) {
        const Id predicate = AsBool(Visit(operation[0]));
        const Id ballot = OpSubgroupBallotKHR(t_uint4, predicate);

        if (!device.IsWarpSizePotentiallyBiggerThanGuest()) {
            // Guest-like devices can just return the first index.
            return {OpCompositeExtract(t_uint, ballot, 0U), Type::Uint};
        }

        // The others will have to return what is local to the current thread.
        // For instance a device with a warp size of 64 will return the upper uint when the current
        // thread is 38.
        const Id tid = OpLoad(t_uint, thread_id);
        const Id thread_index = OpShiftRightLogical(t_uint, tid, Constant(t_uint, 5));
        return {OpVectorExtractDynamic(t_uint, ballot, thread_index), Type::Uint};
    }

    template <Id (Module::*func)(Id, Id)>
    Expression Vote(Operation operation) {
        // TODO(Rodrigo): Handle devices with different warp sizes
        const Id predicate = AsBool(Visit(operation[0]));
        return {(this->*func)(t_bool, predicate), Type::Bool};
    }

    Expression ThreadId(Operation) {
        return {OpLoad(t_uint, thread_id), Type::Uint};
    }

    template <std::size_t index>
    Expression ThreadMask(Operation) {
        // TODO(Rodrigo): Handle devices with different warp sizes
        const Id mask = thread_masks[index];
        return {OpLoad(t_uint, AccessElement(t_in_uint, mask, 0)), Type::Uint};
    }

    Expression ShuffleIndexed(Operation operation) {
        const Id value = AsFloat(Visit(operation[0]));
        const Id index = AsUint(Visit(operation[1]));
        return {OpSubgroupReadInvocationKHR(t_float, value, index), Type::Float};
    }

    Expression Barrier(Operation) {
        if (!ir.IsDecompiled()) {
            LOG_ERROR(Render_Vulkan, "OpBarrier used by shader is not decompiled");
            return {};
        }

        const auto scope = spv::Scope::Workgroup;
        const auto memory = spv::Scope::Workgroup;
        const auto semantics =
            spv::MemorySemanticsMask::WorkgroupMemory | spv::MemorySemanticsMask::AcquireRelease;
        OpControlBarrier(Constant(t_uint, static_cast<u32>(scope)),
                         Constant(t_uint, static_cast<u32>(memory)),
                         Constant(t_uint, static_cast<u32>(semantics)));
        return {};
    }

    template <spv::Scope scope>
    Expression MemoryBarrier(Operation) {
        const auto semantics =
            spv::MemorySemanticsMask::AcquireRelease | spv::MemorySemanticsMask::UniformMemory |
            spv::MemorySemanticsMask::WorkgroupMemory |
            spv::MemorySemanticsMask::AtomicCounterMemory | spv::MemorySemanticsMask::ImageMemory;

        OpMemoryBarrier(Constant(t_uint, static_cast<u32>(scope)),
                        Constant(t_uint, static_cast<u32>(semantics)));
        return {};
    }

    Id DeclareBuiltIn(spv::BuiltIn builtin, spv::StorageClass storage, Id type, std::string name) {
        const Id id = OpVariable(type, storage);
        Decorate(id, spv::Decoration::BuiltIn, static_cast<u32>(builtin));
        AddGlobalVariable(Name(id, std::move(name)));
        interfaces.push_back(id);
        return id;
    }

    Id DeclareInputBuiltIn(spv::BuiltIn builtin, Id type, std::string name) {
        return DeclareBuiltIn(builtin, spv::StorageClass::Input, type, std::move(name));
    }

    template <typename... Args>
    Id AccessElement(Id pointer_type, Id composite, Args... elements_) {
        std::vector<Id> members;
        auto elements = {elements_...};
        for (const auto element : elements) {
            members.push_back(Constant(t_uint, element));
        }

        return OpAccessChain(pointer_type, composite, members);
    }

    Id As(Expression expr, Type wanted_type) {
        switch (wanted_type) {
        case Type::Bool:
            return AsBool(expr);
        case Type::Bool2:
            return AsBool2(expr);
        case Type::Float:
            return AsFloat(expr);
        case Type::Int:
            return AsInt(expr);
        case Type::Uint:
            return AsUint(expr);
        case Type::HalfFloat:
            return AsHalfFloat(expr);
        default:
            UNREACHABLE();
            return expr.id;
        }
    }

    Id AsBool(Expression expr) {
        ASSERT(expr.type == Type::Bool);
        return expr.id;
    }

    Id AsBool2(Expression expr) {
        ASSERT(expr.type == Type::Bool2);
        return expr.id;
    }

    Id AsFloat(Expression expr) {
        switch (expr.type) {
        case Type::Float:
            return expr.id;
        case Type::Int:
        case Type::Uint:
            return OpBitcast(t_float, expr.id);
        case Type::HalfFloat:
            if (device.IsFloat16Supported()) {
                return OpBitcast(t_float, expr.id);
            }
            return OpBitcast(t_float, OpPackHalf2x16(t_uint, expr.id));
        default:
            UNREACHABLE();
            return expr.id;
        }
    }

    Id AsInt(Expression expr) {
        switch (expr.type) {
        case Type::Int:
            return expr.id;
        case Type::Float:
        case Type::Uint:
            return OpBitcast(t_int, expr.id);
        case Type::HalfFloat:
            if (device.IsFloat16Supported()) {
                return OpBitcast(t_int, expr.id);
            }
            return OpPackHalf2x16(t_int, expr.id);
        default:
            UNREACHABLE();
            return expr.id;
        }
    }

    Id AsUint(Expression expr) {
        switch (expr.type) {
        case Type::Uint:
            return expr.id;
        case Type::Float:
        case Type::Int:
            return OpBitcast(t_uint, expr.id);
        case Type::HalfFloat:
            if (device.IsFloat16Supported()) {
                return OpBitcast(t_uint, expr.id);
            }
            return OpPackHalf2x16(t_uint, expr.id);
        default:
            UNREACHABLE();
            return expr.id;
        }
    }

    Id AsHalfFloat(Expression expr) {
        switch (expr.type) {
        case Type::HalfFloat:
            return expr.id;
        case Type::Float:
        case Type::Int:
        case Type::Uint:
            if (device.IsFloat16Supported()) {
                return OpBitcast(t_half, expr.id);
            }
            return OpUnpackHalf2x16(t_half, AsUint(expr));
        default:
            UNREACHABLE();
            return expr.id;
        }
    }

    Id GetHalfScalarFromFloat(Id value) {
        if (device.IsFloat16Supported()) {
            return OpFConvert(t_scalar_half, value);
        }
        return value;
    }

    Id GetFloatFromHalfScalar(Id value) {
        if (device.IsFloat16Supported()) {
            return OpFConvert(t_float, value);
        }
        return value;
    }

    AttributeType GetAttributeType(u32 location) const {
        if (stage != ShaderType::Vertex) {
            return {Type::Float, t_in_float, t_in_float4};
        }
        switch (specialization.attribute_types.at(location)) {
        case Maxwell::VertexAttribute::Type::SignedNorm:
        case Maxwell::VertexAttribute::Type::UnsignedNorm:
        case Maxwell::VertexAttribute::Type::UnsignedScaled:
        case Maxwell::VertexAttribute::Type::SignedScaled:
        case Maxwell::VertexAttribute::Type::Float:
            return {Type::Float, t_in_float, t_in_float4};
        case Maxwell::VertexAttribute::Type::SignedInt:
            return {Type::Int, t_in_int, t_in_int4};
        case Maxwell::VertexAttribute::Type::UnsignedInt:
            return {Type::Uint, t_in_uint, t_in_uint4};
        default:
            UNREACHABLE();
            return {Type::Float, t_in_float, t_in_float4};
        }
    }

    Id GetTypeDefinition(Type type) const {
        switch (type) {
        case Type::Bool:
            return t_bool;
        case Type::Bool2:
            return t_bool2;
        case Type::Float:
            return t_float;
        case Type::Int:
            return t_int;
        case Type::Uint:
            return t_uint;
        case Type::HalfFloat:
            return t_half;
        default:
            UNREACHABLE();
            return {};
        }
    }

    std::array<Id, 4> GetTypeVectorDefinitionLut(Type type) const {
        switch (type) {
        case Type::Float:
            return {t_float, t_float2, t_float3, t_float4};
        case Type::Int:
            return {t_int, t_int2, t_int3, t_int4};
        case Type::Uint:
            return {t_uint, t_uint2, t_uint3, t_uint4};
        default:
            UNIMPLEMENTED();
            return {};
        }
    }

    std::tuple<Id, Id> CreateFlowStack() {
        // TODO(Rodrigo): Figure out the actual depth of the flow stack, for now it seems unlikely
        // that shaders will use 20 nested SSYs and PBKs.
        constexpr u32 FLOW_STACK_SIZE = 20;
        constexpr auto storage_class = spv::StorageClass::Function;

        const Id flow_stack_type = TypeArray(t_uint, Constant(t_uint, FLOW_STACK_SIZE));
        const Id stack = OpVariable(TypePointer(storage_class, flow_stack_type), storage_class,
                                    ConstantNull(flow_stack_type));
        const Id top = OpVariable(t_func_uint, storage_class, Constant(t_uint, 0));
        AddLocalVariable(stack);
        AddLocalVariable(top);
        return std::tie(stack, top);
    }

    std::pair<Id, Id> GetFlowStack(Operation operation) {
        const auto stack_class = std::get<MetaStackClass>(operation.GetMeta());
        switch (stack_class) {
        case MetaStackClass::Ssy:
            return {ssy_flow_stack, ssy_flow_stack_top};
        case MetaStackClass::Pbk:
            return {pbk_flow_stack, pbk_flow_stack_top};
        }
        UNREACHABLE();
        return {};
    }

    Id GetGlobalMemoryPointer(const GmemNode& gmem) {
        const Id real = AsUint(Visit(gmem.GetRealAddress()));
        const Id base = AsUint(Visit(gmem.GetBaseAddress()));
        const Id diff = OpISub(t_uint, real, base);
        const Id offset = OpShiftRightLogical(t_uint, diff, Constant(t_uint, 2));
        const Id buffer = global_buffers.at(gmem.GetDescriptor());
        return OpAccessChain(t_gmem_uint, buffer, Constant(t_uint, 0), offset);
    }

    Id GetSharedMemoryPointer(const SmemNode& smem) {
        ASSERT(stage == ShaderType::Compute);
        Id address = AsUint(Visit(smem.GetAddress()));
        address = OpShiftRightLogical(t_uint, address, Constant(t_uint, 2U));
        return OpAccessChain(t_smem_uint, shared_memory, address);
    }

    static constexpr std::array operation_decompilers = {
        &SPIRVDecompiler::Assign,

        &SPIRVDecompiler::Ternary<&Module::OpSelect, Type::Float, Type::Bool, Type::Float,
                                  Type::Float>,

        &SPIRVDecompiler::Binary<&Module::OpFAdd, Type::Float>,
        &SPIRVDecompiler::Binary<&Module::OpFMul, Type::Float>,
        &SPIRVDecompiler::Binary<&Module::OpFDiv, Type::Float>,
        &SPIRVDecompiler::Ternary<&Module::OpFma, Type::Float>,
        &SPIRVDecompiler::Unary<&Module::OpFNegate, Type::Float>,
        &SPIRVDecompiler::Unary<&Module::OpFAbs, Type::Float>,
        &SPIRVDecompiler::Ternary<&Module::OpFClamp, Type::Float>,
        &SPIRVDecompiler::FCastHalf<0>,
        &SPIRVDecompiler::FCastHalf<1>,
        &SPIRVDecompiler::Binary<&Module::OpFMin, Type::Float>,
        &SPIRVDecompiler::Binary<&Module::OpFMax, Type::Float>,
        &SPIRVDecompiler::Unary<&Module::OpCos, Type::Float>,
        &SPIRVDecompiler::Unary<&Module::OpSin, Type::Float>,
        &SPIRVDecompiler::Unary<&Module::OpExp2, Type::Float>,
        &SPIRVDecompiler::Unary<&Module::OpLog2, Type::Float>,
        &SPIRVDecompiler::Unary<&Module::OpInverseSqrt, Type::Float>,
        &SPIRVDecompiler::Unary<&Module::OpSqrt, Type::Float>,
        &SPIRVDecompiler::Unary<&Module::OpRoundEven, Type::Float>,
        &SPIRVDecompiler::Unary<&Module::OpFloor, Type::Float>,
        &SPIRVDecompiler::Unary<&Module::OpCeil, Type::Float>,
        &SPIRVDecompiler::Unary<&Module::OpTrunc, Type::Float>,
        &SPIRVDecompiler::Unary<&Module::OpConvertSToF, Type::Float, Type::Int>,
        &SPIRVDecompiler::Unary<&Module::OpConvertUToF, Type::Float, Type::Uint>,
        &SPIRVDecompiler::FSwizzleAdd,

        &SPIRVDecompiler::Binary<&Module::OpIAdd, Type::Int>,
        &SPIRVDecompiler::Binary<&Module::OpIMul, Type::Int>,
        &SPIRVDecompiler::Binary<&Module::OpSDiv, Type::Int>,
        &SPIRVDecompiler::Unary<&Module::OpSNegate, Type::Int>,
        &SPIRVDecompiler::Unary<&Module::OpSAbs, Type::Int>,
        &SPIRVDecompiler::Binary<&Module::OpSMin, Type::Int>,
        &SPIRVDecompiler::Binary<&Module::OpSMax, Type::Int>,

        &SPIRVDecompiler::Unary<&Module::OpConvertFToS, Type::Int, Type::Float>,
        &SPIRVDecompiler::Unary<&Module::OpBitcast, Type::Int, Type::Uint>,
        &SPIRVDecompiler::Binary<&Module::OpShiftLeftLogical, Type::Int, Type::Int, Type::Uint>,
        &SPIRVDecompiler::Binary<&Module::OpShiftRightLogical, Type::Int, Type::Int, Type::Uint>,
        &SPIRVDecompiler::Binary<&Module::OpShiftRightArithmetic, Type::Int, Type::Int, Type::Uint>,
        &SPIRVDecompiler::Binary<&Module::OpBitwiseAnd, Type::Int>,
        &SPIRVDecompiler::Binary<&Module::OpBitwiseOr, Type::Int>,
        &SPIRVDecompiler::Binary<&Module::OpBitwiseXor, Type::Int>,
        &SPIRVDecompiler::Unary<&Module::OpNot, Type::Int>,
        &SPIRVDecompiler::Quaternary<&Module::OpBitFieldInsert, Type::Int>,
        &SPIRVDecompiler::Ternary<&Module::OpBitFieldSExtract, Type::Int>,
        &SPIRVDecompiler::Unary<&Module::OpBitCount, Type::Int>,
        &SPIRVDecompiler::Unary<&Module::OpFindSMsb, Type::Int>,

        &SPIRVDecompiler::Binary<&Module::OpIAdd, Type::Uint>,
        &SPIRVDecompiler::Binary<&Module::OpIMul, Type::Uint>,
        &SPIRVDecompiler::Binary<&Module::OpUDiv, Type::Uint>,
        &SPIRVDecompiler::Binary<&Module::OpUMin, Type::Uint>,
        &SPIRVDecompiler::Binary<&Module::OpUMax, Type::Uint>,
        &SPIRVDecompiler::Unary<&Module::OpConvertFToU, Type::Uint, Type::Float>,
        &SPIRVDecompiler::Unary<&Module::OpBitcast, Type::Uint, Type::Int>,
        &SPIRVDecompiler::Binary<&Module::OpShiftLeftLogical, Type::Uint>,
        &SPIRVDecompiler::Binary<&Module::OpShiftRightLogical, Type::Uint>,
        &SPIRVDecompiler::Binary<&Module::OpShiftRightLogical, Type::Uint>,
        &SPIRVDecompiler::Binary<&Module::OpBitwiseAnd, Type::Uint>,
        &SPIRVDecompiler::Binary<&Module::OpBitwiseOr, Type::Uint>,
        &SPIRVDecompiler::Binary<&Module::OpBitwiseXor, Type::Uint>,
        &SPIRVDecompiler::Unary<&Module::OpNot, Type::Uint>,
        &SPIRVDecompiler::Quaternary<&Module::OpBitFieldInsert, Type::Uint>,
        &SPIRVDecompiler::Ternary<&Module::OpBitFieldUExtract, Type::Uint>,
        &SPIRVDecompiler::Unary<&Module::OpBitCount, Type::Uint>,
        &SPIRVDecompiler::Unary<&Module::OpFindUMsb, Type::Uint>,

        &SPIRVDecompiler::Binary<&Module::OpFAdd, Type::HalfFloat>,
        &SPIRVDecompiler::Binary<&Module::OpFMul, Type::HalfFloat>,
        &SPIRVDecompiler::Ternary<&Module::OpFma, Type::HalfFloat>,
        &SPIRVDecompiler::Unary<&Module::OpFAbs, Type::HalfFloat>,
        &SPIRVDecompiler::HNegate,
        &SPIRVDecompiler::HClamp,
        &SPIRVDecompiler::HCastFloat,
        &SPIRVDecompiler::HUnpack,
        &SPIRVDecompiler::HMergeF32,
        &SPIRVDecompiler::HMergeHN<0>,
        &SPIRVDecompiler::HMergeHN<1>,
        &SPIRVDecompiler::HPack2,

        &SPIRVDecompiler::LogicalAssign,
        &SPIRVDecompiler::Binary<&Module::OpLogicalAnd, Type::Bool>,
        &SPIRVDecompiler::Binary<&Module::OpLogicalOr, Type::Bool>,
        &SPIRVDecompiler::Binary<&Module::OpLogicalNotEqual, Type::Bool>,
        &SPIRVDecompiler::Unary<&Module::OpLogicalNot, Type::Bool>,
        &SPIRVDecompiler::Binary<&Module::OpVectorExtractDynamic, Type::Bool, Type::Bool2,
                                 Type::Uint>,
        &SPIRVDecompiler::Unary<&Module::OpAll, Type::Bool, Type::Bool2>,

        &SPIRVDecompiler::Binary<&Module::OpFOrdLessThan, Type::Bool, Type::Float>,
        &SPIRVDecompiler::Binary<&Module::OpFOrdEqual, Type::Bool, Type::Float>,
        &SPIRVDecompiler::Binary<&Module::OpFOrdLessThanEqual, Type::Bool, Type::Float>,
        &SPIRVDecompiler::Binary<&Module::OpFOrdGreaterThan, Type::Bool, Type::Float>,
        &SPIRVDecompiler::Binary<&Module::OpFOrdNotEqual, Type::Bool, Type::Float>,
        &SPIRVDecompiler::Binary<&Module::OpFOrdGreaterThanEqual, Type::Bool, Type::Float>,
        &SPIRVDecompiler::LogicalFOrdered,
        &SPIRVDecompiler::LogicalFUnordered,
        &SPIRVDecompiler::Binary<&Module::OpFUnordLessThan, Type::Bool, Type::Float>,
        &SPIRVDecompiler::Binary<&Module::OpFUnordEqual, Type::Bool, Type::Float>,
        &SPIRVDecompiler::Binary<&Module::OpFUnordLessThanEqual, Type::Bool, Type::Float>,
        &SPIRVDecompiler::Binary<&Module::OpFUnordGreaterThan, Type::Bool, Type::Float>,
        &SPIRVDecompiler::Binary<&Module::OpFUnordNotEqual, Type::Bool, Type::Float>,
        &SPIRVDecompiler::Binary<&Module::OpFUnordGreaterThanEqual, Type::Bool, Type::Float>,

        &SPIRVDecompiler::Binary<&Module::OpSLessThan, Type::Bool, Type::Int>,
        &SPIRVDecompiler::Binary<&Module::OpIEqual, Type::Bool, Type::Int>,
        &SPIRVDecompiler::Binary<&Module::OpSLessThanEqual, Type::Bool, Type::Int>,
        &SPIRVDecompiler::Binary<&Module::OpSGreaterThan, Type::Bool, Type::Int>,
        &SPIRVDecompiler::Binary<&Module::OpINotEqual, Type::Bool, Type::Int>,
        &SPIRVDecompiler::Binary<&Module::OpSGreaterThanEqual, Type::Bool, Type::Int>,

        &SPIRVDecompiler::Binary<&Module::OpULessThan, Type::Bool, Type::Uint>,
        &SPIRVDecompiler::Binary<&Module::OpIEqual, Type::Bool, Type::Uint>,
        &SPIRVDecompiler::Binary<&Module::OpULessThanEqual, Type::Bool, Type::Uint>,
        &SPIRVDecompiler::Binary<&Module::OpUGreaterThan, Type::Bool, Type::Uint>,
        &SPIRVDecompiler::Binary<&Module::OpINotEqual, Type::Bool, Type::Uint>,
        &SPIRVDecompiler::Binary<&Module::OpUGreaterThanEqual, Type::Bool, Type::Uint>,

        &SPIRVDecompiler::LogicalAddCarry,

        &SPIRVDecompiler::Binary<&Module::OpFOrdLessThan, Type::Bool2, Type::HalfFloat>,
        &SPIRVDecompiler::Binary<&Module::OpFOrdEqual, Type::Bool2, Type::HalfFloat>,
        &SPIRVDecompiler::Binary<&Module::OpFOrdLessThanEqual, Type::Bool2, Type::HalfFloat>,
        &SPIRVDecompiler::Binary<&Module::OpFOrdGreaterThan, Type::Bool2, Type::HalfFloat>,
        &SPIRVDecompiler::Binary<&Module::OpFOrdNotEqual, Type::Bool2, Type::HalfFloat>,
        &SPIRVDecompiler::Binary<&Module::OpFOrdGreaterThanEqual, Type::Bool2, Type::HalfFloat>,
        // TODO(Rodrigo): Should these use the OpFUnord* variants?
        &SPIRVDecompiler::Binary<&Module::OpFOrdLessThan, Type::Bool2, Type::HalfFloat>,
        &SPIRVDecompiler::Binary<&Module::OpFOrdEqual, Type::Bool2, Type::HalfFloat>,
        &SPIRVDecompiler::Binary<&Module::OpFOrdLessThanEqual, Type::Bool2, Type::HalfFloat>,
        &SPIRVDecompiler::Binary<&Module::OpFOrdGreaterThan, Type::Bool2, Type::HalfFloat>,
        &SPIRVDecompiler::Binary<&Module::OpFOrdNotEqual, Type::Bool2, Type::HalfFloat>,
        &SPIRVDecompiler::Binary<&Module::OpFOrdGreaterThanEqual, Type::Bool2, Type::HalfFloat>,

        &SPIRVDecompiler::Texture,
        &SPIRVDecompiler::TextureLod,
        &SPIRVDecompiler::TextureGather,
        &SPIRVDecompiler::TextureQueryDimensions,
        &SPIRVDecompiler::TextureQueryLod,
        &SPIRVDecompiler::TexelFetch,
        &SPIRVDecompiler::TextureGradient,

        &SPIRVDecompiler::ImageLoad,
        &SPIRVDecompiler::ImageStore,
        &SPIRVDecompiler::AtomicImage<&Module::OpAtomicIAdd>,
        &SPIRVDecompiler::AtomicImage<&Module::OpAtomicAnd>,
        &SPIRVDecompiler::AtomicImage<&Module::OpAtomicOr>,
        &SPIRVDecompiler::AtomicImage<&Module::OpAtomicXor>,
        &SPIRVDecompiler::AtomicImage<&Module::OpAtomicExchange>,

        &SPIRVDecompiler::Atomic<&Module::OpAtomicExchange>,
        &SPIRVDecompiler::Atomic<&Module::OpAtomicIAdd>,
        &SPIRVDecompiler::Atomic<&Module::OpAtomicUMin>,
        &SPIRVDecompiler::Atomic<&Module::OpAtomicUMax>,
        &SPIRVDecompiler::Atomic<&Module::OpAtomicAnd>,
        &SPIRVDecompiler::Atomic<&Module::OpAtomicOr>,
        &SPIRVDecompiler::Atomic<&Module::OpAtomicXor>,

        &SPIRVDecompiler::Atomic<&Module::OpAtomicExchange>,
        &SPIRVDecompiler::Atomic<&Module::OpAtomicIAdd>,
        &SPIRVDecompiler::Atomic<&Module::OpAtomicSMin>,
        &SPIRVDecompiler::Atomic<&Module::OpAtomicSMax>,
        &SPIRVDecompiler::Atomic<&Module::OpAtomicAnd>,
        &SPIRVDecompiler::Atomic<&Module::OpAtomicOr>,
        &SPIRVDecompiler::Atomic<&Module::OpAtomicXor>,

        &SPIRVDecompiler::Reduce<&Module::OpAtomicIAdd>,
        &SPIRVDecompiler::Reduce<&Module::OpAtomicUMin>,
        &SPIRVDecompiler::Reduce<&Module::OpAtomicUMax>,
        &SPIRVDecompiler::Reduce<&Module::OpAtomicAnd>,
        &SPIRVDecompiler::Reduce<&Module::OpAtomicOr>,
        &SPIRVDecompiler::Reduce<&Module::OpAtomicXor>,

        &SPIRVDecompiler::Reduce<&Module::OpAtomicIAdd>,
        &SPIRVDecompiler::Reduce<&Module::OpAtomicSMin>,
        &SPIRVDecompiler::Reduce<&Module::OpAtomicSMax>,
        &SPIRVDecompiler::Reduce<&Module::OpAtomicAnd>,
        &SPIRVDecompiler::Reduce<&Module::OpAtomicOr>,
        &SPIRVDecompiler::Reduce<&Module::OpAtomicXor>,

        &SPIRVDecompiler::Branch,
        &SPIRVDecompiler::BranchIndirect,
        &SPIRVDecompiler::PushFlowStack,
        &SPIRVDecompiler::PopFlowStack,
        &SPIRVDecompiler::Exit,
        &SPIRVDecompiler::Discard,

        &SPIRVDecompiler::EmitVertex,
        &SPIRVDecompiler::EndPrimitive,

        &SPIRVDecompiler::InvocationId,
        &SPIRVDecompiler::YNegate,
        &SPIRVDecompiler::LocalInvocationId<0>,
        &SPIRVDecompiler::LocalInvocationId<1>,
        &SPIRVDecompiler::LocalInvocationId<2>,
        &SPIRVDecompiler::WorkGroupId<0>,
        &SPIRVDecompiler::WorkGroupId<1>,
        &SPIRVDecompiler::WorkGroupId<2>,

        &SPIRVDecompiler::BallotThread,
        &SPIRVDecompiler::Vote<&Module::OpSubgroupAllKHR>,
        &SPIRVDecompiler::Vote<&Module::OpSubgroupAnyKHR>,
        &SPIRVDecompiler::Vote<&Module::OpSubgroupAllEqualKHR>,

        &SPIRVDecompiler::ThreadId,
        &SPIRVDecompiler::ThreadMask<0>, // Eq
        &SPIRVDecompiler::ThreadMask<1>, // Ge
        &SPIRVDecompiler::ThreadMask<2>, // Gt
        &SPIRVDecompiler::ThreadMask<3>, // Le
        &SPIRVDecompiler::ThreadMask<4>, // Lt
        &SPIRVDecompiler::ShuffleIndexed,

        &SPIRVDecompiler::Barrier,
        &SPIRVDecompiler::MemoryBarrier<spv::Scope::Workgroup>,
        &SPIRVDecompiler::MemoryBarrier<spv::Scope::Device>,
    };
    static_assert(operation_decompilers.size() == static_cast<std::size_t>(OperationCode::Amount));

    const Device& device;
    const ShaderIR& ir;
    const ShaderType stage;
    const Tegra::Shader::Header header;
    const Registry& registry;
    const Specialization& specialization;
    std::unordered_map<u8, VaryingTFB> transform_feedback;

    const Id t_void = Name(TypeVoid(), "void");

    const Id t_bool = Name(TypeBool(), "bool");
    const Id t_bool2 = Name(TypeVector(t_bool, 2), "bool2");

    const Id t_int = Name(TypeInt(32, true), "int");
    const Id t_int2 = Name(TypeVector(t_int, 2), "int2");
    const Id t_int3 = Name(TypeVector(t_int, 3), "int3");
    const Id t_int4 = Name(TypeVector(t_int, 4), "int4");

    const Id t_uint = Name(TypeInt(32, false), "uint");
    const Id t_uint2 = Name(TypeVector(t_uint, 2), "uint2");
    const Id t_uint3 = Name(TypeVector(t_uint, 3), "uint3");
    const Id t_uint4 = Name(TypeVector(t_uint, 4), "uint4");

    const Id t_float = Name(TypeFloat(32), "float");
    const Id t_float2 = Name(TypeVector(t_float, 2), "float2");
    const Id t_float3 = Name(TypeVector(t_float, 3), "float3");
    const Id t_float4 = Name(TypeVector(t_float, 4), "float4");

    const Id t_prv_bool = Name(TypePointer(spv::StorageClass::Private, t_bool), "prv_bool");
    const Id t_prv_float = Name(TypePointer(spv::StorageClass::Private, t_float), "prv_float");

    const Id t_func_uint = Name(TypePointer(spv::StorageClass::Function, t_uint), "func_uint");

    const Id t_in_bool = Name(TypePointer(spv::StorageClass::Input, t_bool), "in_bool");
    const Id t_in_int = Name(TypePointer(spv::StorageClass::Input, t_int), "in_int");
    const Id t_in_int4 = Name(TypePointer(spv::StorageClass::Input, t_int4), "in_int4");
    const Id t_in_uint = Name(TypePointer(spv::StorageClass::Input, t_uint), "in_uint");
    const Id t_in_uint3 = Name(TypePointer(spv::StorageClass::Input, t_uint3), "in_uint3");
    const Id t_in_uint4 = Name(TypePointer(spv::StorageClass::Input, t_uint4), "in_uint4");
    const Id t_in_float = Name(TypePointer(spv::StorageClass::Input, t_float), "in_float");
    const Id t_in_float2 = Name(TypePointer(spv::StorageClass::Input, t_float2), "in_float2");
    const Id t_in_float3 = Name(TypePointer(spv::StorageClass::Input, t_float3), "in_float3");
    const Id t_in_float4 = Name(TypePointer(spv::StorageClass::Input, t_float4), "in_float4");

    const Id t_out_int = Name(TypePointer(spv::StorageClass::Output, t_int), "out_int");

    const Id t_out_float = Name(TypePointer(spv::StorageClass::Output, t_float), "out_float");
    const Id t_out_float4 = Name(TypePointer(spv::StorageClass::Output, t_float4), "out_float4");

    const Id t_cbuf_float = TypePointer(spv::StorageClass::Uniform, t_float);
    const Id t_cbuf_std140 = Decorate(
        Name(TypeArray(t_float4, Constant(t_uint, MaxConstBufferElements)), "CbufStd140Array"),
        spv::Decoration::ArrayStride, 16U);
    const Id t_cbuf_scalar = Decorate(
        Name(TypeArray(t_float, Constant(t_uint, MaxConstBufferFloats)), "CbufScalarArray"),
        spv::Decoration::ArrayStride, 4U);
    const Id t_cbuf_std140_struct = MemberDecorate(
        Decorate(TypeStruct(t_cbuf_std140), spv::Decoration::Block), 0, spv::Decoration::Offset, 0);
    const Id t_cbuf_scalar_struct = MemberDecorate(
        Decorate(TypeStruct(t_cbuf_scalar), spv::Decoration::Block), 0, spv::Decoration::Offset, 0);
    const Id t_cbuf_std140_ubo = TypePointer(spv::StorageClass::Uniform, t_cbuf_std140_struct);
    const Id t_cbuf_scalar_ubo = TypePointer(spv::StorageClass::Uniform, t_cbuf_scalar_struct);

    Id t_smem_uint{};

    const Id t_gmem_uint = TypePointer(spv::StorageClass::StorageBuffer, t_uint);
    const Id t_gmem_array =
        Name(Decorate(TypeRuntimeArray(t_uint), spv::Decoration::ArrayStride, 4U), "GmemArray");
    const Id t_gmem_struct = MemberDecorate(
        Decorate(TypeStruct(t_gmem_array), spv::Decoration::Block), 0, spv::Decoration::Offset, 0);
    const Id t_gmem_ssbo = TypePointer(spv::StorageClass::StorageBuffer, t_gmem_struct);

    const Id t_image_uint = TypePointer(spv::StorageClass::Image, t_uint);

    const Id v_float_zero = Constant(t_float, 0.0f);
    const Id v_float_one = Constant(t_float, 1.0f);
    const Id v_uint_zero = Constant(t_uint, 0);

    // Nvidia uses these defaults for varyings (e.g. position and generic attributes)
    const Id v_varying_default =
        ConstantComposite(t_float4, v_float_zero, v_float_zero, v_float_zero, v_float_one);

    const Id v_true = ConstantTrue(t_bool);
    const Id v_false = ConstantFalse(t_bool);

    Id t_scalar_half{};
    Id t_half{};

    Id out_vertex{};
    Id in_vertex{};
    std::map<u32, Id> registers;
    std::map<u32, Id> custom_variables;
    std::map<Tegra::Shader::Pred, Id> predicates;
    std::map<u32, Id> flow_variables;
    Id local_memory{};
    Id shared_memory{};
    std::array<Id, INTERNAL_FLAGS_COUNT> internal_flags{};
    std::map<Attribute::Index, Id> input_attributes;
    std::unordered_map<u8, GenericVaryingDescription> output_attributes;
    std::map<u32, Id> constant_buffers;
    std::map<GlobalMemoryBase, Id> global_buffers;
    std::map<u32, TexelBuffer> uniform_texels;
    std::map<u32, SampledImage> sampled_images;
    std::map<u32, StorageImage> images;

    std::array<Id, Maxwell::NumRenderTargets> frag_colors{};
    Id instance_index{};
    Id vertex_index{};
    Id base_instance{};
    Id base_vertex{};
    Id frag_depth{};
    Id frag_coord{};
    Id front_facing{};
    Id point_coord{};
    Id tess_level_outer{};
    Id tess_level_inner{};
    Id tess_coord{};
    Id invocation_id{};
    Id workgroup_id{};
    Id local_invocation_id{};
    Id thread_id{};
    std::array<Id, 5> thread_masks{}; // eq, ge, gt, le, lt

    VertexIndices in_indices;
    VertexIndices out_indices;

    std::vector<Id> interfaces;

    Id jmp_to{};
    Id ssy_flow_stack_top{};
    Id pbk_flow_stack_top{};
    Id ssy_flow_stack{};
    Id pbk_flow_stack{};
    Id continue_label{};
    std::map<u32, Id> labels;

    bool conditional_branch_set{};
    bool inside_branch{};
};

class ExprDecompiler {
public:
    explicit ExprDecompiler(SPIRVDecompiler& decomp_) : decomp{decomp_} {}

    Id operator()(const ExprAnd& expr) {
        const Id type_def = decomp.GetTypeDefinition(Type::Bool);
        const Id op1 = Visit(expr.operand1);
        const Id op2 = Visit(expr.operand2);
        return decomp.OpLogicalAnd(type_def, op1, op2);
    }

    Id operator()(const ExprOr& expr) {
        const Id type_def = decomp.GetTypeDefinition(Type::Bool);
        const Id op1 = Visit(expr.operand1);
        const Id op2 = Visit(expr.operand2);
        return decomp.OpLogicalOr(type_def, op1, op2);
    }

    Id operator()(const ExprNot& expr) {
        const Id type_def = decomp.GetTypeDefinition(Type::Bool);
        const Id op1 = Visit(expr.operand1);
        return decomp.OpLogicalNot(type_def, op1);
    }

    Id operator()(const ExprPredicate& expr) {
        const auto pred = static_cast<Tegra::Shader::Pred>(expr.predicate);
        return decomp.OpLoad(decomp.t_bool, decomp.predicates.at(pred));
    }

    Id operator()(const ExprCondCode& expr) {
        return decomp.AsBool(decomp.Visit(decomp.ir.GetConditionCode(expr.cc)));
    }

    Id operator()(const ExprVar& expr) {
        return decomp.OpLoad(decomp.t_bool, decomp.flow_variables.at(expr.var_index));
    }

    Id operator()(const ExprBoolean& expr) {
        return expr.value ? decomp.v_true : decomp.v_false;
    }

    Id operator()(const ExprGprEqual& expr) {
        const Id target = decomp.Constant(decomp.t_uint, expr.value);
        Id gpr = decomp.OpLoad(decomp.t_float, decomp.registers.at(expr.gpr));
        gpr = decomp.OpBitcast(decomp.t_uint, gpr);
        return decomp.OpIEqual(decomp.t_bool, gpr, target);
    }

    Id Visit(const Expr& node) {
        return std::visit(*this, *node);
    }

private:
    SPIRVDecompiler& decomp;
};

class ASTDecompiler {
public:
    explicit ASTDecompiler(SPIRVDecompiler& decomp_) : decomp{decomp_} {}

    void operator()(const ASTProgram& ast) {
        ASTNode current = ast.nodes.GetFirst();
        while (current) {
            Visit(current);
            current = current->GetNext();
        }
    }

    void operator()(const ASTIfThen& ast) {
        ExprDecompiler expr_parser{decomp};
        const Id condition = expr_parser.Visit(ast.condition);
        const Id then_label = decomp.OpLabel();
        const Id endif_label = decomp.OpLabel();
        decomp.OpSelectionMerge(endif_label, spv::SelectionControlMask::MaskNone);
        decomp.OpBranchConditional(condition, then_label, endif_label);
        decomp.AddLabel(then_label);
        ASTNode current = ast.nodes.GetFirst();
        while (current) {
            Visit(current);
            current = current->GetNext();
        }
        decomp.OpBranch(endif_label);
        decomp.AddLabel(endif_label);
    }

    void operator()([[maybe_unused]] const ASTIfElse& ast) {
        UNREACHABLE();
    }

    void operator()([[maybe_unused]] const ASTBlockEncoded& ast) {
        UNREACHABLE();
    }

    void operator()(const ASTBlockDecoded& ast) {
        decomp.VisitBasicBlock(ast.nodes);
    }

    void operator()(const ASTVarSet& ast) {
        ExprDecompiler expr_parser{decomp};
        const Id condition = expr_parser.Visit(ast.condition);
        decomp.OpStore(decomp.flow_variables.at(ast.index), condition);
    }

    void operator()([[maybe_unused]] const ASTLabel& ast) {
        // Do nothing
    }

    void operator()([[maybe_unused]] const ASTGoto& ast) {
        UNREACHABLE();
    }

    void operator()(const ASTDoWhile& ast) {
        const Id loop_label = decomp.OpLabel();
        const Id endloop_label = decomp.OpLabel();
        const Id loop_start_block = decomp.OpLabel();
        const Id loop_continue_block = decomp.OpLabel();
        current_loop_exit = endloop_label;
        decomp.OpBranch(loop_label);
        decomp.AddLabel(loop_label);
        decomp.OpLoopMerge(endloop_label, loop_continue_block, spv::LoopControlMask::MaskNone);
        decomp.OpBranch(loop_start_block);
        decomp.AddLabel(loop_start_block);
        ASTNode current = ast.nodes.GetFirst();
        while (current) {
            Visit(current);
            current = current->GetNext();
        }
        decomp.OpBranch(loop_continue_block);
        decomp.AddLabel(loop_continue_block);
        ExprDecompiler expr_parser{decomp};
        const Id condition = expr_parser.Visit(ast.condition);
        decomp.OpBranchConditional(condition, loop_label, endloop_label);
        decomp.AddLabel(endloop_label);
    }

    void operator()(const ASTReturn& ast) {
        if (!VideoCommon::Shader::ExprIsTrue(ast.condition)) {
            ExprDecompiler expr_parser{decomp};
            const Id condition = expr_parser.Visit(ast.condition);
            const Id then_label = decomp.OpLabel();
            const Id endif_label = decomp.OpLabel();
            decomp.OpSelectionMerge(endif_label, spv::SelectionControlMask::MaskNone);
            decomp.OpBranchConditional(condition, then_label, endif_label);
            decomp.AddLabel(then_label);
            if (ast.kills) {
                decomp.OpKill();
            } else {
                decomp.PreExit();
                decomp.OpReturn();
            }
            decomp.AddLabel(endif_label);
        } else {
            const Id next_block = decomp.OpLabel();
            decomp.OpBranch(next_block);
            decomp.AddLabel(next_block);
            if (ast.kills) {
                decomp.OpKill();
            } else {
                decomp.PreExit();
                decomp.OpReturn();
            }
            decomp.AddLabel(decomp.OpLabel());
        }
    }

    void operator()(const ASTBreak& ast) {
        if (!VideoCommon::Shader::ExprIsTrue(ast.condition)) {
            ExprDecompiler expr_parser{decomp};
            const Id condition = expr_parser.Visit(ast.condition);
            const Id then_label = decomp.OpLabel();
            const Id endif_label = decomp.OpLabel();
            decomp.OpSelectionMerge(endif_label, spv::SelectionControlMask::MaskNone);
            decomp.OpBranchConditional(condition, then_label, endif_label);
            decomp.AddLabel(then_label);
            decomp.OpBranch(current_loop_exit);
            decomp.AddLabel(endif_label);
        } else {
            const Id next_block = decomp.OpLabel();
            decomp.OpBranch(next_block);
            decomp.AddLabel(next_block);
            decomp.OpBranch(current_loop_exit);
            decomp.AddLabel(decomp.OpLabel());
        }
    }

    void Visit(const ASTNode& node) {
        std::visit(*this, *node->GetInnerData());
    }

private:
    SPIRVDecompiler& decomp;
    Id current_loop_exit{};
};

void SPIRVDecompiler::DecompileAST() {
    const u32 num_flow_variables = ir.GetASTNumVariables();
    for (u32 i = 0; i < num_flow_variables; i++) {
        const Id id = OpVariable(t_prv_bool, spv::StorageClass::Private, v_false);
        Name(id, fmt::format("flow_var_{}", i));
        flow_variables.emplace(i, AddGlobalVariable(id));
    }

    DefinePrologue();

    const ASTNode program = ir.GetASTProgram();
    ASTDecompiler decompiler{*this};
    decompiler.Visit(program);

    const Id next_block = OpLabel();
    OpBranch(next_block);
    AddLabel(next_block);
}

} // Anonymous namespace

ShaderEntries GenerateShaderEntries(const VideoCommon::Shader::ShaderIR& ir) {
    ShaderEntries entries;
    for (const auto& cbuf : ir.GetConstantBuffers()) {
        entries.const_buffers.emplace_back(cbuf.second, cbuf.first);
    }
    for (const auto& [base, usage] : ir.GetGlobalMemory()) {
        entries.global_buffers.emplace_back(GlobalBufferEntry{
            .cbuf_index = base.cbuf_index,
            .cbuf_offset = base.cbuf_offset,
            .is_written = usage.is_written,
        });
    }
    for (const auto& sampler : ir.GetSamplers()) {
        if (sampler.is_buffer) {
            entries.uniform_texels.emplace_back(sampler);
        } else {
            entries.samplers.emplace_back(sampler);
        }
    }
    for (const auto& image : ir.GetImages()) {
        if (image.type == Tegra::Shader::ImageType::TextureBuffer) {
            entries.storage_texels.emplace_back(image);
        } else {
            entries.images.emplace_back(image);
        }
    }
    for (const auto& attribute : ir.GetInputAttributes()) {
        if (IsGenericAttribute(attribute)) {
            entries.attributes.insert(GetGenericAttributeLocation(attribute));
        }
    }
    for (const auto& buffer : entries.const_buffers) {
        entries.enabled_uniform_buffers |= 1U << buffer.GetIndex();
    }
    entries.clip_distances = ir.GetClipDistances();
    entries.shader_length = ir.GetLength();
    entries.uses_warps = ir.UsesWarps();
    return entries;
}

std::vector<u32> Decompile(const Device& device, const VideoCommon::Shader::ShaderIR& ir,
                           ShaderType stage, const VideoCommon::Shader::Registry& registry,
                           const Specialization& specialization) {
    return SPIRVDecompiler(device, ir, stage, registry, specialization).Assemble();
}

} // namespace Vulkan
