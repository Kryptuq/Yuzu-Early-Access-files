// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include "mcl/concepts/same_as.hpp"

namespace mcl {

template<typename T, typename... U>
concept IsAnyOf = (SameAs<T, U> || ...);

}  // namespace mcl
