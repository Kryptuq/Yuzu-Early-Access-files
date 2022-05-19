// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/vi/layer/vi_layer.h"

namespace Service::VI {

Layer::Layer(u64 id, NVFlinger::BufferQueue& queue) : layer_id{id}, buffer_queue{queue} {}

Layer::~Layer() = default;

} // namespace Service::VI
