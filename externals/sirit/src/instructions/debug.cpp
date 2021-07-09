/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include "sirit/sirit.h"

#include "common_types.h"
#include "stream.h"

namespace Sirit {

Id Module::Name(Id target, std::string_view name) {
    debug->Reserve(3 + WordsInString(name));
    *debug << spv::Op::OpName << target << name << EndOp{};
    return target;
}

Id Module::MemberName(Id type, u32 member, std::string_view name) {
    debug->Reserve(4 + WordsInString(name));
    *debug << spv::Op::OpMemberName << type << member << name << EndOp{};
    return type;
}

Id Module::String(std::string_view string) {
    debug->Reserve(3 + WordsInString(string));
    return *debug << OpId{spv::Op::OpString} << string << EndOp{};
}

Id Module::OpLine(Id file, Literal line, Literal column) {
    debug->Reserve(4);
    return *debug << spv::Op::OpLine << file << line << column << EndOp{};
}

} // namespace Sirit
