// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <glad/glad.h>

#include "common/assert.h"
#include "common/common_types.h"
#include "core/file_sys/vfs_vector.h"
#include "video_core/engines/shader_type.h"
#include "video_core/shader/registry.h"

namespace Common::FS {
class IOFile;
}

namespace OpenGL {

using ProgramCode = std::vector<u64>;

/// Describes a shader and how it's used by the guest GPU
struct ShaderDiskCacheEntry {
    ShaderDiskCacheEntry();
    ~ShaderDiskCacheEntry();

    bool Load(Common::FS::IOFile& file);

    bool Save(Common::FS::IOFile& file) const;

    bool HasProgramA() const {
        return !code.empty() && !code_b.empty();
    }

    Tegra::Engines::ShaderType type{};
    ProgramCode code;
    ProgramCode code_b;

    u64 unique_identifier = 0;
    std::optional<u32> texture_handler_size;
    u32 bound_buffer = 0;
    VideoCommon::Shader::GraphicsInfo graphics_info;
    VideoCommon::Shader::ComputeInfo compute_info;
    VideoCommon::Shader::KeyMap keys;
    VideoCommon::Shader::BoundSamplerMap bound_samplers;
    VideoCommon::Shader::SeparateSamplerMap separate_samplers;
    VideoCommon::Shader::BindlessSamplerMap bindless_samplers;
};

/// Contains an OpenGL dumped binary program
struct ShaderDiskCachePrecompiled {
    u64 unique_identifier = 0;
    GLenum binary_format = 0;
    std::vector<u8> binary;
};

class ShaderDiskCacheOpenGL {
public:
    explicit ShaderDiskCacheOpenGL();
    ~ShaderDiskCacheOpenGL();

    /// Binds a title ID for all future operations.
    void BindTitleID(u64 title_id);

    /// Loads transferable cache. If file has a old version or on failure, it deletes the file.
    std::optional<std::vector<ShaderDiskCacheEntry>> LoadTransferable();

    /// Loads current game's precompiled cache. Invalidates on failure.
    std::vector<ShaderDiskCachePrecompiled> LoadPrecompiled();

    /// Removes the transferable (and precompiled) cache file.
    void InvalidateTransferable();

    /// Removes the precompiled cache file and clears virtual precompiled cache file.
    void InvalidatePrecompiled();

    /// Saves a raw dump to the transferable file. Checks for collisions.
    void SaveEntry(const ShaderDiskCacheEntry& entry);

    /// Saves a dump entry to the precompiled file. Does not check for collisions.
    void SavePrecompiled(u64 unique_identifier, GLuint program);

    /// Serializes virtual precompiled shader cache file to real file
    void SaveVirtualPrecompiledFile();

private:
    /// Loads the transferable cache. Returns empty on failure.
    std::optional<std::vector<ShaderDiskCachePrecompiled>> LoadPrecompiledFile(
        Common::FS::IOFile& file);

    /// Opens current game's transferable file and write it's header if it doesn't exist
    Common::FS::IOFile AppendTransferableFile() const;

    /// Save precompiled header to precompiled_cache_in_memory
    void SavePrecompiledHeaderToVirtualPrecompiledCache();

    /// Create shader disk cache directories. Returns true on success.
    bool EnsureDirectories() const;

    /// Gets current game's transferable file path
    std::filesystem::path GetTransferablePath() const;

    /// Gets current game's precompiled file path
    std::filesystem::path GetPrecompiledPath() const;

    /// Get user's transferable directory path
    std::filesystem::path GetTransferableDir() const;

    /// Get user's precompiled directory path
    std::filesystem::path GetPrecompiledDir() const;

    /// Get user's shader directory path
    std::filesystem::path GetBaseDir() const;

    /// Get current game's title id
    std::string GetTitleID() const;

    template <typename T>
    bool SaveArrayToPrecompiled(const T* data, std::size_t length) {
        const std::size_t write_length = precompiled_cache_virtual_file.WriteArray(
            data, length, precompiled_cache_virtual_file_offset);
        precompiled_cache_virtual_file_offset += write_length;
        return write_length == sizeof(T) * length;
    }

    template <typename T>
    bool LoadArrayFromPrecompiled(T* data, std::size_t length) {
        const std::size_t read_length = precompiled_cache_virtual_file.ReadArray(
            data, length, precompiled_cache_virtual_file_offset);
        precompiled_cache_virtual_file_offset += read_length;
        return read_length == sizeof(T) * length;
    }

    template <typename T>
    bool SaveObjectToPrecompiled(const T& object) {
        return SaveArrayToPrecompiled(&object, 1);
    }

    bool SaveObjectToPrecompiled(bool object) {
        const auto value = static_cast<u8>(object);
        return SaveArrayToPrecompiled(&value, 1);
    }

    template <typename T>
    bool LoadObjectFromPrecompiled(T& object) {
        return LoadArrayFromPrecompiled(&object, 1);
    }

    // Stores whole precompiled cache which will be read from or saved to the precompiled chache
    // file
    FileSys::VectorVfsFile precompiled_cache_virtual_file;
    // Stores the current offset of the precompiled cache file for IO purposes
    std::size_t precompiled_cache_virtual_file_offset = 0;

    // Stored transferable shaders
    std::unordered_set<u64> stored_transferable;

    /// Title ID to operate on
    u64 title_id = 0;

    // The cache has been loaded at boot
    bool is_usable = false;
};

} // namespace OpenGL
