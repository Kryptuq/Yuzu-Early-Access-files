// SPDX-FileCopyrightText: Copyright 2017 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "yuzu/util/clickable_label.h"

ClickableLabel::ClickableLabel(QWidget* parent, [[maybe_unused]] Qt::WindowFlags f)
    : QLabel(parent) {}

void ClickableLabel::mouseReleaseEvent([[maybe_unused]] QMouseEvent* event) {
    emit clicked();
}
