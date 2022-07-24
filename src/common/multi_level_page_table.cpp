#include "common/multi_level_page_table.inc"

namespace Common {
template class Common::MultiLevelPageTable<u64>;
template class Common::MultiLevelPageTable<u32>;
} // namespace Common
