// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace AudioCore {
class AudioManager;

namespace Sink {

class Sink;

/**
 * Retrieves the IDs for all available audio sinks.
 *
 * @return Vector of available sink names.
 */
std::vector<std::string_view> GetSinkIDs();

/**
 * Gets the list of devices for a particular sink identified by the given ID.
 *
 * @param sink_id - Id of the sink to get devices from.
 * @param capture - Get capture (input) devices, or output devices?
 * @return Vector of device names.
 */
std::vector<std::string> GetDeviceListForSink(std::string_view sink_id, bool capture);

/**
 * Creates an audio sink identified by the given device ID.
 *
 * @param sink_id   - Id of the sink to create.
 * @param device_id - Name of the device to create.
 * @return Pointer to the created sink.
 */
std::unique_ptr<Sink> CreateSinkFromID(std::string_view sink_id, std::string_view device_id);

} // namespace Sink
} // namespace AudioCore
