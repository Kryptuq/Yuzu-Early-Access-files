// SPDX-FileCopyrightText: 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

//
// TODO: remove this file when ranges are supported by all compilation targets
//

#pragma once

#include <algorithm>
#include <utility>
#include <version>

#ifndef __cpp_lib_ranges

namespace std {
namespace ranges {

template <typename T>
concept range = requires(T& t) {
    begin(t);
    end(t);
};

template <typename T>
concept input_range = range<T>;

template <typename T>
concept output_range = range<T>;

template <range R>
using range_difference_t = ptrdiff_t;

//
// find, find_if, find_if_not
//

struct find_fn {
    template <typename Iterator, typename T, typename Proj = std::identity>
    constexpr Iterator operator()(Iterator first, Iterator last, const T& value,
                                  Proj proj = {}) const {
        for (; first != last; ++first) {
            if (std::invoke(proj, *first) == value) {
                return first;
            }
        }
        return first;
    }

    template <ranges::input_range R, typename T, typename Proj = std::identity>
    constexpr ranges::iterator_t<R> operator()(R&& r, const T& value, Proj proj = {}) const {
        return operator()(ranges::begin(r), ranges::end(r), value, std::ref(proj));
    }
};

struct find_if_fn {
    template <typename Iterator, typename Proj = std::identity, typename Pred>
    constexpr Iterator operator()(Iterator first, Iterator last, Pred pred, Proj proj = {}) const {
        for (; first != last; ++first) {
            if (std::invoke(pred, std::invoke(proj, *first))) {
                return first;
            }
        }
        return first;
    }

    template <ranges::input_range R, typename Proj = std::identity, typename Pred>
    constexpr ranges::iterator_t<R> operator()(R&& r, Pred pred, Proj proj = {}) const {
        return operator()(ranges::begin(r), ranges::end(r), std::ref(pred), std::ref(proj));
    }
};

struct find_if_not_fn {
    template <typename Iterator, typename Proj = std::identity, typename Pred>
    constexpr Iterator operator()(Iterator first, Iterator last, Pred pred, Proj proj = {}) const {
        for (; first != last; ++first) {
            if (!std::invoke(pred, std::invoke(proj, *first))) {
                return first;
            }
        }
        return first;
    }

    template <ranges::input_range R, typename Proj = std::identity, typename Pred>
    constexpr ranges::iterator_t<R> operator()(R&& r, Pred pred, Proj proj = {}) const {
        return operator()(ranges::begin(r), ranges::end(r), std::ref(pred), std::ref(proj));
    }
};

inline constexpr find_fn find;
inline constexpr find_if_fn find_if;
inline constexpr find_if_not_fn find_if_not;

//
// any_of, all_of, none_of
//

struct all_of_fn {
    template <typename Iterator, typename Proj = std::identity, typename Pred>
    constexpr bool operator()(Iterator first, Iterator last, Pred pred, Proj proj = {}) const {
        return ranges::find_if_not(first, last, std::ref(pred), std::ref(proj)) == last;
    }

    template <ranges::input_range R, typename Proj = std::identity, typename Pred>
    constexpr bool operator()(R&& r, Pred pred, Proj proj = {}) const {
        return operator()(ranges::begin(r), ranges::end(r), std::ref(pred), std::ref(proj));
    }
};

struct any_of_fn {
    template <typename Iterator, typename Proj = std::identity, typename Pred>
    constexpr bool operator()(Iterator first, Iterator last, Pred pred, Proj proj = {}) const {
        return ranges::find_if(first, last, std::ref(pred), std::ref(proj)) != last;
    }

    template <ranges::input_range R, typename Proj = std::identity, typename Pred>
    constexpr bool operator()(R&& r, Pred pred, Proj proj = {}) const {
        return operator()(ranges::begin(r), ranges::end(r), std::ref(pred), std::ref(proj));
    }
};

struct none_of_fn {
    template <typename Iterator, typename Proj = std::identity, typename Pred>
    constexpr bool operator()(Iterator first, Iterator last, Pred pred, Proj proj = {}) const {
        return ranges::find_if(first, last, std::ref(pred), std::ref(proj)) == last;
    }

    template <ranges::input_range R, typename Proj = std::identity, typename Pred>
    constexpr bool operator()(R&& r, Pred pred, Proj proj = {}) const {
        return operator()(ranges::begin(r), ranges::end(r), std::ref(pred), std::ref(proj));
    }
};

inline constexpr any_of_fn any_of;
inline constexpr all_of_fn all_of;
inline constexpr none_of_fn none_of;

//
// count, count_if
//

struct count_fn {
    template <typename Iterator, typename T, typename Proj = std::identity>
    constexpr ptrdiff_t operator()(Iterator first, Iterator last, const T& value,
                                   Proj proj = {}) const {
        ptrdiff_t counter = 0;
        for (; first != last; ++first)
            if (std::invoke(proj, *first) == value)
                ++counter;
        return counter;
    }

    template <ranges::input_range R, typename T, typename Proj = std::identity>
    constexpr ptrdiff_t operator()(R&& r, const T& value, Proj proj = {}) const {
        return operator()(ranges::begin(r), ranges::end(r), value, std::ref(proj));
    }
};

struct count_if_fn {
    template <typename Iterator, typename Proj = std::identity, typename Pred>
    constexpr ptrdiff_t operator()(Iterator first, Iterator last, Pred pred, Proj proj = {}) const {
        ptrdiff_t counter = 0;
        for (; first != last; ++first)
            if (std::invoke(pred, std::invoke(proj, *first)))
                ++counter;
        return counter;
    }

    template <ranges::input_range R, typename Proj = std::identity, typename Pred>
    constexpr ptrdiff_t operator()(R&& r, Pred pred, Proj proj = {}) const {
        return operator()(ranges::begin(r), ranges::end(r), std::ref(pred), std::ref(proj));
    }
};

inline constexpr count_fn count;
inline constexpr count_if_fn count_if;

//
// transform
//

struct transform_fn {
    template <typename InputIterator, typename OutputIterator, typename F,
              typename Proj = std::identity>
    constexpr void operator()(InputIterator first1, InputIterator last1, OutputIterator result,
                              F op, Proj proj = {}) const {
        for (; first1 != last1; ++first1, (void)++result) {
            *result = std::invoke(op, std::invoke(proj, *first1));
        }
    }

    template <ranges::input_range R, typename OutputIterator, typename F,
              typename Proj = std::identity>
    constexpr void operator()(R&& r, OutputIterator result, F op, Proj proj = {}) const {
        return operator()(ranges::begin(r), ranges::end(r), result, std::ref(op), std::ref(proj));
    }
};

inline constexpr transform_fn transform;

//
// sort
//

struct sort_fn {
    template <typename Iterator, typename Comp = ranges::less, typename Proj = std::identity>
    constexpr void operator()(Iterator first, Iterator last, Comp comp = {}, Proj proj = {}) const {
        if (first == last)
            return;

        Iterator last_iter = ranges::next(first, last);
        std::sort(first, last_iter,
                  [&](auto& lhs, auto& rhs) { return comp(proj(lhs), proj(rhs)); });
    }

    template <ranges::input_range R, typename Comp = ranges::less, typename Proj = std::identity>
    constexpr void operator()(R&& r, Comp comp = {}, Proj proj = {}) const {
        return operator()(ranges::begin(r), ranges::end(r), std::move(comp), std::move(proj));
    }
};

inline constexpr sort_fn sort;

//
// fill
//

struct fill_fn {
    template <typename T, typename OutputIterator>
    constexpr OutputIterator operator()(OutputIterator first, OutputIterator last,
                                        const T& value) const {
        while (first != last) {
            *first++ = value;
        }

        return first;
    }

    template <typename T, ranges::output_range R>
    constexpr ranges::iterator_t<R> operator()(R&& r, const T& value) const {
        return operator()(ranges::begin(r), ranges::end(r), value);
    }
};

inline constexpr fill_fn fill;

//
// for_each
//

struct for_each_fn {
    template <typename Iterator, typename Proj = std::identity, typename Fun>
    constexpr void operator()(Iterator first, Iterator last, Fun f, Proj proj = {}) const {
        for (; first != last; ++first) {
            std::invoke(f, std::invoke(proj, *first));
        }
    }

    template <ranges::input_range R, typename Proj = std::identity, typename Fun>
    constexpr void operator()(R&& r, Fun f, Proj proj = {}) const {
        return operator()(ranges::begin(r), ranges::end(r), std::move(f), std::ref(proj));
    }
};

inline constexpr for_each_fn for_each;

//
// min_element, max_element
//

struct min_element_fn {
    template <typename Iterator, typename Proj = std::identity, typename Comp = ranges::less>
    constexpr Iterator operator()(Iterator first, Iterator last, Comp comp = {},
                                  Proj proj = {}) const {
        if (first == last) {
            return last;
        }

        auto smallest = first;
        ++first;
        for (; first != last; ++first) {
            if (!std::invoke(comp, std::invoke(proj, *smallest), std::invoke(proj, *first))) {
                smallest = first;
            }
        }
        return smallest;
    }

    template <ranges::input_range R, typename Proj = std::identity, typename Comp = ranges::less>
    constexpr ranges::iterator_t<R> operator()(R&& r, Comp comp = {}, Proj proj = {}) const {
        return operator()(ranges::begin(r), ranges::end(r), std::ref(comp), std::ref(proj));
    }
};

struct max_element_fn {
    template <typename Iterator, typename Proj = std::identity, typename Comp = ranges::less>
    constexpr Iterator operator()(Iterator first, Iterator last, Comp comp = {},
                                  Proj proj = {}) const {
        if (first == last) {
            return last;
        }

        auto largest = first;
        ++first;
        for (; first != last; ++first) {
            if (std::invoke(comp, std::invoke(proj, *largest), std::invoke(proj, *first))) {
                largest = first;
            }
        }
        return largest;
    }

    template <ranges::input_range R, typename Proj = std::identity, typename Comp = ranges::less>
    constexpr ranges::iterator_t<R> operator()(R&& r, Comp comp = {}, Proj proj = {}) const {
        return operator()(ranges::begin(r), ranges::end(r), std::ref(comp), std::ref(proj));
    }
};

inline constexpr min_element_fn min_element;
inline constexpr max_element_fn max_element;

//
// replace, replace_if
//

struct replace_fn {
    template <typename Iterator, typename T1, typename T2, typename Proj = std::identity>
    constexpr Iterator operator()(Iterator first, Iterator last, const T1& old_value,
                                  const T2& new_value, Proj proj = {}) const {
        for (; first != last; ++first) {
            if (old_value == std::invoke(proj, *first)) {
                *first = new_value;
            }
        }
        return first;
    }

    template <ranges::input_range R, typename T1, typename T2, typename Proj = std::identity>
    constexpr ranges::iterator_t<R> operator()(R&& r, const T1& old_value, const T2& new_value,
                                               Proj proj = {}) const {
        return operator()(ranges::begin(r), ranges::end(r), old_value, new_value, std::move(proj));
    }
};

struct replace_if_fn {
    template <typename Iterator, typename T, typename Proj = std::identity, typename Pred>
    constexpr Iterator operator()(Iterator first, Iterator last, Pred pred, const T& new_value,
                                  Proj proj = {}) const {
        for (; first != last; ++first) {
            if (!!std::invoke(pred, std::invoke(proj, *first))) {
                *first = new_value;
            }
        }
        return std::move(first);
    }

    template <ranges::input_range R, typename T, typename Proj = std::identity, typename Pred>
    constexpr ranges::iterator_t<R> operator()(R&& r, Pred pred, const T& new_value,
                                               Proj proj = {}) const {
        return operator()(ranges::begin(r), ranges::end(r), std::move(pred), new_value,
                          std::move(proj));
    }
};

inline constexpr replace_fn replace;
inline constexpr replace_if_fn replace_if;

//
// copy, copy_if
//

struct copy_fn {
    template <typename InputIterator, typename OutputIterator>
    constexpr void operator()(InputIterator first, InputIterator last,
                              OutputIterator result) const {
        for (; first != last; ++first, (void)++result) {
            *result = *first;
        }
    }

    template <ranges::input_range R, typename OutputIterator>
    constexpr void operator()(R&& r, OutputIterator result) const {
        return operator()(ranges::begin(r), ranges::end(r), std::move(result));
    }
};

struct copy_if_fn {
    template <typename InputIterator, typename OutputIterator, typename Proj = std::identity,
              typename Pred>
    constexpr void operator()(InputIterator first, InputIterator last, OutputIterator result,
                              Pred pred, Proj proj = {}) const {
        for (; first != last; ++first) {
            if (std::invoke(pred, std::invoke(proj, *first))) {
                *result = *first;
                ++result;
            }
        }
    }

    template <ranges::input_range R, typename OutputIterator, typename Proj = std::identity,
              typename Pred>
    constexpr void operator()(R&& r, OutputIterator result, Pred pred, Proj proj = {}) const {
        return operator()(ranges::begin(r), ranges::end(r), std::move(result), std::ref(pred),
                          std::ref(proj));
    }
};

inline constexpr copy_fn copy;
inline constexpr copy_if_fn copy_if;

//
// generate
//

struct generate_fn {
    template <typename Iterator, typename F>
    constexpr Iterator operator()(Iterator first, Iterator last, F gen) const {
        for (; first != last; *first = std::invoke(gen), ++first)
            ;
        return first;
    }

    template <typename R, std::copy_constructible F>
    requires std::invocable<F&> && ranges::output_range<R>
    constexpr ranges::iterator_t<R> operator()(R&& r, F gen) const {
        return operator()(ranges::begin(r), ranges::end(r), std::move(gen));
    }
};

inline constexpr generate_fn generate;

//
// lower_bound, upper_bound
//

struct lower_bound_fn {
    template <typename Iterator, typename T, typename Proj = std::identity,
              typename Comp = ranges::less>
    constexpr Iterator operator()(Iterator first, Iterator last, const T& value, Comp comp = {},
                                  Proj proj = {}) const {
        Iterator it;
        std::ptrdiff_t _count, _step;
        _count = std::distance(first, last);

        while (_count > 0) {
            it = first;
            _step = _count / 2;
            ranges::advance(it, _step, last);
            if (comp(std::invoke(proj, *it), value)) {
                first = ++it;
                _count -= _step + 1;
            } else {
                _count = _step;
            }
        }
        return first;
    }

    template <ranges::input_range R, typename T, typename Proj = std::identity,
              typename Comp = ranges::less>
    constexpr ranges::iterator_t<R> operator()(R&& r, const T& value, Comp comp = {},
                                               Proj proj = {}) const {
        return operator()(ranges::begin(r), ranges::end(r), value, std::ref(comp), std::ref(proj));
    }
};

struct upper_bound_fn {
    template <typename Iterator, typename T, typename Proj = std::identity,
              typename Comp = ranges::less>
    constexpr Iterator operator()(Iterator first, Iterator last, const T& value, Comp comp = {},
                                  Proj proj = {}) const {
        Iterator it;
        std::ptrdiff_t _count, _step;
        _count = std::distance(first, last);

        while (_count > 0) {
            it = first;
            _step = _count / 2;
            ranges::advance(it, _step, last);
            if (!comp(value, std::invoke(proj, *it))) {
                first = ++it;
                _count -= _step + 1;
            } else {
                _count = _step;
            }
        }
        return first;
    }

    template <ranges::input_range R, typename T, typename Proj = std::identity,
              typename Comp = ranges::less>
    constexpr ranges::iterator_t<R> operator()(R&& r, const T& value, Comp comp = {},
                                               Proj proj = {}) const {
        return operator()(ranges::begin(r), ranges::end(r), value, std::ref(comp), std::ref(proj));
    }
};

inline constexpr lower_bound_fn lower_bound;
inline constexpr upper_bound_fn upper_bound;

//
// adjacent_find
//

struct adjacent_find_fn {
    template <typename Iterator, typename Proj = std::identity, typename Pred = ranges::equal_to>
    constexpr Iterator operator()(Iterator first, Iterator last, Pred pred = {},
                                  Proj proj = {}) const {
        if (first == last)
            return first;
        auto _next = ranges::next(first);
        for (; _next != last; ++_next, ++first)
            if (std::invoke(pred, std::invoke(proj, *first), std::invoke(proj, *_next)))
                return first;
        return _next;
    }

    template <ranges::input_range R, typename Proj = std::identity,
              typename Pred = ranges::equal_to>
    constexpr ranges::iterator_t<R> operator()(R&& r, Pred pred = {}, Proj proj = {}) const {
        return operator()(ranges::begin(r), ranges::end(r), std::ref(pred), std::ref(proj));
    }
};

inline constexpr adjacent_find_fn adjacent_find;

} // namespace ranges
} // namespace std

#endif
