/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include <cassert>
#include <optional>

#include "sirit/sirit.h"

#include "stream.h"

namespace Sirit {

Id Module::TypeVoid() {
    declarations->Reserve(2);
    return *declarations << OpId{spv::Op::OpTypeVoid} << EndOp{};
}

Id Module::TypeBool() {
    declarations->Reserve(2);
    return *declarations << OpId{spv::Op::OpTypeBool} << EndOp{};
}

Id Module::TypeInt(int width, bool is_signed) {
    declarations->Reserve(4);
    return *declarations << OpId{spv::Op::OpTypeInt} << width << is_signed << EndOp{};
}

Id Module::TypeFloat(int width) {
    declarations->Reserve(3);
    return *declarations << OpId{spv::Op::OpTypeFloat} << width << EndOp{};
}

Id Module::TypeVector(Id component_type, int component_count) {
    assert(component_count >= 2);
    declarations->Reserve(4);
    return *declarations << OpId{spv::Op::OpTypeVector} << component_type << component_count
                         << EndOp{};
}

Id Module::TypeMatrix(Id column_type, int column_count) {
    assert(column_count >= 2);
    declarations->Reserve(4);
    return *declarations << OpId{spv::Op::OpTypeMatrix} << column_type << column_count << EndOp{};
}

Id Module::TypeImage(Id sampled_type, spv::Dim dim, int depth, bool arrayed, bool ms, int sampled,
                     spv::ImageFormat image_format,
                     std::optional<spv::AccessQualifier> access_qualifier) {
    declarations->Reserve(10);
    return *declarations << OpId{spv::Op::OpTypeImage} << sampled_type << dim << depth << arrayed
                         << ms << sampled << image_format << access_qualifier << EndOp{};
}

Id Module::TypeSampler() {
    declarations->Reserve(2);
    return *declarations << OpId{spv::Op::OpTypeSampler} << EndOp{};
}

Id Module::TypeSampledImage(Id image_type) {
    declarations->Reserve(3);
    return *declarations << OpId{spv::Op::OpTypeSampledImage} << image_type << EndOp{};
}

Id Module::TypeArray(Id element_type, Id length) {
    declarations->Reserve(4);
    return *declarations << OpId{spv::Op::OpTypeArray} << element_type << length << EndOp{};
}

Id Module::TypeRuntimeArray(Id element_type) {
    declarations->Reserve(3);
    return *declarations << OpId{spv::Op::OpTypeRuntimeArray} << element_type << EndOp{};
}

Id Module::TypeStruct(std::span<const Id> members) {
    declarations->Reserve(2 + members.size());
    return *declarations << OpId{spv::Op::OpTypeStruct} << members << EndOp{};
}

Id Module::TypeOpaque(std::string_view name) {
    declarations->Reserve(3 + WordsInString(name));
    return *declarations << OpId{spv::Op::OpTypeOpaque} << name << EndOp{};
}

Id Module::TypePointer(spv::StorageClass storage_class, Id type) {
    declarations->Reserve(4);
    return *declarations << OpId{spv::Op::OpTypePointer} << storage_class << type << EndOp{};
}

Id Module::TypeFunction(Id return_type, std::span<const Id> arguments) {
    declarations->Reserve(3 + arguments.size());
    return *declarations << OpId{spv::Op::OpTypeFunction} << return_type << arguments << EndOp{};
}

Id Module::TypeEvent() {
    declarations->Reserve(2);
    return *declarations << OpId{spv::Op::OpTypeEvent} << EndOp{};
}

Id Module::TypeDeviceEvent() {
    declarations->Reserve(2);
    return *declarations << OpId{spv::Op::OpTypeDeviceEvent} << EndOp{};
}

Id Module::TypeReserveId() {
    declarations->Reserve(2);
    return *declarations << OpId{spv::Op::OpTypeReserveId} << EndOp{};
}

Id Module::TypeQueue() {
    declarations->Reserve(2);
    return *declarations << OpId{spv::Op::OpTypeQueue} << EndOp{};
}

Id Module::TypePipe(spv::AccessQualifier access_qualifier) {
    declarations->Reserve(2);
    return *declarations << OpId{spv::Op::OpTypePipe} << access_qualifier << EndOp{};
}

} // namespace Sirit
