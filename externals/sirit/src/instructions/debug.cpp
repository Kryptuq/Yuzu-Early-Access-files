/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include <memory>
#include <string>
#include "op.h"
#include "sirit/sirit.h"

namespace Sirit {

Id Module::Name(Id target, std::string name) {
    auto op{std::make_unique<Op>(spv::Op::OpName)};
    op->Add(target);
    op->Add(std::move(name));
    debug.push_back(std::move(op));
    return target;
}

Id Module::MemberName(Id type, u32 member, std::string name) {
    auto op{std::make_unique<Op>(spv::Op::OpMemberName)};
    op->Add(type);
    op->Add(member);
    op->Add(std::move(name));
    debug.push_back(std::move(op));
    return type;
}

Id Module::String(std::string string) {
    auto op{std::make_unique<Op>(spv::Op::OpString, bound++)};
    op->Add(std::move(string));
    const auto id = op.get();
    debug.push_back(std::move(op));
    return id;
}

Id Module::OpLine(Id file, Literal line, Literal column) {
    auto op{std::make_unique<Op>(spv::Op::OpLine)};
    op->Add(file);
    op->Add(line);
    op->Add(column);
    return AddCode(std::move(op));
}

} // namespace Sirit
