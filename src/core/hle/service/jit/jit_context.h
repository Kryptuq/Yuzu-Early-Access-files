// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <span>
#include <string>

#include "common/common_types.h"

namespace Core::Memory {
class Memory;
}

namespace Service::JIT {

class JITContextImpl;

class JITContext {
public:
    explicit JITContext(Core::Memory::Memory& memory);
    ~JITContext();

    [[nodiscard]] bool LoadNRO(std::span<const u8> data);
    void MapProcessMemory(VAddr dest_address, std::size_t size);

    template <typename T, typename... Ts>
    u64 CallFunction(VAddr func, T argument, Ts... rest) {
        static_assert(std::is_trivially_copyable_v<T>);
        PushArgument(&argument, sizeof(argument));

        if constexpr (sizeof...(rest) > 0) {
            return CallFunction(func, rest...);
        } else {
            return CallFunction(func);
        }
    }

    u64 CallFunction(VAddr func);
    VAddr GetHelper(const std::string& name);

    template <typename T>
    VAddr AddHeap(T argument) {
        return AddHeap(&argument, sizeof(argument));
    }
    VAddr AddHeap(const void* data, size_t size);

    template <typename T>
    T GetHeap(VAddr location) {
        static_assert(std::is_trivially_copyable_v<T>);
        T result;
        GetHeap(location, &result, sizeof(result));
        return result;
    }
    void GetHeap(VAddr location, void* data, size_t size);

private:
    std::unique_ptr<JITContextImpl> impl;

    void PushArgument(const void* data, size_t size);
};

} // namespace Service::JIT
