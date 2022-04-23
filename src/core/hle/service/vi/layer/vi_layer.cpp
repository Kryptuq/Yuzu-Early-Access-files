// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/vi/layer/vi_layer.h"

namespace Service::VI {

Layer::Layer(u64 layer_id_, u32 binder_id_, android::BufferQueueCore& core_,
             android::BufferQueueProducer& binder_,
             std::shared_ptr<android::BufferItemConsumer>&& consumer_)
    : layer_id{layer_id_}, binder_id{binder_id_}, core{core_}, binder{binder_}, consumer{std::move(
                                                                                    consumer_)} {}

Layer::~Layer() = default;

} // namespace Service::VI
