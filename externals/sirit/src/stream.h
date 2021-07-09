/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#pragma once

#include <bit>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <functional>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#ifndef __cpp_lib_bit_cast
#include <cstring>
#endif

#include <spirv/unified1/spirv.hpp>

#include "common_types.h"

namespace Sirit {

class Declarations;

struct OpId {
    OpId(spv::Op opcode_) : opcode{opcode_} {}
    OpId(spv::Op opcode_, Id result_type_) : opcode{opcode_}, result_type{result_type_} {
        assert(result_type.value != 0);
    }

    spv::Op opcode{};
    Id result_type{};
};

struct EndOp {};

inline size_t WordsInString(std::string_view string) {
    return string.size() / sizeof(u32) + 1;
}

inline void InsertStringView(std::vector<u32>& words, size_t& insert_index,
                             std::string_view string) {
    const size_t size = string.size();
    const auto read = [string, size](size_t offset) {
        return offset < size ? static_cast<u32>(string[offset]) : 0u;
    };

    for (size_t i = 0; i < size; i += sizeof(u32)) {
        words[insert_index++] = read(i) | read(i + 1) << 8 | read(i + 2) << 16 | read(i + 3) << 24;
    }
    if (size % sizeof(u32) == 0) {
        words[insert_index++] = 0;
    }
}

class Stream {
    friend Declarations;

public:
    explicit Stream(u32* bound_) : bound{bound_} {}

    void Reserve(size_t num_words) {
        if (insert_index + num_words <= words.size()) {
            return;
        }
        words.resize(insert_index + num_words);
    }

    std::span<const u32> Words() const noexcept {
        return std::span(words.data(), insert_index);
    }

    u32 LocalAddress() const noexcept {
        return static_cast<u32>(words.size());
    }

    u32 Value(u32 index) const noexcept {
        return words[index];
    }

    void SetValue(u32 index, u32 value) noexcept {
        words[index] = value;
    }

    Stream& operator<<(spv::Op op) {
        op_index = insert_index;
        words[insert_index++] = static_cast<u32>(op);
        return *this;
    }

    Stream& operator<<(OpId op) {
        op_index = insert_index;
        words[insert_index++] = static_cast<u32>(op.opcode);
        if (op.result_type.value != 0) {
            words[insert_index++] = op.result_type.value;
        }
        words[insert_index++] = ++*bound;
        return *this;
    }

    Id operator<<(EndOp) {
        const size_t num_words = insert_index - op_index;
        words[op_index] |= static_cast<u32>(num_words) << 16;
        return Id{*bound};
    }

    Stream& operator<<(u32 value) {
        words[insert_index++] = value;
        return *this;
    }

    Stream& operator<<(s32 value) {
        return *this << static_cast<u32>(value);
    }

    Stream& operator<<(u64 value) {
        return *this << static_cast<u32>(value) << static_cast<u32>(value >> 32);
    }

    Stream& operator<<(s64 value) {
        return *this << static_cast<u64>(value);
    }

    Stream& operator<<(float value) {
#ifdef __cpp_lib_bit_cast
        return *this << std::bit_cast<u32>(value);
#else
        static_assert(sizeof(float) == sizeof(u32));
        u32 int_value;
        std::memcpy(&int_value, &value, sizeof(int_value));
        return *this << int_value;
#endif
    }

    Stream& operator<<(double value) {
#ifdef __cpp_lib_bit_cast
        return *this << std::bit_cast<u64>(value);
#else
        static_assert(sizeof(double) == sizeof(u64));
        u64 int_value;
        std::memcpy(&int_value, &value, sizeof(int_value));
        return *this << int_value;
#endif
    }

    Stream& operator<<(bool value) {
        return *this << static_cast<u32>(value ? 1 : 0);
    }

    Stream& operator<<(Id value) {
        assert(value.value != 0);
        return *this << value.value;
    }

    Stream& operator<<(const Literal& literal) {
        std::visit([this](auto value) { *this << value; }, literal);
        return *this;
    }

    Stream& operator<<(std::string_view string) {
        InsertStringView(words, insert_index, string);
        return *this;
    }

    Stream& operator<<(const char* string) {
        return *this << std::string_view{string};
    }

    template <typename T>
    requires std::is_enum_v<T> Stream& operator<<(T value) {
        static_assert(sizeof(T) == sizeof(u32));
        return *this << static_cast<u32>(value);
    }

    template <typename T>
    Stream& operator<<(std::optional<T> value) {
        if (value) {
            *this << *value;
        }
        return *this;
    }

    template <typename T>
    Stream& operator<<(std::span<const T> values) {
        for (const auto& value : values) {
            *this << value;
        }
        return *this;
    }

private:
    u32* bound = nullptr;
    std::vector<u32> words;
    size_t insert_index = 0;
    size_t op_index = 0;
};

class Declarations {
public:
    explicit Declarations(u32* bound) : stream{bound} {}

    void Reserve(size_t num_words) {
        return stream.Reserve(num_words);
    }

    std::span<const u32> Words() const noexcept {
        return stream.Words();
    }

    template <typename T>
    Declarations& operator<<(const T& value) {
        stream << value;
        return *this;
    }

    // Declarations without an id don't exist
    Declarations& operator<<(spv::Op) = delete;

    Declarations& operator<<(OpId op) {
        id_index = op.result_type.value != 0 ? 2 : 1;
        stream << op;
        return *this;
    }

    Id operator<<(EndOp) {
        const auto begin = stream.words.data();
        std::vector<u32> declarations(begin + stream.op_index, begin + stream.insert_index);

        // Normalize result id for lookups
        const u32 id = std::exchange(declarations[id_index], 0);

        const auto [entry, inserted] = existing_declarations.emplace(declarations, id);
        if (inserted) {
            return stream << EndOp{};
        }
        // If the declaration already exists, undo the operation
        stream.insert_index = stream.op_index;
        --*stream.bound;

        return Id{entry->second};
    }

private:
    struct HashVector {
        size_t operator()(const std::vector<u32>& vector) const noexcept {
            size_t hash = std::hash<size_t>{}(vector.size());
            for (const u32 value : vector) {
                hash ^= std::hash<u32>{}(value);
            }
            return hash;
        }
    };

    Stream stream;
    std::unordered_map<std::vector<u32>, u32, HashVector> existing_declarations;
    size_t id_index = 0;
};

} // namespace Sirit
