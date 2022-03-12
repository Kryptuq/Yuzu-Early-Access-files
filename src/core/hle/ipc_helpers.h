// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <cstring>
#include <memory>
#include <type_traits>
#include <utility>
#include "common/assert.h"
#include "common/common_types.h"
#include "core/hle/ipc.h"
#include "core/hle/kernel/hle_ipc.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_resource_limit.h"
#include "core/hle/kernel/k_session.h"
#include "core/hle/result.h"

namespace IPC {

constexpr ResultCode ERR_REMOTE_PROCESS_DEAD{ErrorModule::HIPC, 301};

class RequestHelperBase {
protected:
    Kernel::HLERequestContext* context = nullptr;
    u32* cmdbuf;
    u32 index = 0;

public:
    explicit RequestHelperBase(u32* command_buffer) : cmdbuf(command_buffer) {}

    explicit RequestHelperBase(Kernel::HLERequestContext& ctx)
        : context(&ctx), cmdbuf(ctx.CommandBuffer()) {}

    void Skip(u32 size_in_words, bool set_to_null) {
        if (set_to_null) {
            memset(cmdbuf + index, 0, size_in_words * sizeof(u32));
        }
        index += size_in_words;
    }

    /**
     * Aligns the current position forward to a 16-byte boundary, padding with zeros.
     */
    void AlignWithPadding() {
        if (index & 3) {
            Skip(static_cast<u32>(4 - (index & 3)), true);
        }
    }

    u32 GetCurrentOffset() const {
        return index;
    }

    void SetCurrentOffset(u32 offset) {
        index = offset;
    }
};

class ResponseBuilder : public RequestHelperBase {
public:
    /// Flags used for customizing the behavior of ResponseBuilder
    enum class Flags : u32 {
        None = 0,
        /// Uses move handles to move objects in the response, even when in a domain. This is
        /// required when PushMoveObjects is used.
        AlwaysMoveHandles = 1,
    };

    explicit ResponseBuilder(Kernel::HLERequestContext& ctx, u32 normal_params_size_,
                             u32 num_handles_to_copy_ = 0, u32 num_objects_to_move_ = 0,
                             Flags flags = Flags::None)
        : RequestHelperBase(ctx), normal_params_size(normal_params_size_),
          num_handles_to_copy(num_handles_to_copy_),
          num_objects_to_move(num_objects_to_move_), kernel{ctx.kernel} {

        memset(cmdbuf, 0, sizeof(u32) * IPC::COMMAND_BUFFER_LENGTH);

        IPC::CommandHeader header{};

        // The entire size of the raw data section in u32 units, including the 16 bytes of mandatory
        // padding.
        u32 raw_data_size = ctx.write_size =
            ctx.IsTipc() ? normal_params_size - 1 : normal_params_size;
        u32 num_handles_to_move{};
        u32 num_domain_objects{};
        const bool always_move_handles{
            (static_cast<u32>(flags) & static_cast<u32>(Flags::AlwaysMoveHandles)) != 0};
        if (!ctx.Session()->IsDomain() || always_move_handles) {
            num_handles_to_move = num_objects_to_move;
        } else {
            num_domain_objects = num_objects_to_move;
        }

        if (ctx.Session()->IsDomain()) {
            raw_data_size +=
                static_cast<u32>(sizeof(DomainMessageHeader) / sizeof(u32) + num_domain_objects);
            ctx.write_size += num_domain_objects;
        }

        if (ctx.IsTipc()) {
            header.type.Assign(ctx.GetCommandType());
        } else {
            raw_data_size += static_cast<u32>(sizeof(IPC::DataPayloadHeader) / sizeof(u32) + 4 +
                                              normal_params_size);
        }

        header.data_size.Assign(raw_data_size);
        if (num_handles_to_copy || num_handles_to_move) {
            header.enable_handle_descriptor.Assign(1);
        }
        PushRaw(header);

        if (header.enable_handle_descriptor) {
            IPC::HandleDescriptorHeader handle_descriptor_header{};
            handle_descriptor_header.num_handles_to_copy.Assign(num_handles_to_copy_);
            handle_descriptor_header.num_handles_to_move.Assign(num_handles_to_move);
            PushRaw(handle_descriptor_header);

            ctx.handles_offset = index;

            Skip(num_handles_to_copy + num_handles_to_move, true);
        }

        if (!ctx.IsTipc()) {
            AlignWithPadding();

            if (ctx.Session()->IsDomain() && ctx.HasDomainMessageHeader()) {
                IPC::DomainMessageHeader domain_header{};
                domain_header.num_objects = num_domain_objects;
                PushRaw(domain_header);
            }

            IPC::DataPayloadHeader data_payload_header{};
            data_payload_header.magic = Common::MakeMagic('S', 'F', 'C', 'O');
            PushRaw(data_payload_header);
        }

        data_payload_index = index;

        ctx.data_payload_offset = index;
        ctx.write_size += index;
        ctx.domain_offset = static_cast<u32>(index + raw_data_size / sizeof(u32));
    }

    template <class T>
    void PushIpcInterface(std::shared_ptr<T> iface) {
        if (context->Session()->IsDomain()) {
            context->AddDomainObject(std::move(iface));
        } else {
            kernel.CurrentProcess()->GetResourceLimit()->Reserve(
                Kernel::LimitableResource::Sessions, 1);

            auto* session = Kernel::KSession::Create(kernel);
            session->Initialize(nullptr, iface->GetServiceName());

            context->AddMoveObject(&session->GetClientSession());
            iface->ClientConnected(&session->GetServerSession());
        }
    }

    template <class T, class... Args>
    void PushIpcInterface(Args&&... args) {
        PushIpcInterface<T>(std::make_shared<T>(std::forward<Args>(args)...));
    }

    void PushImpl(s8 value);
    void PushImpl(s16 value);
    void PushImpl(s32 value);
    void PushImpl(s64 value);
    void PushImpl(u8 value);
    void PushImpl(u16 value);
    void PushImpl(u32 value);
    void PushImpl(u64 value);
    void PushImpl(float value);
    void PushImpl(double value);
    void PushImpl(bool value);
    void PushImpl(ResultCode value);

    template <typename T>
    void Push(T value) {
        return PushImpl(value);
    }

    template <typename First, typename... Other>
    void Push(const First& first_value, const Other&... other_values);

    /**
     * Helper function for pushing strongly-typed enumeration values.
     *
     * @tparam Enum The enumeration type to be pushed
     *
     * @param value The value to push.
     *
     * @note The underlying size of the enumeration type is the size of the
     *       data that gets pushed. e.g. "enum class SomeEnum : u16" will
     *       push a u16-sized amount of data.
     */
    template <typename Enum>
    void PushEnum(Enum value) {
        static_assert(std::is_enum_v<Enum>, "T must be an enum type within a PushEnum call.");
        static_assert(!std::is_convertible_v<Enum, int>,
                      "enum type in PushEnum must be a strongly typed enum.");
        Push(static_cast<std::underlying_type_t<Enum>>(value));
    }

    /**
     * @brief Copies the content of the given trivially copyable class to the buffer as a normal
     * param
     * @note: The input class must be correctly packed/padded to fit hardware layout.
     */
    template <typename T>
    void PushRaw(const T& value);

    template <typename... O>
    void PushMoveObjects(O*... pointers);

    template <typename... O>
    void PushMoveObjects(O&... pointers);

    template <typename... O>
    void PushCopyObjects(O*... pointers);

    template <typename... O>
    void PushCopyObjects(O&... pointers);

private:
    u32 normal_params_size{};
    u32 num_handles_to_copy{};
    u32 num_objects_to_move{}; ///< Domain objects or move handles, context dependent
    u32 data_payload_index{};
    Kernel::KernelCore& kernel;
};

/// Push ///

inline void ResponseBuilder::PushImpl(s32 value) {
    cmdbuf[index++] = value;
}

inline void ResponseBuilder::PushImpl(u32 value) {
    cmdbuf[index++] = value;
}

template <typename T>
void ResponseBuilder::PushRaw(const T& value) {
    static_assert(std::is_trivially_copyable_v<T>,
                  "It's undefined behavior to use memcpy with non-trivially copyable objects");
    std::memcpy(cmdbuf + index, &value, sizeof(T));
    index += (sizeof(T) + 3) / 4; // round up to word length
}

inline void ResponseBuilder::PushImpl(ResultCode value) {
    // Result codes are actually 64-bit in the IPC buffer, but only the high part is discarded.
    Push(value.raw);
    Push<u32>(0);
}

inline void ResponseBuilder::PushImpl(s8 value) {
    PushRaw(value);
}

inline void ResponseBuilder::PushImpl(s16 value) {
    PushRaw(value);
}

inline void ResponseBuilder::PushImpl(s64 value) {
    PushImpl(static_cast<u32>(value));
    PushImpl(static_cast<u32>(value >> 32));
}

inline void ResponseBuilder::PushImpl(u8 value) {
    PushRaw(value);
}

inline void ResponseBuilder::PushImpl(u16 value) {
    PushRaw(value);
}

inline void ResponseBuilder::PushImpl(u64 value) {
    PushImpl(static_cast<u32>(value));
    PushImpl(static_cast<u32>(value >> 32));
}

inline void ResponseBuilder::PushImpl(float value) {
    u32 integral;
    std::memcpy(&integral, &value, sizeof(u32));
    PushImpl(integral);
}

inline void ResponseBuilder::PushImpl(double value) {
    u64 integral;
    std::memcpy(&integral, &value, sizeof(u64));
    PushImpl(integral);
}

inline void ResponseBuilder::PushImpl(bool value) {
    PushImpl(static_cast<u8>(value));
}

template <typename First, typename... Other>
void ResponseBuilder::Push(const First& first_value, const Other&... other_values) {
    Push(first_value);
    Push(other_values...);
}

template <typename... O>
inline void ResponseBuilder::PushCopyObjects(O*... pointers) {
    auto objects = {pointers...};
    for (auto& object : objects) {
        context->AddCopyObject(object);
    }
}

template <typename... O>
inline void ResponseBuilder::PushCopyObjects(O&... pointers) {
    auto objects = {&pointers...};
    for (auto& object : objects) {
        context->AddCopyObject(object);
    }
}

template <typename... O>
inline void ResponseBuilder::PushMoveObjects(O*... pointers) {
    auto objects = {pointers...};
    for (auto& object : objects) {
        context->AddMoveObject(object);
    }
}

template <typename... O>
inline void ResponseBuilder::PushMoveObjects(O&... pointers) {
    auto objects = {&pointers...};
    for (auto& object : objects) {
        context->AddMoveObject(object);
    }
}

class RequestParser : public RequestHelperBase {
public:
    explicit RequestParser(u32* command_buffer) : RequestHelperBase(command_buffer) {}

    explicit RequestParser(Kernel::HLERequestContext& ctx) : RequestHelperBase(ctx) {
        // TIPC does not have data payload offset
        if (!ctx.IsTipc()) {
            ASSERT_MSG(ctx.GetDataPayloadOffset(), "context is incomplete");
            Skip(ctx.GetDataPayloadOffset(), false);
        }

        // Skip the u64 command id, it's already stored in the context
        static constexpr u32 CommandIdSize = 2;
        Skip(CommandIdSize, false);
    }

    template <typename T>
    T Pop();

    template <typename T>
    void Pop(T& value);

    template <typename First, typename... Other>
    void Pop(First& first_value, Other&... other_values);

    template <typename T>
    T PopEnum() {
        static_assert(std::is_enum_v<T>, "T must be an enum type within a PopEnum call.");
        static_assert(!std::is_convertible_v<T, int>,
                      "enum type in PopEnum must be a strongly typed enum.");
        return static_cast<T>(Pop<std::underlying_type_t<T>>());
    }

    /**
     * @brief Reads the next normal parameters as a struct, by copying it
     * @note: The output class must be correctly packed/padded to fit hardware layout.
     */
    template <typename T>
    void PopRaw(T& value);

    /**
     * @brief Reads the next normal parameters as a struct, by copying it into a new value
     * @note: The output class must be correctly packed/padded to fit hardware layout.
     */
    template <typename T>
    T PopRaw();

    template <class T>
    std::weak_ptr<T> PopIpcInterface() {
        ASSERT(context->Session()->IsDomain());
        ASSERT(context->GetDomainMessageHeader().input_object_count > 0);
        return context->GetDomainHandler<T>(Pop<u32>() - 1);
    }
};

/// Pop ///

template <>
inline u32 RequestParser::Pop() {
    return cmdbuf[index++];
}

template <>
inline s32 RequestParser::Pop() {
    return static_cast<s32>(Pop<u32>());
}

// Ignore the -Wclass-memaccess warning on memcpy for non-trivially default constructible objects.
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif
template <typename T>
void RequestParser::PopRaw(T& value) {
    static_assert(std::is_trivially_copyable_v<T>,
                  "It's undefined behavior to use memcpy with non-trivially copyable objects");
    std::memcpy(&value, cmdbuf + index, sizeof(T));
    index += (sizeof(T) + 3) / 4; // round up to word length
}
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

template <typename T>
T RequestParser::PopRaw() {
    T value;
    PopRaw(value);
    return value;
}

template <>
inline u8 RequestParser::Pop() {
    return PopRaw<u8>();
}

template <>
inline u16 RequestParser::Pop() {
    return PopRaw<u16>();
}

template <>
inline u64 RequestParser::Pop() {
    const u64 lsw = Pop<u32>();
    const u64 msw = Pop<u32>();
    return msw << 32 | lsw;
}

template <>
inline s8 RequestParser::Pop() {
    return static_cast<s8>(Pop<u8>());
}

template <>
inline s16 RequestParser::Pop() {
    return static_cast<s16>(Pop<u16>());
}

template <>
inline s64 RequestParser::Pop() {
    return static_cast<s64>(Pop<u64>());
}

template <>
inline float RequestParser::Pop() {
    const u32 value = Pop<u32>();
    float real;
    std::memcpy(&real, &value, sizeof(real));
    return real;
}

template <>
inline double RequestParser::Pop() {
    const u64 value = Pop<u64>();
    double real;
    std::memcpy(&real, &value, sizeof(real));
    return real;
}

template <>
inline bool RequestParser::Pop() {
    return Pop<u8>() != 0;
}

template <>
inline ResultCode RequestParser::Pop() {
    return ResultCode{Pop<u32>()};
}

template <typename T>
void RequestParser::Pop(T& value) {
    value = Pop<T>();
}

template <typename First, typename... Other>
void RequestParser::Pop(First& first_value, Other&... other_values) {
    first_value = Pop<First>();
    Pop(other_values...);
}

} // namespace IPC
