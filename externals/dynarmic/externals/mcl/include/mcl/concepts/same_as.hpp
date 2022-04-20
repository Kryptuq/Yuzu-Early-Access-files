// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

namespace mcl {

namespace detail {

template<typename T, typename U>
concept SameHelper = std::is_same_v<T, U>;

}  // namespace detail

template<typename T, typename U>
concept SameAs = detail::SameHelper<T, U> && detail::SameHelper<U, T>;

}  // namespace mcl
