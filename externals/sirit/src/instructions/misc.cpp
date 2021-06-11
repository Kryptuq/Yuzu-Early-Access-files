/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include "op.h"
#include "sirit/sirit.h"

namespace Sirit {

Id Module::OpUndef(Id result_type) {
    return AddCode(std::make_unique<Op>(spv::Op::OpUndef, bound++, result_type));
}

Id Module::OpEmitVertex() {
    return AddCode(std::make_unique<Op>(spv::Op::OpEmitVertex));
}

Id Module::OpEndPrimitive() {
    return AddCode(std::make_unique<Op>(spv::Op::OpEndPrimitive));
}

} // namespace Sirit
