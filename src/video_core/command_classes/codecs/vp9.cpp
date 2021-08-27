// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm> // for std::copy
#include <numeric>
#include "common/assert.h"
#include "video_core/command_classes/codecs/vp9.h"
#include "video_core/gpu.h"
#include "video_core/memory_manager.h"

namespace Tegra::Decoder {
namespace {
constexpr u32 diff_update_probability = 252;
constexpr u32 frame_sync_code = 0x498342;

// Default compressed header probabilities once frame context resets
constexpr Vp9EntropyProbs default_probs{
    .y_mode_prob{
        65,  32, 18, 144, 162, 194, 41, 51, 98, 132, 68,  18, 165, 217, 196, 45, 40, 78,
        173, 80, 19, 176, 240, 193, 64, 35, 46, 221, 135, 38, 194, 248, 121, 96, 85, 29,
    },
    .partition_prob{
        199, 122, 141, 0, 147, 63, 159, 0, 148, 133, 118, 0, 121, 104, 114, 0,
        174, 73,  87,  0, 92,  41, 83,  0, 82,  99,  50,  0, 53,  39,  39,  0,
        177, 58,  59,  0, 68,  26, 63,  0, 52,  79,  25,  0, 17,  14,  12,  0,
        222, 34,  30,  0, 72,  16, 44,  0, 58,  32,  12,  0, 10,  7,   6,   0,
    },
    .coef_probs{
        195, 29,  183, 84,  49,  136, 8,   42,  71,  0,   0,   0,   0,  0,   0,   0,  0,  0,
        31,  107, 169, 35,  99,  159, 17,  82,  140, 8,   66,  114, 2,  44,  76,  1,  19, 32,
        40,  132, 201, 29,  114, 187, 13,  91,  157, 7,   75,  127, 3,  58,  95,  1,  28, 47,
        69,  142, 221, 42,  122, 201, 15,  91,  159, 6,   67,  121, 1,  42,  77,  1,  17, 31,
        102, 148, 228, 67,  117, 204, 17,  82,  154, 6,   59,  114, 2,  39,  75,  1,  15, 29,
        156, 57,  233, 119, 57,  212, 58,  48,  163, 29,  40,  124, 12, 30,  81,  3,  12, 31,
        191, 107, 226, 124, 117, 204, 25,  99,  155, 0,   0,   0,   0,  0,   0,   0,  0,  0,
        29,  148, 210, 37,  126, 194, 8,   93,  157, 2,   68,  118, 1,  39,  69,  1,  17, 33,
        41,  151, 213, 27,  123, 193, 3,   82,  144, 1,   58,  105, 1,  32,  60,  1,  13, 26,
        59,  159, 220, 23,  126, 198, 4,   88,  151, 1,   66,  114, 1,  38,  71,  1,  18, 34,
        114, 136, 232, 51,  114, 207, 11,  83,  155, 3,   56,  105, 1,  33,  65,  1,  17, 34,
        149, 65,  234, 121, 57,  215, 61,  49,  166, 28,  36,  114, 12, 25,  76,  3,  16, 42,
        214, 49,  220, 132, 63,  188, 42,  65,  137, 0,   0,   0,   0,  0,   0,   0,  0,  0,
        85,  137, 221, 104, 131, 216, 49,  111, 192, 21,  87,  155, 2,  49,  87,  1,  16, 28,
        89,  163, 230, 90,  137, 220, 29,  100, 183, 10,  70,  135, 2,  42,  81,  1,  17, 33,
        108, 167, 237, 55,  133, 222, 15,  97,  179, 4,   72,  135, 1,  45,  85,  1,  19, 38,
        124, 146, 240, 66,  124, 224, 17,  88,  175, 4,   58,  122, 1,  36,  75,  1,  18, 37,
        141, 79,  241, 126, 70,  227, 66,  58,  182, 30,  44,  136, 12, 34,  96,  2,  20, 47,
        229, 99,  249, 143, 111, 235, 46,  109, 192, 0,   0,   0,   0,  0,   0,   0,  0,  0,
        82,  158, 236, 94,  146, 224, 25,  117, 191, 9,   87,  149, 3,  56,  99,  1,  33, 57,
        83,  167, 237, 68,  145, 222, 10,  103, 177, 2,   72,  131, 1,  41,  79,  1,  20, 39,
        99,  167, 239, 47,  141, 224, 10,  104, 178, 2,   73,  133, 1,  44,  85,  1,  22, 47,
        127, 145, 243, 71,  129, 228, 17,  93,  177, 3,   61,  124, 1,  41,  84,  1,  21, 52,
        157, 78,  244, 140, 72,  231, 69,  58,  184, 31,  44,  137, 14, 38,  105, 8,  23, 61,
        125, 34,  187, 52,  41,  133, 6,   31,  56,  0,   0,   0,   0,  0,   0,   0,  0,  0,
        37,  109, 153, 51,  102, 147, 23,  87,  128, 8,   67,  101, 1,  41,  63,  1,  19, 29,
        31,  154, 185, 17,  127, 175, 6,   96,  145, 2,   73,  114, 1,  51,  82,  1,  28, 45,
        23,  163, 200, 10,  131, 185, 2,   93,  148, 1,   67,  111, 1,  41,  69,  1,  14, 24,
        29,  176, 217, 12,  145, 201, 3,   101, 156, 1,   69,  111, 1,  39,  63,  1,  14, 23,
        57,  192, 233, 25,  154, 215, 6,   109, 167, 3,   78,  118, 1,  48,  69,  1,  21, 29,
        202, 105, 245, 108, 106, 216, 18,  90,  144, 0,   0,   0,   0,  0,   0,   0,  0,  0,
        33,  172, 219, 64,  149, 206, 14,  117, 177, 5,   90,  141, 2,  61,  95,  1,  37, 57,
        33,  179, 220, 11,  140, 198, 1,   89,  148, 1,   60,  104, 1,  33,  57,  1,  12, 21,
        30,  181, 221, 8,   141, 198, 1,   87,  145, 1,   58,  100, 1,  31,  55,  1,  12, 20,
        32,  186, 224, 7,   142, 198, 1,   86,  143, 1,   58,  100, 1,  31,  55,  1,  12, 22,
        57,  192, 227, 20,  143, 204, 3,   96,  154, 1,   68,  112, 1,  42,  69,  1,  19, 32,
        212, 35,  215, 113, 47,  169, 29,  48,  105, 0,   0,   0,   0,  0,   0,   0,  0,  0,
        74,  129, 203, 106, 120, 203, 49,  107, 178, 19,  84,  144, 4,  50,  84,  1,  15, 25,
        71,  172, 217, 44,  141, 209, 15,  102, 173, 6,   76,  133, 2,  51,  89,  1,  24, 42,
        64,  185, 231, 31,  148, 216, 8,   103, 175, 3,   74,  131, 1,  46,  81,  1,  18, 30,
        65,  196, 235, 25,  157, 221, 5,   105, 174, 1,   67,  120, 1,  38,  69,  1,  15, 30,
        65,  204, 238, 30,  156, 224, 7,   107, 177, 2,   70,  124, 1,  42,  73,  1,  18, 34,
        225, 86,  251, 144, 104, 235, 42,  99,  181, 0,   0,   0,   0,  0,   0,   0,  0,  0,
        85,  175, 239, 112, 165, 229, 29,  136, 200, 12,  103, 162, 6,  77,  123, 2,  53, 84,
        75,  183, 239, 30,  155, 221, 3,   106, 171, 1,   74,  128, 1,  44,  76,  1,  17, 28,
        73,  185, 240, 27,  159, 222, 2,   107, 172, 1,   75,  127, 1,  42,  73,  1,  17, 29,
        62,  190, 238, 21,  159, 222, 2,   107, 172, 1,   72,  122, 1,  40,  71,  1,  18, 32,
        61,  199, 240, 27,  161, 226, 4,   113, 180, 1,   76,  129, 1,  46,  80,  1,  23, 41,
        7,   27,  153, 5,   30,  95,  1,   16,  30,  0,   0,   0,   0,  0,   0,   0,  0,  0,
        50,  75,  127, 57,  75,  124, 27,  67,  108, 10,  54,  86,  1,  33,  52,  1,  12, 18,
        43,  125, 151, 26,  108, 148, 7,   83,  122, 2,   59,  89,  1,  38,  60,  1,  17, 27,
        23,  144, 163, 13,  112, 154, 2,   75,  117, 1,   50,  81,  1,  31,  51,  1,  14, 23,
        18,  162, 185, 6,   123, 171, 1,   78,  125, 1,   51,  86,  1,  31,  54,  1,  14, 23,
        15,  199, 227, 3,   150, 204, 1,   91,  146, 1,   55,  95,  1,  30,  53,  1,  11, 20,
        19,  55,  240, 19,  59,  196, 3,   52,  105, 0,   0,   0,   0,  0,   0,   0,  0,  0,
        41,  166, 207, 104, 153, 199, 31,  123, 181, 14,  101, 152, 5,  72,  106, 1,  36, 52,
        35,  176, 211, 12,  131, 190, 2,   88,  144, 1,   60,  101, 1,  36,  60,  1,  16, 28,
        28,  183, 213, 8,   134, 191, 1,   86,  142, 1,   56,  96,  1,  30,  53,  1,  12, 20,
        20,  190, 215, 4,   135, 192, 1,   84,  139, 1,   53,  91,  1,  28,  49,  1,  11, 20,
        13,  196, 216, 2,   137, 192, 1,   86,  143, 1,   57,  99,  1,  32,  56,  1,  13, 24,
        211, 29,  217, 96,  47,  156, 22,  43,  87,  0,   0,   0,   0,  0,   0,   0,  0,  0,
        78,  120, 193, 111, 116, 186, 46,  102, 164, 15,  80,  128, 2,  49,  76,  1,  18, 28,
        71,  161, 203, 42,  132, 192, 10,  98,  150, 3,   69,  109, 1,  44,  70,  1,  18, 29,
        57,  186, 211, 30,  140, 196, 4,   93,  146, 1,   62,  102, 1,  38,  65,  1,  16, 27,
        47,  199, 217, 14,  145, 196, 1,   88,  142, 1,   57,  98,  1,  36,  62,  1,  15, 26,
        26,  219, 229, 5,   155, 207, 1,   94,  151, 1,   60,  104, 1,  36,  62,  1,  16, 28,
        233, 29,  248, 146, 47,  220, 43,  52,  140, 0,   0,   0,   0,  0,   0,   0,  0,  0,
        100, 163, 232, 179, 161, 222, 63,  142, 204, 37,  113, 174, 26, 89,  137, 18, 68, 97,
        85,  181, 230, 32,  146, 209, 7,   100, 164, 3,   71,  121, 1,  45,  77,  1,  18, 30,
        65,  187, 230, 20,  148, 207, 2,   97,  159, 1,   68,  116, 1,  40,  70,  1,  14, 29,
        40,  194, 227, 8,   147, 204, 1,   94,  155, 1,   65,  112, 1,  39,  66,  1,  14, 26,
        16,  208, 228, 3,   151, 207, 1,   98,  160, 1,   67,  117, 1,  41,  74,  1,  17, 31,
        17,  38,  140, 7,   34,  80,  1,   17,  29,  0,   0,   0,   0,  0,   0,   0,  0,  0,
        37,  75,  128, 41,  76,  128, 26,  66,  116, 12,  52,  94,  2,  32,  55,  1,  10, 16,
        50,  127, 154, 37,  109, 152, 16,  82,  121, 5,   59,  85,  1,  35,  54,  1,  13, 20,
        40,  142, 167, 17,  110, 157, 2,   71,  112, 1,   44,  72,  1,  27,  45,  1,  11, 17,
        30,  175, 188, 9,   124, 169, 1,   74,  116, 1,   48,  78,  1,  30,  49,  1,  11, 18,
        10,  222, 223, 2,   150, 194, 1,   83,  128, 1,   48,  79,  1,  27,  45,  1,  11, 17,
        36,  41,  235, 29,  36,  193, 10,  27,  111, 0,   0,   0,   0,  0,   0,   0,  0,  0,
        85,  165, 222, 177, 162, 215, 110, 135, 195, 57,  113, 168, 23, 83,  120, 10, 49, 61,
        85,  190, 223, 36,  139, 200, 5,   90,  146, 1,   60,  103, 1,  38,  65,  1,  18, 30,
        72,  202, 223, 23,  141, 199, 2,   86,  140, 1,   56,  97,  1,  36,  61,  1,  16, 27,
        55,  218, 225, 13,  145, 200, 1,   86,  141, 1,   57,  99,  1,  35,  61,  1,  13, 22,
        15,  235, 212, 1,   132, 184, 1,   84,  139, 1,   57,  97,  1,  34,  56,  1,  14, 23,
        181, 21,  201, 61,  37,  123, 10,  38,  71,  0,   0,   0,   0,  0,   0,   0,  0,  0,
        47,  106, 172, 95,  104, 173, 42,  93,  159, 18,  77,  131, 4,  50,  81,  1,  17, 23,
        62,  147, 199, 44,  130, 189, 28,  102, 154, 18,  75,  115, 2,  44,  65,  1,  12, 19,
        55,  153, 210, 24,  130, 194, 3,   93,  146, 1,   61,  97,  1,  31,  50,  1,  10, 16,
        49,  186, 223, 17,  148, 204, 1,   96,  142, 1,   53,  83,  1,  26,  44,  1,  11, 17,
        13,  217, 212, 2,   136, 180, 1,   78,  124, 1,   50,  83,  1,  29,  49,  1,  14, 23,
        197, 13,  247, 82,  17,  222, 25,  17,  162, 0,   0,   0,   0,  0,   0,   0,  0,  0,
        126, 186, 247, 234, 191, 243, 176, 177, 234, 104, 158, 220, 66, 128, 186, 55, 90, 137,
        111, 197, 242, 46,  158, 219, 9,   104, 171, 2,   65,  125, 1,  44,  80,  1,  17, 91,
        104, 208, 245, 39,  168, 224, 3,   109, 162, 1,   79,  124, 1,  50,  102, 1,  43, 102,
        84,  220, 246, 31,  177, 231, 2,   115, 180, 1,   79,  134, 1,  55,  77,  1,  60, 79,
        43,  243, 240, 8,   180, 217, 1,   115, 166, 1,   84,  121, 1,  51,  67,  1,  16, 6,
    },
    .switchable_interp_prob{235, 162, 36, 255, 34, 3, 149, 144},
    .inter_mode_prob{
        2,  173, 34, 0,  7,  145, 85, 0,  7,  166, 63, 0,  7,  94,
        66, 0,   8,  64, 46, 0,   17, 81, 31, 0,   25, 29, 30, 0,
    },
    .intra_inter_prob{9, 102, 187, 225},
    .comp_inter_prob{9, 102, 187, 225, 0},
    .single_ref_prob{33, 16, 77, 74, 142, 142, 172, 170, 238, 247},
    .comp_ref_prob{50, 126, 123, 221, 226},
    .tx_32x32_prob{3, 136, 37, 5, 52, 13},
    .tx_16x16_prob{20, 152, 15, 101},
    .tx_8x8_prob{100, 66},
    .skip_probs{192, 128, 64},
    .joints{32, 64, 96},
    .sign{128, 128},
    .classes{
        224, 144, 192, 168, 192, 176, 192, 198, 198, 245,
        216, 128, 176, 160, 176, 176, 192, 198, 198, 208,
    },
    .class_0{216, 208},
    .prob_bits{
        136, 140, 148, 160, 176, 192, 224, 234, 234, 240,
        136, 140, 148, 160, 176, 192, 224, 234, 234, 240,
    },
    .class_0_fr{128, 128, 64, 96, 112, 64, 128, 128, 64, 96, 112, 64},
    .fr{64, 96, 64, 64, 96, 64},
    .class_0_hp{160, 160},
    .high_precision{128, 128},
};

constexpr std::array<s32, 256> norm_lut{
    0, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

constexpr std::array<s32, 254> map_lut{
    20,  21,  22,  23,  24,  25,  0,   26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,
    1,   38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  2,   50,  51,  52,  53,  54,
    55,  56,  57,  58,  59,  60,  61,  3,   62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,
    73,  4,   74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  5,   86,  87,  88,  89,
    90,  91,  92,  93,  94,  95,  96,  97,  6,   98,  99,  100, 101, 102, 103, 104, 105, 106, 107,
    108, 109, 7,   110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 8,   122, 123, 124,
    125, 126, 127, 128, 129, 130, 131, 132, 133, 9,   134, 135, 136, 137, 138, 139, 140, 141, 142,
    143, 144, 145, 10,  146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 11,  158, 159,
    160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 12,  170, 171, 172, 173, 174, 175, 176, 177,
    178, 179, 180, 181, 13,  182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 14,  194,
    195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 15,  206, 207, 208, 209, 210, 211, 212,
    213, 214, 215, 216, 217, 16,  218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 17,
    230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 18,  242, 243, 244, 245, 246, 247,
    248, 249, 250, 251, 252, 253, 19,
};

// 6.2.14 Tile size calculation

[[nodiscard]] s32 CalcMinLog2TileCols(s32 frame_width) {
    const s32 sb64_cols = (frame_width + 63) / 64;
    s32 min_log2 = 0;

    while ((64 << min_log2) < sb64_cols) {
        min_log2++;
    }

    return min_log2;
}

[[nodiscard]] s32 CalcMaxLog2TileCols(s32 frame_width) {
    const s32 sb64_cols = (frame_width + 63) / 64;
    s32 max_log2 = 1;

    while ((sb64_cols >> max_log2) >= 4) {
        max_log2++;
    }

    return max_log2 - 1;
}

// Recenters probability. Based on section 6.3.6 of VP9 Specification
[[nodiscard]] s32 RecenterNonNeg(s32 new_prob, s32 old_prob) {
    if (new_prob > old_prob * 2) {
        return new_prob;
    }

    if (new_prob >= old_prob) {
        return (new_prob - old_prob) * 2;
    }

    return (old_prob - new_prob) * 2 - 1;
}

// Adjusts old_prob depending on new_prob. Based on section 6.3.5 of VP9 Specification
[[nodiscard]] s32 RemapProbability(s32 new_prob, s32 old_prob) {
    new_prob--;
    old_prob--;

    std::size_t index{};

    if (old_prob * 2 <= 0xff) {
        index = static_cast<std::size_t>(std::max(0, RecenterNonNeg(new_prob, old_prob) - 1));
    } else {
        index = static_cast<std::size_t>(
            std::max(0, RecenterNonNeg(0xff - 1 - new_prob, 0xff - 1 - old_prob) - 1));
    }

    return map_lut[index];
}
} // Anonymous namespace

VP9::VP9(GPU& gpu_) : gpu{gpu_} {}

VP9::~VP9() = default;

void VP9::WriteProbabilityUpdate(VpxRangeEncoder& writer, u8 new_prob, u8 old_prob) {
    const bool update = new_prob != old_prob;

    writer.Write(update, diff_update_probability);

    if (update) {
        WriteProbabilityDelta(writer, new_prob, old_prob);
    }
}
template <typename T, std::size_t N>
void VP9::WriteProbabilityUpdate(VpxRangeEncoder& writer, const std::array<T, N>& new_prob,
                                 const std::array<T, N>& old_prob) {
    for (std::size_t offset = 0; offset < new_prob.size(); ++offset) {
        WriteProbabilityUpdate(writer, new_prob[offset], old_prob[offset]);
    }
}

template <typename T, std::size_t N>
void VP9::WriteProbabilityUpdateAligned4(VpxRangeEncoder& writer, const std::array<T, N>& new_prob,
                                         const std::array<T, N>& old_prob) {
    for (std::size_t offset = 0; offset < new_prob.size(); offset += 4) {
        WriteProbabilityUpdate(writer, new_prob[offset + 0], old_prob[offset + 0]);
        WriteProbabilityUpdate(writer, new_prob[offset + 1], old_prob[offset + 1]);
        WriteProbabilityUpdate(writer, new_prob[offset + 2], old_prob[offset + 2]);
    }
}

void VP9::WriteProbabilityDelta(VpxRangeEncoder& writer, u8 new_prob, u8 old_prob) {
    const int delta = RemapProbability(new_prob, old_prob);

    EncodeTermSubExp(writer, delta);
}

void VP9::EncodeTermSubExp(VpxRangeEncoder& writer, s32 value) {
    if (WriteLessThan(writer, value, 16)) {
        writer.Write(value, 4);
    } else if (WriteLessThan(writer, value, 32)) {
        writer.Write(value - 16, 4);
    } else if (WriteLessThan(writer, value, 64)) {
        writer.Write(value - 32, 5);
    } else {
        value -= 64;

        constexpr s32 size = 8;

        const s32 mask = (1 << size) - 191;

        const s32 delta = value - mask;

        if (delta < 0) {
            writer.Write(value, size - 1);
        } else {
            writer.Write(delta / 2 + mask, size - 1);
            writer.Write(delta & 1, 1);
        }
    }
}

bool VP9::WriteLessThan(VpxRangeEncoder& writer, s32 value, s32 test) {
    const bool is_lt = value < test;
    writer.Write(!is_lt);
    return is_lt;
}

void VP9::WriteCoefProbabilityUpdate(VpxRangeEncoder& writer, s32 tx_mode,
                                     const std::array<u8, 1728>& new_prob,
                                     const std::array<u8, 1728>& old_prob) {
    constexpr u32 block_bytes = 2 * 2 * 6 * 6 * 3;

    const auto needs_update = [&](u32 base_index) {
        return !std::equal(new_prob.begin() + base_index,
                           new_prob.begin() + base_index + block_bytes,
                           old_prob.begin() + base_index);
    };

    for (u32 block_index = 0; block_index < 4; block_index++) {
        const u32 base_index = block_index * block_bytes;
        const bool update = needs_update(base_index);
        writer.Write(update);

        if (update) {
            u32 index = base_index;
            for (s32 i = 0; i < 2; i++) {
                for (s32 j = 0; j < 2; j++) {
                    for (s32 k = 0; k < 6; k++) {
                        for (s32 l = 0; l < 6; l++) {
                            if (k != 0 || l < 3) {
                                WriteProbabilityUpdate(writer, new_prob[index + 0],
                                                       old_prob[index + 0]);
                                WriteProbabilityUpdate(writer, new_prob[index + 1],
                                                       old_prob[index + 1]);
                                WriteProbabilityUpdate(writer, new_prob[index + 2],
                                                       old_prob[index + 2]);
                            }
                            index += 3;
                        }
                    }
                }
            }
        }
        if (block_index == static_cast<u32>(tx_mode)) {
            break;
        }
    }
}

void VP9::WriteMvProbabilityUpdate(VpxRangeEncoder& writer, u8 new_prob, u8 old_prob) {
    const bool update = new_prob != old_prob;
    writer.Write(update, diff_update_probability);

    if (update) {
        writer.Write(new_prob >> 1, 7);
    }
}

Vp9PictureInfo VP9::GetVp9PictureInfo(const NvdecCommon::NvdecRegisters& state) {
    PictureInfo picture_info;
    gpu.MemoryManager().ReadBlock(state.picture_info_offset, &picture_info, sizeof(PictureInfo));
    Vp9PictureInfo vp9_info = picture_info.Convert();

    InsertEntropy(state.vp9_entropy_probs_offset, vp9_info.entropy);

    // surface_luma_offset[0:3] contains the address of the reference frame offsets in the following
    // order: last, golden, altref, current.
    std::copy(state.surface_luma_offset.begin(), state.surface_luma_offset.begin() + 4,
              vp9_info.frame_offsets.begin());

    return vp9_info;
}

void VP9::InsertEntropy(u64 offset, Vp9EntropyProbs& dst) {
    EntropyProbs entropy;
    gpu.MemoryManager().ReadBlock(offset, &entropy, sizeof(EntropyProbs));
    entropy.Convert(dst);
}

Vp9FrameContainer VP9::GetCurrentFrame(const NvdecCommon::NvdecRegisters& state) {
    Vp9FrameContainer current_frame{};
    {
        gpu.SyncGuestHost();
        current_frame.info = GetVp9PictureInfo(state);
        current_frame.bit_stream.resize(current_frame.info.bitstream_size);
        gpu.MemoryManager().ReadBlock(state.frame_bitstream_offset, current_frame.bit_stream.data(),
                                      current_frame.info.bitstream_size);
    }
    if (!next_frame.bit_stream.empty()) {
        Vp9FrameContainer temp{
            .info = current_frame.info,
            .bit_stream = std::move(current_frame.bit_stream),
        };
        next_frame.info.show_frame = current_frame.info.last_frame_shown;
        current_frame.info = next_frame.info;
        current_frame.bit_stream = std::move(next_frame.bit_stream);
        next_frame = std::move(temp);
    } else {
        next_frame.info = current_frame.info;
        next_frame.bit_stream = current_frame.bit_stream;
    }
    return current_frame;
}

std::vector<u8> VP9::ComposeCompressedHeader() {
    VpxRangeEncoder writer{};
    const bool update_probs = !current_frame_info.is_key_frame && current_frame_info.show_frame;
    if (!current_frame_info.lossless) {
        if (static_cast<u32>(current_frame_info.transform_mode) >= 3) {
            writer.Write(3, 2);
            writer.Write(current_frame_info.transform_mode == 4);
        } else {
            writer.Write(current_frame_info.transform_mode, 2);
        }
    }

    if (current_frame_info.transform_mode == 4) {
        // tx_mode_probs() in the spec
        WriteProbabilityUpdate(writer, current_frame_info.entropy.tx_8x8_prob,
                               prev_frame_probs.tx_8x8_prob);
        WriteProbabilityUpdate(writer, current_frame_info.entropy.tx_16x16_prob,
                               prev_frame_probs.tx_16x16_prob);
        WriteProbabilityUpdate(writer, current_frame_info.entropy.tx_32x32_prob,
                               prev_frame_probs.tx_32x32_prob);
        if (update_probs) {
            prev_frame_probs.tx_8x8_prob = current_frame_info.entropy.tx_8x8_prob;
            prev_frame_probs.tx_16x16_prob = current_frame_info.entropy.tx_16x16_prob;
            prev_frame_probs.tx_32x32_prob = current_frame_info.entropy.tx_32x32_prob;
        }
    }
    // read_coef_probs()  in the spec
    WriteCoefProbabilityUpdate(writer, current_frame_info.transform_mode,
                               current_frame_info.entropy.coef_probs, prev_frame_probs.coef_probs);
    // read_skip_probs()  in the spec
    WriteProbabilityUpdate(writer, current_frame_info.entropy.skip_probs,
                           prev_frame_probs.skip_probs);

    if (update_probs) {
        prev_frame_probs.coef_probs = current_frame_info.entropy.coef_probs;
        prev_frame_probs.skip_probs = current_frame_info.entropy.skip_probs;
    }

    if (!current_frame_info.intra_only) {
        // read_inter_probs() in the spec
        WriteProbabilityUpdateAligned4(writer, current_frame_info.entropy.inter_mode_prob,
                                       prev_frame_probs.inter_mode_prob);

        if (current_frame_info.interp_filter == 4) {
            // read_interp_filter_probs() in the spec
            WriteProbabilityUpdate(writer, current_frame_info.entropy.switchable_interp_prob,
                                   prev_frame_probs.switchable_interp_prob);
            if (update_probs) {
                prev_frame_probs.switchable_interp_prob =
                    current_frame_info.entropy.switchable_interp_prob;
            }
        }

        // read_is_inter_probs() in the spec
        WriteProbabilityUpdate(writer, current_frame_info.entropy.intra_inter_prob,
                               prev_frame_probs.intra_inter_prob);

        // frame_reference_mode() in the spec
        if ((current_frame_info.ref_frame_sign_bias[1] & 1) !=
                (current_frame_info.ref_frame_sign_bias[2] & 1) ||
            (current_frame_info.ref_frame_sign_bias[1] & 1) !=
                (current_frame_info.ref_frame_sign_bias[3] & 1)) {
            if (current_frame_info.reference_mode >= 1) {
                writer.Write(1, 1);
                writer.Write(current_frame_info.reference_mode == 2);
            } else {
                writer.Write(0, 1);
            }
        }

        // frame_reference_mode_probs() in the spec
        if (current_frame_info.reference_mode == 2) {
            WriteProbabilityUpdate(writer, current_frame_info.entropy.comp_inter_prob,
                                   prev_frame_probs.comp_inter_prob);
            if (update_probs) {
                prev_frame_probs.comp_inter_prob = current_frame_info.entropy.comp_inter_prob;
            }
        }

        if (current_frame_info.reference_mode != 1) {
            WriteProbabilityUpdate(writer, current_frame_info.entropy.single_ref_prob,
                                   prev_frame_probs.single_ref_prob);
            if (update_probs) {
                prev_frame_probs.single_ref_prob = current_frame_info.entropy.single_ref_prob;
            }
        }

        if (current_frame_info.reference_mode != 0) {
            WriteProbabilityUpdate(writer, current_frame_info.entropy.comp_ref_prob,
                                   prev_frame_probs.comp_ref_prob);
            if (update_probs) {
                prev_frame_probs.comp_ref_prob = current_frame_info.entropy.comp_ref_prob;
            }
        }

        // read_y_mode_probs
        for (std::size_t index = 0; index < current_frame_info.entropy.y_mode_prob.size();
             ++index) {
            WriteProbabilityUpdate(writer, current_frame_info.entropy.y_mode_prob[index],
                                   prev_frame_probs.y_mode_prob[index]);
        }

        // read_partition_probs
        WriteProbabilityUpdateAligned4(writer, current_frame_info.entropy.partition_prob,
                                       prev_frame_probs.partition_prob);

        // mv_probs
        for (s32 i = 0; i < 3; i++) {
            WriteMvProbabilityUpdate(writer, current_frame_info.entropy.joints[i],
                                     prev_frame_probs.joints[i]);
        }
        if (update_probs) {
            prev_frame_probs.inter_mode_prob = current_frame_info.entropy.inter_mode_prob;
            prev_frame_probs.intra_inter_prob = current_frame_info.entropy.intra_inter_prob;
            prev_frame_probs.y_mode_prob = current_frame_info.entropy.y_mode_prob;
            prev_frame_probs.partition_prob = current_frame_info.entropy.partition_prob;
            prev_frame_probs.joints = current_frame_info.entropy.joints;
        }

        for (s32 i = 0; i < 2; i++) {
            WriteMvProbabilityUpdate(writer, current_frame_info.entropy.sign[i],
                                     prev_frame_probs.sign[i]);
            for (s32 j = 0; j < 10; j++) {
                const int index = i * 10 + j;
                WriteMvProbabilityUpdate(writer, current_frame_info.entropy.classes[index],
                                         prev_frame_probs.classes[index]);
            }
            WriteMvProbabilityUpdate(writer, current_frame_info.entropy.class_0[i],
                                     prev_frame_probs.class_0[i]);

            for (s32 j = 0; j < 10; j++) {
                const int index = i * 10 + j;
                WriteMvProbabilityUpdate(writer, current_frame_info.entropy.prob_bits[index],
                                         prev_frame_probs.prob_bits[index]);
            }
        }

        for (s32 i = 0; i < 2; i++) {
            for (s32 j = 0; j < 2; j++) {
                for (s32 k = 0; k < 3; k++) {
                    const int index = i * 2 * 3 + j * 3 + k;
                    WriteMvProbabilityUpdate(writer, current_frame_info.entropy.class_0_fr[index],
                                             prev_frame_probs.class_0_fr[index]);
                }
            }

            for (s32 j = 0; j < 3; j++) {
                const int index = i * 3 + j;
                WriteMvProbabilityUpdate(writer, current_frame_info.entropy.fr[index],
                                         prev_frame_probs.fr[index]);
            }
        }

        if (current_frame_info.allow_high_precision_mv) {
            for (s32 index = 0; index < 2; index++) {
                WriteMvProbabilityUpdate(writer, current_frame_info.entropy.class_0_hp[index],
                                         prev_frame_probs.class_0_hp[index]);
                WriteMvProbabilityUpdate(writer, current_frame_info.entropy.high_precision[index],
                                         prev_frame_probs.high_precision[index]);
            }
        }

        // save previous probs
        if (update_probs) {
            prev_frame_probs.sign = current_frame_info.entropy.sign;
            prev_frame_probs.classes = current_frame_info.entropy.classes;
            prev_frame_probs.class_0 = current_frame_info.entropy.class_0;
            prev_frame_probs.prob_bits = current_frame_info.entropy.prob_bits;
            prev_frame_probs.class_0_fr = current_frame_info.entropy.class_0_fr;
            prev_frame_probs.fr = current_frame_info.entropy.fr;
            prev_frame_probs.class_0_hp = current_frame_info.entropy.class_0_hp;
            prev_frame_probs.high_precision = current_frame_info.entropy.high_precision;
        }
    }
    writer.End();
    return writer.GetBuffer();
}

VpxBitStreamWriter VP9::ComposeUncompressedHeader() {
    VpxBitStreamWriter uncomp_writer{};

    uncomp_writer.WriteU(2, 2);                                      // Frame marker.
    uncomp_writer.WriteU(0, 2);                                      // Profile.
    uncomp_writer.WriteBit(false);                                   // Show existing frame.
    uncomp_writer.WriteBit(!current_frame_info.is_key_frame);        // is key frame?
    uncomp_writer.WriteBit(current_frame_info.show_frame);           // show frame?
    uncomp_writer.WriteBit(current_frame_info.error_resilient_mode); // error reslience

    if (current_frame_info.is_key_frame) {
        uncomp_writer.WriteU(frame_sync_code, 24);
        uncomp_writer.WriteU(0, 3); // Color space.
        uncomp_writer.WriteU(0, 1); // Color range.
        uncomp_writer.WriteU(current_frame_info.frame_size.width - 1, 16);
        uncomp_writer.WriteU(current_frame_info.frame_size.height - 1, 16);
        uncomp_writer.WriteBit(false); // Render and frame size different.

        // Reset context
        prev_frame_probs = default_probs;
        swap_ref_indices = false;
        loop_filter_ref_deltas.fill(0);
        loop_filter_mode_deltas.fill(0);
        frame_ctxs.fill(default_probs);

        // intra only, meaning the frame can be recreated with no other references
        current_frame_info.intra_only = true;
    } else {
        if (!current_frame_info.show_frame) {
            uncomp_writer.WriteBit(current_frame_info.intra_only);
        } else {
            current_frame_info.intra_only = false;
        }
        if (!current_frame_info.error_resilient_mode) {
            uncomp_writer.WriteU(0, 2); // Reset frame context.
        }
        const auto& curr_offsets = current_frame_info.frame_offsets;
        const auto& next_offsets = next_frame.info.frame_offsets;
        const bool ref_frames_different = curr_offsets[1] != curr_offsets[2];
        const bool next_references_swap =
            (next_offsets[1] == curr_offsets[2]) || (next_offsets[2] == curr_offsets[1]);
        const bool needs_ref_swap = ref_frames_different && next_references_swap;
        if (needs_ref_swap) {
            swap_ref_indices = !swap_ref_indices;
        }
        union {
            u32 raw;
            BitField<0, 1, u32> refresh_last;
            BitField<1, 2, u32> refresh_golden;
            BitField<2, 1, u32> refresh_alt;
        } refresh_frame_flags;

        refresh_frame_flags.raw = 0;
        for (u32 index = 0; index < 3; ++index) {
            // Refresh indices that use the current frame as an index
            if (curr_offsets[3] == next_offsets[index]) {
                refresh_frame_flags.raw |= 1u << index;
            }
        }
        if (swap_ref_indices) {
            const u32 temp = refresh_frame_flags.refresh_golden;
            refresh_frame_flags.refresh_golden.Assign(refresh_frame_flags.refresh_alt.Value());
            refresh_frame_flags.refresh_alt.Assign(temp);
        }
        if (current_frame_info.intra_only) {
            uncomp_writer.WriteU(frame_sync_code, 24);
            uncomp_writer.WriteU(refresh_frame_flags.raw, 8);
            uncomp_writer.WriteU(current_frame_info.frame_size.width - 1, 16);
            uncomp_writer.WriteU(current_frame_info.frame_size.height - 1, 16);
            uncomp_writer.WriteBit(false); // Render and frame size different.
        } else {
            const bool swap_indices = needs_ref_swap ^ swap_ref_indices;
            const auto ref_frame_index = swap_indices ? std::array{0, 2, 1} : std::array{0, 1, 2};
            uncomp_writer.WriteU(refresh_frame_flags.raw, 8);
            for (size_t index = 1; index < 4; index++) {
                uncomp_writer.WriteU(ref_frame_index[index - 1], 3);
                uncomp_writer.WriteU(current_frame_info.ref_frame_sign_bias[index], 1);
            }
            uncomp_writer.WriteBit(true);  // Frame size with refs.
            uncomp_writer.WriteBit(false); // Render and frame size different.
            uncomp_writer.WriteBit(current_frame_info.allow_high_precision_mv);
            uncomp_writer.WriteBit(current_frame_info.interp_filter == 4);

            if (current_frame_info.interp_filter != 4) {
                uncomp_writer.WriteU(current_frame_info.interp_filter, 2);
            }
        }
    }

    if (!current_frame_info.error_resilient_mode) {
        uncomp_writer.WriteBit(true); // Refresh frame context. where do i get this info from?
        uncomp_writer.WriteBit(true); // Frame parallel decoding mode.
    }

    int frame_ctx_idx = 0;
    if (!current_frame_info.show_frame) {
        frame_ctx_idx = 1;
    }

    uncomp_writer.WriteU(frame_ctx_idx, 2);       // Frame context index.
    prev_frame_probs = frame_ctxs[frame_ctx_idx]; // reference probabilities for compressed header
    frame_ctxs[frame_ctx_idx] = current_frame_info.entropy;

    uncomp_writer.WriteU(current_frame_info.first_level, 6);
    uncomp_writer.WriteU(current_frame_info.sharpness_level, 3);
    uncomp_writer.WriteBit(current_frame_info.mode_ref_delta_enabled);

    if (current_frame_info.mode_ref_delta_enabled) {
        // check if ref deltas are different, update accordingly
        std::array<bool, 4> update_loop_filter_ref_deltas;
        std::array<bool, 2> update_loop_filter_mode_deltas;

        bool loop_filter_delta_update = false;

        for (std::size_t index = 0; index < current_frame_info.ref_deltas.size(); index++) {
            const s8 old_deltas = loop_filter_ref_deltas[index];
            const s8 new_deltas = current_frame_info.ref_deltas[index];
            const bool differing_delta = old_deltas != new_deltas;

            update_loop_filter_ref_deltas[index] = differing_delta;
            loop_filter_delta_update |= differing_delta;
        }

        for (std::size_t index = 0; index < current_frame_info.mode_deltas.size(); index++) {
            const s8 old_deltas = loop_filter_mode_deltas[index];
            const s8 new_deltas = current_frame_info.mode_deltas[index];
            const bool differing_delta = old_deltas != new_deltas;

            update_loop_filter_mode_deltas[index] = differing_delta;
            loop_filter_delta_update |= differing_delta;
        }

        uncomp_writer.WriteBit(loop_filter_delta_update);

        if (loop_filter_delta_update) {
            for (std::size_t index = 0; index < current_frame_info.ref_deltas.size(); index++) {
                uncomp_writer.WriteBit(update_loop_filter_ref_deltas[index]);

                if (update_loop_filter_ref_deltas[index]) {
                    uncomp_writer.WriteS(current_frame_info.ref_deltas[index], 6);
                }
            }

            for (std::size_t index = 0; index < current_frame_info.mode_deltas.size(); index++) {
                uncomp_writer.WriteBit(update_loop_filter_mode_deltas[index]);

                if (update_loop_filter_mode_deltas[index]) {
                    uncomp_writer.WriteS(current_frame_info.mode_deltas[index], 6);
                }
            }
            // save new deltas
            loop_filter_ref_deltas = current_frame_info.ref_deltas;
            loop_filter_mode_deltas = current_frame_info.mode_deltas;
        }
    }

    uncomp_writer.WriteU(current_frame_info.base_q_index, 8);

    uncomp_writer.WriteDeltaQ(current_frame_info.y_dc_delta_q);
    uncomp_writer.WriteDeltaQ(current_frame_info.uv_dc_delta_q);
    uncomp_writer.WriteDeltaQ(current_frame_info.uv_ac_delta_q);

    ASSERT(!current_frame_info.segment_enabled);
    uncomp_writer.WriteBit(false); // Segmentation enabled (TODO).

    const s32 min_tile_cols_log2 = CalcMinLog2TileCols(current_frame_info.frame_size.width);
    const s32 max_tile_cols_log2 = CalcMaxLog2TileCols(current_frame_info.frame_size.width);

    const s32 tile_cols_log2_diff = current_frame_info.log2_tile_cols - min_tile_cols_log2;
    const s32 tile_cols_log2_inc_mask = (1 << tile_cols_log2_diff) - 1;

    // If it's less than the maximum, we need to add an extra 0 on the bitstream
    // to indicate that it should stop reading.
    if (current_frame_info.log2_tile_cols < max_tile_cols_log2) {
        uncomp_writer.WriteU(tile_cols_log2_inc_mask << 1, tile_cols_log2_diff + 1);
    } else {
        uncomp_writer.WriteU(tile_cols_log2_inc_mask, tile_cols_log2_diff);
    }

    const bool tile_rows_log2_is_nonzero = current_frame_info.log2_tile_rows != 0;

    uncomp_writer.WriteBit(tile_rows_log2_is_nonzero);

    if (tile_rows_log2_is_nonzero) {
        uncomp_writer.WriteBit(current_frame_info.log2_tile_rows > 1);
    }

    return uncomp_writer;
}

const std::vector<u8>& VP9::ComposeFrameHeader(const NvdecCommon::NvdecRegisters& state) {
    std::vector<u8> bitstream;
    {
        Vp9FrameContainer curr_frame = GetCurrentFrame(state);
        current_frame_info = curr_frame.info;
        bitstream = std::move(curr_frame.bit_stream);
    }
    // The uncompressed header routine sets PrevProb parameters needed for the compressed header
    auto uncomp_writer = ComposeUncompressedHeader();
    std::vector<u8> compressed_header = ComposeCompressedHeader();

    uncomp_writer.WriteU(static_cast<s32>(compressed_header.size()), 16);
    uncomp_writer.Flush();
    std::vector<u8> uncompressed_header = uncomp_writer.GetByteArray();

    // Write headers and frame to buffer
    frame.resize(uncompressed_header.size() + compressed_header.size() + bitstream.size());
    std::copy(uncompressed_header.begin(), uncompressed_header.end(), frame.begin());
    std::copy(compressed_header.begin(), compressed_header.end(),
              frame.begin() + uncompressed_header.size());
    std::copy(bitstream.begin(), bitstream.end(),
              frame.begin() + uncompressed_header.size() + compressed_header.size());
    return frame;
}

VpxRangeEncoder::VpxRangeEncoder() {
    Write(false);
}

VpxRangeEncoder::~VpxRangeEncoder() = default;

void VpxRangeEncoder::Write(s32 value, s32 value_size) {
    for (s32 bit = value_size - 1; bit >= 0; bit--) {
        Write(((value >> bit) & 1) != 0);
    }
}

void VpxRangeEncoder::Write(bool bit) {
    Write(bit, half_probability);
}

void VpxRangeEncoder::Write(bool bit, s32 probability) {
    u32 local_range = range;
    const u32 split = 1 + (((local_range - 1) * static_cast<u32>(probability)) >> 8);
    local_range = split;

    if (bit) {
        low_value += split;
        local_range = range - split;
    }

    s32 shift = norm_lut[local_range];
    local_range <<= shift;
    count += shift;

    if (count >= 0) {
        const s32 offset = shift - count;

        if (((low_value << (offset - 1)) >> 31) != 0) {
            const s32 current_pos = static_cast<s32>(base_stream.GetPosition());
            base_stream.Seek(-1, Common::SeekOrigin::FromCurrentPos);
            while (PeekByte() == 0xff) {
                base_stream.WriteByte(0);

                base_stream.Seek(-2, Common::SeekOrigin::FromCurrentPos);
            }
            base_stream.WriteByte(static_cast<u8>((PeekByte() + 1)));
            base_stream.Seek(current_pos, Common::SeekOrigin::SetOrigin);
        }
        base_stream.WriteByte(static_cast<u8>((low_value >> (24 - offset))));

        low_value <<= offset;
        shift = count;
        low_value &= 0xffffff;
        count -= 8;
    }

    low_value <<= shift;
    range = local_range;
}

void VpxRangeEncoder::End() {
    for (std::size_t index = 0; index < 32; ++index) {
        Write(false);
    }
}

u8 VpxRangeEncoder::PeekByte() {
    const u8 value = base_stream.ReadByte();
    base_stream.Seek(-1, Common::SeekOrigin::FromCurrentPos);

    return value;
}

VpxBitStreamWriter::VpxBitStreamWriter() = default;

VpxBitStreamWriter::~VpxBitStreamWriter() = default;

void VpxBitStreamWriter::WriteU(u32 value, u32 value_size) {
    WriteBits(value, value_size);
}

void VpxBitStreamWriter::WriteS(s32 value, u32 value_size) {
    const bool sign = value < 0;
    if (sign) {
        value = -value;
    }

    WriteBits(static_cast<u32>(value << 1) | (sign ? 1 : 0), value_size + 1);
}

void VpxBitStreamWriter::WriteDeltaQ(u32 value) {
    const bool delta_coded = value != 0;
    WriteBit(delta_coded);

    if (delta_coded) {
        WriteBits(value, 4);
    }
}

void VpxBitStreamWriter::WriteBits(u32 value, u32 bit_count) {
    s32 value_pos = 0;
    s32 remaining = bit_count;

    while (remaining > 0) {
        s32 copy_size = remaining;

        const s32 free = GetFreeBufferBits();

        if (copy_size > free) {
            copy_size = free;
        }

        const s32 mask = (1 << copy_size) - 1;

        const s32 src_shift = (bit_count - value_pos) - copy_size;
        const s32 dst_shift = (buffer_size - buffer_pos) - copy_size;

        buffer |= ((value >> src_shift) & mask) << dst_shift;

        value_pos += copy_size;
        buffer_pos += copy_size;
        remaining -= copy_size;
    }
}

void VpxBitStreamWriter::WriteBit(bool state) {
    WriteBits(state ? 1 : 0, 1);
}

s32 VpxBitStreamWriter::GetFreeBufferBits() {
    if (buffer_pos == buffer_size) {
        Flush();
    }

    return buffer_size - buffer_pos;
}

void VpxBitStreamWriter::Flush() {
    if (buffer_pos == 0) {
        return;
    }
    byte_array.push_back(static_cast<u8>(buffer));
    buffer = 0;
    buffer_pos = 0;
}

std::vector<u8>& VpxBitStreamWriter::GetByteArray() {
    return byte_array;
}

const std::vector<u8>& VpxBitStreamWriter::GetByteArray() const {
    return byte_array;
}

} // namespace Tegra::Decoder
