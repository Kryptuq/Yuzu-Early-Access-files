// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>

#include "core/hle/kernel/k_auto_object.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/kernel/k_memory_block.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/slab_helpers.h"

namespace Kernel {

class KSessionRequest final : public KSlabAllocated<KSessionRequest>,
                              public KAutoObject,
                              public boost::intrusive::list_base_hook<> {
    KERNEL_AUTOOBJECT_TRAITS(KSessionRequest, KAutoObject);

public:
    class SessionMappings {
    private:
        static constexpr size_t NumStaticMappings = 8;

        class Mapping {
        public:
            constexpr void Set(VAddr c, VAddr s, size_t sz, KMemoryState st) {
                m_client_address = c;
                m_server_address = s;
                m_size = sz;
                m_state = st;
            }

            constexpr VAddr GetClientAddress() const {
                return m_client_address;
            }
            constexpr VAddr GetServerAddress() const {
                return m_server_address;
            }
            constexpr size_t GetSize() const {
                return m_size;
            }
            constexpr KMemoryState GetMemoryState() const {
                return m_state;
            }

        private:
            VAddr m_client_address;
            VAddr m_server_address;
            size_t m_size;
            KMemoryState m_state;
        };

    public:
        explicit SessionMappings(KernelCore& kernel_) : kernel(kernel_) {}

        void Initialize() {}
        void Finalize();

        size_t GetSendCount() const {
            return m_num_send;
        }
        size_t GetReceiveCount() const {
            return m_num_recv;
        }
        size_t GetExchangeCount() const {
            return m_num_exch;
        }

        Result PushSend(VAddr client, VAddr server, size_t size, KMemoryState state);
        Result PushReceive(VAddr client, VAddr server, size_t size, KMemoryState state);
        Result PushExchange(VAddr client, VAddr server, size_t size, KMemoryState state);

        VAddr GetSendClientAddress(size_t i) const {
            return GetSendMapping(i).GetClientAddress();
        }
        VAddr GetSendServerAddress(size_t i) const {
            return GetSendMapping(i).GetServerAddress();
        }
        size_t GetSendSize(size_t i) const {
            return GetSendMapping(i).GetSize();
        }
        KMemoryState GetSendMemoryState(size_t i) const {
            return GetSendMapping(i).GetMemoryState();
        }

        VAddr GetReceiveClientAddress(size_t i) const {
            return GetReceiveMapping(i).GetClientAddress();
        }
        VAddr GetReceiveServerAddress(size_t i) const {
            return GetReceiveMapping(i).GetServerAddress();
        }
        size_t GetReceiveSize(size_t i) const {
            return GetReceiveMapping(i).GetSize();
        }
        KMemoryState GetReceiveMemoryState(size_t i) const {
            return GetReceiveMapping(i).GetMemoryState();
        }

        VAddr GetExchangeClientAddress(size_t i) const {
            return GetExchangeMapping(i).GetClientAddress();
        }
        VAddr GetExchangeServerAddress(size_t i) const {
            return GetExchangeMapping(i).GetServerAddress();
        }
        size_t GetExchangeSize(size_t i) const {
            return GetExchangeMapping(i).GetSize();
        }
        KMemoryState GetExchangeMemoryState(size_t i) const {
            return GetExchangeMapping(i).GetMemoryState();
        }

    private:
        Result PushMap(VAddr client, VAddr server, size_t size, KMemoryState state, size_t index);

        const Mapping& GetSendMapping(size_t i) const {
            ASSERT(i < m_num_send);

            const size_t index = i;
            if (index < NumStaticMappings) {
                return m_static_mappings[index];
            } else {
                return m_mappings[index - NumStaticMappings];
            }
        }

        const Mapping& GetReceiveMapping(size_t i) const {
            ASSERT(i < m_num_recv);

            const size_t index = m_num_send + i;
            if (index < NumStaticMappings) {
                return m_static_mappings[index];
            } else {
                return m_mappings[index - NumStaticMappings];
            }
        }

        const Mapping& GetExchangeMapping(size_t i) const {
            ASSERT(i < m_num_exch);

            const size_t index = m_num_send + m_num_recv + i;
            if (index < NumStaticMappings) {
                return m_static_mappings[index];
            } else {
                return m_mappings[index - NumStaticMappings];
            }
        }

    private:
        KernelCore& kernel;
        std::array<Mapping, NumStaticMappings> m_static_mappings;
        Mapping* m_mappings{};
        u8 m_num_send{};
        u8 m_num_recv{};
        u8 m_num_exch{};
    };

public:
    explicit KSessionRequest(KernelCore& kernel_) : KAutoObject(kernel_), m_mappings(kernel_) {}

    static KSessionRequest* Create(KernelCore& kernel) {
        KSessionRequest* req = KSessionRequest::Allocate(kernel);
        if (req != nullptr) [[likely]] {
            KAutoObject::Create(req);
        }
        return req;
    }

    void Destroy() override {
        this->Finalize();
        KSessionRequest::Free(kernel, this);
    }

    void Initialize(KEvent* event, uintptr_t address, size_t size) {
        m_mappings.Initialize();

        m_thread = GetCurrentThreadPointer(kernel);
        m_event = event;
        m_address = address;
        m_size = size;

        m_thread->Open();
        if (m_event != nullptr) {
            m_event->Open();
        }
    }

    static void PostDestroy(uintptr_t arg) {}

    KThread* GetThread() const {
        return m_thread;
    }
    KEvent* GetEvent() const {
        return m_event;
    }
    uintptr_t GetAddress() const {
        return m_address;
    }
    size_t GetSize() const {
        return m_size;
    }
    KProcess* GetServerProcess() const {
        return m_server;
    }

    void SetServerProcess(KProcess* process) {
        m_server = process;
        m_server->Open();
    }

    void ClearThread() {
        m_thread = nullptr;
    }
    void ClearEvent() {
        m_event = nullptr;
    }

    size_t GetSendCount() const {
        return m_mappings.GetSendCount();
    }
    size_t GetReceiveCount() const {
        return m_mappings.GetReceiveCount();
    }
    size_t GetExchangeCount() const {
        return m_mappings.GetExchangeCount();
    }

    Result PushSend(VAddr client, VAddr server, size_t size, KMemoryState state) {
        return m_mappings.PushSend(client, server, size, state);
    }

    Result PushReceive(VAddr client, VAddr server, size_t size, KMemoryState state) {
        return m_mappings.PushReceive(client, server, size, state);
    }

    Result PushExchange(VAddr client, VAddr server, size_t size, KMemoryState state) {
        return m_mappings.PushExchange(client, server, size, state);
    }

    VAddr GetSendClientAddress(size_t i) const {
        return m_mappings.GetSendClientAddress(i);
    }
    VAddr GetSendServerAddress(size_t i) const {
        return m_mappings.GetSendServerAddress(i);
    }
    size_t GetSendSize(size_t i) const {
        return m_mappings.GetSendSize(i);
    }
    KMemoryState GetSendMemoryState(size_t i) const {
        return m_mappings.GetSendMemoryState(i);
    }

    VAddr GetReceiveClientAddress(size_t i) const {
        return m_mappings.GetReceiveClientAddress(i);
    }
    VAddr GetReceiveServerAddress(size_t i) const {
        return m_mappings.GetReceiveServerAddress(i);
    }
    size_t GetReceiveSize(size_t i) const {
        return m_mappings.GetReceiveSize(i);
    }
    KMemoryState GetReceiveMemoryState(size_t i) const {
        return m_mappings.GetReceiveMemoryState(i);
    }

    VAddr GetExchangeClientAddress(size_t i) const {
        return m_mappings.GetExchangeClientAddress(i);
    }
    VAddr GetExchangeServerAddress(size_t i) const {
        return m_mappings.GetExchangeServerAddress(i);
    }
    size_t GetExchangeSize(size_t i) const {
        return m_mappings.GetExchangeSize(i);
    }
    KMemoryState GetExchangeMemoryState(size_t i) const {
        return m_mappings.GetExchangeMemoryState(i);
    }

private:
    // NOTE: This is public and virtual in Nintendo's kernel.
    void Finalize() override {
        m_mappings.Finalize();

        if (m_thread) {
            m_thread->Close();
        }
        if (m_event) {
            m_event->Close();
        }
        if (m_server) {
            m_server->Close();
        }
    }

private:
    SessionMappings m_mappings;
    KThread* m_thread{};
    KProcess* m_server{};
    KEvent* m_event{};
    uintptr_t m_address{};
    size_t m_size{};
};

} // namespace Kernel
