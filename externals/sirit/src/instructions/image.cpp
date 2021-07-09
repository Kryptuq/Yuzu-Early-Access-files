/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include <cassert>

#include "sirit/sirit.h"

#include "stream.h"

namespace Sirit {

#define DEFINE_IMAGE_OP(opcode)                                                                    \
    Id Module::opcode(Id result_type, Id sampled_image, Id coordinate,                             \
                      std::optional<spv::ImageOperandsMask> image_operands,                        \
                      std::span<const Id> operands) {                                              \
        code->Reserve(6 + operands.size());                                                        \
        return *code << OpId{spv::Op::opcode, result_type} << sampled_image << coordinate          \
                     << image_operands << operands << EndOp{};                                     \
    }

#define DEFINE_IMAGE_EXP_OP(opcode)                                                                \
    Id Module::opcode(Id result_type, Id sampled_image, Id coordinate,                             \
                      spv::ImageOperandsMask image_operands, std::span<const Id> operands) {       \
        code->Reserve(6 + operands.size());                                                        \
        return *code << OpId{spv::Op::opcode, result_type} << sampled_image << coordinate          \
                     << image_operands << operands << EndOp{};                                     \
    }

#define DEFINE_IMAGE_EXTRA_OP(opcode)                                                              \
    Id Module::opcode(Id result_type, Id sampled_image, Id coordinate, Id extra,                   \
                      std::optional<spv::ImageOperandsMask> image_operands,                        \
                      std::span<const Id> operands) {                                              \
        code->Reserve(7 + operands.size());                                                        \
        return *code << OpId{spv::Op::opcode, result_type} << sampled_image << coordinate << extra \
                     << image_operands << operands << EndOp{};                                     \
    }

#define DEFINE_IMAGE_EXTRA_EXP_OP(opcode)                                                          \
    Id Module::opcode(Id result_type, Id sampled_image, Id coordinate, Id extra,                   \
                      spv::ImageOperandsMask image_operands, std::span<const Id> operands) {       \
        code->Reserve(8 + operands.size());                                                        \
        return *code << OpId{spv::Op::opcode, result_type} << sampled_image << coordinate << extra \
                     << image_operands << operands << EndOp{};                                     \
    }

#define DEFINE_IMAGE_QUERY_OP(opcode)                                                              \
    Id Module::opcode(Id result_type, Id image) {                                                  \
        code->Reserve(5);                                                                          \
        return *code << OpId{spv::Op::opcode, result_type} << image << EndOp{};                    \
    }

#define DEFINE_IMAGE_QUERY_BIN_OP(opcode)                                                          \
    Id Module::opcode(Id result_type, Id image, Id extra) {                                        \
        code->Reserve(5);                                                                          \
        return *code << OpId{spv::Op::opcode, result_type} << image << extra << EndOp{};           \
    }

DEFINE_IMAGE_OP(OpImageSampleImplicitLod)
DEFINE_IMAGE_EXP_OP(OpImageSampleExplicitLod)
DEFINE_IMAGE_EXTRA_OP(OpImageSampleDrefImplicitLod)
DEFINE_IMAGE_EXTRA_EXP_OP(OpImageSampleDrefExplicitLod)
DEFINE_IMAGE_OP(OpImageSampleProjImplicitLod)
DEFINE_IMAGE_EXP_OP(OpImageSampleProjExplicitLod)
DEFINE_IMAGE_EXTRA_OP(OpImageSampleProjDrefImplicitLod)
DEFINE_IMAGE_EXTRA_EXP_OP(OpImageSampleProjDrefExplicitLod)
DEFINE_IMAGE_OP(OpImageFetch)
DEFINE_IMAGE_EXTRA_OP(OpImageGather)
DEFINE_IMAGE_EXTRA_OP(OpImageDrefGather)
DEFINE_IMAGE_OP(OpImageRead)
DEFINE_IMAGE_QUERY_BIN_OP(OpImageQuerySizeLod)
DEFINE_IMAGE_QUERY_OP(OpImageQuerySize)
DEFINE_IMAGE_QUERY_BIN_OP(OpImageQueryLod)
DEFINE_IMAGE_QUERY_OP(OpImageQueryLevels)
DEFINE_IMAGE_QUERY_OP(OpImageQuerySamples)

Id Module::OpSampledImage(Id result_type, Id image, Id sampler) {
    code->Reserve(5);
    return *code << OpId{spv::Op::OpSampledImage, result_type} << image << sampler << EndOp{};
}

Id Module::OpImageWrite(Id image, Id coordinate, Id texel,
                        std::optional<spv::ImageOperandsMask> image_operands,
                        std::span<const Id> operands) {
    assert(image_operands.has_value() != operands.empty());
    code->Reserve(5 + operands.size());
    return *code << spv::Op::OpImageWrite << image << coordinate << texel << image_operands
                 << operands << EndOp{};
}

Id Module::OpImage(Id result_type, Id sampled_image) {
    code->Reserve(4);
    return *code << OpId{spv::Op::OpImage, result_type} << sampled_image << EndOp{};
}

Id Module::OpImageSparseSampleImplicitLod(Id result_type, Id sampled_image, Id coordinate,
                                          std::optional<spv::ImageOperandsMask> image_operands,
                                          std::span<const Id> operands) {
    code->Reserve(5 + (image_operands.has_value() ? 1 : 0) + operands.size());
    return *code << OpId{spv::Op::OpImageSparseSampleImplicitLod, result_type} << sampled_image
                 << coordinate << image_operands << operands << EndOp{};
}

Id Module::OpImageSparseSampleExplicitLod(Id result_type, Id sampled_image, Id coordinate,
                                          spv::ImageOperandsMask image_operands,
                                          std::span<const Id> operands) {
    code->Reserve(6 + operands.size());
    return *code << OpId{spv::Op::OpImageSparseSampleExplicitLod, result_type} << sampled_image
                 << coordinate << image_operands << operands << EndOp{};
}

Id Module::OpImageSparseSampleDrefImplicitLod(Id result_type, Id sampled_image, Id coordinate,
                                              Id dref,
                                              std::optional<spv::ImageOperandsMask> image_operands,
                                              std::span<const Id> operands) {
    code->Reserve(6 + (image_operands.has_value() ? 1 : 0) + operands.size());
    return *code << OpId{spv::Op::OpImageSparseSampleDrefImplicitLod, result_type} << sampled_image
                 << coordinate << dref << image_operands << operands << EndOp{};
}

Id Module::OpImageSparseSampleDrefExplicitLod(Id result_type, Id sampled_image, Id coordinate,
                                              Id dref, spv::ImageOperandsMask image_operands,
                                              std::span<const Id> operands) {
    code->Reserve(7 + operands.size());
    return *code << OpId{spv::Op::OpImageSparseSampleDrefExplicitLod, result_type} << sampled_image
                 << coordinate << dref << image_operands << operands << EndOp{};
}

Id Module::OpImageSparseFetch(Id result_type, Id image, Id coordinate,
                              std::optional<spv::ImageOperandsMask> image_operands,
                              std::span<const Id> operands) {
    code->Reserve(5 + (image_operands.has_value() ? 1 : 0) + operands.size());
    return *code << OpId{spv::Op::OpImageSparseFetch, result_type} << image << coordinate
                 << image_operands << operands << EndOp{};
}

Id Module::OpImageSparseGather(Id result_type, Id sampled_image, Id coordinate, Id component,
                               std::optional<spv::ImageOperandsMask> image_operands,
                               std::span<const Id> operands) {
    code->Reserve(6 + operands.size());
    return *code << OpId{spv::Op::OpImageSparseGather, result_type} << sampled_image << coordinate
                 << component << image_operands << operands << EndOp{};
}

Id Module::OpImageSparseDrefGather(Id result_type, Id sampled_image, Id coordinate, Id dref,
                                   std::optional<spv::ImageOperandsMask> image_operands,
                                   std::span<const Id> operands) {
    code->Reserve(6 + operands.size());
    return *code << OpId{spv::Op::OpImageSparseDrefGather, result_type} << sampled_image
                 << coordinate << dref << image_operands << operands << EndOp{};
}

Id Module::OpImageSparseTexelsResident(Id result_type, Id resident_code) {
    code->Reserve(4);
    return *code << OpId{spv::Op::OpImageSparseTexelsResident, result_type} << resident_code
                 << EndOp{};
}

Id Module::OpImageSparseRead(Id result_type, Id image, Id coordinate,
                             std::optional<spv::ImageOperandsMask> image_operands,
                             std::span<const Id> operands) {
    code->Reserve(5 + (image_operands.has_value() ? 1 : 0) + operands.size());
    return *code << OpId{spv::Op::OpImageSparseTexelsResident, result_type} << image << coordinate
                 << image_operands << operands << EndOp{};
}

} // namespace Sirit
