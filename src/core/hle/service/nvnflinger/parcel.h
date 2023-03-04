// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <span>
#include <vector>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/common_types.h"

namespace Service::android {

struct ParcelHeader {
    u32 data_size;
    u32 data_offset;
    u32 objects_size;
    u32 objects_offset;
};
static_assert(sizeof(ParcelHeader) == 16, "ParcelHeader has wrong size");

class InputParcel final {
public:
    explicit InputParcel(std::span<const u8> in_data) : read_buffer(std::move(in_data)) {
        DeserializeHeader();
        [[maybe_unused]] const std::u16string token = ReadInterfaceToken();
    }

    template <typename T>
    void Read(T& val) {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable.");
        ASSERT(read_index + sizeof(T) <= read_buffer.size());

        std::memcpy(&val, read_buffer.data() + read_index, sizeof(T));
        read_index += sizeof(T);
        read_index = Common::AlignUp(read_index, 4);
    }

    template <typename T>
    T Read() {
        T val;
        Read(val);
        return val;
    }

    template <typename T>
    void ReadFlattened(T& val) {
        const auto flattened_size = Read<s64>();
        ASSERT(sizeof(T) == flattened_size);
        Read(val);
    }

    template <typename T>
    T ReadFlattened() {
        T val;
        ReadFlattened(val);
        return val;
    }

    template <typename T>
    T ReadUnaligned() {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable.");
        ASSERT(read_index + sizeof(T) <= read_buffer.size());

        T val;
        std::memcpy(&val, read_buffer.data() + read_index, sizeof(T));
        read_index += sizeof(T);
        return val;
    }

    template <typename T>
    const std::shared_ptr<T> ReadObject() {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable.");

        const auto is_valid{Read<bool>()};

        if (is_valid) {
            auto result = std::make_shared<T>();
            ReadFlattened(*result);
            return result;
        }

        return {};
    }

    std::u16string ReadInterfaceToken() {
        [[maybe_unused]] const u32 unknown = Read<u32>();
        const u32 length = Read<u32>();

        std::u16string token;
        token.reserve(length + 1);

        for (u32 ch = 0; ch < length + 1; ++ch) {
            token.push_back(ReadUnaligned<u16>());
        }

        read_index = Common::AlignUp(read_index, 4);

        return token;
    }

    void DeserializeHeader() {
        ASSERT(read_buffer.size() > sizeof(ParcelHeader));

        ParcelHeader header{};
        std::memcpy(&header, read_buffer.data(), sizeof(ParcelHeader));

        read_index = header.data_offset;
    }

private:
    std::span<const u8> read_buffer;
    std::size_t read_index = 0;
};

class OutputParcel final {
public:
    static constexpr std::size_t DefaultBufferSize = 0x40;

    OutputParcel() : buffer(DefaultBufferSize) {}

    template <typename T>
    explicit OutputParcel(const T& out_data) : buffer(DefaultBufferSize) {
        Write(out_data);
    }

    template <typename T>
    void Write(const T& val) {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable.");

        if (buffer.size() < write_index + sizeof(T)) {
            buffer.resize(buffer.size() + sizeof(T) + DefaultBufferSize);
        }

        std::memcpy(buffer.data() + write_index, &val, sizeof(T));
        write_index += sizeof(T);
        write_index = Common::AlignUp(write_index, 4);
    }

    template <typename T>
    void WriteObject(const T* ptr) {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable.");

        if (!ptr) {
            Write<u32>(0);
            return;
        }

        Write<u32>(1);
        Write<s64>(sizeof(T));
        Write(*ptr);
    }

    template <typename T>
    void WriteObject(const std::shared_ptr<T> ptr) {
        WriteObject(ptr.get());
    }

    std::vector<u8> Serialize() const {
        ParcelHeader header{};
        header.data_size = static_cast<u32>(write_index - sizeof(ParcelHeader));
        header.data_offset = sizeof(ParcelHeader);
        header.objects_size = 4;
        header.objects_offset = static_cast<u32>(sizeof(ParcelHeader) + header.data_size);
        std::memcpy(buffer.data(), &header, sizeof(ParcelHeader));

        return buffer;
    }

private:
    mutable std::vector<u8> buffer;
    std::size_t write_index = sizeof(ParcelHeader);
};

} // namespace Service::android
