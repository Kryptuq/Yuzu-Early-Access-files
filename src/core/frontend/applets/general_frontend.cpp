// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/frontend/applets/general_frontend.h"

namespace Core::Frontend {

ParentalControlsApplet::~ParentalControlsApplet() = default;

DefaultParentalControlsApplet::~DefaultParentalControlsApplet() = default;

void DefaultParentalControlsApplet::VerifyPIN(std::function<void(bool)> finished,
                                              bool suspend_future_verification_temporarily) {
    LOG_INFO(Service_AM,
             "Application requested frontend to verify PIN (normal), "
             "suspend_future_verification_temporarily={}, verifying as correct.",
             suspend_future_verification_temporarily);
    finished(true);
}

void DefaultParentalControlsApplet::VerifyPINForSettings(std::function<void(bool)> finished) {
    LOG_INFO(Service_AM,
             "Application requested frontend to verify PIN (settings), verifying as correct.");
    finished(true);
}

void DefaultParentalControlsApplet::RegisterPIN(std::function<void()> finished) {
    LOG_INFO(Service_AM, "Application requested frontend to register new PIN");
    finished();
}

void DefaultParentalControlsApplet::ChangePIN(std::function<void()> finished) {
    LOG_INFO(Service_AM, "Application requested frontend to change PIN to new value");
    finished();
}

PhotoViewerApplet::~PhotoViewerApplet() = default;

DefaultPhotoViewerApplet::~DefaultPhotoViewerApplet() = default;

void DefaultPhotoViewerApplet::ShowPhotosForApplication(u64 title_id,
                                                        std::function<void()> finished) const {
    LOG_INFO(Service_AM,
             "Application requested frontend to display stored photos for title_id={:016X}",
             title_id);
    finished();
}

void DefaultPhotoViewerApplet::ShowAllPhotos(std::function<void()> finished) const {
    LOG_INFO(Service_AM, "Application requested frontend to display all stored photos.");
    finished();
}

} // namespace Core::Frontend
