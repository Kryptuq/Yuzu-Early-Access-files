// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/vi/layer/vi_layer.h"

namespace Service::VI {

Layer::Layer(u64 layer_id_, u32 binder_id_, android::BufferQueueCore& core_,
             android::BufferQueueProducer& binder_,
             std::shared_ptr<android::BufferItemConsumer>&& consumer_)
    : layer_id{layer_id_}, binder_id{binder_id_}, core{core_}, binder{binder_}, consumer{std::move(
                                                                                    consumer_)} {}

Layer::~Layer() = default;

} // namespace Service::VI
