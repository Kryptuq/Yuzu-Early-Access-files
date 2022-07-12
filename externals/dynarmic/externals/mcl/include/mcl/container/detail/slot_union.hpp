// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

namespace mcl::detail {

template<typename ValueType>
union slot_union {
    slot_union() {}
    ~slot_union() {}
    ValueType value;
};

}  // namespace mcl::detail
