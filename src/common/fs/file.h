// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <span>
#include <type_traits>
#include <vector>

#include "common/concepts.h"
#include "common/fs/fs_types.h"
#include "common/fs/fs_util.h"

namespace Common::FS {

enum class SeekOrigin {
    SetOrigin,       // Seeks from the start of the file.
    CurrentPosition, // Seeks from the current file pointer position.
    End,             // Seeks from the end of the file.
};

/**
 * Opens a file stream at path with the specified open mode.
 *
 * @param file_stream Reference to file stream
 * @param path Filesystem path
 * @param open_mode File stream open mode
 */
template <typename FileStream>
void OpenFileStream(FileStream& file_stream, const std::filesystem::path& path,
                    std::ios_base::openmode open_mode) {
    file_stream.open(path, open_mode);
}

#ifdef _WIN32
template <typename FileStream, typename Path>
void OpenFileStream(FileStream& file_stream, const Path& path, std::ios_base::openmode open_mode) {
    if constexpr (IsChar<typename Path::value_type>) {
        file_stream.open(ToU8String(path), open_mode);
    } else {
        file_stream.open(std::filesystem::path{path}, open_mode);
    }
}
#endif

/**
 * Reads an entire file at path and returns a string of the contents read from the file.
 * If the filesystem object at path is not a file, this function returns an empty string.
 *
 * @param path Filesystem path
 * @param type File type
 *
 * @returns A string of the contents read from the file.
 */
[[nodiscard]] std::string ReadStringFromFile(const std::filesystem::path& path, FileType type);

#ifdef _WIN32
template <typename Path>
[[nodiscard]] std::string ReadStringFromFile(const Path& path, FileType type) {
    if constexpr (IsChar<typename Path::value_type>) {
        return ReadStringFromFile(ToU8String(path), type);
    } else {
        return ReadStringFromFile(std::filesystem::path{path}, type);
    }
}
#endif

/**
 * Writes a string to a file at path and returns the number of characters successfully written.
 * If an file already exists at path, its contents will be erased.
 * If the filesystem object at path is not a file, this function returns 0.
 *
 * @param path Filesystem path
 * @param type File type
 *
 * @returns Number of characters successfully written.
 */
[[nodiscard]] size_t WriteStringToFile(const std::filesystem::path& path, FileType type,
                                       std::string_view string);

#ifdef _WIN32
template <typename Path>
[[nodiscard]] size_t WriteStringToFile(const Path& path, FileType type, std::string_view string) {
    if constexpr (IsChar<typename Path::value_type>) {
        return WriteStringToFile(ToU8String(path), type, string);
    } else {
        return WriteStringToFile(std::filesystem::path{path}, type, string);
    }
}
#endif

/**
 * Appends a string to a file at path and returns the number of characters successfully written.
 * If a file does not exist at path, WriteStringToFile is called instead.
 * If the filesystem object at path is not a file, this function returns 0.
 *
 * @param path Filesystem path
 * @param type File type
 *
 * @returns Number of characters successfully written.
 */
[[nodiscard]] size_t AppendStringToFile(const std::filesystem::path& path, FileType type,
                                        std::string_view string);

#ifdef _WIN32
template <typename Path>
[[nodiscard]] size_t AppendStringToFile(const Path& path, FileType type, std::string_view string) {
    if constexpr (IsChar<typename Path::value_type>) {
        return AppendStringToFile(ToU8String(path), type, string);
    } else {
        return AppendStringToFile(std::filesystem::path{path}, type, string);
    }
}
#endif

class IOFile final {
public:
    IOFile();

    explicit IOFile(const std::string& path, FileAccessMode mode,
                    FileType type = FileType::BinaryFile,
                    FileShareFlag flag = FileShareFlag::ShareReadOnly);

    explicit IOFile(std::string_view path, FileAccessMode mode,
                    FileType type = FileType::BinaryFile,
                    FileShareFlag flag = FileShareFlag::ShareReadOnly);

    /**
     * An IOFile is a lightweight wrapper on C Library file operations.
     * Automatically closes an open file on the destruction of an IOFile object.
     *
     * @param path Filesystem path
     * @param mode File access mode
     * @param type File type, default is BinaryFile. Use TextFile to open the file as a text file
     * @param flag (Windows only) File-share access flag, default is ShareReadOnly
     */
    explicit IOFile(const std::filesystem::path& path, FileAccessMode mode,
                    FileType type = FileType::BinaryFile,
                    FileShareFlag flag = FileShareFlag::ShareReadOnly);

    ~IOFile();

    IOFile(const IOFile&) = delete;
    IOFile& operator=(const IOFile&) = delete;

    IOFile(IOFile&& other) noexcept;
    IOFile& operator=(IOFile&& other) noexcept;

    /**
     * Gets the path of the file.
     *
     * @returns The path of the file.
     */
    [[nodiscard]] std::filesystem::path GetPath() const;

    /**
     * Gets the access mode of the file.
     *
     * @returns The access mode of the file.
     */
    [[nodiscard]] FileAccessMode GetAccessMode() const;

    /**
     * Gets the type of the file.
     *
     * @returns The type of the file.
     */
    [[nodiscard]] FileType GetType() const;

    /**
     * Opens a file at path with the specified file access mode.
     * This function behaves differently depending on the FileAccessMode.
     * These behaviors are documented in each enum value of FileAccessMode.
     *
     * @param path Filesystem path
     * @param mode File access mode
     * @param type File type, default is BinaryFile. Use TextFile to open the file as a text file
     * @param flag (Windows only) File-share access flag, default is ShareReadOnly
     */
    void Open(const std::filesystem::path& path, FileAccessMode mode,
              FileType type = FileType::BinaryFile,
              FileShareFlag flag = FileShareFlag::ShareReadOnly);

#ifdef _WIN32
    template <typename Path>
    [[nodiscard]] void Open(const Path& path, FileAccessMode mode,
                            FileType type = FileType::BinaryFile,
                            FileShareFlag flag = FileShareFlag::ShareReadOnly) {
        using ValueType = typename Path::value_type;
        if constexpr (IsChar<ValueType>) {
            Open(ToU8String(path), mode, type, flag);
        } else {
            Open(std::filesystem::path{path}, mode, type, flag);
        }
    }
#endif

    /// Closes the file if it is opened.
    void Close();

    /**
     * Checks whether the file is open.
     * Use this to check whether the calls to Open() or Close() succeeded.
     *
     * @returns True if the file is open, false otherwise.
     */
    [[nodiscard]] bool IsOpen() const;

    /**
     * Helper function which deduces the value type of a contiguous STL container used in ReadSpan.
     * If T is not a contiguous STL container as defined by the concept IsSTLContainer, this calls
     * ReadObject and T must be a trivially copyable object.
     *
     * See ReadSpan for more details if T is a contiguous container.
     * See ReadObject for more details if T is a trivially copyable object.
     *
     * @tparam T Contiguous container or trivially copyable object
     *
     * @param data Container of T::value_type data or reference to object
     *
     * @returns Count of T::value_type data or objects successfully read.
     */
    template <typename T>
    [[nodiscard]] size_t Read(T& data) const {
        if constexpr (IsSTLContainer<T>) {
            using ContiguousType = typename T::value_type;
            static_assert(std::is_trivially_copyable_v<ContiguousType>,
                          "Data type must be trivially copyable.");
            return ReadSpan<ContiguousType>(data);
        } else {
            return ReadObject(data) ? 1 : 0;
        }
    }

    /**
     * Helper function which deduces the value type of a contiguous STL container used in WriteSpan.
     * If T is not a contiguous STL container as defined by the concept IsSTLContainer, this calls
     * WriteObject and T must be a trivially copyable object.
     *
     * See WriteSpan for more details if T is a contiguous container.
     * See WriteObject for more details if T is a trivially copyable object.
     *
     * @tparam T Contiguous container or trivially copyable object
     *
     * @param data Container of T::value_type data or const reference to object
     *
     * @returns Count of T::value_type data or objects successfully written.
     */
    template <typename T>
    [[nodiscard]] size_t Write(const T& data) const {
        if constexpr (IsSTLContainer<T>) {
            using ContiguousType = typename T::value_type;
            static_assert(std::is_trivially_copyable_v<ContiguousType>,
                          "Data type must be trivially copyable.");
            return WriteSpan<ContiguousType>(data);
        } else {
            static_assert(std::is_trivially_copyable_v<T>, "Data type must be trivially copyable.");
            return WriteObject(data) ? 1 : 0;
        }
    }

    /**
     * Reads a span of T data from a file sequentially.
     * This function reads from the current position of the file pointer and
     * advances it by the (count of T * sizeof(T)) bytes successfully read.
     *
     * Failures occur when:
     * - The file is not open
     * - The opened file lacks read permissions
     * - Attempting to read beyond the end-of-file
     *
     * @tparam T Data type
     *
     * @param data Span of T data
     *
     * @returns Count of T data successfully read.
     */
    template <typename T>
    [[nodiscard]] size_t ReadSpan(std::span<T> data) const {
        static_assert(std::is_trivially_copyable_v<T>, "Data type must be trivially copyable.");

        if (!IsOpen()) {
            return 0;
        }

        return std::fread(data.data(), sizeof(T), data.size(), file);
    }

    /**
     * Writes a span of T data to a file sequentially.
     * This function writes from the current position of the file pointer and
     * advances it by the (count of T * sizeof(T)) bytes successfully written.
     *
     * Failures occur when:
     * - The file is not open
     * - The opened file lacks write permissions
     *
     * @tparam T Data type
     *
     * @param data Span of T data
     *
     * @returns Count of T data successfully written.
     */
    template <typename T>
    [[nodiscard]] size_t WriteSpan(std::span<const T> data) const {
        static_assert(std::is_trivially_copyable_v<T>, "Data type must be trivially copyable.");

        if (!IsOpen()) {
            return 0;
        }

        return std::fwrite(data.data(), sizeof(T), data.size(), file);
    }

    /**
     * Reads a T object from a file sequentially.
     * This function reads from the current position of the file pointer and
     * advances it by the sizeof(T) bytes successfully read.
     *
     * Failures occur when:
     * - The file is not open
     * - The opened file lacks read permissions
     * - Attempting to read beyond the end-of-file
     *
     * @tparam T Data type
     *
     * @param object Reference to object
     *
     * @returns True if the object is successfully read from the file, false otherwise.
     */
    template <typename T>
    [[nodiscard]] bool ReadObject(T& object) const {
        static_assert(std::is_trivially_copyable_v<T>, "Data type must be trivially copyable.");
        static_assert(!std::is_pointer_v<T>, "T must not be a pointer to an object.");

        if (!IsOpen()) {
            return false;
        }

        return std::fread(&object, sizeof(T), 1, file) == 1;
    }

    /**
     * Writes a T object to a file sequentially.
     * This function writes from the current position of the file pointer and
     * advances it by the sizeof(T) bytes successfully written.
     *
     * Failures occur when:
     * - The file is not open
     * - The opened file lacks write permissions
     *
     * @tparam T Data type
     *
     * @param object Const reference to object
     *
     * @returns True if the object is successfully written to the file, false otherwise.
     */
    template <typename T>
    [[nodiscard]] bool WriteObject(const T& object) const {
        static_assert(std::is_trivially_copyable_v<T>, "Data type must be trivially copyable.");
        static_assert(!std::is_pointer_v<T>, "T must not be a pointer to an object.");

        if (!IsOpen()) {
            return false;
        }

        return std::fwrite(&object, sizeof(T), 1, file) == 1;
    }

    /**
     * Specialized function to read a string of a given length from a file sequentially.
     * This function writes from the current position of the file pointer and
     * advances it by the number of characters successfully read.
     * The size of the returned string may not match length if not all bytes are successfully read.
     *
     * @param length Length of the string
     *
     * @returns A string read from the file.
     */
    [[nodiscard]] std::string ReadString(size_t length) const;

    /**
     * Specialized function to write a string to a file sequentially.
     * This function writes from the current position of the file pointer and
     * advances it by the number of characters successfully written.
     *
     * @param string Span of const char backed std::string or std::string_view
     *
     * @returns Number of characters successfully written.
     */
    [[nodiscard]] size_t WriteString(std::span<const char> string) const;

    /**
     * Flushes any unwritten buffered data into the file.
     *
     * @returns True if the flush was successful, false otherwise.
     */
    [[nodiscard]] bool Flush() const;

    /**
     * Resizes the file to a given size.
     * If the file is resized to a smaller size, the remainder of the file is discarded.
     * If the file is resized to a larger size, the new area appears as if zero-filled.
     *
     * Failures occur when:
     * - The file is not open
     *
     * @param size File size in bytes
     *
     * @returns True if the file resize succeeded, false otherwise.
     */
    [[nodiscard]] bool SetSize(u64 size) const;

    /**
     * Gets the size of the file.
     *
     * Failures occur when:
     * - The file is not open
     *
     * @returns The file size in bytes of the file. Returns 0 on failure.
     */
    [[nodiscard]] u64 GetSize() const;

    /**
     * Moves the current position of the file pointer with the specified offset and seek origin.
     *
     * @param offset Offset from seek origin
     * @param origin Seek origin
     *
     * @returns True if the file pointer has moved to the specified offset, false otherwise.
     */
    [[nodiscard]] bool Seek(s64 offset, SeekOrigin origin = SeekOrigin::SetOrigin) const;

    /**
     * Gets the current position of the file pointer.
     *
     * @returns The current position of the file pointer.
     */
    [[nodiscard]] s64 Tell() const;

private:
    std::filesystem::path file_path;
    FileAccessMode file_access_mode{};
    FileType file_type{};

    std::FILE* file = nullptr;
};

} // namespace Common::FS
