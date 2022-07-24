// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

#include <mcl/mp/metavalue/value.hpp>
#include <mcl/mp/typelist/append.hpp>
#include <mcl/mp/typelist/cartesian_product.hpp>
#include <mcl/mp/typelist/concat.hpp>
#include <mcl/mp/typelist/contains.hpp>
#include <mcl/mp/typelist/drop.hpp>
#include <mcl/mp/typelist/get.hpp>
#include <mcl/mp/typelist/head.hpp>
#include <mcl/mp/typelist/length.hpp>
#include <mcl/mp/typelist/lift_sequence.hpp>
#include <mcl/mp/typelist/list.hpp>
#include <mcl/mp/typelist/lower_to_tuple.hpp>
#include <mcl/mp/typelist/prepend.hpp>
#include <mcl/mp/typelist/tail.hpp>

using namespace mcl::mp;

// append

static_assert(std::is_same_v<append<list<int, bool>, double>, list<int, bool, double>>);
static_assert(std::is_same_v<append<list<>, int, int>, list<int, int>>);

// cartesian_product

static_assert(
    std::is_same_v<
        cartesian_product<list<int, bool>, list<double, float>, list<char, unsigned>>,
        list<
            list<int, double, char>,
            list<int, double, unsigned>,
            list<int, float, char>,
            list<int, float, unsigned>,
            list<bool, double, char>,
            list<bool, double, unsigned>,
            list<bool, float, char>,
            list<bool, float, unsigned>>>);

// concat

static_assert(std::is_same_v<concat<list<int, bool>, list<double>>, list<int, bool, double>>);
static_assert(std::is_same_v<concat<list<>, list<int>, list<int>>, list<int, int>>);

// contains

static_assert(contains_v<list<int>, int>);
static_assert(!contains_v<list<>, int>);
static_assert(!contains_v<list<double>, int>);
static_assert(contains_v<list<double, int>, int>);

// drop

static_assert(std::is_same_v<list<>, drop<3, list<int, int>>>);
static_assert(std::is_same_v<list<>, drop<3, list<int, int, int>>>);
static_assert(std::is_same_v<list<int>, drop<3, list<int, int, int, int>>>);
static_assert(std::is_same_v<list<double>, drop<3, list<int, int, int, double>>>);
static_assert(std::is_same_v<list<int, double, bool>, drop<0, list<int, double, bool>>>);

// get

static_assert(std::is_same_v<get<0, list<int, double>>, int>);
static_assert(std::is_same_v<get<1, list<int, double>>, double>);

// head

static_assert(std::is_same_v<head<list<int, double>>, int>);
static_assert(std::is_same_v<head<list<int>>, int>);

// length

static_assert(length_v<list<>> == 0);
static_assert(length_v<list<int>> == 1);
static_assert(length_v<list<int, int, int>> == 3);

// lift_sequence

static_assert(
    std::is_same_v<
        lift_sequence<std::make_index_sequence<3>>,
        list<size_value<0>, size_value<1>, size_value<2>>>);

// lower_to_tuple

static_assert(lower_to_tuple_v<list<size_value<0>, size_value<1>, size_value<2>>> == std::tuple<std::size_t, std::size_t, std::size_t>(0, 1, 2));
static_assert(lower_to_tuple_v<list<std::true_type, std::false_type>> == std::make_tuple(true, false));

// prepend

static_assert(std::is_same_v<prepend<list<int, int>, double>, list<double, int, int>>);
static_assert(std::is_same_v<prepend<list<>, double>, list<double>>);
static_assert(std::is_same_v<prepend<list<int>, double, bool>, list<double, bool, int>>);

// tail

static_assert(std::is_same_v<tail<list<int, double>>, list<double>>);
static_assert(std::is_same_v<tail<list<int>>, list<>>);
