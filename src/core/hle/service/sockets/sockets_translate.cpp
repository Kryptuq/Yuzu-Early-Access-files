// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <utility>

#include "common/assert.h"
#include "common/common_types.h"
#include "core/hle/service/sockets/sockets.h"
#include "core/hle/service/sockets/sockets_translate.h"
#include "core/internal_network/network.h"

namespace Service::Sockets {

Errno Translate(Network::Errno value) {
    switch (value) {
    case Network::Errno::SUCCESS:
        return Errno::SUCCESS;
    case Network::Errno::BADF:
        return Errno::BADF;
    case Network::Errno::AGAIN:
        return Errno::AGAIN;
    case Network::Errno::INVAL:
        return Errno::INVAL;
    case Network::Errno::MFILE:
        return Errno::MFILE;
    case Network::Errno::NOTCONN:
        return Errno::NOTCONN;
    case Network::Errno::TIMEDOUT:
        return Errno::TIMEDOUT;
    case Network::Errno::CONNRESET:
        return Errno::CONNRESET;
    default:
        UNIMPLEMENTED_MSG("Unimplemented errno={}", value);
        return Errno::SUCCESS;
    }
}

std::pair<s32, Errno> Translate(std::pair<s32, Network::Errno> value) {
    return {value.first, Translate(value.second)};
}

Network::Domain Translate(Domain domain) {
    switch (domain) {
    case Domain::INET:
        return Network::Domain::INET;
    default:
        UNIMPLEMENTED_MSG("Unimplemented domain={}", domain);
        return {};
    }
}

Domain Translate(Network::Domain domain) {
    switch (domain) {
    case Network::Domain::INET:
        return Domain::INET;
    default:
        UNIMPLEMENTED_MSG("Unimplemented domain={}", domain);
        return {};
    }
}

Network::Type Translate(Type type) {
    switch (type) {
    case Type::STREAM:
        return Network::Type::STREAM;
    case Type::DGRAM:
        return Network::Type::DGRAM;
    default:
        UNIMPLEMENTED_MSG("Unimplemented type={}", type);
        return Network::Type{};
    }
}

Network::Protocol Translate(Type type, Protocol protocol) {
    switch (protocol) {
    case Protocol::UNSPECIFIED:
        LOG_WARNING(Service, "Unspecified protocol, assuming protocol from type");
        switch (type) {
        case Type::DGRAM:
            return Network::Protocol::UDP;
        case Type::STREAM:
            return Network::Protocol::TCP;
        default:
            return Network::Protocol::TCP;
        }
    case Protocol::TCP:
        return Network::Protocol::TCP;
    case Protocol::UDP:
        return Network::Protocol::UDP;
    default:
        UNIMPLEMENTED_MSG("Unimplemented protocol={}", protocol);
        return Network::Protocol::TCP;
    }
}

Network::PollEvents TranslatePollEventsToHost(PollEvents flags) {
    Network::PollEvents result{};
    const auto translate = [&result, &flags](PollEvents from, Network::PollEvents to) {
        if (True(flags & from)) {
            flags &= ~from;
            result |= to;
        }
    };
    translate(PollEvents::In, Network::PollEvents::In);
    translate(PollEvents::Pri, Network::PollEvents::Pri);
    translate(PollEvents::Out, Network::PollEvents::Out);
    translate(PollEvents::Err, Network::PollEvents::Err);
    translate(PollEvents::Hup, Network::PollEvents::Hup);
    translate(PollEvents::Nval, Network::PollEvents::Nval);

    UNIMPLEMENTED_IF_MSG((u16)flags != 0, "Unimplemented flags={}", (u16)flags);
    return result;
}

PollEvents TranslatePollEventsToGuest(Network::PollEvents flags) {
    PollEvents result{};
    const auto translate = [&result, &flags](Network::PollEvents from, PollEvents to) {
        if (True(flags & from)) {
            flags &= ~from;
            result |= to;
        }
    };

    translate(Network::PollEvents::In, PollEvents::In);
    translate(Network::PollEvents::Pri, PollEvents::Pri);
    translate(Network::PollEvents::Out, PollEvents::Out);
    translate(Network::PollEvents::Err, PollEvents::Err);
    translate(Network::PollEvents::Hup, PollEvents::Hup);
    translate(Network::PollEvents::Nval, PollEvents::Nval);

    UNIMPLEMENTED_IF_MSG((u16)flags != 0, "Unimplemented flags={}", (u16)flags);
    return result;
}

Network::SockAddrIn Translate(SockAddrIn value) {
    ASSERT(value.len == 0 || value.len == sizeof(value));

    return {
        .family = Translate(static_cast<Domain>(value.family)),
        .ip = value.ip,
        .portno = static_cast<u16>(value.portno >> 8 | value.portno << 8),
    };
}

SockAddrIn Translate(Network::SockAddrIn value) {
    return {
        .len = sizeof(SockAddrIn),
        .family = static_cast<u8>(Translate(value.family)),
        .portno = static_cast<u16>(value.portno >> 8 | value.portno << 8),
        .ip = value.ip,
        .zeroes = {},
    };
}

Network::ShutdownHow Translate(ShutdownHow how) {
    switch (how) {
    case ShutdownHow::RD:
        return Network::ShutdownHow::RD;
    case ShutdownHow::WR:
        return Network::ShutdownHow::WR;
    case ShutdownHow::RDWR:
        return Network::ShutdownHow::RDWR;
    default:
        UNIMPLEMENTED_MSG("Unimplemented how={}", how);
        return {};
    }
}

} // namespace Service::Sockets
