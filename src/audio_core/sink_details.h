// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace AudioCore {

class Sink;

/// Retrieves the IDs for all available audio sinks.
std::vector<const char*> GetSinkIDs();

/// Gets the list of devices for a particular sink identified by the given ID.
std::vector<std::string> GetDeviceListForSink(std::string_view sink_id);

/// Creates an audio sink identified by the given device ID.
std::unique_ptr<Sink> CreateSinkFromID(std::string_view sink_id, std::string_view device_id);

} // namespace AudioCore
