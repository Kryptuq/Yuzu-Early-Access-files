// Copyright 2022 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <functional>

#include "core/hle/result.h"
#include "core/hle/service/mii/mii_manager.h"

namespace Core::Frontend {

struct MiiParameters {
    bool is_editable;
    Service::Mii::MiiInfo mii_data{};
};

class MiiApplet {
public:
    virtual ~MiiApplet();

    virtual void ShowMii(const MiiParameters& parameters,
                         const std::function<void(const Core::Frontend::MiiParameters& parameters)>
                             callback) const = 0;
};

class DefaultMiiApplet final : public MiiApplet {
public:
    void ShowMii(const MiiParameters& parameters,
                 const std::function<void(const Core::Frontend::MiiParameters& parameters)>
                     callback) const override;
};

} // namespace Core::Frontend
