// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <utility>

#include "common/common_types.h"
#include "core/hle/service/sockets/sockets.h"
#include "core/network/network.h"

namespace Service::Sockets {

/// Translate abstract errno to guest errno
Errno Translate(Network::Errno value);

/// Translate abstract return value errno pair to guest return value errno pair
std::pair<s32, Errno> Translate(std::pair<s32, Network::Errno> value);

/// Translate guest domain to abstract domain
Network::Domain Translate(Domain domain);

/// Translate abstract domain to guest domain
Domain Translate(Network::Domain domain);

/// Translate guest type to abstract type
Network::Type Translate(Type type);

/// Translate guest protocol to abstract protocol
Network::Protocol Translate(Type type, Protocol protocol);

/// Translate abstract poll event flags to guest poll event flags
Network::PollEvents TranslatePollEventsToHost(PollEvents flags);

/// Translate guest poll event flags to abstract poll event flags
PollEvents TranslatePollEventsToGuest(Network::PollEvents flags);

/// Translate guest socket address structure to abstract socket address structure
Network::SockAddrIn Translate(SockAddrIn value);

/// Translate abstract socket address structure to guest socket address structure
SockAddrIn Translate(Network::SockAddrIn value);

/// Translate guest shutdown mode to abstract shutdown mode
Network::ShutdownHow Translate(ShutdownHow how);

} // namespace Service::Sockets
