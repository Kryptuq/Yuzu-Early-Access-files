/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include <cassert>
#include "operand.h"

namespace Sirit {

Operand::Operand(OperandType operand_type_) : operand_type{operand_type_} {}

Operand::~Operand() = default;

} // namespace Sirit
