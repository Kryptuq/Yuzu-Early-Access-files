mp
===

A small, 0BSD-licensed metaprogramming library for C++17.

This is intended to be a lightweight and easy to understand implementation of a subset of useful metaprogramming utilities.

Usage
-----

Just add the `include` directory to your include path. That's it.

`typelist`
----------

A `mp::list` is a list of types. This set of headers provide metafunctions for manipulating lists of types.

### Constructor

* `mp::list`: Constructs a list.
* `mp::lift_sequence`: Lifts a value sequence into a list. Intended for use on `std::integral_sequence`.

### Element access

* `mp::get`: Gets a numbered element of a list.
* `mp::head`: Gets the first element of a list.
* `mp::tail`: Gets all-but-the-first-element as a list.

### Properties

* `mp::length`: Gets the length of a list.
* `mp::contains`: Determines if this list contains a specified element.

### Modifiers

* `mp::append`: Constructs a list with the provided elements appended to it.
* `mp::prepend`: Constructs a list with the provided elements prepended to it.

### Operations

* `mp::concat`: Concantenates multiple lists together.
* `mp::cartesian_product`: Construct a list containing the [cartesian product](https://en.wikipedia.org/wiki/Cartesian_product) of the provided lists.

### Conversions

* `mp::lower_to_tuple`: This operation only works on a list solely containing metavalues. Results in a `std::tuple` with equivalent values.


`metavalue`
-----------

A metavalue is a type of template `std::integral_constant`.

### Constants

* mp::true_type: Aliases to [`std::true_type`](https://en.cppreference.com/w/cpp/types/integral_constant)
* mp::false_type: Aliases to [`mp::false_type`](https://en.cppreference.com/w/cpp/types/integral_constant)

### Constructor

* mp::value: Aliases to [`std::integral_constant`](https://en.cppreference.com/w/cpp/types/integral_constant)
* mp::bool_value: Aliases to [`std::bool_constant`](https://en.cppreference.com/w/cpp/types/integral_constant)
* mp::size_value: Constructs a metavalue with value of type std::size_t
* `mp::lift_value`: Lifts a value of any arbitrary type to become a metavalue

### Conversions

* `mp::value_cast`

### Operations

* `mp::value_equal`: Compares value equality, ignores type. Use `std::is_same` for strict comparison.
* `mp::logic_if`: Like std::conditional but has a bool metavalue as first argument.
* `mp::bit_not`: Bitwise not
* `mp::bit_and`: Bitwise and
* `mp::bit_or`: Bitwise or
* `mp::bit_xor`: Bitwise xor
* `mp::logic_not`: Logical not
* `mp::logic_and`: Logical conjunction (no short circuiting, always results in a mp:bool_value)
* `mp::logic_or`: Logical disjunction (no short circuiting, always results in a mp:bool_value)
* `mp::conjunction`: Logical conjunction (with short circuiting, preserves type)
* `mp::disjunction`: Logical disjunction (with short circuiting, preserves type)
* `mp::sum`: Sum of values
* `mp::product`: Product of values

`metafunction`
--------------

* `std::void_t`: Always returns `void`.
* `mp::identity`: Identity metafunction. Can be used to establish a non-deduced context. See also C++20 `std::type_identity`.
* `mp::apply`: Invoke a provided metafunction with arguments specified in a list.
* `mp::map`: Apply a provided metafunction to each element of a list.
* `mp::bind`: Curry a metafunction. A macro `MM_MP_BIND` is provided to make this a little prettier.

`traits`
--------

Type traits not in the standard library.

### `function_info`

* `mp::parameter_count_v`: Number of parameters a function has
* `mp::parameter_list`: Get a typelist of the parameter types
* `mp::get_parameter`: Get the type of a parameter by index
* `mp::equivalent_function_type`: Get an equivalent function type (for MFPs this does not include the class)
* `mp::equivalent_function_type_with_class`: Get an equivalent function type with explicit `this` argument (MFPs only)
* `mp::return_type`: Return type of the function
* `mp::class_type`: Only valid for member function pointer types. Gets the class the member function is associated with.

### `integer_of_size`

* `mp::signed_integer_of_size`: Gets a signed integer of the specified bit-size (if it exists)
* `mp::unsigned_integer_of_size`: Gets an unsigned integer of the specified bit-size (if it exists)

### Misc

* `mp::is_instance_of_template`: Checks if a type is an instance of a template class.

License
-------

Please see [LICENSE-0BSD](LICENSE-0BSD).
