// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <string>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#endif
#include <zip.h>
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include "common/fs/path_util.h"
#include "common/logging/backend.h"
#include "core/file_sys/vfs.h"
#include "core/file_sys/vfs_libzip.h"
#include "core/file_sys/vfs_vector.h"

namespace FileSys {

VirtualDir ExtractZIP(VirtualFile file) {
    zip_error_t error{};

    const auto data = file->ReadAllBytes();
    std::unique_ptr<zip_source_t, decltype(&zip_source_close)> src{
        zip_source_buffer_create(data.data(), data.size(), 0, &error), zip_source_close};
    if (src == nullptr)
        return nullptr;

    std::unique_ptr<zip_t, decltype(&zip_close)> zip{zip_open_from_source(src.get(), 0, &error),
                                                     zip_close};
    if (zip == nullptr)
        return nullptr;

    std::shared_ptr<VectorVfsDirectory> out = std::make_shared<VectorVfsDirectory>();

    const auto num_entries = static_cast<std::size_t>(zip_get_num_entries(zip.get(), 0));

    zip_stat_t stat{};
    zip_stat_init(&stat);

    for (std::size_t i = 0; i < num_entries; ++i) {
        const auto stat_res = zip_stat_index(zip.get(), i, 0, &stat);
        if (stat_res == -1)
            return nullptr;

        const std::string name(stat.name);
        if (name.empty())
            continue;

        if (name.back() != '/') {
            std::unique_ptr<zip_file_t, decltype(&zip_fclose)> file2{
                zip_fopen_index(zip.get(), i, 0), zip_fclose};

            std::vector<u8> buf(stat.size);
            if (zip_fread(file2.get(), buf.data(), buf.size()) != s64(buf.size()))
                return nullptr;

            const auto parts = Common::FS::SplitPathComponents(stat.name);
            const auto new_file = std::make_shared<VectorVfsFile>(buf, parts.back());

            std::shared_ptr<VectorVfsDirectory> dtrv = out;
            for (std::size_t j = 0; j < parts.size() - 1; ++j) {
                if (dtrv == nullptr)
                    return nullptr;
                const auto subdir = dtrv->GetSubdirectory(parts[j]);
                if (subdir == nullptr) {
                    const auto temp = std::make_shared<VectorVfsDirectory>(
                        std::vector<VirtualFile>{}, std::vector<VirtualDir>{}, parts[j]);
                    dtrv->AddDirectory(temp);
                    dtrv = temp;
                } else {
                    dtrv = std::dynamic_pointer_cast<VectorVfsDirectory>(subdir);
                }
            }

            if (dtrv == nullptr)
                return nullptr;
            dtrv->AddFile(new_file);
        }
    }

    return out;
}

} // namespace FileSys
