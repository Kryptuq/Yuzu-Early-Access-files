// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/frontend/applets/mii_edit.h"

namespace Core::Frontend {

MiiEditApplet::~MiiEditApplet() = default;

void DefaultMiiEditApplet::ShowMiiEdit(const MiiEditCallback& callback) const {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    callback();
}

} // namespace Core::Frontend
