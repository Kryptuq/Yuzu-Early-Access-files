// SPDX-FileCopyrightText: 2018 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace DiscordRPC {

class DiscordInterface {
public:
    virtual ~DiscordInterface() = default;

    virtual void Pause() = 0;
    virtual void Update() = 0;
};

class NullImpl : public DiscordInterface {
public:
    ~NullImpl() = default;

    void Pause() override {}
    void Update() override {}
};

} // namespace DiscordRPC
