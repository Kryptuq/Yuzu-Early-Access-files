// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <functional>

// Algorithms that operate on iterators, much like the <algorithm> header.
//
// Note: If the algorithm is not general-purpose and/or doesn't operate on iterators,
//       it should probably not be placed within this header.

namespace Common {

template <class ForwardIt, class T, class Compare = std::less<>>
[[nodiscard]] ForwardIt BinaryFind(ForwardIt first, ForwardIt last, const T& value,
                                   Compare comp = {}) {
    // Note: BOTH type T and the type after ForwardIt is dereferenced
    // must be implicitly convertible to BOTH Type1 and Type2, used in Compare.
    // This is stricter than lower_bound requirement (see above)

    first = std::lower_bound(first, last, value, comp);
    return first != last && !comp(value, *first) ? first : last;
}

template <typename T, typename Func, typename... Args>
T FoldRight(T initial_value, Func&& func, Args&&... args) {
    T value{initial_value};
    const auto high_func = [&value, &func]<typename U>(U x) { value = func(value, x); };
    (std::invoke(high_func, std::forward<Args>(args)), ...);
    return value;
}

} // namespace Common
