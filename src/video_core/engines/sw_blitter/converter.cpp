// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <bit>
#include <cmath>
#include <span>
#include <unordered_map>

#include "common/assert.h"
#include "video_core/engines/sw_blitter/converter.h"
#include "video_core/surface.h"
#include "video_core/textures/decoders.h"

#ifdef _MSC_VER
#define FORCE_INLINE __forceinline
#else
#define FORCE_INLINE inline __attribute__((always_inline))
#endif

namespace Tegra::Engines::Blitter {

enum class Swizzle : size_t {
    R = 0,
    G = 1,
    B = 2,
    A = 3,
    None,
};

enum class ComponentType : u32 {
    SNORM = 1,
    UNORM = 2,
    SINT = 3,
    UINT = 4,
    SNORM_FORCE_FP16 = 5,
    UNORM_FORCE_FP16 = 6,
    FLOAT = 7,
    SRGB = 8,
};

namespace {

/*
 * Note: Use generate_converters.py to generate the structs and searches for new render target
 * formats and copy paste them to this file in order to update. just call "python
 * generate_converters.py" and get the code from the output. modify the file to add new formats.
 */

constexpr std::array<f32, 256> SRGB_TO_RGB_LUT = {
    0.000000e+00f, 3.035270e-04f, 6.070540e-04f, 9.105810e-04f, 1.214108e-03f, 1.517635e-03f,
    1.821162e-03f, 2.124689e-03f, 2.428216e-03f, 2.731743e-03f, 3.035270e-03f, 3.346536e-03f,
    3.676507e-03f, 4.024717e-03f, 4.391442e-03f, 4.776953e-03f, 5.181517e-03f, 5.605392e-03f,
    6.048833e-03f, 6.512091e-03f, 6.995410e-03f, 7.499032e-03f, 8.023193e-03f, 8.568126e-03f,
    9.134059e-03f, 9.721218e-03f, 1.032982e-02f, 1.096009e-02f, 1.161224e-02f, 1.228649e-02f,
    1.298303e-02f, 1.370208e-02f, 1.444384e-02f, 1.520851e-02f, 1.599629e-02f, 1.680738e-02f,
    1.764195e-02f, 1.850022e-02f, 1.938236e-02f, 2.028856e-02f, 2.121901e-02f, 2.217389e-02f,
    2.315337e-02f, 2.415763e-02f, 2.518686e-02f, 2.624122e-02f, 2.732089e-02f, 2.842604e-02f,
    2.955684e-02f, 3.071344e-02f, 3.189603e-02f, 3.310477e-02f, 3.433981e-02f, 3.560131e-02f,
    3.688945e-02f, 3.820437e-02f, 3.954624e-02f, 4.091520e-02f, 4.231141e-02f, 4.373503e-02f,
    4.518620e-02f, 4.666509e-02f, 4.817183e-02f, 4.970657e-02f, 5.126946e-02f, 5.286065e-02f,
    5.448028e-02f, 5.612849e-02f, 5.780543e-02f, 5.951124e-02f, 6.124605e-02f, 6.301001e-02f,
    6.480327e-02f, 6.662594e-02f, 6.847817e-02f, 7.036009e-02f, 7.227185e-02f, 7.421357e-02f,
    7.618538e-02f, 7.818742e-02f, 8.021982e-02f, 8.228271e-02f, 8.437621e-02f, 8.650046e-02f,
    8.865558e-02f, 9.084171e-02f, 9.305897e-02f, 9.530747e-02f, 9.758735e-02f, 9.989873e-02f,
    1.022417e-01f, 1.046165e-01f, 1.070231e-01f, 1.094617e-01f, 1.119324e-01f, 1.144354e-01f,
    1.169707e-01f, 1.195384e-01f, 1.221388e-01f, 1.247718e-01f, 1.274377e-01f, 1.301365e-01f,
    1.328683e-01f, 1.356333e-01f, 1.384316e-01f, 1.412633e-01f, 1.441285e-01f, 1.470273e-01f,
    1.499598e-01f, 1.529261e-01f, 1.559265e-01f, 1.589608e-01f, 1.620294e-01f, 1.651322e-01f,
    1.682694e-01f, 1.714411e-01f, 1.746474e-01f, 1.778884e-01f, 1.811642e-01f, 1.844750e-01f,
    1.878208e-01f, 1.912017e-01f, 1.946178e-01f, 1.980693e-01f, 2.015563e-01f, 2.050787e-01f,
    2.086369e-01f, 2.122308e-01f, 2.158605e-01f, 2.195262e-01f, 2.232280e-01f, 2.269659e-01f,
    2.307401e-01f, 2.345506e-01f, 2.383976e-01f, 2.422811e-01f, 2.462013e-01f, 2.501583e-01f,
    2.541521e-01f, 2.581829e-01f, 2.622507e-01f, 2.663556e-01f, 2.704978e-01f, 2.746773e-01f,
    2.788943e-01f, 2.831487e-01f, 2.874408e-01f, 2.917706e-01f, 2.961383e-01f, 3.005438e-01f,
    3.049873e-01f, 3.094689e-01f, 3.139887e-01f, 3.185468e-01f, 3.231432e-01f, 3.277781e-01f,
    3.324515e-01f, 3.371636e-01f, 3.419144e-01f, 3.467041e-01f, 3.515326e-01f, 3.564001e-01f,
    3.613068e-01f, 3.662526e-01f, 3.712377e-01f, 3.762621e-01f, 3.813260e-01f, 3.864294e-01f,
    3.915725e-01f, 3.967552e-01f, 4.019778e-01f, 4.072402e-01f, 4.125426e-01f, 4.178851e-01f,
    4.232677e-01f, 4.286905e-01f, 4.341536e-01f, 4.396572e-01f, 4.452012e-01f, 4.507858e-01f,
    4.564110e-01f, 4.620770e-01f, 4.677838e-01f, 4.735315e-01f, 4.793202e-01f, 4.851499e-01f,
    4.910209e-01f, 4.969330e-01f, 5.028865e-01f, 5.088813e-01f, 5.149177e-01f, 5.209956e-01f,
    5.271151e-01f, 5.332764e-01f, 5.394795e-01f, 5.457245e-01f, 5.520114e-01f, 5.583404e-01f,
    5.647115e-01f, 5.711249e-01f, 5.775805e-01f, 5.840784e-01f, 5.906188e-01f, 5.972018e-01f,
    6.038274e-01f, 6.104956e-01f, 6.172066e-01f, 6.239604e-01f, 6.307572e-01f, 6.375968e-01f,
    6.444797e-01f, 6.514056e-01f, 6.583748e-01f, 6.653873e-01f, 6.724432e-01f, 6.795425e-01f,
    6.866853e-01f, 6.938717e-01f, 7.011019e-01f, 7.083758e-01f, 7.156935e-01f, 7.230551e-01f,
    7.304608e-01f, 7.379104e-01f, 7.454042e-01f, 7.529422e-01f, 7.605245e-01f, 7.681512e-01f,
    7.758222e-01f, 7.835378e-01f, 7.912979e-01f, 7.991027e-01f, 8.069522e-01f, 8.148466e-01f,
    8.227857e-01f, 8.307699e-01f, 8.387990e-01f, 8.468732e-01f, 8.549926e-01f, 8.631572e-01f,
    8.713671e-01f, 8.796224e-01f, 8.879231e-01f, 8.962694e-01f, 9.046612e-01f, 9.130986e-01f,
    9.215819e-01f, 9.301109e-01f, 9.386857e-01f, 9.473065e-01f, 9.559733e-01f, 9.646863e-01f,
    9.734453e-01f, 9.822506e-01f, 9.911021e-01f, 1.000000e+00f};

constexpr std::array<f32, 256> RGB_TO_SRGB_LUT = {
    0.000000e+00f, 4.984009e-02f, 8.494473e-02f, 1.107021e-01f, 1.318038e-01f, 1.500052e-01f,
    1.661857e-01f, 1.808585e-01f, 1.943532e-01f, 2.068957e-01f, 2.186491e-01f, 2.297351e-01f,
    2.402475e-01f, 2.502604e-01f, 2.598334e-01f, 2.690152e-01f, 2.778465e-01f, 2.863614e-01f,
    2.945889e-01f, 3.025538e-01f, 3.102778e-01f, 3.177796e-01f, 3.250757e-01f, 3.321809e-01f,
    3.391081e-01f, 3.458689e-01f, 3.524737e-01f, 3.589320e-01f, 3.652521e-01f, 3.714419e-01f,
    3.775084e-01f, 3.834581e-01f, 3.892968e-01f, 3.950301e-01f, 4.006628e-01f, 4.061998e-01f,
    4.116451e-01f, 4.170030e-01f, 4.222770e-01f, 4.274707e-01f, 4.325873e-01f, 4.376298e-01f,
    4.426010e-01f, 4.475037e-01f, 4.523403e-01f, 4.571131e-01f, 4.618246e-01f, 4.664766e-01f,
    4.710712e-01f, 4.756104e-01f, 4.800958e-01f, 4.845292e-01f, 4.889122e-01f, 4.932462e-01f,
    4.975329e-01f, 5.017734e-01f, 5.059693e-01f, 5.101216e-01f, 5.142317e-01f, 5.183006e-01f,
    5.223295e-01f, 5.263194e-01f, 5.302714e-01f, 5.341862e-01f, 5.380651e-01f, 5.419087e-01f,
    5.457181e-01f, 5.494938e-01f, 5.532369e-01f, 5.569480e-01f, 5.606278e-01f, 5.642771e-01f,
    5.678965e-01f, 5.714868e-01f, 5.750484e-01f, 5.785821e-01f, 5.820884e-01f, 5.855680e-01f,
    5.890211e-01f, 5.924487e-01f, 5.958509e-01f, 5.992285e-01f, 6.025819e-01f, 6.059114e-01f,
    6.092176e-01f, 6.125010e-01f, 6.157619e-01f, 6.190008e-01f, 6.222180e-01f, 6.254140e-01f,
    6.285890e-01f, 6.317436e-01f, 6.348780e-01f, 6.379926e-01f, 6.410878e-01f, 6.441637e-01f,
    6.472208e-01f, 6.502595e-01f, 6.532799e-01f, 6.562824e-01f, 6.592672e-01f, 6.622347e-01f,
    6.651851e-01f, 6.681187e-01f, 6.710356e-01f, 6.739363e-01f, 6.768209e-01f, 6.796897e-01f,
    6.825429e-01f, 6.853807e-01f, 6.882034e-01f, 6.910111e-01f, 6.938041e-01f, 6.965826e-01f,
    6.993468e-01f, 7.020969e-01f, 7.048331e-01f, 7.075556e-01f, 7.102645e-01f, 7.129600e-01f,
    7.156424e-01f, 7.183118e-01f, 7.209683e-01f, 7.236121e-01f, 7.262435e-01f, 7.288625e-01f,
    7.314693e-01f, 7.340640e-01f, 7.366470e-01f, 7.392181e-01f, 7.417776e-01f, 7.443256e-01f,
    7.468624e-01f, 7.493880e-01f, 7.519025e-01f, 7.544061e-01f, 7.568989e-01f, 7.593810e-01f,
    7.618526e-01f, 7.643137e-01f, 7.667645e-01f, 7.692052e-01f, 7.716358e-01f, 7.740564e-01f,
    7.764671e-01f, 7.788681e-01f, 7.812595e-01f, 7.836413e-01f, 7.860138e-01f, 7.883768e-01f,
    7.907307e-01f, 7.930754e-01f, 7.954110e-01f, 7.977377e-01f, 8.000556e-01f, 8.023647e-01f,
    8.046651e-01f, 8.069569e-01f, 8.092403e-01f, 8.115152e-01f, 8.137818e-01f, 8.160402e-01f,
    8.182903e-01f, 8.205324e-01f, 8.227665e-01f, 8.249926e-01f, 8.272109e-01f, 8.294214e-01f,
    8.316242e-01f, 8.338194e-01f, 8.360070e-01f, 8.381871e-01f, 8.403597e-01f, 8.425251e-01f,
    8.446831e-01f, 8.468339e-01f, 8.489776e-01f, 8.511142e-01f, 8.532437e-01f, 8.553662e-01f,
    8.574819e-01f, 8.595907e-01f, 8.616927e-01f, 8.637881e-01f, 8.658767e-01f, 8.679587e-01f,
    8.700342e-01f, 8.721032e-01f, 8.741657e-01f, 8.762218e-01f, 8.782716e-01f, 8.803151e-01f,
    8.823524e-01f, 8.843835e-01f, 8.864085e-01f, 8.884274e-01f, 8.904402e-01f, 8.924471e-01f,
    8.944480e-01f, 8.964431e-01f, 8.984324e-01f, 9.004158e-01f, 9.023935e-01f, 9.043654e-01f,
    9.063318e-01f, 9.082925e-01f, 9.102476e-01f, 9.121972e-01f, 9.141413e-01f, 9.160800e-01f,
    9.180133e-01f, 9.199412e-01f, 9.218637e-01f, 9.237810e-01f, 9.256931e-01f, 9.276000e-01f,
    9.295017e-01f, 9.313982e-01f, 9.332896e-01f, 9.351761e-01f, 9.370575e-01f, 9.389339e-01f,
    9.408054e-01f, 9.426719e-01f, 9.445336e-01f, 9.463905e-01f, 9.482424e-01f, 9.500897e-01f,
    9.519322e-01f, 9.537700e-01f, 9.556032e-01f, 9.574316e-01f, 9.592555e-01f, 9.610748e-01f,
    9.628896e-01f, 9.646998e-01f, 9.665055e-01f, 9.683068e-01f, 9.701037e-01f, 9.718961e-01f,
    9.736842e-01f, 9.754679e-01f, 9.772474e-01f, 9.790225e-01f, 9.807934e-01f, 9.825601e-01f,
    9.843225e-01f, 9.860808e-01f, 9.878350e-01f, 9.895850e-01f, 9.913309e-01f, 9.930727e-01f,
    9.948106e-01f, 9.965444e-01f, 9.982741e-01f, 1.000000e+00f};

} // namespace

struct R32G32B32A32_FLOATTraits {
    static constexpr size_t num_components = 4;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::FLOAT, ComponentType::FLOAT, ComponentType::FLOAT, ComponentType::FLOAT};
    static constexpr std::array<size_t, num_components> component_sizes = {32, 32, 32, 32};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {
        Swizzle::R, Swizzle::G, Swizzle::B, Swizzle::A};
};

struct R32G32B32A32_SINTTraits {
    static constexpr size_t num_components = 4;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::SINT, ComponentType::SINT, ComponentType::SINT, ComponentType::SINT};
    static constexpr std::array<size_t, num_components> component_sizes = {32, 32, 32, 32};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {
        Swizzle::R, Swizzle::G, Swizzle::B, Swizzle::A};
};

struct R32G32B32A32_UINTTraits {
    static constexpr size_t num_components = 4;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::UINT, ComponentType::UINT, ComponentType::UINT, ComponentType::UINT};
    static constexpr std::array<size_t, num_components> component_sizes = {32, 32, 32, 32};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {
        Swizzle::R, Swizzle::G, Swizzle::B, Swizzle::A};
};

struct R32G32B32X32_FLOATTraits {
    static constexpr size_t num_components = 4;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::FLOAT, ComponentType::FLOAT, ComponentType::FLOAT, ComponentType::FLOAT};
    static constexpr std::array<size_t, num_components> component_sizes = {32, 32, 32, 32};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {
        Swizzle::R, Swizzle::G, Swizzle::B, Swizzle::None};
};

struct R32G32B32X32_SINTTraits {
    static constexpr size_t num_components = 4;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::SINT, ComponentType::SINT, ComponentType::SINT, ComponentType::SINT};
    static constexpr std::array<size_t, num_components> component_sizes = {32, 32, 32, 32};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {
        Swizzle::R, Swizzle::G, Swizzle::B, Swizzle::None};
};

struct R32G32B32X32_UINTTraits {
    static constexpr size_t num_components = 4;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::UINT, ComponentType::UINT, ComponentType::UINT, ComponentType::UINT};
    static constexpr std::array<size_t, num_components> component_sizes = {32, 32, 32, 32};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {
        Swizzle::R, Swizzle::G, Swizzle::B, Swizzle::None};
};

struct R16G16B16A16_UNORMTraits {
    static constexpr size_t num_components = 4;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::UNORM, ComponentType::UNORM, ComponentType::UNORM, ComponentType::UNORM};
    static constexpr std::array<size_t, num_components> component_sizes = {16, 16, 16, 16};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {
        Swizzle::R, Swizzle::G, Swizzle::B, Swizzle::A};
};

struct R16G16B16A16_SNORMTraits {
    static constexpr size_t num_components = 4;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::SNORM, ComponentType::SNORM, ComponentType::SNORM, ComponentType::SNORM};
    static constexpr std::array<size_t, num_components> component_sizes = {16, 16, 16, 16};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {
        Swizzle::R, Swizzle::G, Swizzle::B, Swizzle::A};
};

struct R16G16B16A16_SINTTraits {
    static constexpr size_t num_components = 4;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::SINT, ComponentType::SINT, ComponentType::SINT, ComponentType::SINT};
    static constexpr std::array<size_t, num_components> component_sizes = {16, 16, 16, 16};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {
        Swizzle::R, Swizzle::G, Swizzle::B, Swizzle::A};
};

struct R16G16B16A16_UINTTraits {
    static constexpr size_t num_components = 4;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::UINT, ComponentType::UINT, ComponentType::UINT, ComponentType::UINT};
    static constexpr std::array<size_t, num_components> component_sizes = {16, 16, 16, 16};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {
        Swizzle::R, Swizzle::G, Swizzle::B, Swizzle::A};
};

struct R16G16B16A16_FLOATTraits {
    static constexpr size_t num_components = 4;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::FLOAT, ComponentType::FLOAT, ComponentType::FLOAT, ComponentType::FLOAT};
    static constexpr std::array<size_t, num_components> component_sizes = {16, 16, 16, 16};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {
        Swizzle::R, Swizzle::G, Swizzle::B, Swizzle::A};
};

struct R32G32_FLOATTraits {
    static constexpr size_t num_components = 2;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::FLOAT, ComponentType::FLOAT};
    static constexpr std::array<size_t, num_components> component_sizes = {32, 32};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {Swizzle::R,
                                                                              Swizzle::G};
};

struct R32G32_SINTTraits {
    static constexpr size_t num_components = 2;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::SINT, ComponentType::SINT};
    static constexpr std::array<size_t, num_components> component_sizes = {32, 32};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {Swizzle::R,
                                                                              Swizzle::G};
};

struct R32G32_UINTTraits {
    static constexpr size_t num_components = 2;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::UINT, ComponentType::UINT};
    static constexpr std::array<size_t, num_components> component_sizes = {32, 32};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {Swizzle::R,
                                                                              Swizzle::G};
};

struct R16G16B16X16_FLOATTraits {
    static constexpr size_t num_components = 4;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::FLOAT, ComponentType::FLOAT, ComponentType::FLOAT, ComponentType::FLOAT};
    static constexpr std::array<size_t, num_components> component_sizes = {16, 16, 16, 16};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {
        Swizzle::R, Swizzle::G, Swizzle::B, Swizzle::None};
};

struct A8R8G8B8_UNORMTraits {
    static constexpr size_t num_components = 4;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::UNORM, ComponentType::UNORM, ComponentType::UNORM, ComponentType::UNORM};
    static constexpr std::array<size_t, num_components> component_sizes = {8, 8, 8, 8};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {
        Swizzle::A, Swizzle::R, Swizzle::G, Swizzle::B};
};

struct A8R8G8B8_SRGBTraits {
    static constexpr size_t num_components = 4;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::SRGB, ComponentType::SRGB, ComponentType::SRGB, ComponentType::SRGB};
    static constexpr std::array<size_t, num_components> component_sizes = {8, 8, 8, 8};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {
        Swizzle::A, Swizzle::R, Swizzle::G, Swizzle::B};
};

struct A2B10G10R10_UNORMTraits {
    static constexpr size_t num_components = 4;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::UNORM, ComponentType::UNORM, ComponentType::UNORM, ComponentType::UNORM};
    static constexpr std::array<size_t, num_components> component_sizes = {2, 10, 10, 10};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {
        Swizzle::A, Swizzle::B, Swizzle::G, Swizzle::R};
};

struct A2B10G10R10_UINTTraits {
    static constexpr size_t num_components = 4;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::UINT, ComponentType::UINT, ComponentType::UINT, ComponentType::UINT};
    static constexpr std::array<size_t, num_components> component_sizes = {2, 10, 10, 10};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {
        Swizzle::A, Swizzle::B, Swizzle::G, Swizzle::R};
};

struct A2R10G10B10_UNORMTraits {
    static constexpr size_t num_components = 4;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::UNORM, ComponentType::UNORM, ComponentType::UNORM, ComponentType::UNORM};
    static constexpr std::array<size_t, num_components> component_sizes = {2, 10, 10, 10};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {
        Swizzle::A, Swizzle::R, Swizzle::G, Swizzle::B};
};

struct A8B8G8R8_UNORMTraits {
    static constexpr size_t num_components = 4;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::UNORM, ComponentType::UNORM, ComponentType::UNORM, ComponentType::UNORM};
    static constexpr std::array<size_t, num_components> component_sizes = {8, 8, 8, 8};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {
        Swizzle::A, Swizzle::B, Swizzle::G, Swizzle::R};
};

struct A8B8G8R8_SRGBTraits {
    static constexpr size_t num_components = 4;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::SRGB, ComponentType::SRGB, ComponentType::SRGB, ComponentType::SRGB};
    static constexpr std::array<size_t, num_components> component_sizes = {8, 8, 8, 8};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {
        Swizzle::A, Swizzle::B, Swizzle::G, Swizzle::R};
};

struct A8B8G8R8_SNORMTraits {
    static constexpr size_t num_components = 4;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::SNORM, ComponentType::SNORM, ComponentType::SNORM, ComponentType::SNORM};
    static constexpr std::array<size_t, num_components> component_sizes = {8, 8, 8, 8};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {
        Swizzle::A, Swizzle::B, Swizzle::G, Swizzle::R};
};

struct A8B8G8R8_SINTTraits {
    static constexpr size_t num_components = 4;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::SINT, ComponentType::SINT, ComponentType::SINT, ComponentType::SINT};
    static constexpr std::array<size_t, num_components> component_sizes = {8, 8, 8, 8};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {
        Swizzle::A, Swizzle::B, Swizzle::G, Swizzle::R};
};

struct A8B8G8R8_UINTTraits {
    static constexpr size_t num_components = 4;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::UINT, ComponentType::UINT, ComponentType::UINT, ComponentType::UINT};
    static constexpr std::array<size_t, num_components> component_sizes = {8, 8, 8, 8};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {
        Swizzle::A, Swizzle::B, Swizzle::G, Swizzle::R};
};

struct R16G16_UNORMTraits {
    static constexpr size_t num_components = 2;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::UNORM, ComponentType::UNORM};
    static constexpr std::array<size_t, num_components> component_sizes = {16, 16};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {Swizzle::R,
                                                                              Swizzle::G};
};

struct R16G16_SNORMTraits {
    static constexpr size_t num_components = 2;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::SNORM, ComponentType::SNORM};
    static constexpr std::array<size_t, num_components> component_sizes = {16, 16};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {Swizzle::R,
                                                                              Swizzle::G};
};

struct R16G16_SINTTraits {
    static constexpr size_t num_components = 2;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::SINT, ComponentType::SINT};
    static constexpr std::array<size_t, num_components> component_sizes = {16, 16};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {Swizzle::R,
                                                                              Swizzle::G};
};

struct R16G16_UINTTraits {
    static constexpr size_t num_components = 2;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::UINT, ComponentType::UINT};
    static constexpr std::array<size_t, num_components> component_sizes = {16, 16};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {Swizzle::R,
                                                                              Swizzle::G};
};

struct R16G16_FLOATTraits {
    static constexpr size_t num_components = 2;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::FLOAT, ComponentType::FLOAT};
    static constexpr std::array<size_t, num_components> component_sizes = {16, 16};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {Swizzle::R,
                                                                              Swizzle::G};
};

struct B10G11R11_FLOATTraits {
    static constexpr size_t num_components = 3;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::FLOAT, ComponentType::FLOAT, ComponentType::FLOAT};
    static constexpr std::array<size_t, num_components> component_sizes = {10, 11, 11};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {
        Swizzle::B, Swizzle::G, Swizzle::R};
};

struct R32_SINTTraits {
    static constexpr size_t num_components = 1;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::SINT};
    static constexpr std::array<size_t, num_components> component_sizes = {32};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {Swizzle::R};
};

struct R32_UINTTraits {
    static constexpr size_t num_components = 1;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::UINT};
    static constexpr std::array<size_t, num_components> component_sizes = {32};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {Swizzle::R};
};

struct R32_FLOATTraits {
    static constexpr size_t num_components = 1;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::FLOAT};
    static constexpr std::array<size_t, num_components> component_sizes = {32};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {Swizzle::R};
};

struct X8R8G8B8_UNORMTraits {
    static constexpr size_t num_components = 4;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::UNORM, ComponentType::UNORM, ComponentType::UNORM, ComponentType::UNORM};
    static constexpr std::array<size_t, num_components> component_sizes = {8, 8, 8, 8};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {
        Swizzle::None, Swizzle::R, Swizzle::G, Swizzle::B};
};

struct X8R8G8B8_SRGBTraits {
    static constexpr size_t num_components = 4;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::SRGB, ComponentType::SRGB, ComponentType::SRGB, ComponentType::SRGB};
    static constexpr std::array<size_t, num_components> component_sizes = {8, 8, 8, 8};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {
        Swizzle::None, Swizzle::R, Swizzle::G, Swizzle::B};
};

struct R5G6B5_UNORMTraits {
    static constexpr size_t num_components = 3;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::UNORM, ComponentType::UNORM, ComponentType::UNORM};
    static constexpr std::array<size_t, num_components> component_sizes = {5, 6, 5};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {
        Swizzle::R, Swizzle::G, Swizzle::B};
};

struct A1R5G5B5_UNORMTraits {
    static constexpr size_t num_components = 4;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::UNORM, ComponentType::UNORM, ComponentType::UNORM, ComponentType::UNORM};
    static constexpr std::array<size_t, num_components> component_sizes = {1, 5, 5, 5};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {
        Swizzle::A, Swizzle::R, Swizzle::G, Swizzle::B};
};

struct R8G8_UNORMTraits {
    static constexpr size_t num_components = 2;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::UNORM, ComponentType::UNORM};
    static constexpr std::array<size_t, num_components> component_sizes = {8, 8};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {Swizzle::R,
                                                                              Swizzle::G};
};

struct R8G8_SNORMTraits {
    static constexpr size_t num_components = 2;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::SNORM, ComponentType::SNORM};
    static constexpr std::array<size_t, num_components> component_sizes = {8, 8};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {Swizzle::R,
                                                                              Swizzle::G};
};

struct R8G8_SINTTraits {
    static constexpr size_t num_components = 2;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::SINT, ComponentType::SINT};
    static constexpr std::array<size_t, num_components> component_sizes = {8, 8};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {Swizzle::R,
                                                                              Swizzle::G};
};

struct R8G8_UINTTraits {
    static constexpr size_t num_components = 2;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::UINT, ComponentType::UINT};
    static constexpr std::array<size_t, num_components> component_sizes = {8, 8};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {Swizzle::R,
                                                                              Swizzle::G};
};

struct R16_UNORMTraits {
    static constexpr size_t num_components = 1;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::UNORM};
    static constexpr std::array<size_t, num_components> component_sizes = {16};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {Swizzle::R};
};

struct R16_SNORMTraits {
    static constexpr size_t num_components = 1;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::SNORM};
    static constexpr std::array<size_t, num_components> component_sizes = {16};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {Swizzle::R};
};

struct R16_SINTTraits {
    static constexpr size_t num_components = 1;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::SINT};
    static constexpr std::array<size_t, num_components> component_sizes = {16};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {Swizzle::R};
};

struct R16_UINTTraits {
    static constexpr size_t num_components = 1;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::UINT};
    static constexpr std::array<size_t, num_components> component_sizes = {16};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {Swizzle::R};
};

struct R16_FLOATTraits {
    static constexpr size_t num_components = 1;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::FLOAT};
    static constexpr std::array<size_t, num_components> component_sizes = {16};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {Swizzle::R};
};

struct R8_UNORMTraits {
    static constexpr size_t num_components = 1;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::UNORM};
    static constexpr std::array<size_t, num_components> component_sizes = {8};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {Swizzle::R};
};

struct R8_SNORMTraits {
    static constexpr size_t num_components = 1;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::SNORM};
    static constexpr std::array<size_t, num_components> component_sizes = {8};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {Swizzle::R};
};

struct R8_SINTTraits {
    static constexpr size_t num_components = 1;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::SINT};
    static constexpr std::array<size_t, num_components> component_sizes = {8};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {Swizzle::R};
};

struct R8_UINTTraits {
    static constexpr size_t num_components = 1;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::UINT};
    static constexpr std::array<size_t, num_components> component_sizes = {8};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {Swizzle::R};
};

struct X1R5G5B5_UNORMTraits {
    static constexpr size_t num_components = 4;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::UNORM, ComponentType::UNORM, ComponentType::UNORM, ComponentType::UNORM};
    static constexpr std::array<size_t, num_components> component_sizes = {1, 5, 5, 5};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {
        Swizzle::None, Swizzle::R, Swizzle::G, Swizzle::B};
};

struct X8B8G8R8_UNORMTraits {
    static constexpr size_t num_components = 4;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::UNORM, ComponentType::UNORM, ComponentType::UNORM, ComponentType::UNORM};
    static constexpr std::array<size_t, num_components> component_sizes = {8, 8, 8, 8};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {
        Swizzle::None, Swizzle::B, Swizzle::G, Swizzle::R};
};

struct X8B8G8R8_SRGBTraits {
    static constexpr size_t num_components = 4;
    static constexpr std::array<ComponentType, num_components> component_types = {
        ComponentType::SRGB, ComponentType::SRGB, ComponentType::SRGB, ComponentType::SRGB};
    static constexpr std::array<size_t, num_components> component_sizes = {8, 8, 8, 8};
    static constexpr std::array<Swizzle, num_components> component_swizzle = {
        Swizzle::None, Swizzle::B, Swizzle::G, Swizzle::R};
};

template <class ConverterTraits>
class ConverterImpl : public Converter {
private:
    static constexpr size_t num_components = ConverterTraits::num_components;
    static constexpr std::array<ComponentType, num_components> component_types =
        ConverterTraits::component_types;
    static constexpr std::array<size_t, num_components> component_sizes =
        ConverterTraits::component_sizes;
    static constexpr std::array<Swizzle, num_components> component_swizzle =
        ConverterTraits::component_swizzle;

    static constexpr size_t CalculateByteSize() {
        size_t size = 0;
        for (const size_t component_size : component_sizes) {
            size += component_size;
        }
        const size_t power = (sizeof(size_t) * 8) - std::countl_zero(size) - 1ULL;
        const size_t base_size = 1ULL << power;
        const size_t mask = base_size - 1ULL;
        return ((size & mask) != 0 ? base_size << 1ULL : base_size) / 8;
    }

    static constexpr size_t total_bytes_per_pixel = CalculateByteSize();
    static constexpr size_t total_words_per_pixel =
        (total_bytes_per_pixel + sizeof(u32) - 1U) / sizeof(u32);
    static constexpr size_t components_per_ir_rep = 4;

    template <bool get_offsets>
    static constexpr std::array<size_t, num_components> GetBoundWordsOffsets() {
        std::array<size_t, num_components> result;
        result.fill(0);
        constexpr size_t total_bits_per_word = sizeof(u32) * 8;
        size_t accumulated_size = 0;
        size_t count = 0;
        for (size_t i = 0; i < num_components; i++) {
            if constexpr (get_offsets) {
                result[i] = accumulated_size;
            } else {
                result[i] = count;
            }
            accumulated_size += component_sizes[i];
            if (accumulated_size > total_bits_per_word) {
                if constexpr (get_offsets) {
                    result[i] = 0;
                } else {
                    result[i]++;
                }
                count++;
                accumulated_size = component_sizes[i];
            }
        }
        return result;
    }

    static constexpr std::array<size_t, num_components> bound_words = GetBoundWordsOffsets<false>();
    static constexpr std::array<size_t, num_components> bound_offsets =
        GetBoundWordsOffsets<true>();

    static constexpr std::array<u32, num_components> GetComponentsMask() {
        std::array<u32, num_components> result;
        for (size_t i = 0; i < num_components; i++) {
            result[i] = (((u32)~0) >> (8 * sizeof(u32) - component_sizes[i])) << bound_offsets[i];
        }
        return result;
    }

    static constexpr std::array<u32, num_components> component_mask = GetComponentsMask();

    // We are forcing inline so the compiler can SIMD the conversations, since it may do 4 function
    // calls, it may fail to detect the benefit of inlining.
    template <size_t which_component>
    FORCE_INLINE void ConvertToComponent(u32 which_word, f32& out_component) {
        const u32 value = (which_word >> bound_offsets[which_component]) &
                          static_cast<u32>((1ULL << component_sizes[which_component]) - 1ULL);
        const auto sign_extend = [](u32 base_value, size_t bits) {
            const size_t shift_amount = sizeof(u32) * 8 - bits;
            s32 shifted_value = static_cast<s32>(base_value << shift_amount);
            return shifted_value >> shift_amount;
        };
        const auto force_to_fp16 = [](f32 base_value) {
            u32 tmp = std::bit_cast<u32>(base_value);
            constexpr size_t fp32_mantissa_bits = 23;
            constexpr size_t fp16_mantissa_bits = 10;
            constexpr size_t mantissa_mask =
                ~((1ULL << (fp32_mantissa_bits - fp16_mantissa_bits)) - 1ULL);
            tmp = tmp & static_cast<u32>(mantissa_mask);
            // TODO: force the exponent within the range of half float. Not needed in UNORM / SNORM
            return std::bit_cast<f32>(tmp);
        };
        const auto from_fp_n = [&sign_extend](u32 base_value, size_t bits, size_t mantissa) {
            constexpr size_t fp32_mantissa_bits = 23;
            size_t shift_towards = fp32_mantissa_bits - mantissa;
            const u32 new_value =
                static_cast<u32>(sign_extend(base_value, bits) << shift_towards) & (~(1U << 31));
            return std::bit_cast<f32>(new_value);
        };
        const auto calculate_snorm = [&]() {
            return static_cast<f32>(
                static_cast<f32>(sign_extend(value, component_sizes[which_component])) /
                static_cast<f32>((1ULL << (component_sizes[which_component] - 1ULL)) - 1ULL));
        };
        const auto calculate_unorm = [&]() {
            return static_cast<f32>(
                static_cast<f32>(value) /
                static_cast<f32>((1ULL << (component_sizes[which_component])) - 1ULL));
        };
        if constexpr (component_types[which_component] == ComponentType::SNORM) {
            out_component = calculate_snorm();
        } else if constexpr (component_types[which_component] == ComponentType::UNORM) {
            out_component = calculate_unorm();
        } else if constexpr (component_types[which_component] == ComponentType::SINT) {
            out_component = static_cast<f32>(
                static_cast<s32>(sign_extend(value, component_sizes[which_component])));
        } else if constexpr (component_types[which_component] == ComponentType::UINT) {
            out_component = static_cast<f32>(
                static_cast<s32>(sign_extend(value, component_sizes[which_component])));
        } else if constexpr (component_types[which_component] == ComponentType::SNORM_FORCE_FP16) {
            out_component = calculate_snorm();
            out_component = force_to_fp16(out_component);
        } else if constexpr (component_types[which_component] == ComponentType::UNORM_FORCE_FP16) {
            out_component = calculate_unorm();
            out_component = force_to_fp16(out_component);
        } else if constexpr (component_types[which_component] == ComponentType::FLOAT) {
            if constexpr (component_sizes[which_component] == 32) {
                out_component = std::bit_cast<f32>(value);
            } else if constexpr (component_sizes[which_component] == 16) {
                static constexpr u32 sign_mask = 0x8000;
                static constexpr u32 mantissa_mask = 0x8000;
                out_component = std::bit_cast<f32>(((value & sign_mask) << 16) |
                                                   (((value & 0x7c00) + 0x1C000) << 13) |
                                                   ((value & mantissa_mask) << 13));
            } else {
                out_component = from_fp_n(value, component_sizes[which_component],
                                          component_sizes[which_component] - 5);
            }
        } else if constexpr (component_types[which_component] == ComponentType::SRGB) {
            if constexpr (component_swizzle[which_component] == Swizzle::A) {
                out_component = calculate_unorm();
            } else if constexpr (component_sizes[which_component] == 8) {
                out_component = SRGB_TO_RGB_LUT[value];
            } else {
                out_component = calculate_unorm();
                UNIMPLEMENTED_MSG("SRGB Conversion with component sizes of {} is unimplemented",
                                  component_sizes[which_component]);
            }
        }
    }

    // We are forcing inline so the compiler can SIMD the conversations, since it may do 4 function
    // calls, it may fail to detect the benefit of inlining.
    template <size_t which_component>
    FORCE_INLINE void ConvertFromComponent(u32& which_word, f32 in_component) {
        const auto insert_to_word = [&]<typename T>(T new_word) {
            which_word |= (static_cast<u32>(new_word) << bound_offsets[which_component]) &
                          component_mask[which_component];
        };
        const auto to_fp_n = [](f32 base_value, size_t bits, size_t mantissa) {
            constexpr size_t fp32_mantissa_bits = 23;
            u32 tmp_value = std::bit_cast<u32>(std::max(base_value, 0.0f));
            size_t shift_towards = fp32_mantissa_bits - mantissa;
            return tmp_value >> shift_towards;
        };
        const auto calculate_unorm = [&]() {
            return static_cast<u32>(
                static_cast<f32>(in_component) *
                static_cast<f32>((1ULL << (component_sizes[which_component])) - 1ULL));
        };
        if constexpr (component_types[which_component] == ComponentType::SNORM ||
                      component_types[which_component] == ComponentType::SNORM_FORCE_FP16) {
            s32 tmp_word = static_cast<s32>(
                static_cast<f32>(in_component) *
                static_cast<f32>((1ULL << (component_sizes[which_component] - 1ULL)) - 1ULL));
            insert_to_word(tmp_word);

        } else if constexpr (component_types[which_component] == ComponentType::UNORM ||
                             component_types[which_component] == ComponentType::UNORM_FORCE_FP16) {
            u32 tmp_word = calculate_unorm();
            insert_to_word(tmp_word);
        } else if constexpr (component_types[which_component] == ComponentType::SINT) {
            s32 tmp_word = static_cast<s32>(in_component);
            insert_to_word(tmp_word);
        } else if constexpr (component_types[which_component] == ComponentType::UINT) {
            u32 tmp_word = static_cast<u32>(in_component);
            insert_to_word(tmp_word);
        } else if constexpr (component_types[which_component] == ComponentType::FLOAT) {
            if constexpr (component_sizes[which_component] == 32) {
                u32 tmp_word = std::bit_cast<u32>(in_component);
                insert_to_word(tmp_word);
            } else if constexpr (component_sizes[which_component] == 16) {
                static constexpr u32 sign_mask = 0x8000;
                static constexpr u32 mantissa_mask = 0x03ff;
                static constexpr u32 exponent_mask = 0x7c00;
                const u32 tmp_word = std::bit_cast<u32>(in_component);
                const u32 half = ((tmp_word >> 16) & sign_mask) |
                                 ((((tmp_word & 0x7f800000) - 0x38000000) >> 13) & exponent_mask) |
                                 ((tmp_word >> 13) & mantissa_mask);
                insert_to_word(half);
            } else {
                insert_to_word(to_fp_n(in_component, component_sizes[which_component],
                                       component_sizes[which_component] - 5));
            }
        } else if constexpr (component_types[which_component] == ComponentType::SRGB) {
            if constexpr (component_swizzle[which_component] != Swizzle::A) {
                if constexpr (component_sizes[which_component] == 8) {
                    const u32 index = calculate_unorm();
                    in_component = RGB_TO_SRGB_LUT[index];
                } else {
                    UNIMPLEMENTED_MSG("SRGB Conversion with component sizes of {} is unimplemented",
                                      component_sizes[which_component]);
                }
            }
            const u32 tmp_word = calculate_unorm();
            insert_to_word(tmp_word);
        }
    }

public:
    void ConvertTo(std::span<const u8> input, std::span<f32> output) override {
        const size_t num_pixels = output.size() / components_per_ir_rep;
        for (size_t pixel = 0; pixel < num_pixels; pixel++) {
            std::array<u32, total_words_per_pixel> words{};

            std::memcpy(words.data(), &input[pixel * total_bytes_per_pixel], total_bytes_per_pixel);
            std::span<f32> new_components(&output[pixel * components_per_ir_rep],
                                          components_per_ir_rep);
            if constexpr (component_swizzle[0] != Swizzle::None) {
                ConvertToComponent<0>(words[bound_words[0]],
                                      new_components[static_cast<size_t>(component_swizzle[0])]);
            } else {
                new_components[0] = 0.0f;
            }
            if constexpr (num_components >= 2) {
                if constexpr (component_swizzle[1] != Swizzle::None) {
                    ConvertToComponent<1>(
                        words[bound_words[1]],
                        new_components[static_cast<size_t>(component_swizzle[1])]);
                } else {
                    new_components[1] = 0.0f;
                }
            } else {
                new_components[1] = 0.0f;
            }
            if constexpr (num_components >= 3) {
                if constexpr (component_swizzle[2] != Swizzle::None) {
                    ConvertToComponent<2>(
                        words[bound_words[2]],
                        new_components[static_cast<size_t>(component_swizzle[2])]);
                } else {
                    new_components[2] = 0.0f;
                }
            } else {
                new_components[2] = 0.0f;
            }
            if constexpr (num_components >= 4) {
                if constexpr (component_swizzle[3] != Swizzle::None) {
                    ConvertToComponent<3>(
                        words[bound_words[3]],
                        new_components[static_cast<size_t>(component_swizzle[3])]);
                } else {
                    new_components[3] = 0.0f;
                }
            } else {
                new_components[3] = 0.0f;
            }
        }
    }

    void ConvertFrom(std::span<const f32> input, std::span<u8> output) override {
        const size_t num_pixels = output.size() / total_bytes_per_pixel;
        for (size_t pixel = 0; pixel < num_pixels; pixel++) {
            std::span<const f32> old_components(&input[pixel * components_per_ir_rep],
                                                components_per_ir_rep);
            std::array<u32, total_words_per_pixel> words{};
            if constexpr (component_swizzle[0] != Swizzle::None) {
                ConvertFromComponent<0>(words[bound_words[0]],
                                        old_components[static_cast<size_t>(component_swizzle[0])]);
            }
            if constexpr (num_components >= 2) {
                if constexpr (component_swizzle[1] != Swizzle::None) {
                    ConvertFromComponent<1>(
                        words[bound_words[1]],
                        old_components[static_cast<size_t>(component_swizzle[1])]);
                }
            }
            if constexpr (num_components >= 3) {
                if constexpr (component_swizzle[2] != Swizzle::None) {
                    ConvertFromComponent<2>(
                        words[bound_words[2]],
                        old_components[static_cast<size_t>(component_swizzle[2])]);
                }
            }
            if constexpr (num_components >= 4) {
                if constexpr (component_swizzle[3] != Swizzle::None) {
                    ConvertFromComponent<3>(
                        words[bound_words[3]],
                        old_components[static_cast<size_t>(component_swizzle[3])]);
                }
            }
            std::memcpy(&output[pixel * total_bytes_per_pixel], words.data(),
                        total_bytes_per_pixel);
        }
    }

    ConverterImpl() = default;
    ~ConverterImpl() override = default;
};

struct ConverterFactory::ConverterFactoryImpl {
    std::unordered_map<RenderTargetFormat, std::unique_ptr<Converter>> converters_cache;
};

ConverterFactory::ConverterFactory() {
    impl = std::make_unique<ConverterFactoryImpl>();
}

ConverterFactory::~ConverterFactory() = default;

Converter* ConverterFactory::GetFormatConverter(RenderTargetFormat format) {
    auto it = impl->converters_cache.find(format);
    if (it == impl->converters_cache.end()) [[unlikely]] {
        return BuildConverter(format);
    }
    return it->second.get();
}

class NullConverter : public Converter {
public:
    void ConvertTo([[maybe_unused]] std::span<const u8> input, std::span<f32> output) override {
        std::fill(output.begin(), output.end(), 0.0f);
    }
    void ConvertFrom([[maybe_unused]] std::span<const f32> input, std::span<u8> output) override {
        const u8 fill_value = 0U;
        std::fill(output.begin(), output.end(), fill_value);
    }
    NullConverter() = default;
    ~NullConverter() = default;
};

Converter* ConverterFactory::BuildConverter(RenderTargetFormat format) {
    switch (format) {
    case RenderTargetFormat::R32G32B32A32_FLOAT:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R32G32B32A32_FLOATTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R32G32B32A32_SINT:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R32G32B32A32_SINTTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R32G32B32A32_UINT:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R32G32B32A32_UINTTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R32G32B32X32_FLOAT:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R32G32B32X32_FLOATTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R32G32B32X32_SINT:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R32G32B32X32_SINTTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R32G32B32X32_UINT:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R32G32B32X32_UINTTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R16G16B16A16_UNORM:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R16G16B16A16_UNORMTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R16G16B16A16_SNORM:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R16G16B16A16_SNORMTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R16G16B16A16_SINT:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R16G16B16A16_SINTTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R16G16B16A16_UINT:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R16G16B16A16_UINTTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R16G16B16A16_FLOAT:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R16G16B16A16_FLOATTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R32G32_FLOAT:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R32G32_FLOATTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R32G32_SINT:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R32G32_SINTTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R32G32_UINT:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R32G32_UINTTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R16G16B16X16_FLOAT:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R16G16B16X16_FLOATTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::A8R8G8B8_UNORM:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<A8R8G8B8_UNORMTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::A8R8G8B8_SRGB:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<A8R8G8B8_SRGBTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::A2B10G10R10_UNORM:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<A2B10G10R10_UNORMTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::A2B10G10R10_UINT:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<A2B10G10R10_UINTTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::A2R10G10B10_UNORM:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<A2R10G10B10_UNORMTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::A8B8G8R8_UNORM:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<A8B8G8R8_UNORMTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::A8B8G8R8_SRGB:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<A8B8G8R8_SRGBTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::A8B8G8R8_SNORM:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<A8B8G8R8_SNORMTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::A8B8G8R8_SINT:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<A8B8G8R8_SINTTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::A8B8G8R8_UINT:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<A8B8G8R8_UINTTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R16G16_UNORM:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R16G16_UNORMTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R16G16_SNORM:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R16G16_SNORMTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R16G16_SINT:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R16G16_SINTTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R16G16_UINT:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R16G16_UINTTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R16G16_FLOAT:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R16G16_FLOATTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::B10G11R11_FLOAT:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<B10G11R11_FLOATTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R32_SINT:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R32_SINTTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R32_UINT:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R32_UINTTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R32_FLOAT:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R32_FLOATTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::X8R8G8B8_UNORM:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<X8R8G8B8_UNORMTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::X8R8G8B8_SRGB:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<X8R8G8B8_SRGBTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R5G6B5_UNORM:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R5G6B5_UNORMTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::A1R5G5B5_UNORM:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<A1R5G5B5_UNORMTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R8G8_UNORM:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R8G8_UNORMTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R8G8_SNORM:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R8G8_SNORMTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R8G8_SINT:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R8G8_SINTTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R8G8_UINT:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R8G8_UINTTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R16_UNORM:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R16_UNORMTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R16_SNORM:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R16_SNORMTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R16_SINT:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R16_SINTTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R16_UINT:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R16_UINTTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R16_FLOAT:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R16_FLOATTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R8_UNORM:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R8_UNORMTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R8_SNORM:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R8_SNORMTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R8_SINT:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R8_SINTTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::R8_UINT:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<R8_UINTTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::X1R5G5B5_UNORM:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<X1R5G5B5_UNORMTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::X8B8G8R8_UNORM:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<X8B8G8R8_UNORMTraits>>())
            .first->second.get();
        break;
    case RenderTargetFormat::X8B8G8R8_SRGB:
        return impl->converters_cache
            .emplace(format, std::make_unique<ConverterImpl<X8B8G8R8_SRGBTraits>>())
            .first->second.get();
        break;
    default: {
        UNIMPLEMENTED_MSG("This format {} converter is not implemented", format);
        return impl->converters_cache.emplace(format, std::make_unique<NullConverter>())
            .first->second.get();
    }
    }
}

} // namespace Tegra::Engines::Blitter
