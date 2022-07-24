// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/microprofile.h"

namespace VideoCommon {

MICROPROFILE_DEFINE(GPU_PrepareBuffers, "GPU", "Prepare buffers", MP_RGB(224, 128, 128));
MICROPROFILE_DEFINE(GPU_BindUploadBuffers, "GPU", "Bind and upload buffers", MP_RGB(224, 128, 128));
MICROPROFILE_DEFINE(GPU_DownloadMemory, "GPU", "Download buffers", MP_RGB(224, 128, 128));

} // namespace VideoCommon
