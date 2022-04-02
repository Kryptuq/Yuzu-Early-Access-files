// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include "common/assert.h"
#include "common/common_types.h"
#include "common/concepts.h"
#include "common/swap.h"
#include "core/hle/ipc.h"
#include "core/hle/kernel/svc_common.h"

union ResultCode;

namespace Core::Memory {
class Memory;
}

namespace IPC {
class ResponseBuilder;
}

namespace Service {
class ServiceFrameworkBase;
}

enum class ServiceThreadType {
    Default,
    CreateNew,
};

namespace Kernel {

class Domain;
class HLERequestContext;
class KAutoObject;
class KernelCore;
class KHandleTable;
class KProcess;
class KServerSession;
class KThread;
class KReadableEvent;
class KSession;
class KWritableEvent;
class ServiceThread;

enum class ThreadWakeupReason;

/**
 * Interface implemented by HLE Session handlers.
 * This can be provided to a ServerSession in order to hook into several relevant events
 * (such as a new connection or a SyncRequest) so they can be implemented in the emulator.
 */
class SessionRequestHandler : public std::enable_shared_from_this<SessionRequestHandler> {
public:
    SessionRequestHandler(KernelCore& kernel_, const char* service_name_,
                          ServiceThreadType thread_type);
    virtual ~SessionRequestHandler();

    /**
     * Handles a sync request from the emulated application.
     * @param server_session The ServerSession that was triggered for this sync request,
     * it should be used to differentiate which client (As in ClientSession) we're answering to.
     * TODO(Subv): Use a wrapper structure to hold all the information relevant to
     * this request (ServerSession, Originator thread, Translated command buffer, etc).
     * @returns ResultCode the result code of the translate operation.
     */
    virtual ResultCode HandleSyncRequest(Kernel::KServerSession& session,
                                         Kernel::HLERequestContext& context) = 0;

    /**
     * Signals that a client has just connected to this HLE handler and keeps the
     * associated ServerSession alive for the duration of the connection.
     * @param server_session Owning pointer to the ServerSession associated with the connection.
     */
    void ClientConnected(KServerSession* session);

    /**
     * Signals that a client has just disconnected from this HLE handler and releases the
     * associated ServerSession.
     * @param server_session ServerSession associated with the connection.
     */
    void ClientDisconnected(KServerSession* session);

    std::weak_ptr<ServiceThread> GetServiceThread() const {
        return service_thread;
    }

protected:
    KernelCore& kernel;
    std::weak_ptr<ServiceThread> service_thread;
};

using SessionRequestHandlerWeakPtr = std::weak_ptr<SessionRequestHandler>;
using SessionRequestHandlerPtr = std::shared_ptr<SessionRequestHandler>;

/**
 * Manages the underlying HLE requests for a session, and whether (or not) the session should be
 * treated as a domain. This is managed separately from server sessions, as this state is shared
 * when objects are cloned.
 */
class SessionRequestManager final {
public:
    explicit SessionRequestManager(KernelCore& kernel);
    ~SessionRequestManager();

    bool IsDomain() const {
        return is_domain;
    }

    void ConvertToDomain() {
        domain_handlers = {session_handler};
        is_domain = true;
    }

    std::size_t DomainHandlerCount() const {
        return domain_handlers.size();
    }

    bool HasSessionHandler() const {
        return session_handler != nullptr;
    }

    SessionRequestHandler& SessionHandler() {
        return *session_handler;
    }

    const SessionRequestHandler& SessionHandler() const {
        return *session_handler;
    }

    void CloseDomainHandler(std::size_t index) {
        if (index < DomainHandlerCount()) {
            domain_handlers[index] = nullptr;
        } else {
            UNREACHABLE_MSG("Unexpected handler index {}", index);
        }
    }

    SessionRequestHandlerWeakPtr DomainHandler(std::size_t index) const {
        ASSERT_MSG(index < DomainHandlerCount(), "Unexpected handler index {}", index);
        return domain_handlers.at(index);
    }

    void AppendDomainHandler(SessionRequestHandlerPtr&& handler) {
        domain_handlers.emplace_back(std::move(handler));
    }

    void SetSessionHandler(SessionRequestHandlerPtr&& handler) {
        session_handler = std::move(handler);
    }

    std::weak_ptr<ServiceThread> GetServiceThread() const {
        return session_handler->GetServiceThread();
    }

    bool HasSessionRequestHandler(const HLERequestContext& context) const;

private:
    bool is_domain{};
    SessionRequestHandlerPtr session_handler;
    std::vector<SessionRequestHandlerPtr> domain_handlers;

private:
    KernelCore& kernel;
};

/**
 * Class containing information about an in-flight IPC request being handled by an HLE service
 * implementation. Services should avoid using old global APIs (e.g. Kernel::GetCommandBuffer()) and
 * when possible use the APIs in this class to service the request.
 *
 * HLE handle protocol
 * ===================
 *
 * To avoid needing HLE services to keep a separate handle table, or having to directly modify the
 * requester's table, a tweaked protocol is used to receive and send handles in requests. The kernel
 * will decode the incoming handles into object pointers and insert a id in the buffer where the
 * handle would normally be. The service then calls GetIncomingHandle() with that id to get the
 * pointer to the object. Similarly, instead of inserting a handle into the command buffer, the
 * service calls AddOutgoingHandle() and stores the returned id where the handle would normally go.
 *
 * The end result is similar to just giving services their own real handle tables, but since these
 * ids are local to a specific context, it avoids requiring services to manage handles for objects
 * across multiple calls and ensuring that unneeded handles are cleaned up.
 */
class HLERequestContext {
public:
    explicit HLERequestContext(KernelCore& kernel, Core::Memory::Memory& memory,
                               KServerSession* session, KThread* thread);
    ~HLERequestContext();

    /// Returns a pointer to the IPC command buffer for this request.
    u32* CommandBuffer() {
        return cmd_buf.data();
    }

    /**
     * Returns the session through which this request was made. This can be used as a map key to
     * access per-client data on services.
     */
    Kernel::KServerSession* Session() {
        return server_session;
    }

    /// Populates this context with data from the requesting process/thread.
    ResultCode PopulateFromIncomingCommandBuffer(const KHandleTable& handle_table,
                                                 u32_le* src_cmdbuf);

    /// Writes data from this context back to the requesting process/thread.
    ResultCode WriteToOutgoingCommandBuffer(KThread& requesting_thread);

    u32_le GetHipcCommand() const {
        return command;
    }

    u32_le GetTipcCommand() const {
        return static_cast<u32_le>(command_header->type.Value()) -
               static_cast<u32_le>(IPC::CommandType::TIPC_CommandRegion);
    }

    u32_le GetCommand() const {
        return command_header->IsTipc() ? GetTipcCommand() : GetHipcCommand();
    }

    bool IsTipc() const {
        return command_header->IsTipc();
    }

    IPC::CommandType GetCommandType() const {
        return command_header->type;
    }

    u64 GetPID() const {
        return pid;
    }

    u32 GetDataPayloadOffset() const {
        return data_payload_offset;
    }

    const std::vector<IPC::BufferDescriptorX>& BufferDescriptorX() const {
        return buffer_x_desciptors;
    }

    const std::vector<IPC::BufferDescriptorABW>& BufferDescriptorA() const {
        return buffer_a_desciptors;
    }

    const std::vector<IPC::BufferDescriptorABW>& BufferDescriptorB() const {
        return buffer_b_desciptors;
    }

    const std::vector<IPC::BufferDescriptorC>& BufferDescriptorC() const {
        return buffer_c_desciptors;
    }

    const IPC::DomainMessageHeader& GetDomainMessageHeader() const {
        return domain_message_header.value();
    }

    bool HasDomainMessageHeader() const {
        return domain_message_header.has_value();
    }

    /// Helper function to read a buffer using the appropriate buffer descriptor
    std::vector<u8> ReadBuffer(std::size_t buffer_index = 0) const;

    /// Helper function to write a buffer using the appropriate buffer descriptor
    std::size_t WriteBuffer(const void* buffer, std::size_t size,
                            std::size_t buffer_index = 0) const;

    /* Helper function to write a buffer using the appropriate buffer descriptor
     *
     * @tparam T an arbitrary container that satisfies the
     *         ContiguousContainer concept in the C++ standard library or a trivially copyable type.
     *
     * @param data         The container/data to write into a buffer.
     * @param buffer_index The buffer in particular to write to.
     */
    template <typename T, typename = std::enable_if_t<!std::is_pointer_v<T>>>
    std::size_t WriteBuffer(const T& data, std::size_t buffer_index = 0) const {
        if constexpr (Common::IsSTLContainer<T>) {
            using ContiguousType = typename T::value_type;
            static_assert(std::is_trivially_copyable_v<ContiguousType>,
                          "Container to WriteBuffer must contain trivially copyable objects");
            return WriteBuffer(std::data(data), std::size(data) * sizeof(ContiguousType),
                               buffer_index);
        } else {
            static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
            return WriteBuffer(&data, sizeof(T), buffer_index);
        }
    }

    /// Helper function to get the size of the input buffer
    std::size_t GetReadBufferSize(std::size_t buffer_index = 0) const;

    /// Helper function to get the size of the output buffer
    std::size_t GetWriteBufferSize(std::size_t buffer_index = 0) const;

    /// Helper function to test whether the input buffer at buffer_index can be read
    bool CanReadBuffer(std::size_t buffer_index = 0) const;

    /// Helper function to test whether the output buffer at buffer_index can be written
    bool CanWriteBuffer(std::size_t buffer_index = 0) const;

    Handle GetCopyHandle(std::size_t index) const {
        return incoming_copy_handles.at(index);
    }

    Handle GetMoveHandle(std::size_t index) const {
        return incoming_move_handles.at(index);
    }

    void AddMoveObject(KAutoObject* object) {
        outgoing_move_objects.emplace_back(object);
    }

    void AddCopyObject(KAutoObject* object) {
        outgoing_copy_objects.emplace_back(object);
    }

    void AddDomainObject(SessionRequestHandlerPtr object) {
        outgoing_domain_objects.emplace_back(std::move(object));
    }

    template <typename T>
    std::shared_ptr<T> GetDomainHandler(std::size_t index) const {
        return std::static_pointer_cast<T>(manager.lock()->DomainHandler(index).lock());
    }

    void SetSessionRequestManager(std::weak_ptr<SessionRequestManager> manager_) {
        manager = std::move(manager_);
    }

    std::string Description() const;

    KThread& GetThread() {
        return *thread;
    }

private:
    friend class IPC::ResponseBuilder;

    void ParseCommandBuffer(const KHandleTable& handle_table, u32_le* src_cmdbuf, bool incoming);

    std::array<u32, IPC::COMMAND_BUFFER_LENGTH> cmd_buf;
    Kernel::KServerSession* server_session{};
    KThread* thread;

    std::vector<Handle> incoming_move_handles;
    std::vector<Handle> incoming_copy_handles;

    std::vector<KAutoObject*> outgoing_move_objects;
    std::vector<KAutoObject*> outgoing_copy_objects;
    std::vector<SessionRequestHandlerPtr> outgoing_domain_objects;

    std::optional<IPC::CommandHeader> command_header;
    std::optional<IPC::HandleDescriptorHeader> handle_descriptor_header;
    std::optional<IPC::DataPayloadHeader> data_payload_header;
    std::optional<IPC::DomainMessageHeader> domain_message_header;
    std::vector<IPC::BufferDescriptorX> buffer_x_desciptors;
    std::vector<IPC::BufferDescriptorABW> buffer_a_desciptors;
    std::vector<IPC::BufferDescriptorABW> buffer_b_desciptors;
    std::vector<IPC::BufferDescriptorABW> buffer_w_desciptors;
    std::vector<IPC::BufferDescriptorC> buffer_c_desciptors;

    u32_le command{};
    u64 pid{};
    u32 write_size{};
    u32 data_payload_offset{};
    u32 handles_offset{};
    u32 domain_offset{};

    std::weak_ptr<SessionRequestManager> manager;

    KernelCore& kernel;
    Core::Memory::Memory& memory;
};

} // namespace Kernel
