// SPDX-FileCopyrightText: 2016 The University of North Carolina at Chapel Hill
// SPDX-License-Identifier: Apache-2.0

// Please send all BUG REPORTS to <pavel@cs.unc.edu>.
// <http://gamma.cs.unc.edu/FasTC/>

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstring>
#include <span>
#include <vector>

#include <boost/container/static_vector.hpp>

#include "common/common_types.h"
#include "video_core/textures/astc.h"

class InputBitStream {
public:
    constexpr explicit InputBitStream(std::span<const u8> data, size_t start_offset = 0)
        : cur_byte{data.data()}, total_bits{data.size()}, next_bit{start_offset % 8} {}

    constexpr size_t GetBitsRead() const {
        return bits_read;
    }

    constexpr bool ReadBit() {
        if (bits_read >= total_bits * 8) {
            return 0;
        }
        const bool bit = ((*cur_byte >> next_bit) & 1) != 0;
        ++next_bit;
        while (next_bit >= 8) {
            next_bit -= 8;
            ++cur_byte;
        }
        ++bits_read;
        return bit;
    }

    constexpr u32 ReadBits(std::size_t nBits) {
        u32 ret = 0;
        for (std::size_t i = 0; i < nBits; ++i) {
            ret |= (ReadBit() & 1) << i;
        }
        return ret;
    }

    template <std::size_t nBits>
    constexpr u32 ReadBits() {
        u32 ret = 0;
        for (std::size_t i = 0; i < nBits; ++i) {
            ret |= (ReadBit() & 1) << i;
        }
        return ret;
    }

private:
    const u8* cur_byte;
    size_t total_bits = 0;
    size_t next_bit = 0;
    size_t bits_read = 0;
};

class OutputBitStream {
public:
    constexpr explicit OutputBitStream(u8* ptr, std::size_t bits = 0, std::size_t start_offset = 0)
        : cur_byte{ptr}, num_bits{bits}, next_bit{start_offset % 8} {}

    constexpr std::size_t GetBitsWritten() const {
        return bits_written;
    }

    constexpr void WriteBitsR(u32 val, u32 nBits) {
        for (u32 i = 0; i < nBits; i++) {
            WriteBit((val >> (nBits - i - 1)) & 1);
        }
    }

    constexpr void WriteBits(u32 val, u32 nBits) {
        for (u32 i = 0; i < nBits; i++) {
            WriteBit((val >> i) & 1);
        }
    }

private:
    constexpr void WriteBit(bool b) {
        if (bits_written >= num_bits) {
            return;
        }

        const u32 mask = 1 << next_bit++;

        // clear the bit
        *cur_byte &= static_cast<u8>(~mask);

        // Write the bit, if necessary
        if (b)
            *cur_byte |= static_cast<u8>(mask);

        // Next byte?
        if (next_bit >= 8) {
            cur_byte += 1;
            next_bit = 0;
        }
    }

    u8* cur_byte;
    std::size_t num_bits;
    std::size_t bits_written = 0;
    std::size_t next_bit = 0;
};

template <typename IntType>
class Bits {
public:
    explicit Bits(const IntType& v) : m_Bits(v) {}

    Bits(const Bits&) = delete;
    Bits& operator=(const Bits&) = delete;

    u8 operator[](u32 bitPos) const {
        return static_cast<u8>((m_Bits >> bitPos) & 1);
    }

    IntType operator()(u32 start, u32 end) const {
        if (start == end) {
            return (*this)[start];
        } else if (start > end) {
            u32 t = start;
            start = end;
            end = t;
        }

        u64 mask = (1 << (end - start + 1)) - 1;
        return (m_Bits >> start) & static_cast<IntType>(mask);
    }

private:
    const IntType& m_Bits;
};

enum class IntegerEncoding { JustBits, Quint, Trit };

struct IntegerEncodedValue {
    constexpr IntegerEncodedValue() = default;

    constexpr IntegerEncodedValue(IntegerEncoding encoding_, u32 num_bits_)
        : encoding{encoding_}, num_bits{num_bits_} {}

    constexpr bool MatchesEncoding(const IntegerEncodedValue& other) const {
        return encoding == other.encoding && num_bits == other.num_bits;
    }

    // Returns the number of bits required to encode num_vals values.
    u32 GetBitLength(u32 num_vals) const {
        u32 total_bits = num_bits * num_vals;
        if (encoding == IntegerEncoding::Trit) {
            total_bits += (num_vals * 8 + 4) / 5;
        } else if (encoding == IntegerEncoding::Quint) {
            total_bits += (num_vals * 7 + 2) / 3;
        }
        return total_bits;
    }

    IntegerEncoding encoding{};
    u32 num_bits = 0;
    u32 bit_value = 0;
    union {
        u32 quint_value = 0;
        u32 trit_value;
    };
};

// Returns a new instance of this struct that corresponds to the
// can take no more than mav_value values
static constexpr IntegerEncodedValue CreateEncoding(u32 mav_value) {
    while (mav_value > 0) {
        u32 check = mav_value + 1;

        // Is mav_value a power of two?
        if (!(check & (check - 1))) {
            return IntegerEncodedValue(IntegerEncoding::JustBits, std::popcount(mav_value));
        }

        // Is mav_value of the type 3*2^n - 1?
        if ((check % 3 == 0) && !((check / 3) & ((check / 3) - 1))) {
            return IntegerEncodedValue(IntegerEncoding::Trit, std::popcount(check / 3 - 1));
        }

        // Is mav_value of the type 5*2^n - 1?
        if ((check % 5 == 0) && !((check / 5) & ((check / 5) - 1))) {
            return IntegerEncodedValue(IntegerEncoding::Quint, std::popcount(check / 5 - 1));
        }

        // Apparently it can't be represented with a bounded integer sequence...
        // just iterate.
        mav_value--;
    }
    return IntegerEncodedValue(IntegerEncoding::JustBits, 0);
}

static constexpr std::array<IntegerEncodedValue, 256> MakeEncodedValues() {
    std::array<IntegerEncodedValue, 256> encodings{};
    for (std::size_t i = 0; i < encodings.size(); ++i) {
        encodings[i] = CreateEncoding(static_cast<u32>(i));
    }
    return encodings;
}

static constexpr std::array<IntegerEncodedValue, 256> ASTC_ENCODINGS_VALUES = MakeEncodedValues();

namespace Tegra::Texture::ASTC {
using IntegerEncodedVector = boost::container::static_vector<
    IntegerEncodedValue, 256,
    boost::container::static_vector_options<
        boost::container::inplace_alignment<alignof(IntegerEncodedValue)>,
        boost::container::throw_on_overflow<false>>::type>;

static void DecodeTritBlock(InputBitStream& bits, IntegerEncodedVector& result, u32 nBitsPerValue) {
    // Implement the algorithm in section C.2.12
    std::array<u32, 5> m;
    std::array<u32, 5> t;
    u32 T;

    // Read the trit encoded block according to
    // table C.2.14
    m[0] = bits.ReadBits(nBitsPerValue);
    T = bits.ReadBits<2>();
    m[1] = bits.ReadBits(nBitsPerValue);
    T |= bits.ReadBits<2>() << 2;
    m[2] = bits.ReadBits(nBitsPerValue);
    T |= bits.ReadBit() << 4;
    m[3] = bits.ReadBits(nBitsPerValue);
    T |= bits.ReadBits<2>() << 5;
    m[4] = bits.ReadBits(nBitsPerValue);
    T |= bits.ReadBit() << 7;

    u32 C = 0;

    Bits<u32> Tb(T);
    if (Tb(2, 4) == 7) {
        C = (Tb(5, 7) << 2) | Tb(0, 1);
        t[4] = t[3] = 2;
    } else {
        C = Tb(0, 4);
        if (Tb(5, 6) == 3) {
            t[4] = 2;
            t[3] = Tb[7];
        } else {
            t[4] = Tb[7];
            t[3] = Tb(5, 6);
        }
    }

    Bits<u32> Cb(C);
    if (Cb(0, 1) == 3) {
        t[2] = 2;
        t[1] = Cb[4];
        t[0] = (Cb[3] << 1) | (Cb[2] & ~Cb[3]);
    } else if (Cb(2, 3) == 3) {
        t[2] = 2;
        t[1] = 2;
        t[0] = Cb(0, 1);
    } else {
        t[2] = Cb[4];
        t[1] = Cb(2, 3);
        t[0] = (Cb[1] << 1) | (Cb[0] & ~Cb[1]);
    }

    for (std::size_t i = 0; i < 5; ++i) {
        IntegerEncodedValue& val = result.emplace_back(IntegerEncoding::Trit, nBitsPerValue);
        val.bit_value = m[i];
        val.trit_value = t[i];
    }
}

static void DecodeQuintBlock(InputBitStream& bits, IntegerEncodedVector& result,
                             u32 nBitsPerValue) {
    // Implement the algorithm in section C.2.12
    u32 m[3];
    u32 q[3];
    u32 Q;

    // Read the trit encoded block according to
    // table C.2.15
    m[0] = bits.ReadBits(nBitsPerValue);
    Q = bits.ReadBits<3>();
    m[1] = bits.ReadBits(nBitsPerValue);
    Q |= bits.ReadBits<2>() << 3;
    m[2] = bits.ReadBits(nBitsPerValue);
    Q |= bits.ReadBits<2>() << 5;

    Bits<u32> Qb(Q);
    if (Qb(1, 2) == 3 && Qb(5, 6) == 0) {
        q[0] = q[1] = 4;
        q[2] = (Qb[0] << 2) | ((Qb[4] & ~Qb[0]) << 1) | (Qb[3] & ~Qb[0]);
    } else {
        u32 C = 0;
        if (Qb(1, 2) == 3) {
            q[2] = 4;
            C = (Qb(3, 4) << 3) | ((~Qb(5, 6) & 3) << 1) | Qb[0];
        } else {
            q[2] = Qb(5, 6);
            C = Qb(0, 4);
        }

        Bits<u32> Cb(C);
        if (Cb(0, 2) == 5) {
            q[1] = 4;
            q[0] = Cb(3, 4);
        } else {
            q[1] = Cb(3, 4);
            q[0] = Cb(0, 2);
        }
    }

    for (std::size_t i = 0; i < 3; ++i) {
        IntegerEncodedValue& val = result.emplace_back(IntegerEncoding::Quint, nBitsPerValue);
        val.bit_value = m[i];
        val.quint_value = q[i];
    }
}

// Fills result with the values that are encoded in the given
// bitstream. We must know beforehand what the maximum possible
// value is, and how many values we're decoding.
static void DecodeIntegerSequence(IntegerEncodedVector& result, InputBitStream& bits, u32 maxRange,
                                  u32 nValues) {
    // Determine encoding parameters
    IntegerEncodedValue val = ASTC_ENCODINGS_VALUES[maxRange];

    // Start decoding
    u32 nValsDecoded = 0;
    while (nValsDecoded < nValues) {
        switch (val.encoding) {
        case IntegerEncoding::Quint:
            DecodeQuintBlock(bits, result, val.num_bits);
            nValsDecoded += 3;
            break;

        case IntegerEncoding::Trit:
            DecodeTritBlock(bits, result, val.num_bits);
            nValsDecoded += 5;
            break;

        case IntegerEncoding::JustBits:
            val.bit_value = bits.ReadBits(val.num_bits);
            result.push_back(val);
            nValsDecoded++;
            break;
        }
    }
}

struct TexelWeightParams {
    u32 m_Width = 0;
    u32 m_Height = 0;
    bool m_bDualPlane = false;
    u32 m_MaxWeight = 0;
    bool m_bError = false;
    bool m_bVoidExtentLDR = false;
    bool m_bVoidExtentHDR = false;

    u32 GetPackedBitSize() const {
        // How many indices do we have?
        u32 nIdxs = m_Height * m_Width;
        if (m_bDualPlane) {
            nIdxs *= 2;
        }

        return ASTC_ENCODINGS_VALUES[m_MaxWeight].GetBitLength(nIdxs);
    }

    u32 GetNumWeightValues() const {
        u32 ret = m_Width * m_Height;
        if (m_bDualPlane) {
            ret *= 2;
        }
        return ret;
    }
};

static TexelWeightParams DecodeBlockInfo(InputBitStream& strm) {
    TexelWeightParams params;

    // Read the entire block mode all at once
    u16 modeBits = static_cast<u16>(strm.ReadBits<11>());

    // Does this match the void extent block mode?
    if ((modeBits & 0x01FF) == 0x1FC) {
        if (modeBits & 0x200) {
            params.m_bVoidExtentHDR = true;
        } else {
            params.m_bVoidExtentLDR = true;
        }

        // Next two bits must be one.
        if (!(modeBits & 0x400) || !strm.ReadBit()) {
            params.m_bError = true;
        }

        return params;
    }

    // First check if the last four bits are zero
    if ((modeBits & 0xF) == 0) {
        params.m_bError = true;
        return params;
    }

    // If the last two bits are zero, then if bits
    // [6-8] are all ones, this is also reserved.
    if ((modeBits & 0x3) == 0 && (modeBits & 0x1C0) == 0x1C0) {
        params.m_bError = true;
        return params;
    }

    // Otherwise, there is no error... Figure out the layout
    // of the block mode. Layout is determined by a number
    // between 0 and 9 corresponding to table C.2.8 of the
    // ASTC spec.
    u32 layout = 0;

    if ((modeBits & 0x1) || (modeBits & 0x2)) {
        // layout is in [0-4]
        if (modeBits & 0x8) {
            // layout is in [2-4]
            if (modeBits & 0x4) {
                // layout is in [3-4]
                if (modeBits & 0x100) {
                    layout = 4;
                } else {
                    layout = 3;
                }
            } else {
                layout = 2;
            }
        } else {
            // layout is in [0-1]
            if (modeBits & 0x4) {
                layout = 1;
            } else {
                layout = 0;
            }
        }
    } else {
        // layout is in [5-9]
        if (modeBits & 0x100) {
            // layout is in [7-9]
            if (modeBits & 0x80) {
                // layout is in [7-8]
                assert((modeBits & 0x40) == 0U);
                if (modeBits & 0x20) {
                    layout = 8;
                } else {
                    layout = 7;
                }
            } else {
                layout = 9;
            }
        } else {
            // layout is in [5-6]
            if (modeBits & 0x80) {
                layout = 6;
            } else {
                layout = 5;
            }
        }
    }

    assert(layout < 10);

    // Determine R
    u32 R = !!(modeBits & 0x10);
    if (layout < 5) {
        R |= (modeBits & 0x3) << 1;
    } else {
        R |= (modeBits & 0xC) >> 1;
    }
    assert(2 <= R && R <= 7);

    // Determine width & height
    switch (layout) {
    case 0: {
        u32 A = (modeBits >> 5) & 0x3;
        u32 B = (modeBits >> 7) & 0x3;
        params.m_Width = B + 4;
        params.m_Height = A + 2;
        break;
    }

    case 1: {
        u32 A = (modeBits >> 5) & 0x3;
        u32 B = (modeBits >> 7) & 0x3;
        params.m_Width = B + 8;
        params.m_Height = A + 2;
        break;
    }

    case 2: {
        u32 A = (modeBits >> 5) & 0x3;
        u32 B = (modeBits >> 7) & 0x3;
        params.m_Width = A + 2;
        params.m_Height = B + 8;
        break;
    }

    case 3: {
        u32 A = (modeBits >> 5) & 0x3;
        u32 B = (modeBits >> 7) & 0x1;
        params.m_Width = A + 2;
        params.m_Height = B + 6;
        break;
    }

    case 4: {
        u32 A = (modeBits >> 5) & 0x3;
        u32 B = (modeBits >> 7) & 0x1;
        params.m_Width = B + 2;
        params.m_Height = A + 2;
        break;
    }

    case 5: {
        u32 A = (modeBits >> 5) & 0x3;
        params.m_Width = 12;
        params.m_Height = A + 2;
        break;
    }

    case 6: {
        u32 A = (modeBits >> 5) & 0x3;
        params.m_Width = A + 2;
        params.m_Height = 12;
        break;
    }

    case 7: {
        params.m_Width = 6;
        params.m_Height = 10;
        break;
    }

    case 8: {
        params.m_Width = 10;
        params.m_Height = 6;
        break;
    }

    case 9: {
        u32 A = (modeBits >> 5) & 0x3;
        u32 B = (modeBits >> 9) & 0x3;
        params.m_Width = A + 6;
        params.m_Height = B + 6;
        break;
    }

    default:
        assert(false && "Don't know this layout...");
        params.m_bError = true;
        break;
    }

    // Determine whether or not we're using dual planes
    // and/or high precision layouts.
    bool D = (layout != 9) && (modeBits & 0x400);
    bool H = (layout != 9) && (modeBits & 0x200);

    if (H) {
        const u32 maxWeights[6] = {9, 11, 15, 19, 23, 31};
        params.m_MaxWeight = maxWeights[R - 2];
    } else {
        const u32 maxWeights[6] = {1, 2, 3, 4, 5, 7};
        params.m_MaxWeight = maxWeights[R - 2];
    }

    params.m_bDualPlane = D;

    return params;
}

// Replicates low num_bits such that [(to_bit - 1):(to_bit - 1 - from_bit)]
// is the same as [(num_bits - 1):0] and repeats all the way down.
template <typename IntType>
static constexpr IntType Replicate(IntType val, u32 num_bits, u32 to_bit) {
    if (num_bits == 0 || to_bit == 0) {
        return 0;
    }
    const IntType v = val & static_cast<IntType>((1 << num_bits) - 1);
    IntType res = v;
    u32 reslen = num_bits;
    while (reslen < to_bit) {
        u32 comp = 0;
        if (num_bits > to_bit - reslen) {
            u32 newshift = to_bit - reslen;
            comp = num_bits - newshift;
            num_bits = newshift;
        }
        res = static_cast<IntType>(res << num_bits);
        res = static_cast<IntType>(res | (v >> comp));
        reslen += num_bits;
    }
    return res;
}

static constexpr std::size_t NumReplicateEntries(u32 num_bits) {
    return std::size_t(1) << num_bits;
}

template <typename IntType, u32 num_bits, u32 to_bit>
static constexpr auto MakeReplicateTable() {
    std::array<IntType, NumReplicateEntries(num_bits)> table{};
    for (IntType value = 0; value < static_cast<IntType>(std::size(table)); ++value) {
        table[value] = Replicate(value, num_bits, to_bit);
    }
    return table;
}

static constexpr auto REPLICATE_BYTE_TO_16_TABLE = MakeReplicateTable<u32, 8, 16>();
static constexpr u32 ReplicateByteTo16(std::size_t value) {
    return REPLICATE_BYTE_TO_16_TABLE[value];
}

static constexpr auto REPLICATE_BIT_TO_7_TABLE = MakeReplicateTable<u32, 1, 7>();
static constexpr u32 ReplicateBitTo7(std::size_t value) {
    return REPLICATE_BIT_TO_7_TABLE[value];
}

static constexpr auto REPLICATE_BIT_TO_9_TABLE = MakeReplicateTable<u32, 1, 9>();
static constexpr u32 ReplicateBitTo9(std::size_t value) {
    return REPLICATE_BIT_TO_9_TABLE[value];
}

static constexpr auto REPLICATE_1_BIT_TO_8_TABLE = MakeReplicateTable<u32, 1, 8>();
static constexpr auto REPLICATE_2_BIT_TO_8_TABLE = MakeReplicateTable<u32, 2, 8>();
static constexpr auto REPLICATE_3_BIT_TO_8_TABLE = MakeReplicateTable<u32, 3, 8>();
static constexpr auto REPLICATE_4_BIT_TO_8_TABLE = MakeReplicateTable<u32, 4, 8>();
static constexpr auto REPLICATE_5_BIT_TO_8_TABLE = MakeReplicateTable<u32, 5, 8>();
static constexpr auto REPLICATE_6_BIT_TO_8_TABLE = MakeReplicateTable<u32, 6, 8>();
static constexpr auto REPLICATE_7_BIT_TO_8_TABLE = MakeReplicateTable<u32, 7, 8>();
static constexpr auto REPLICATE_8_BIT_TO_8_TABLE = MakeReplicateTable<u32, 8, 8>();
/// Use a precompiled table with the most common usages, if it's not in the expected range, fallback
/// to the runtime implementation
static constexpr u32 FastReplicateTo8(u32 value, u32 num_bits) {
    switch (num_bits) {
    case 1:
        return REPLICATE_1_BIT_TO_8_TABLE[value];
    case 2:
        return REPLICATE_2_BIT_TO_8_TABLE[value];
    case 3:
        return REPLICATE_3_BIT_TO_8_TABLE[value];
    case 4:
        return REPLICATE_4_BIT_TO_8_TABLE[value];
    case 5:
        return REPLICATE_5_BIT_TO_8_TABLE[value];
    case 6:
        return REPLICATE_6_BIT_TO_8_TABLE[value];
    case 7:
        return REPLICATE_7_BIT_TO_8_TABLE[value];
    case 8:
        return REPLICATE_8_BIT_TO_8_TABLE[value];
    default:
        return Replicate(value, num_bits, 8);
    }
}

static constexpr auto REPLICATE_1_BIT_TO_6_TABLE = MakeReplicateTable<u32, 1, 6>();
static constexpr auto REPLICATE_2_BIT_TO_6_TABLE = MakeReplicateTable<u32, 2, 6>();
static constexpr auto REPLICATE_3_BIT_TO_6_TABLE = MakeReplicateTable<u32, 3, 6>();
static constexpr auto REPLICATE_4_BIT_TO_6_TABLE = MakeReplicateTable<u32, 4, 6>();
static constexpr auto REPLICATE_5_BIT_TO_6_TABLE = MakeReplicateTable<u32, 5, 6>();
static constexpr u32 FastReplicateTo6(u32 value, u32 num_bits) {
    switch (num_bits) {
    case 1:
        return REPLICATE_1_BIT_TO_6_TABLE[value];
    case 2:
        return REPLICATE_2_BIT_TO_6_TABLE[value];
    case 3:
        return REPLICATE_3_BIT_TO_6_TABLE[value];
    case 4:
        return REPLICATE_4_BIT_TO_6_TABLE[value];
    case 5:
        return REPLICATE_5_BIT_TO_6_TABLE[value];
    default:
        return Replicate(value, num_bits, 6);
    }
}

class Pixel {
protected:
    using ChannelType = s16;
    u8 m_BitDepth[4] = {8, 8, 8, 8};
    s16 color[4] = {};

public:
    Pixel() = default;
    Pixel(u32 a, u32 r, u32 g, u32 b, u32 bitDepth = 8)
        : m_BitDepth{u8(bitDepth), u8(bitDepth), u8(bitDepth), u8(bitDepth)},
          color{static_cast<ChannelType>(a), static_cast<ChannelType>(r),
                static_cast<ChannelType>(g), static_cast<ChannelType>(b)} {}

    // Changes the depth of each pixel. This scales the values to
    // the appropriate bit depth by either truncating the least
    // significant bits when going from larger to smaller bit depth
    // or by repeating the most significant bits when going from
    // smaller to larger bit depths.
    void ChangeBitDepth() {
        for (u32 i = 0; i < 4; i++) {
            Component(i) = ChangeBitDepth(Component(i), m_BitDepth[i]);
            m_BitDepth[i] = 8;
        }
    }

    template <typename IntType>
    static float ConvertChannelToFloat(IntType channel, u8 bitDepth) {
        float denominator = static_cast<float>((1 << bitDepth) - 1);
        return static_cast<float>(channel) / denominator;
    }

    // Changes the bit depth of a single component. See the comment
    // above for how we do this.
    static ChannelType ChangeBitDepth(Pixel::ChannelType val, u8 oldDepth) {
        assert(oldDepth <= 8);

        if (oldDepth == 8) {
            // Do nothing
            return val;
        } else if (oldDepth == 0) {
            return static_cast<ChannelType>((1 << 8) - 1);
        } else if (8 > oldDepth) {
            return static_cast<ChannelType>(FastReplicateTo8(static_cast<u32>(val), oldDepth));
        } else {
            // oldDepth > newDepth
            const u8 bitsWasted = static_cast<u8>(oldDepth - 8);
            u16 v = static_cast<u16>(val);
            v = static_cast<u16>((v + (1 << (bitsWasted - 1))) >> bitsWasted);
            v = ::std::min<u16>(::std::max<u16>(0, v), static_cast<u16>((1 << 8) - 1));
            return static_cast<u8>(v);
        }

        assert(false && "We shouldn't get here.");
        return 0;
    }

    const ChannelType& A() const {
        return color[0];
    }
    ChannelType& A() {
        return color[0];
    }
    const ChannelType& R() const {
        return color[1];
    }
    ChannelType& R() {
        return color[1];
    }
    const ChannelType& G() const {
        return color[2];
    }
    ChannelType& G() {
        return color[2];
    }
    const ChannelType& B() const {
        return color[3];
    }
    ChannelType& B() {
        return color[3];
    }
    const ChannelType& Component(u32 idx) const {
        return color[idx];
    }
    ChannelType& Component(u32 idx) {
        return color[idx];
    }

    void GetBitDepth(u8 (&outDepth)[4]) const {
        for (s32 i = 0; i < 4; i++) {
            outDepth[i] = m_BitDepth[i];
        }
    }

    // Take all of the components, transform them to their 8-bit variants,
    // and then pack each channel into an R8G8B8A8 32-bit integer. We assume
    // that the architecture is little-endian, so the alpha channel will end
    // up in the most-significant byte.
    u32 Pack() const {
        Pixel eightBit(*this);
        eightBit.ChangeBitDepth();

        u32 r = 0;
        r |= eightBit.A();
        r <<= 8;
        r |= eightBit.B();
        r <<= 8;
        r |= eightBit.G();
        r <<= 8;
        r |= eightBit.R();
        return r;
    }

    // Clamps the pixel to the range [0,255]
    void ClampByte() {
        for (u32 i = 0; i < 4; i++) {
            color[i] = (color[i] < 0) ? 0 : ((color[i] > 255) ? 255 : color[i]);
        }
    }

    void MakeOpaque() {
        A() = 255;
    }
};

static void DecodeColorValues(u32* out, std::span<u8> data, const u32* modes, const u32 nPartitions,
                              const u32 nBitsForColorData) {
    // First figure out how many color values we have
    u32 nValues = 0;
    for (u32 i = 0; i < nPartitions; i++) {
        nValues += ((modes[i] >> 2) + 1) << 1;
    }

    // Then based on the number of values and the remaining number of bits,
    // figure out the max value for each of them...
    u32 range = 256;
    while (--range > 0) {
        IntegerEncodedValue val = ASTC_ENCODINGS_VALUES[range];
        u32 bitLength = val.GetBitLength(nValues);
        if (bitLength <= nBitsForColorData) {
            // Find the smallest possible range that matches the given encoding
            while (--range > 0) {
                IntegerEncodedValue newval = ASTC_ENCODINGS_VALUES[range];
                if (!newval.MatchesEncoding(val)) {
                    break;
                }
            }

            // Return to last matching range.
            range++;
            break;
        }
    }

    // We now have enough to decode our integer sequence.
    IntegerEncodedVector decodedColorValues;

    InputBitStream colorStream(data, 0);
    DecodeIntegerSequence(decodedColorValues, colorStream, range, nValues);

    // Once we have the decoded values, we need to dequantize them to the 0-255 range
    // This procedure is outlined in ASTC spec C.2.13
    u32 outIdx = 0;
    for (auto itr = decodedColorValues.begin(); itr != decodedColorValues.end(); ++itr) {
        // Have we already decoded all that we need?
        if (outIdx >= nValues) {
            break;
        }

        const IntegerEncodedValue& val = *itr;
        u32 bitlen = val.num_bits;
        u32 bitval = val.bit_value;

        assert(bitlen >= 1);

        u32 A = 0, B = 0, C = 0, D = 0;
        // A is just the lsb replicated 9 times.
        A = ReplicateBitTo9(bitval & 1);

        switch (val.encoding) {
        // Replicate bits
        case IntegerEncoding::JustBits:
            out[outIdx++] = FastReplicateTo8(bitval, bitlen);
            break;

        // Use algorithm in C.2.13
        case IntegerEncoding::Trit: {

            D = val.trit_value;

            switch (bitlen) {
            case 1: {
                C = 204;
            } break;

            case 2: {
                C = 93;
                // B = b000b0bb0
                u32 b = (bitval >> 1) & 1;
                B = (b << 8) | (b << 4) | (b << 2) | (b << 1);
            } break;

            case 3: {
                C = 44;
                // B = cb000cbcb
                u32 cb = (bitval >> 1) & 3;
                B = (cb << 7) | (cb << 2) | cb;
            } break;

            case 4: {
                C = 22;
                // B = dcb000dcb
                u32 dcb = (bitval >> 1) & 7;
                B = (dcb << 6) | dcb;
            } break;

            case 5: {
                C = 11;
                // B = edcb000ed
                u32 edcb = (bitval >> 1) & 0xF;
                B = (edcb << 5) | (edcb >> 2);
            } break;

            case 6: {
                C = 5;
                // B = fedcb000f
                u32 fedcb = (bitval >> 1) & 0x1F;
                B = (fedcb << 4) | (fedcb >> 4);
            } break;

            default:
                assert(false && "Unsupported trit encoding for color values!");
                break;
            } // switch(bitlen)
        }     // case IntegerEncoding::Trit
        break;

        case IntegerEncoding::Quint: {

            D = val.quint_value;

            switch (bitlen) {
            case 1: {
                C = 113;
            } break;

            case 2: {
                C = 54;
                // B = b0000bb00
                u32 b = (bitval >> 1) & 1;
                B = (b << 8) | (b << 3) | (b << 2);
            } break;

            case 3: {
                C = 26;
                // B = cb0000cbc
                u32 cb = (bitval >> 1) & 3;
                B = (cb << 7) | (cb << 1) | (cb >> 1);
            } break;

            case 4: {
                C = 13;
                // B = dcb0000dc
                u32 dcb = (bitval >> 1) & 7;
                B = (dcb << 6) | (dcb >> 1);
            } break;

            case 5: {
                C = 6;
                // B = edcb0000e
                u32 edcb = (bitval >> 1) & 0xF;
                B = (edcb << 5) | (edcb >> 3);
            } break;

            default:
                assert(false && "Unsupported quint encoding for color values!");
                break;
            } // switch(bitlen)
        }     // case IntegerEncoding::Quint
        break;
        } // switch(val.encoding)

        if (val.encoding != IntegerEncoding::JustBits) {
            u32 T = D * C + B;
            T ^= A;
            T = (A & 0x80) | (T >> 2);
            out[outIdx++] = T;
        }
    }

    // Make sure that each of our values is in the proper range...
    for (u32 i = 0; i < nValues; i++) {
        assert(out[i] <= 255);
    }
}

static u32 UnquantizeTexelWeight(const IntegerEncodedValue& val) {
    u32 bitval = val.bit_value;
    u32 bitlen = val.num_bits;

    u32 A = ReplicateBitTo7(bitval & 1);
    u32 B = 0, C = 0, D = 0;

    u32 result = 0;
    switch (val.encoding) {
    case IntegerEncoding::JustBits:
        result = FastReplicateTo6(bitval, bitlen);
        break;

    case IntegerEncoding::Trit: {
        D = val.trit_value;
        assert(D < 3);

        switch (bitlen) {
        case 0: {
            u32 results[3] = {0, 32, 63};
            result = results[D];
        } break;

        case 1: {
            C = 50;
        } break;

        case 2: {
            C = 23;
            u32 b = (bitval >> 1) & 1;
            B = (b << 6) | (b << 2) | b;
        } break;

        case 3: {
            C = 11;
            u32 cb = (bitval >> 1) & 3;
            B = (cb << 5) | cb;
        } break;

        default:
            assert(false && "Invalid trit encoding for texel weight");
            break;
        }
    } break;

    case IntegerEncoding::Quint: {
        D = val.quint_value;
        assert(D < 5);

        switch (bitlen) {
        case 0: {
            u32 results[5] = {0, 16, 32, 47, 63};
            result = results[D];
        } break;

        case 1: {
            C = 28;
        } break;

        case 2: {
            C = 13;
            u32 b = (bitval >> 1) & 1;
            B = (b << 6) | (b << 1);
        } break;

        default:
            assert(false && "Invalid quint encoding for texel weight");
            break;
        }
    } break;
    }

    if (val.encoding != IntegerEncoding::JustBits && bitlen > 0) {
        // Decode the value...
        result = D * C + B;
        result ^= A;
        result = (A & 0x20) | (result >> 2);
    }

    assert(result < 64);

    // Change from [0,63] to [0,64]
    if (result > 32) {
        result += 1;
    }

    return result;
}

static void UnquantizeTexelWeights(u32 out[2][144], const IntegerEncodedVector& weights,
                                   const TexelWeightParams& params, const u32 blockWidth,
                                   const u32 blockHeight) {
    u32 weightIdx = 0;
    u32 unquantized[2][144];

    for (auto itr = weights.begin(); itr != weights.end(); ++itr) {
        unquantized[0][weightIdx] = UnquantizeTexelWeight(*itr);

        if (params.m_bDualPlane) {
            ++itr;
            unquantized[1][weightIdx] = UnquantizeTexelWeight(*itr);
            if (itr == weights.end()) {
                break;
            }
        }

        if (++weightIdx >= (params.m_Width * params.m_Height))
            break;
    }

    // Do infill if necessary (Section C.2.18) ...
    u32 Ds = (1024 + (blockWidth / 2)) / (blockWidth - 1);
    u32 Dt = (1024 + (blockHeight / 2)) / (blockHeight - 1);

    const u32 kPlaneScale = params.m_bDualPlane ? 2U : 1U;
    for (u32 plane = 0; plane < kPlaneScale; plane++)
        for (u32 t = 0; t < blockHeight; t++)
            for (u32 s = 0; s < blockWidth; s++) {
                u32 cs = Ds * s;
                u32 ct = Dt * t;

                u32 gs = (cs * (params.m_Width - 1) + 32) >> 6;
                u32 gt = (ct * (params.m_Height - 1) + 32) >> 6;

                u32 js = gs >> 4;
                u32 fs = gs & 0xF;

                u32 jt = gt >> 4;
                u32 ft = gt & 0x0F;

                u32 w11 = (fs * ft + 8) >> 4;
                u32 w10 = ft - w11;
                u32 w01 = fs - w11;
                u32 w00 = 16 - fs - ft + w11;

                u32 v0 = js + jt * params.m_Width;

#define FIND_TEXEL(tidx, bidx)                                                                     \
    u32 p##bidx = 0;                                                                               \
    do {                                                                                           \
        if ((tidx) < (params.m_Width * params.m_Height)) {                                         \
            p##bidx = unquantized[plane][(tidx)];                                                  \
        }                                                                                          \
    } while (0)

                FIND_TEXEL(v0, 00);
                FIND_TEXEL(v0 + 1, 01);
                FIND_TEXEL(v0 + params.m_Width, 10);
                FIND_TEXEL(v0 + params.m_Width + 1, 11);

#undef FIND_TEXEL

                out[plane][t * blockWidth + s] =
                    (p00 * w00 + p01 * w01 + p10 * w10 + p11 * w11 + 8) >> 4;
            }
}

// Transfers a bit as described in C.2.14
static inline void BitTransferSigned(int& a, int& b) {
    b >>= 1;
    b |= a & 0x80;
    a >>= 1;
    a &= 0x3F;
    if (a & 0x20)
        a -= 0x40;
}

// Adds more precision to the blue channel as described
// in C.2.14
static inline Pixel BlueContract(s32 a, s32 r, s32 g, s32 b) {
    return Pixel(static_cast<s16>(a), static_cast<s16>((r + b) >> 1),
                 static_cast<s16>((g + b) >> 1), static_cast<s16>(b));
}

// Partition selection functions as specified in
// C.2.21
static inline u32 hash52(u32 p) {
    p ^= p >> 15;
    p -= p << 17;
    p += p << 7;
    p += p << 4;
    p ^= p >> 5;
    p += p << 16;
    p ^= p >> 7;
    p ^= p >> 3;
    p ^= p << 6;
    p ^= p >> 17;
    return p;
}

static u32 SelectPartition(s32 seed, s32 x, s32 y, s32 z, s32 partitionCount, s32 smallBlock) {
    if (1 == partitionCount)
        return 0;

    if (smallBlock) {
        x <<= 1;
        y <<= 1;
        z <<= 1;
    }

    seed += (partitionCount - 1) * 1024;

    u32 rnum = hash52(static_cast<u32>(seed));
    u8 seed1 = static_cast<u8>(rnum & 0xF);
    u8 seed2 = static_cast<u8>((rnum >> 4) & 0xF);
    u8 seed3 = static_cast<u8>((rnum >> 8) & 0xF);
    u8 seed4 = static_cast<u8>((rnum >> 12) & 0xF);
    u8 seed5 = static_cast<u8>((rnum >> 16) & 0xF);
    u8 seed6 = static_cast<u8>((rnum >> 20) & 0xF);
    u8 seed7 = static_cast<u8>((rnum >> 24) & 0xF);
    u8 seed8 = static_cast<u8>((rnum >> 28) & 0xF);
    u8 seed9 = static_cast<u8>((rnum >> 18) & 0xF);
    u8 seed10 = static_cast<u8>((rnum >> 22) & 0xF);
    u8 seed11 = static_cast<u8>((rnum >> 26) & 0xF);
    u8 seed12 = static_cast<u8>(((rnum >> 30) | (rnum << 2)) & 0xF);

    seed1 = static_cast<u8>(seed1 * seed1);
    seed2 = static_cast<u8>(seed2 * seed2);
    seed3 = static_cast<u8>(seed3 * seed3);
    seed4 = static_cast<u8>(seed4 * seed4);
    seed5 = static_cast<u8>(seed5 * seed5);
    seed6 = static_cast<u8>(seed6 * seed6);
    seed7 = static_cast<u8>(seed7 * seed7);
    seed8 = static_cast<u8>(seed8 * seed8);
    seed9 = static_cast<u8>(seed9 * seed9);
    seed10 = static_cast<u8>(seed10 * seed10);
    seed11 = static_cast<u8>(seed11 * seed11);
    seed12 = static_cast<u8>(seed12 * seed12);

    s32 sh1, sh2, sh3;
    if (seed & 1) {
        sh1 = (seed & 2) ? 4 : 5;
        sh2 = (partitionCount == 3) ? 6 : 5;
    } else {
        sh1 = (partitionCount == 3) ? 6 : 5;
        sh2 = (seed & 2) ? 4 : 5;
    }
    sh3 = (seed & 0x10) ? sh1 : sh2;

    seed1 = static_cast<u8>(seed1 >> sh1);
    seed2 = static_cast<u8>(seed2 >> sh2);
    seed3 = static_cast<u8>(seed3 >> sh1);
    seed4 = static_cast<u8>(seed4 >> sh2);
    seed5 = static_cast<u8>(seed5 >> sh1);
    seed6 = static_cast<u8>(seed6 >> sh2);
    seed7 = static_cast<u8>(seed7 >> sh1);
    seed8 = static_cast<u8>(seed8 >> sh2);
    seed9 = static_cast<u8>(seed9 >> sh3);
    seed10 = static_cast<u8>(seed10 >> sh3);
    seed11 = static_cast<u8>(seed11 >> sh3);
    seed12 = static_cast<u8>(seed12 >> sh3);

    s32 a = seed1 * x + seed2 * y + seed11 * z + (rnum >> 14);
    s32 b = seed3 * x + seed4 * y + seed12 * z + (rnum >> 10);
    s32 c = seed5 * x + seed6 * y + seed9 * z + (rnum >> 6);
    s32 d = seed7 * x + seed8 * y + seed10 * z + (rnum >> 2);

    a &= 0x3F;
    b &= 0x3F;
    c &= 0x3F;
    d &= 0x3F;

    if (partitionCount < 4)
        d = 0;
    if (partitionCount < 3)
        c = 0;

    if (a >= b && a >= c && a >= d)
        return 0;
    else if (b >= c && b >= d)
        return 1;
    else if (c >= d)
        return 2;
    return 3;
}

static inline u32 Select2DPartition(s32 seed, s32 x, s32 y, s32 partitionCount, s32 smallBlock) {
    return SelectPartition(seed, x, y, 0, partitionCount, smallBlock);
}

// Section C.2.14
static void ComputeEndpoints(Pixel& ep1, Pixel& ep2, const u32*& colorValues,
                             u32 colorEndpointMode) {
#define READ_UINT_VALUES(N)                                                                        \
    u32 v[N];                                                                                      \
    for (u32 i = 0; i < N; i++) {                                                                  \
        v[i] = *(colorValues++);                                                                   \
    }

#define READ_INT_VALUES(N)                                                                         \
    s32 v[N];                                                                                      \
    for (u32 i = 0; i < N; i++) {                                                                  \
        v[i] = static_cast<int>(*(colorValues++));                                                 \
    }

    switch (colorEndpointMode) {
    case 0: {
        READ_UINT_VALUES(2)
        ep1 = Pixel(0xFF, v[0], v[0], v[0]);
        ep2 = Pixel(0xFF, v[1], v[1], v[1]);
    } break;

    case 1: {
        READ_UINT_VALUES(2)
        u32 L0 = (v[0] >> 2) | (v[1] & 0xC0);
        u32 L1 = std::min(L0 + (v[1] & 0x3F), 0xFFU);
        ep1 = Pixel(0xFF, L0, L0, L0);
        ep2 = Pixel(0xFF, L1, L1, L1);
    } break;

    case 4: {
        READ_UINT_VALUES(4)
        ep1 = Pixel(v[2], v[0], v[0], v[0]);
        ep2 = Pixel(v[3], v[1], v[1], v[1]);
    } break;

    case 5: {
        READ_INT_VALUES(4)
        BitTransferSigned(v[1], v[0]);
        BitTransferSigned(v[3], v[2]);
        ep1 = Pixel(v[2], v[0], v[0], v[0]);
        ep2 = Pixel(v[2] + v[3], v[0] + v[1], v[0] + v[1], v[0] + v[1]);
        ep1.ClampByte();
        ep2.ClampByte();
    } break;

    case 6: {
        READ_UINT_VALUES(4)
        ep1 = Pixel(0xFF, v[0] * v[3] >> 8, v[1] * v[3] >> 8, v[2] * v[3] >> 8);
        ep2 = Pixel(0xFF, v[0], v[1], v[2]);
    } break;

    case 8: {
        READ_UINT_VALUES(6)
        if (v[1] + v[3] + v[5] >= v[0] + v[2] + v[4]) {
            ep1 = Pixel(0xFF, v[0], v[2], v[4]);
            ep2 = Pixel(0xFF, v[1], v[3], v[5]);
        } else {
            ep1 = BlueContract(0xFF, v[1], v[3], v[5]);
            ep2 = BlueContract(0xFF, v[0], v[2], v[4]);
        }
    } break;

    case 9: {
        READ_INT_VALUES(6)
        BitTransferSigned(v[1], v[0]);
        BitTransferSigned(v[3], v[2]);
        BitTransferSigned(v[5], v[4]);
        if (v[1] + v[3] + v[5] >= 0) {
            ep1 = Pixel(0xFF, v[0], v[2], v[4]);
            ep2 = Pixel(0xFF, v[0] + v[1], v[2] + v[3], v[4] + v[5]);
        } else {
            ep1 = BlueContract(0xFF, v[0] + v[1], v[2] + v[3], v[4] + v[5]);
            ep2 = BlueContract(0xFF, v[0], v[2], v[4]);
        }
        ep1.ClampByte();
        ep2.ClampByte();
    } break;

    case 10: {
        READ_UINT_VALUES(6)
        ep1 = Pixel(v[4], v[0] * v[3] >> 8, v[1] * v[3] >> 8, v[2] * v[3] >> 8);
        ep2 = Pixel(v[5], v[0], v[1], v[2]);
    } break;

    case 12: {
        READ_UINT_VALUES(8)
        if (v[1] + v[3] + v[5] >= v[0] + v[2] + v[4]) {
            ep1 = Pixel(v[6], v[0], v[2], v[4]);
            ep2 = Pixel(v[7], v[1], v[3], v[5]);
        } else {
            ep1 = BlueContract(v[7], v[1], v[3], v[5]);
            ep2 = BlueContract(v[6], v[0], v[2], v[4]);
        }
    } break;

    case 13: {
        READ_INT_VALUES(8)
        BitTransferSigned(v[1], v[0]);
        BitTransferSigned(v[3], v[2]);
        BitTransferSigned(v[5], v[4]);
        BitTransferSigned(v[7], v[6]);
        if (v[1] + v[3] + v[5] >= 0) {
            ep1 = Pixel(v[6], v[0], v[2], v[4]);
            ep2 = Pixel(v[7] + v[6], v[0] + v[1], v[2] + v[3], v[4] + v[5]);
        } else {
            ep1 = BlueContract(v[6] + v[7], v[0] + v[1], v[2] + v[3], v[4] + v[5]);
            ep2 = BlueContract(v[6], v[0], v[2], v[4]);
        }
        ep1.ClampByte();
        ep2.ClampByte();
    } break;

    default:
        assert(false && "Unsupported color endpoint mode (is it HDR?)");
        break;
    }

#undef READ_UINT_VALUES
#undef READ_INT_VALUES
}

static void FillVoidExtentLDR(InputBitStream& strm, std::span<u32> outBuf, u32 blockWidth,
                              u32 blockHeight) {
    // Don't actually care about the void extent, just read the bits...
    for (s32 i = 0; i < 4; ++i) {
        strm.ReadBits<13>();
    }

    // Decode the RGBA components and renormalize them to the range [0, 255]
    u16 r = static_cast<u16>(strm.ReadBits<16>());
    u16 g = static_cast<u16>(strm.ReadBits<16>());
    u16 b = static_cast<u16>(strm.ReadBits<16>());
    u16 a = static_cast<u16>(strm.ReadBits<16>());

    u32 rgba = (r >> 8) | (g & 0xFF00) | (static_cast<u32>(b) & 0xFF00) << 8 |
               (static_cast<u32>(a) & 0xFF00) << 16;

    for (u32 j = 0; j < blockHeight; j++) {
        for (u32 i = 0; i < blockWidth; i++) {
            outBuf[j * blockWidth + i] = rgba;
        }
    }
}

static void FillError(std::span<u32> outBuf, u32 blockWidth, u32 blockHeight) {
    for (u32 j = 0; j < blockHeight; j++) {
        for (u32 i = 0; i < blockWidth; i++) {
            outBuf[j * blockWidth + i] = 0xFFFF00FF;
        }
    }
}

static void DecompressBlock(std::span<const u8, 16> inBuf, const u32 blockWidth,
                            const u32 blockHeight, std::span<u32, 12 * 12> outBuf) {
    InputBitStream strm(inBuf);
    TexelWeightParams weightParams = DecodeBlockInfo(strm);

    // Was there an error?
    if (weightParams.m_bError) {
        assert(false && "Invalid block mode");
        FillError(outBuf, blockWidth, blockHeight);
        return;
    }

    if (weightParams.m_bVoidExtentLDR) {
        FillVoidExtentLDR(strm, outBuf, blockWidth, blockHeight);
        return;
    }

    if (weightParams.m_bVoidExtentHDR) {
        assert(false && "HDR void extent blocks are unsupported!");
        FillError(outBuf, blockWidth, blockHeight);
        return;
    }

    if (weightParams.m_Width > blockWidth) {
        assert(false && "Texel weight grid width should be smaller than block width");
        FillError(outBuf, blockWidth, blockHeight);
        return;
    }

    if (weightParams.m_Height > blockHeight) {
        assert(false && "Texel weight grid height should be smaller than block height");
        FillError(outBuf, blockWidth, blockHeight);
        return;
    }

    // Read num partitions
    u32 nPartitions = strm.ReadBits<2>() + 1;
    assert(nPartitions <= 4);

    if (nPartitions == 4 && weightParams.m_bDualPlane) {
        assert(false && "Dual plane mode is incompatible with four partition blocks");
        FillError(outBuf, blockWidth, blockHeight);
        return;
    }

    // Based on the number of partitions, read the color endpoint mode for
    // each partition.

    // Determine partitions, partition index, and color endpoint modes
    u32 planeIdx{UINT32_MAX};
    u32 partitionIndex{};
    u32 colorEndpointMode[4] = {0, 0, 0, 0};

    // Define color data.
    u8 colorEndpointData[16];
    memset(colorEndpointData, 0, sizeof(colorEndpointData));
    OutputBitStream colorEndpointStream(colorEndpointData, 16 * 8, 0);

    // Read extra config data...
    u32 baseCEM = 0;
    if (nPartitions == 1) {
        colorEndpointMode[0] = strm.ReadBits<4>();
        partitionIndex = 0;
    } else {
        partitionIndex = strm.ReadBits<10>();
        baseCEM = strm.ReadBits<6>();
    }
    u32 baseMode = (baseCEM & 3);

    // Remaining bits are color endpoint data...
    u32 nWeightBits = weightParams.GetPackedBitSize();
    s32 remainingBits = 128 - nWeightBits - static_cast<int>(strm.GetBitsRead());

    // Consider extra bits prior to texel data...
    u32 extraCEMbits = 0;
    if (baseMode) {
        switch (nPartitions) {
        case 2:
            extraCEMbits += 2;
            break;
        case 3:
            extraCEMbits += 5;
            break;
        case 4:
            extraCEMbits += 8;
            break;
        default:
            assert(false);
            break;
        }
    }
    remainingBits -= extraCEMbits;

    // Do we have a dual plane situation?
    u32 planeSelectorBits = 0;
    if (weightParams.m_bDualPlane) {
        planeSelectorBits = 2;
    }
    remainingBits -= planeSelectorBits;

    // Read color data...
    u32 colorDataBits = remainingBits;
    while (remainingBits > 0) {
        u32 nb = std::min(remainingBits, 8);
        u32 b = strm.ReadBits(nb);
        colorEndpointStream.WriteBits(b, nb);
        remainingBits -= 8;
    }

    // Read the plane selection bits
    planeIdx = strm.ReadBits(planeSelectorBits);

    // Read the rest of the CEM
    if (baseMode) {
        u32 extraCEM = strm.ReadBits(extraCEMbits);
        u32 CEM = (extraCEM << 6) | baseCEM;
        CEM >>= 2;

        bool C[4] = {0};
        for (u32 i = 0; i < nPartitions; i++) {
            C[i] = CEM & 1;
            CEM >>= 1;
        }

        u8 M[4] = {0};
        for (u32 i = 0; i < nPartitions; i++) {
            M[i] = CEM & 3;
            CEM >>= 2;
            assert(M[i] <= 3);
        }

        for (u32 i = 0; i < nPartitions; i++) {
            colorEndpointMode[i] = baseMode;
            if (!(C[i]))
                colorEndpointMode[i] -= 1;
            colorEndpointMode[i] <<= 2;
            colorEndpointMode[i] |= M[i];
        }
    } else if (nPartitions > 1) {
        u32 CEM = baseCEM >> 2;
        for (u32 i = 0; i < nPartitions; i++) {
            colorEndpointMode[i] = CEM;
        }
    }

    // Make sure everything up till here is sane.
    for (u32 i = 0; i < nPartitions; i++) {
        assert(colorEndpointMode[i] < 16);
    }
    assert(strm.GetBitsRead() + weightParams.GetPackedBitSize() == 128);

    // Decode both color data and texel weight data
    u32 colorValues[32]; // Four values, two endpoints, four maximum paritions
    DecodeColorValues(colorValues, colorEndpointData, colorEndpointMode, nPartitions,
                      colorDataBits);

    Pixel endpoints[4][2];
    const u32* colorValuesPtr = colorValues;
    for (u32 i = 0; i < nPartitions; i++) {
        ComputeEndpoints(endpoints[i][0], endpoints[i][1], colorValuesPtr, colorEndpointMode[i]);
    }

    // Read the texel weight data..
    std::array<u8, 16> texelWeightData;
    std::ranges::copy(inBuf, texelWeightData.begin());

    // Reverse everything
    for (u32 i = 0; i < 8; i++) {
// Taken from http://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith64Bits
#define REVERSE_BYTE(b) (((b)*0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32
        u8 a = static_cast<u8>(REVERSE_BYTE(texelWeightData[i]));
        u8 b = static_cast<u8>(REVERSE_BYTE(texelWeightData[15 - i]));
#undef REVERSE_BYTE

        texelWeightData[i] = b;
        texelWeightData[15 - i] = a;
    }

    // Make sure that higher non-texel bits are set to zero
    const u32 clearByteStart = (weightParams.GetPackedBitSize() >> 3) + 1;
    if (clearByteStart > 0 && clearByteStart <= texelWeightData.size()) {
        texelWeightData[clearByteStart - 1] &=
            static_cast<u8>((1 << (weightParams.GetPackedBitSize() % 8)) - 1);
        std::memset(texelWeightData.data() + clearByteStart, 0,
                    std::min(16U - clearByteStart, 16U));
    }

    IntegerEncodedVector texelWeightValues;

    InputBitStream weightStream(texelWeightData);

    DecodeIntegerSequence(texelWeightValues, weightStream, weightParams.m_MaxWeight,
                          weightParams.GetNumWeightValues());

    // Blocks can be at most 12x12, so we can have as many as 144 weights
    u32 weights[2][144];
    UnquantizeTexelWeights(weights, texelWeightValues, weightParams, blockWidth, blockHeight);

    // Now that we have endpoints and weights, we can interpolate and generate
    // the proper decoding...
    for (u32 j = 0; j < blockHeight; j++)
        for (u32 i = 0; i < blockWidth; i++) {
            u32 partition = Select2DPartition(partitionIndex, i, j, nPartitions,
                                              (blockHeight * blockWidth) < 32);
            assert(partition < nPartitions);

            Pixel p;
            for (u32 c = 0; c < 4; c++) {
                u32 C0 = endpoints[partition][0].Component(c);
                C0 = ReplicateByteTo16(C0);
                u32 C1 = endpoints[partition][1].Component(c);
                C1 = ReplicateByteTo16(C1);

                u32 plane = 0;
                if (weightParams.m_bDualPlane && (((planeIdx + 1) & 3) == c)) {
                    plane = 1;
                }

                u32 weight = weights[plane][j * blockWidth + i];
                u32 C = (C0 * (64 - weight) + C1 * weight + 32) / 64;
                if (C == 65535) {
                    p.Component(c) = 255;
                } else {
                    double Cf = static_cast<double>(C);
                    p.Component(c) = static_cast<u16>(255.0 * (Cf / 65536.0) + 0.5);
                }
            }

            outBuf[j * blockWidth + i] = p.Pack();
        }
}

void Decompress(std::span<const uint8_t> data, uint32_t width, uint32_t height, uint32_t depth,
                uint32_t block_width, uint32_t block_height, std::span<uint8_t> output) {
    u32 block_index = 0;
    std::size_t depth_offset = 0;
    for (u32 z = 0; z < depth; z++) {
        for (u32 y = 0; y < height; y += block_height) {
            for (u32 x = 0; x < width; x += block_width) {
                const std::span<const u8, 16> blockPtr{data.subspan(block_index * 16, 16)};

                // Blocks can be at most 12x12
                std::array<u32, 12 * 12> uncompData;
                DecompressBlock(blockPtr, block_width, block_height, uncompData);

                u32 decompWidth = std::min(block_width, width - x);
                u32 decompHeight = std::min(block_height, height - y);

                const std::span<u8> outRow = output.subspan(depth_offset + (y * width + x) * 4);
                for (u32 jj = 0; jj < decompHeight; jj++) {
                    std::memcpy(outRow.data() + jj * width * 4,
                                uncompData.data() + jj * block_width, decompWidth * 4);
                }
                ++block_index;
            }
        }
        depth_offset += height * width * 4;
    }
}

} // namespace Tegra::Texture::ASTC
