// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>

#include "common/alignment.h"
#include "common/common_types.h"

namespace Common {

// Implementation of TinyMT (mersenne twister RNG).
// Like Nintendo, we will use the sample parameters.
class TinyMT {
public:
    static constexpr std::size_t NumStateWords = 4;

    struct State {
        std::array<u32, NumStateWords> data{};
    };

private:
    static constexpr u32 ParamMat1 = 0x8F7011EE;
    static constexpr u32 ParamMat2 = 0xFC78FF1F;
    static constexpr u32 ParamTmat = 0x3793FDFF;

    static constexpr u32 ParamMult = 0x6C078965;
    static constexpr u32 ParamPlus = 0x0019660D;
    static constexpr u32 ParamXor = 0x5D588B65;

    static constexpr u32 TopBitmask = 0x7FFFFFFF;

    static constexpr int MinimumInitIterations = 8;
    static constexpr int NumDiscardedInitOutputs = 8;

    static constexpr u32 XorByShifted27(u32 value) {
        return value ^ (value >> 27);
    }

    static constexpr u32 XorByShifted30(u32 value) {
        return value ^ (value >> 30);
    }

private:
    State state{};

private:
    // Internal API.
    void FinalizeInitialization() {
        const u32 state0 = this->state.data[0] & TopBitmask;
        const u32 state1 = this->state.data[1];
        const u32 state2 = this->state.data[2];
        const u32 state3 = this->state.data[3];

        if (state0 == 0 && state1 == 0 && state2 == 0 && state3 == 0) {
            this->state.data[0] = 'T';
            this->state.data[1] = 'I';
            this->state.data[2] = 'N';
            this->state.data[3] = 'Y';
        }

        for (int i = 0; i < NumDiscardedInitOutputs; i++) {
            this->GenerateRandomU32();
        }
    }

    u32 GenerateRandomU24() {
        return (this->GenerateRandomU32() >> 8);
    }

    static void GenerateInitialValuePlus(TinyMT::State* state, int index, u32 value) {
        u32& state0 = state->data[(index + 0) % NumStateWords];
        u32& state1 = state->data[(index + 1) % NumStateWords];
        u32& state2 = state->data[(index + 2) % NumStateWords];
        u32& state3 = state->data[(index + 3) % NumStateWords];

        const u32 x = XorByShifted27(state0 ^ state1 ^ state3) * ParamPlus;
        const u32 y = x + index + value;

        state0 = y;
        state1 += x;
        state2 += y;
    }

    static void GenerateInitialValueXor(TinyMT::State* state, int index) {
        u32& state0 = state->data[(index + 0) % NumStateWords];
        u32& state1 = state->data[(index + 1) % NumStateWords];
        u32& state2 = state->data[(index + 2) % NumStateWords];
        u32& state3 = state->data[(index + 3) % NumStateWords];

        const u32 x = XorByShifted27(state0 + state1 + state3) * ParamXor;
        const u32 y = x - index;

        state0 = y;
        state1 ^= x;
        state2 ^= y;
    }

public:
    constexpr TinyMT() = default;

    // Public API.

    // Initialization.
    void Initialize(u32 seed) {
        this->state.data[0] = seed;
        this->state.data[1] = ParamMat1;
        this->state.data[2] = ParamMat2;
        this->state.data[3] = ParamTmat;

        for (int i = 1; i < MinimumInitIterations; i++) {
            const u32 mixed = XorByShifted30(this->state.data[(i - 1) % NumStateWords]);
            this->state.data[i % NumStateWords] ^= mixed * ParamMult + i;
        }

        this->FinalizeInitialization();
    }

    void Initialize(const u32* seed, int seed_count) {
        this->state.data[0] = 0;
        this->state.data[1] = ParamMat1;
        this->state.data[2] = ParamMat2;
        this->state.data[3] = ParamTmat;

        {
            const int num_init_iterations = std::max(seed_count + 1, MinimumInitIterations) - 1;

            GenerateInitialValuePlus(&this->state, 0, seed_count);

            for (int i = 0; i < num_init_iterations; i++) {
                GenerateInitialValuePlus(&this->state, (i + 1) % NumStateWords,
                                         (i < seed_count) ? seed[i] : 0);
            }

            for (int i = 0; i < static_cast<int>(NumStateWords); i++) {
                GenerateInitialValueXor(&this->state,
                                        (i + 1 + num_init_iterations) % NumStateWords);
            }
        }

        this->FinalizeInitialization();
    }

    // State management.
    void GetState(TinyMT::State& out) const {
        out.data = this->state.data;
    }

    void SetState(const TinyMT::State& state_) {
        this->state.data = state_.data;
    }

    // Random generation.
    void GenerateRandomBytes(void* dst, std::size_t size) {
        const uintptr_t start = reinterpret_cast<uintptr_t>(dst);
        const uintptr_t end = start + size;
        const uintptr_t aligned_start = Common::AlignUp(start, 4);
        const uintptr_t aligned_end = Common::AlignDown(end, 4);

        // Make sure we're aligned.
        if (start < aligned_start) {
            const u32 rnd = this->GenerateRandomU32();
            std::memcpy(dst, &rnd, aligned_start - start);
        }

        // Write as many aligned u32s as we can.
        {
            u32* cur_dst = reinterpret_cast<u32*>(aligned_start);
            u32* const end_dst = reinterpret_cast<u32*>(aligned_end);

            while (cur_dst < end_dst) {
                *(cur_dst++) = this->GenerateRandomU32();
            }
        }

        // Handle any leftover unaligned data.
        if (aligned_end < end) {
            const u32 rnd = this->GenerateRandomU32();
            std::memcpy(reinterpret_cast<void*>(aligned_end), &rnd, end - aligned_end);
        }
    }

    u32 GenerateRandomU32() {
        // Advance state.
        const u32 x0 =
            (this->state.data[0] & TopBitmask) ^ this->state.data[1] ^ this->state.data[2];
        const u32 y0 = this->state.data[3];
        const u32 x1 = x0 ^ (x0 << 1);
        const u32 y1 = y0 ^ (y0 >> 1) ^ x1;

        const u32 state0 = this->state.data[1];
        u32 state1 = this->state.data[2];
        u32 state2 = x1 ^ (y1 << 10);
        const u32 state3 = y1;

        if ((y1 & 1) != 0) {
            state1 ^= ParamMat1;
            state2 ^= ParamMat2;
        }

        this->state.data[0] = state0;
        this->state.data[1] = state1;
        this->state.data[2] = state2;
        this->state.data[3] = state3;

        // Temper.
        const u32 t1 = state0 + (state2 >> 8);
        u32 t0 = state3 ^ t1;

        if ((t1 & 1) != 0) {
            t0 ^= ParamTmat;
        }

        return t0;
    }

    u64 GenerateRandomU64() {
        const u32 lo = this->GenerateRandomU32();
        const u32 hi = this->GenerateRandomU32();
        return (u64{hi} << 32) | u64{lo};
    }

    float GenerateRandomF32() {
        // Floats have 24 bits of mantissa.
        constexpr u32 MantissaBits = 24;
        return static_cast<float>(GenerateRandomU24()) * (1.0f / (1U << MantissaBits));
    }

    double GenerateRandomF64() {
        // Doubles have 53 bits of mantissa.
        // The smart way to generate 53 bits of random would be to use 32 bits
        // from the first rnd32() call, and then 21 from the second.
        // Nintendo does not. They use (32 - 5) = 27 bits from the first rnd32()
        // call, and (32 - 6) bits from the second. We'll do what they do, but
        // There's not a clear reason why.
        constexpr u32 MantissaBits = 53;
        constexpr u32 Shift1st = (64 - MantissaBits) / 2;
        constexpr u32 Shift2nd = (64 - MantissaBits) - Shift1st;

        const u32 first = (this->GenerateRandomU32() >> Shift1st);
        const u32 second = (this->GenerateRandomU32() >> Shift2nd);

        return (1.0 * first * (u64{1} << (32 - Shift2nd)) + second) *
               (1.0 / (u64{1} << MantissaBits));
    }
};

} // namespace Common
