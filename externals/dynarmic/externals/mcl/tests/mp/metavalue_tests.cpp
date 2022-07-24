// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#include <type_traits>

#include <mcl/mp/metavalue/bit_and.hpp>
#include <mcl/mp/metavalue/bit_not.hpp>
#include <mcl/mp/metavalue/bit_or.hpp>
#include <mcl/mp/metavalue/bit_xor.hpp>
#include <mcl/mp/metavalue/conjunction.hpp>
#include <mcl/mp/metavalue/disjunction.hpp>
#include <mcl/mp/metavalue/lift_value.hpp>
#include <mcl/mp/metavalue/logic_and.hpp>
#include <mcl/mp/metavalue/logic_not.hpp>
#include <mcl/mp/metavalue/logic_or.hpp>
#include <mcl/mp/metavalue/product.hpp>
#include <mcl/mp/metavalue/sum.hpp>
#include <mcl/mp/metavalue/value.hpp>
#include <mcl/mp/metavalue/value_cast.hpp>
#include <mcl/mp/metavalue/value_equal.hpp>

using namespace mcl::mp;

// bit_and

static_assert(bit_and<lift_value<3>, lift_value<1>>::value == 1);

// bit_not

static_assert(bit_not<lift_value<0>>::value == ~0);

// bit_or

static_assert(bit_or<lift_value<1>, lift_value<3>>::value == 3);

// bit_xor

static_assert(bit_xor<lift_value<1>, lift_value<3>>::value == 2);

// conjunction

static_assert(std::is_same_v<conjunction<std::true_type>, std::true_type>);
static_assert(std::is_same_v<conjunction<std::true_type, lift_value<0>>, lift_value<0>>);
static_assert(std::is_same_v<conjunction<std::true_type, lift_value<42>, std::true_type>, std::true_type>);

// disjunction

static_assert(std::is_same_v<disjunction<std::true_type>, std::true_type>);
static_assert(std::is_same_v<disjunction<std::false_type, lift_value<0>>, lift_value<0>>);
static_assert(std::is_same_v<disjunction<std::false_type, lift_value<42>, std::true_type>, lift_value<42>>);

// lift_value

static_assert(std::is_same_v<lift_value<3>, std::integral_constant<int, 3>>);
static_assert(std::is_same_v<lift_value<false>, std::false_type>);

// logic_and

static_assert(std::is_same_v<logic_and<>, std::true_type>);
static_assert(std::is_same_v<logic_and<std::true_type>, std::true_type>);
static_assert(std::is_same_v<logic_and<lift_value<1>>, std::true_type>);
static_assert(std::is_same_v<logic_and<std::true_type, std::false_type>, std::false_type>);

// logic_not

static_assert(std::is_same_v<logic_not<std::false_type>, std::true_type>);

// logic_or

static_assert(std::is_same_v<logic_or<>, std::false_type>);
static_assert(std::is_same_v<logic_or<std::true_type>, std::true_type>);
static_assert(std::is_same_v<logic_or<lift_value<0>>, std::false_type>);
static_assert(std::is_same_v<logic_or<std::true_type, std::false_type>, std::true_type>);

// product

static_assert(product<lift_value<1>, lift_value<2>, lift_value<3>, lift_value<4>>::value == 24);

// sum

static_assert(sum<lift_value<1>, lift_value<2>, lift_value<3>, lift_value<4>>::value == 10);

// value_cast

static_assert(std::is_same_v<value_cast<int, std::true_type>, std::integral_constant<int, 1>>);

// value_equal

static_assert(std::is_same_v<value_equal<std::true_type, std::integral_constant<int, 1>>, std::true_type>);
