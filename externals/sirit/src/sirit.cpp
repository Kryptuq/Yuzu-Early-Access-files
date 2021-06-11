/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include <algorithm>
#include <cassert>
#include "common_types.h"
#include "op.h"
#include "sirit/sirit.h"
#include "stream.h"

namespace Sirit {

template <typename T>
static void WriteSet(Stream& stream, const T& set) {
    for (const auto& item : set) {
        item->Write(stream);
    }
}

Module::Module(u32 version_) : version{version_} {}

Module::~Module() = default;

std::vector<u32> Module::Assemble() const {
    std::vector<u32> bytes;
    Stream stream{bytes};

    stream.Write(spv::MagicNumber);
    stream.Write(version);
    stream.Write(GENERATOR_MAGIC_NUMBER);
    stream.Write(bound);
    stream.Write(static_cast<u32>(0));

    for (const auto capability : capabilities) {
        Op op(spv::Op::OpCapability);
        op.Add(static_cast<u32>(capability));
        op.Write(stream);
    }

    for (const auto& extension_name : extensions) {
        Op op(spv::Op::OpExtension);
        op.Add(extension_name);
        op.Write(stream);
    }

    if (glsl_std_450) {
        glsl_std_450->Write(stream);
    }

    Op memory_model_ref{spv::Op::OpMemoryModel};
    memory_model_ref.Add(static_cast<u32>(addressing_model));
    memory_model_ref.Add(static_cast<u32>(memory_model));
    memory_model_ref.Write(stream);

    WriteSet(stream, entry_points);
    WriteSet(stream, execution_modes);
    WriteSet(stream, debug);
    WriteSet(stream, annotations);
    WriteSet(stream, declarations);
    WriteSet(stream, global_variables);
    WriteSet(stream, code);

    return bytes;
}

void Module::AddExtension(std::string extension_name) {
    extensions.insert(std::move(extension_name));
}

void Module::AddCapability(spv::Capability capability) {
    capabilities.insert(capability);
}

void Module::SetMemoryModel(spv::AddressingModel addressing_model_, spv::MemoryModel memory_model_) {
    this->addressing_model = addressing_model_;
    this->memory_model = memory_model_;
}

void Module::AddEntryPoint(spv::ExecutionModel execution_model, Id entry_point,
                           std::string name, const std::vector<Id>& interfaces) {
    auto op{std::make_unique<Op>(spv::Op::OpEntryPoint)};
    op->Add(static_cast<u32>(execution_model));
    op->Add(entry_point);
    op->Add(std::move(name));
    op->Add(interfaces);
    entry_points.push_back(std::move(op));
}

void Module::AddExecutionMode(Id entry_point, spv::ExecutionMode mode,
                              const std::vector<Literal>& literals) {
    auto op{std::make_unique<Op>(spv::Op::OpExecutionMode)};
    op->Add(entry_point);
    op->Add(static_cast<u32>(mode));
    op->Add(literals);
    execution_modes.push_back(std::move(op));
}

Id Module::AddLabel(Id label) {
    assert(label != nullptr);
    return code.emplace_back(label);
}

Id Module::AddLocalVariable(Id variable) {
    assert(variable != nullptr);
    return code.emplace_back(variable);
}

Id Module::AddGlobalVariable(Id variable) {
    assert(variable);
    return global_variables.emplace_back(variable);
}

Id Module::AddCode(std::unique_ptr<Op> op) {
    const Id id = code_store.emplace_back(std::move(op)).get();
    return code.emplace_back(id);
}

Id Module::AddCode(spv::Op opcode, std::optional<u32> id) {
    return AddCode(std::make_unique<Op>(opcode, id));
}

Id Module::AddDeclaration(std::unique_ptr<Op> op) {
    const auto& found{std::find_if(declarations.begin(), declarations.end(),
                                   [&op](const auto& other) { return *other == *op; })};
    if (found != declarations.end()) {
        return found->get();
    }
    const auto id = op.get();
    declarations.push_back(std::move(op));
    bound++;
    return id;
}

void Module::AddAnnotation(std::unique_ptr<Op> op) {
    annotations.push_back(std::move(op));
}

Id Module::GetGLSLstd450() {
    if (!glsl_std_450) {
        glsl_std_450 = std::make_unique<Op>(spv::Op::OpExtInstImport, bound++);
        glsl_std_450->Add("GLSL.std.450");
    }
    return glsl_std_450.get();
}

} // namespace Sirit
