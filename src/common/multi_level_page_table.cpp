// SPDX-FileCopyrightText: 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/multi_level_page_table.inc"

namespace Common {
template class Common::MultiLevelPageTable<u64>;
template class Common::MultiLevelPageTable<u32>;
} // namespace Common
