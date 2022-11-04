// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/kernel/k_auto_object.h"
#include "core/hle/kernel/k_class_token.h"
#include "core/hle/kernel/k_client_port.h"
#include "core/hle/kernel/k_client_session.h"
#include "core/hle/kernel/k_code_memory.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/kernel/k_port.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_readable_event.h"
#include "core/hle/kernel/k_resource_limit.h"
#include "core/hle/kernel/k_server_port.h"
#include "core/hle/kernel/k_server_session.h"
#include "core/hle/kernel/k_session.h"
#include "core/hle/kernel/k_shared_memory.h"
#include "core/hle/kernel/k_synchronization_object.h"
#include "core/hle/kernel/k_system_resource.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/k_transfer_memory.h"

namespace Kernel {

// Ensure that we generate correct class tokens for all types.

// Ensure that the absolute token values are correct.
static_assert(ClassToken<KAutoObject> == 0b00000000'00000000);
static_assert(ClassToken<KSynchronizationObject> == 0b00000000'00000001);
static_assert(ClassToken<KReadableEvent> == 0b00000000'00000011);
// static_assert(ClassToken<KInterruptEvent> == 0b00000111'00000011);
// static_assert(ClassToken<KDebug> == 0b00001011'00000001);
static_assert(ClassToken<KThread> == 0b00010011'00000001);
static_assert(ClassToken<KServerPort> == 0b00100011'00000001);
static_assert(ClassToken<KServerSession> == 0b01000011'00000001);
static_assert(ClassToken<KClientPort> == 0b10000011'00000001);
static_assert(ClassToken<KClientSession> == 0b00001101'00000000);
static_assert(ClassToken<KProcess> == 0b00010101'00000001);
static_assert(ClassToken<KResourceLimit> == 0b00100101'00000000);
// static_assert(ClassToken<KLightSession> == 0b01000101'00000000);
static_assert(ClassToken<KPort> == 0b10000101'00000000);
static_assert(ClassToken<KSession> == 0b00011001'00000000);
static_assert(ClassToken<KSharedMemory> == 0b00101001'00000000);
static_assert(ClassToken<KEvent> == 0b01001001'00000000);
// static_assert(ClassToken<KLightClientSession> == 0b00110001'00000000);
// static_assert(ClassToken<KLightServerSession> == 0b01010001'00000000);
static_assert(ClassToken<KTransferMemory> == 0b01010001'00000000);
// static_assert(ClassToken<KDeviceAddressSpace> == 0b01100001'00000000);
// static_assert(ClassToken<KSessionRequest> == 0b10100001'00000000);
static_assert(ClassToken<KCodeMemory> == 0b10100001'00000000);

// Ensure that the token hierarchy is correct.

// Base classes
static_assert(ClassToken<KAutoObject> == (0b00000000));
static_assert(ClassToken<KSynchronizationObject> == (0b00000001 | ClassToken<KAutoObject>));
static_assert(ClassToken<KReadableEvent> == (0b00000010 | ClassToken<KSynchronizationObject>));

// Final classes
// static_assert(ClassToken<KInterruptEvent> == ((0b00000111 << 8) | ClassToken<KReadableEvent>));
// static_assert(ClassToken<KDebug> == ((0b00001011 << 8) | ClassToken<KSynchronizationObject>));
static_assert(ClassToken<KThread> == ((0b00010011 << 8) | ClassToken<KSynchronizationObject>));
static_assert(ClassToken<KServerPort> == ((0b00100011 << 8) | ClassToken<KSynchronizationObject>));
static_assert(ClassToken<KServerSession> ==
              ((0b01000011 << 8) | ClassToken<KSynchronizationObject>));
static_assert(ClassToken<KClientPort> == ((0b10000011 << 8) | ClassToken<KSynchronizationObject>));
static_assert(ClassToken<KClientSession> == ((0b00001101 << 8) | ClassToken<KAutoObject>));
static_assert(ClassToken<KProcess> == ((0b00010101 << 8) | ClassToken<KSynchronizationObject>));
static_assert(ClassToken<KResourceLimit> == ((0b00100101 << 8) | ClassToken<KAutoObject>));
// static_assert(ClassToken<KLightSession> == ((0b01000101 << 8) | ClassToken<KAutoObject>));
static_assert(ClassToken<KPort> == ((0b10000101 << 8) | ClassToken<KAutoObject>));
static_assert(ClassToken<KSession> == ((0b00011001 << 8) | ClassToken<KAutoObject>));
static_assert(ClassToken<KSharedMemory> == ((0b00101001 << 8) | ClassToken<KAutoObject>));
static_assert(ClassToken<KEvent> == ((0b01001001 << 8) | ClassToken<KAutoObject>));
// static_assert(ClassToken<KLightClientSession> == ((0b00110001 << 8) | ClassToken<KAutoObject>));
// static_assert(ClassToken<KLightServerSession> == ((0b01010001 << 8) | ClassToken<KAutoObject>));
static_assert(ClassToken<KTransferMemory> == ((0b01010001 << 8) | ClassToken<KAutoObject>));
// static_assert(ClassToken<KDeviceAddressSpace> == ((0b01100001 << 8) | ClassToken<KAutoObject>));
// static_assert(ClassToken<KSessionRequest> == ((0b10100001 << 8) | ClassToken<KAutoObject>));
static_assert(ClassToken<KCodeMemory> == ((0b10100001 << 8) | ClassToken<KAutoObject>));

// Ensure that the token hierarchy reflects the class hierarchy.

// Base classes.
static_assert(!std::is_final_v<KSynchronizationObject> &&
              std::is_base_of_v<KAutoObject, KSynchronizationObject>);
static_assert(!std::is_final_v<KReadableEvent> &&
              std::is_base_of_v<KSynchronizationObject, KReadableEvent>);

// Final classes
// static_assert(std::is_final_v<KInterruptEvent> &&
//              std::is_base_of_v<KReadableEvent, KInterruptEvent>);
// static_assert(std::is_final_v<KDebug> &&
//              std::is_base_of_v<KSynchronizationObject, KDebug>);
static_assert(std::is_final_v<KThread> && std::is_base_of_v<KSynchronizationObject, KThread>);
static_assert(std::is_final_v<KServerPort> &&
              std::is_base_of_v<KSynchronizationObject, KServerPort>);
static_assert(std::is_final_v<KServerSession> &&
              std::is_base_of_v<KSynchronizationObject, KServerSession>);
static_assert(std::is_final_v<KClientPort> &&
              std::is_base_of_v<KSynchronizationObject, KClientPort>);
static_assert(std::is_final_v<KClientSession> && std::is_base_of_v<KAutoObject, KClientSession>);
static_assert(std::is_final_v<KProcess> && std::is_base_of_v<KSynchronizationObject, KProcess>);
static_assert(std::is_final_v<KResourceLimit> && std::is_base_of_v<KAutoObject, KResourceLimit>);
// static_assert(std::is_final_v<KLightSession> &&
//              std::is_base_of_v<KAutoObject, KLightSession>);
static_assert(std::is_final_v<KPort> && std::is_base_of_v<KAutoObject, KPort>);
static_assert(std::is_final_v<KSession> && std::is_base_of_v<KAutoObject, KSession>);
static_assert(std::is_final_v<KSharedMemory> && std::is_base_of_v<KAutoObject, KSharedMemory>);
static_assert(std::is_final_v<KEvent> && std::is_base_of_v<KAutoObject, KEvent>);
// static_assert(std::is_final_v<KLightClientSession> &&
//              std::is_base_of_v<KAutoObject, KLightClientSession>);
// static_assert(std::is_final_v<KLightServerSession> &&
//              std::is_base_of_v<KAutoObject, KLightServerSession>);
static_assert(std::is_final_v<KTransferMemory> && std::is_base_of_v<KAutoObject, KTransferMemory>);
// static_assert(std::is_final_v<KDeviceAddressSpace> &&
//              std::is_base_of_v<KAutoObject, KDeviceAddressSpace>);
// static_assert(std::is_final_v<KSessionRequest> &&
//              std::is_base_of_v<KAutoObject, KSessionRequest>);
// static_assert(std::is_final_v<KCodeMemory> &&
//              std::is_base_of_v<KAutoObject, KCodeMemory>);

static_assert(std::is_base_of<KAutoObject, KSystemResource>::value);

} // namespace Kernel
