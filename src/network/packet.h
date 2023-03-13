// SPDX-FileCopyrightText: Copyright 2017 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <vector>
#include "common/common_types.h"

namespace Network {

/// A class that serializes data for network transfer. It also handles endianness
class Packet {
public:
    Packet() = default;
    ~Packet() = default;

    /**
     * Append data to the end of the packet
     * @param data        Pointer to the sequence of bytes to append
     * @param size_in_bytes Number of bytes to append
     */
    void Append(const void* data, std::size_t size_in_bytes);

    /**
     * Reads data from the current read position of the packet
     * @param out_data        Pointer where the data should get written to
     * @param size_in_bytes Number of bytes to read
     */
    void Read(void* out_data, std::size_t size_in_bytes);

    /**
     * Clear the packet
     * After calling Clear, the packet is empty.
     */
    void Clear();

    /**
     * Ignores bytes while reading
     * @param length THe number of bytes to ignore
     */
    void IgnoreBytes(u32 length);

    /**
     * Get a pointer to the data contained in the packet
     * @return Pointer to the data
     */
    const void* GetData() const;

    /**
     * This function returns the number of bytes pointed to by
     * what getData returns.
     * @return Data size, in bytes
     */
    std::size_t GetDataSize() const;

    /**
     * This function is useful to know if there is some data
     * left to be read, without actually reading it.
     * @return True if all data was read, false otherwise
     */
    bool EndOfPacket() const;

    explicit operator bool() const;

    /// Overloads of read function to read data from the packet
    Packet& Read(bool& out_data);
    Packet& Read(s8& out_data);
    Packet& Read(u8& out_data);
    Packet& Read(s16& out_data);
    Packet& Read(u16& out_data);
    Packet& Read(s32& out_data);
    Packet& Read(u32& out_data);
    Packet& Read(s64& out_data);
    Packet& Read(u64& out_data);
    Packet& Read(float& out_data);
    Packet& Read(double& out_data);
    Packet& Read(char* out_data);
    Packet& Read(std::string& out_data);
    template <typename T>
    Packet& Read(std::vector<T>& out_data);
    template <typename T, std::size_t S>
    Packet& Read(std::array<T, S>& out_data);

    /// Overloads of write function to write data into the packet
    Packet& Write(bool in_data);
    Packet& Write(s8 in_data);
    Packet& Write(u8 in_data);
    Packet& Write(s16 in_data);
    Packet& Write(u16 in_data);
    Packet& Write(s32 in_data);
    Packet& Write(u32 in_data);
    Packet& Write(s64 in_data);
    Packet& Write(u64 in_data);
    Packet& Write(float in_data);
    Packet& Write(double in_data);
    Packet& Write(const char* in_data);
    Packet& Write(const std::string& in_data);
    template <typename T>
    Packet& Write(const std::vector<T>& in_data);
    template <typename T, std::size_t S>
    Packet& Write(const std::array<T, S>& data);

private:
    /**
     * Check if the packet can extract a given number of bytes
     * This function updates accordingly the state of the packet.
     * @param size Size to check
     * @return True if size bytes can be read from the packet
     */
    bool CheckSize(std::size_t size);

    // Member data
    std::vector<char> data;   ///< Data stored in the packet
    std::size_t read_pos = 0; ///< Current reading position in the packet
    bool is_valid = true;     ///< Reading state of the packet
};

template <typename T>
Packet& Packet::Read(std::vector<T>& out_data) {
    // First extract the size
    u32 size = 0;
    Read(size);
    out_data.resize(size);

    // Then extract the data
    for (std::size_t i = 0; i < out_data.size(); ++i) {
        T character;
        Read(character);
        out_data[i] = character;
    }
    return *this;
}

template <typename T, std::size_t S>
Packet& Packet::Read(std::array<T, S>& out_data) {
    for (std::size_t i = 0; i < out_data.size(); ++i) {
        T character;
        Read(character);
        out_data[i] = character;
    }
    return *this;
}

template <typename T>
Packet& Packet::Write(const std::vector<T>& in_data) {
    // First insert the size
    Write(static_cast<u32>(in_data.size()));

    // Then insert the data
    for (std::size_t i = 0; i < in_data.size(); ++i) {
        Write(in_data[i]);
    }
    return *this;
}

template <typename T, std::size_t S>
Packet& Packet::Write(const std::array<T, S>& in_data) {
    for (std::size_t i = 0; i < in_data.size(); ++i) {
        Write(in_data[i]);
    }
    return *this;
}

} // namespace Network
