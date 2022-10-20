// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/kernel/k_page_buffer.h"
#include "core/hle/kernel/k_session_request.h"

namespace Kernel {

Result KSessionRequest::SessionMappings::PushMap(VAddr client, VAddr server, size_t size,
                                                 KMemoryState state, size_t index) {
    // At most 15 buffers of each type (4-bit descriptor counts).
    ASSERT(index < ((1ul << 4) - 1) * 3);

    // Get the mapping.
    Mapping* mapping;
    if (index < NumStaticMappings) {
        mapping = &m_static_mappings[index];
    } else {
        // Allocate a page for the extra mappings.
        if (m_mappings == nullptr) {
            KPageBuffer* page_buffer = KPageBuffer::Allocate(kernel);
            R_UNLESS(page_buffer != nullptr, ResultOutOfMemory);

            m_mappings = reinterpret_cast<Mapping*>(page_buffer);
        }

        mapping = &m_mappings[index - NumStaticMappings];
    }

    // Set the mapping.
    mapping->Set(client, server, size, state);

    return ResultSuccess;
}

Result KSessionRequest::SessionMappings::PushSend(VAddr client, VAddr server, size_t size,
                                                  KMemoryState state) {
    ASSERT(m_num_recv == 0);
    ASSERT(m_num_exch == 0);
    return this->PushMap(client, server, size, state, m_num_send++);
}

Result KSessionRequest::SessionMappings::PushReceive(VAddr client, VAddr server, size_t size,
                                                     KMemoryState state) {
    ASSERT(m_num_exch == 0);
    return this->PushMap(client, server, size, state, m_num_send + m_num_recv++);
}

Result KSessionRequest::SessionMappings::PushExchange(VAddr client, VAddr server, size_t size,
                                                      KMemoryState state) {
    return this->PushMap(client, server, size, state, m_num_send + m_num_recv + m_num_exch++);
}

void KSessionRequest::SessionMappings::Finalize() {
    if (m_mappings) {
        KPageBuffer::Free(kernel, reinterpret_cast<KPageBuffer*>(m_mappings));
        m_mappings = nullptr;
    }
}

} // namespace Kernel
