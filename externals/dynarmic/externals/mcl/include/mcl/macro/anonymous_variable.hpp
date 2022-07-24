// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include <mcl/macro/concatenate_tokens.hpp>

#ifdef __COUNTER__
#    define ANONYMOUS_VARIABLE(str) CONCATENATE_TOKENS(str, __COUNTER__)
#else
#    define ANONYMOUS_VARIABLE(str) CONCATENATE_TOKENS(str, __LINE__)
#endif
