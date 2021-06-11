// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <vector>
#include <fmt/format.h>

#include "common/assert.h"
#include "common/bit_field.h"
#include "common/common_types.h"
#include "common/logging/log.h"
#include "video_core/engines/shader_bytecode.h"
#include "video_core/shader/node_helper.h"
#include "video_core/shader/shader_ir.h"
#include "video_core/textures/texture.h"

namespace VideoCommon::Shader {

using Tegra::Shader::Instruction;
using Tegra::Shader::OpCode;
using Tegra::Shader::PredCondition;
using Tegra::Shader::StoreType;
using Tegra::Texture::ComponentType;
using Tegra::Texture::TextureFormat;
using Tegra::Texture::TICEntry;

namespace {

ComponentType GetComponentType(Tegra::Engines::SamplerDescriptor descriptor,
                               std::size_t component) {
    const TextureFormat format{descriptor.format};
    switch (format) {
    case TextureFormat::R16G16B16A16:
    case TextureFormat::R32G32B32A32:
    case TextureFormat::R32G32B32:
    case TextureFormat::R32G32:
    case TextureFormat::R16G16:
    case TextureFormat::R32:
    case TextureFormat::R16:
    case TextureFormat::R8:
    case TextureFormat::R1:
        if (component == 0) {
            return descriptor.r_type;
        }
        if (component == 1) {
            return descriptor.g_type;
        }
        if (component == 2) {
            return descriptor.b_type;
        }
        if (component == 3) {
            return descriptor.a_type;
        }
        break;
    case TextureFormat::A8R8G8B8:
        if (component == 0) {
            return descriptor.a_type;
        }
        if (component == 1) {
            return descriptor.r_type;
        }
        if (component == 2) {
            return descriptor.g_type;
        }
        if (component == 3) {
            return descriptor.b_type;
        }
        break;
    case TextureFormat::A2B10G10R10:
    case TextureFormat::A4B4G4R4:
    case TextureFormat::A5B5G5R1:
    case TextureFormat::A1B5G5R5:
        if (component == 0) {
            return descriptor.a_type;
        }
        if (component == 1) {
            return descriptor.b_type;
        }
        if (component == 2) {
            return descriptor.g_type;
        }
        if (component == 3) {
            return descriptor.r_type;
        }
        break;
    case TextureFormat::R32_B24G8:
        if (component == 0) {
            return descriptor.r_type;
        }
        if (component == 1) {
            return descriptor.b_type;
        }
        if (component == 2) {
            return descriptor.g_type;
        }
        break;
    case TextureFormat::B5G6R5:
    case TextureFormat::B6G5R5:
    case TextureFormat::B10G11R11:
        if (component == 0) {
            return descriptor.b_type;
        }
        if (component == 1) {
            return descriptor.g_type;
        }
        if (component == 2) {
            return descriptor.r_type;
        }
        break;
    case TextureFormat::R24G8:
    case TextureFormat::R8G24:
    case TextureFormat::R8G8:
    case TextureFormat::G4R4:
        if (component == 0) {
            return descriptor.g_type;
        }
        if (component == 1) {
            return descriptor.r_type;
        }
        break;
    default:
        break;
    }
    UNIMPLEMENTED_MSG("Texture format not implemented={}", format);
    return ComponentType::FLOAT;
}

bool IsComponentEnabled(std::size_t component_mask, std::size_t component) {
    constexpr u8 R = 0b0001;
    constexpr u8 G = 0b0010;
    constexpr u8 B = 0b0100;
    constexpr u8 A = 0b1000;
    constexpr std::array<u8, 16> mask = {
        0,   (R),     (G),     (R | G),     (B),     (R | B),     (G | B),     (R | G | B),
        (A), (R | A), (G | A), (R | G | A), (B | A), (R | B | A), (G | B | A), (R | G | B | A)};
    return std::bitset<4>{mask.at(component_mask)}.test(component);
}

u32 GetComponentSize(TextureFormat format, std::size_t component) {
    switch (format) {
    case TextureFormat::R32G32B32A32:
        return 32;
    case TextureFormat::R16G16B16A16:
        return 16;
    case TextureFormat::R32G32B32:
        return component <= 2 ? 32 : 0;
    case TextureFormat::R32G32:
        return component <= 1 ? 32 : 0;
    case TextureFormat::R16G16:
        return component <= 1 ? 16 : 0;
    case TextureFormat::R32:
        return component == 0 ? 32 : 0;
    case TextureFormat::R16:
        return component == 0 ? 16 : 0;
    case TextureFormat::R8:
        return component == 0 ? 8 : 0;
    case TextureFormat::R1:
        return component == 0 ? 1 : 0;
    case TextureFormat::A8R8G8B8:
        return 8;
    case TextureFormat::A2B10G10R10:
        return (component == 3 || component == 2 || component == 1) ? 10 : 2;
    case TextureFormat::A4B4G4R4:
        return 4;
    case TextureFormat::A5B5G5R1:
        return (component == 0 || component == 1 || component == 2) ? 5 : 1;
    case TextureFormat::A1B5G5R5:
        return (component == 1 || component == 2 || component == 3) ? 5 : 1;
    case TextureFormat::R32_B24G8:
        if (component == 0) {
            return 32;
        }
        if (component == 1) {
            return 24;
        }
        if (component == 2) {
            return 8;
        }
        return 0;
    case TextureFormat::B5G6R5:
        if (component == 0 || component == 2) {
            return 5;
        }
        if (component == 1) {
            return 6;
        }
        return 0;
    case TextureFormat::B6G5R5:
        if (component == 1 || component == 2) {
            return 5;
        }
        if (component == 0) {
            return 6;
        }
        return 0;
    case TextureFormat::B10G11R11:
        if (component == 1 || component == 2) {
            return 11;
        }
        if (component == 0) {
            return 10;
        }
        return 0;
    case TextureFormat::R24G8:
        if (component == 0) {
            return 8;
        }
        if (component == 1) {
            return 24;
        }
        return 0;
    case TextureFormat::R8G24:
        if (component == 0) {
            return 24;
        }
        if (component == 1) {
            return 8;
        }
        return 0;
    case TextureFormat::R8G8:
        return (component == 0 || component == 1) ? 8 : 0;
    case TextureFormat::G4R4:
        return (component == 0 || component == 1) ? 4 : 0;
    default:
        UNIMPLEMENTED_MSG("Texture format not implemented={}", format);
        return 0;
    }
}

std::size_t GetImageComponentMask(TextureFormat format) {
    constexpr u8 R = 0b0001;
    constexpr u8 G = 0b0010;
    constexpr u8 B = 0b0100;
    constexpr u8 A = 0b1000;
    switch (format) {
    case TextureFormat::R32G32B32A32:
    case TextureFormat::R16G16B16A16:
    case TextureFormat::A8R8G8B8:
    case TextureFormat::A2B10G10R10:
    case TextureFormat::A4B4G4R4:
    case TextureFormat::A5B5G5R1:
    case TextureFormat::A1B5G5R5:
        return std::size_t{R | G | B | A};
    case TextureFormat::R32G32B32:
    case TextureFormat::R32_B24G8:
    case TextureFormat::B5G6R5:
    case TextureFormat::B6G5R5:
    case TextureFormat::B10G11R11:
        return std::size_t{R | G | B};
    case TextureFormat::R32G32:
    case TextureFormat::R16G16:
    case TextureFormat::R24G8:
    case TextureFormat::R8G24:
    case TextureFormat::R8G8:
    case TextureFormat::G4R4:
        return std::size_t{R | G};
    case TextureFormat::R32:
    case TextureFormat::R16:
    case TextureFormat::R8:
    case TextureFormat::R1:
        return std::size_t{R};
    default:
        UNIMPLEMENTED_MSG("Texture format not implemented={}", format);
        return std::size_t{R | G | B | A};
    }
}

std::size_t GetImageTypeNumCoordinates(Tegra::Shader::ImageType image_type) {
    switch (image_type) {
    case Tegra::Shader::ImageType::Texture1D:
    case Tegra::Shader::ImageType::TextureBuffer:
        return 1;
    case Tegra::Shader::ImageType::Texture1DArray:
    case Tegra::Shader::ImageType::Texture2D:
        return 2;
    case Tegra::Shader::ImageType::Texture2DArray:
    case Tegra::Shader::ImageType::Texture3D:
        return 3;
    }
    UNREACHABLE();
    return 1;
}
} // Anonymous namespace

std::pair<Node, bool> ShaderIR::GetComponentValue(ComponentType component_type, u32 component_size,
                                                  Node original_value) {
    switch (component_type) {
    case ComponentType::SNORM: {
        // range [-1.0, 1.0]
        auto cnv_value = Operation(OperationCode::FMul, original_value,
                                   Immediate(static_cast<float>(1 << component_size) / 2.f - 1.f));
        cnv_value = Operation(OperationCode::ICastFloat, std::move(cnv_value));
        return {BitfieldExtract(std::move(cnv_value), 0, component_size), true};
    }
    case ComponentType::SINT:
    case ComponentType::UNORM: {
        bool is_signed = component_type == ComponentType::SINT;
        // range [0.0, 1.0]
        auto cnv_value = Operation(OperationCode::FMul, original_value,
                                   Immediate(static_cast<float>(1 << component_size) - 1.f));
        return {SignedOperation(OperationCode::ICastFloat, is_signed, std::move(cnv_value)),
                is_signed};
    }
    case ComponentType::UINT: // range [0, (1 << component_size) - 1]
        return {std::move(original_value), false};
    case ComponentType::FLOAT:
        if (component_size == 16) {
            return {Operation(OperationCode::HCastFloat, original_value), true};
        } else {
            return {std::move(original_value), true};
        }
    default:
        UNIMPLEMENTED_MSG("Unimplemented component type={}", component_type);
        return {std::move(original_value), true};
    }
}

u32 ShaderIR::DecodeImage(NodeBlock& bb, u32 pc) {
    const Instruction instr = {program_code[pc]};
    const auto opcode = OpCode::Decode(instr);

    const auto GetCoordinates = [this, instr](Tegra::Shader::ImageType image_type) {
        std::vector<Node> coords;
        const std::size_t num_coords{GetImageTypeNumCoordinates(image_type)};
        coords.reserve(num_coords);
        for (std::size_t i = 0; i < num_coords; ++i) {
            coords.push_back(GetRegister(instr.gpr8.Value() + i));
        }
        return coords;
    };

    switch (opcode->get().GetId()) {
    case OpCode::Id::SULD: {
        UNIMPLEMENTED_IF(instr.suldst.out_of_bounds_store !=
                         Tegra::Shader::OutOfBoundsStore::Ignore);

        const auto type{instr.suldst.image_type};
        auto& image{instr.suldst.is_immediate ? GetImage(instr.image, type)
                                              : GetBindlessImage(instr.gpr39, type)};
        image.MarkRead();

        if (instr.suldst.mode == Tegra::Shader::SurfaceDataMode::P) {
            u32 indexer = 0;
            for (u32 element = 0; element < 4; ++element) {
                if (!instr.suldst.IsComponentEnabled(element)) {
                    continue;
                }
                MetaImage meta{image, {}, element};
                Node value = Operation(OperationCode::ImageLoad, meta, GetCoordinates(type));
                SetTemporary(bb, indexer++, std::move(value));
            }
            for (u32 i = 0; i < indexer; ++i) {
                SetRegister(bb, instr.gpr0.Value() + i, GetTemporary(i));
            }
        } else if (instr.suldst.mode == Tegra::Shader::SurfaceDataMode::D_BA) {
            UNIMPLEMENTED_IF(instr.suldst.GetStoreDataLayout() != StoreType::Bits32 &&
                             instr.suldst.GetStoreDataLayout() != StoreType::Bits64);

            auto descriptor = [this, instr] {
                std::optional<Tegra::Engines::SamplerDescriptor> sampler_descriptor;
                if (instr.suldst.is_immediate) {
                    sampler_descriptor =
                        registry.ObtainBoundSampler(static_cast<u32>(instr.image.index.Value()));
                } else {
                    const Node image_register = GetRegister(instr.gpr39);
                    const auto result = TrackCbuf(image_register, global_code,
                                                  static_cast<s64>(global_code.size()));
                    const auto buffer = std::get<1>(result);
                    const auto offset = std::get<2>(result);
                    sampler_descriptor = registry.ObtainBindlessSampler(buffer, offset);
                }
                if (!sampler_descriptor) {
                    UNREACHABLE_MSG("Failed to obtain image descriptor");
                }
                return *sampler_descriptor;
            }();

            const auto comp_mask = GetImageComponentMask(descriptor.format);

            switch (instr.suldst.GetStoreDataLayout()) {
            case StoreType::Bits32:
            case StoreType::Bits64: {
                u32 indexer = 0;
                u32 shifted_counter = 0;
                Node value = Immediate(0);
                for (u32 element = 0; element < 4; ++element) {
                    if (!IsComponentEnabled(comp_mask, element)) {
                        continue;
                    }
                    const auto component_type = GetComponentType(descriptor, element);
                    const auto component_size = GetComponentSize(descriptor.format, element);
                    MetaImage meta{image, {}, element};

                    auto [converted_value, is_signed] = GetComponentValue(
                        component_type, component_size,
                        Operation(OperationCode::ImageLoad, meta, GetCoordinates(type)));

                    // shift element to correct position
                    const auto shifted = shifted_counter;
                    if (shifted > 0) {
                        converted_value =
                            SignedOperation(OperationCode::ILogicalShiftLeft, is_signed,
                                            std::move(converted_value), Immediate(shifted));
                    }
                    shifted_counter += component_size;

                    // add value into result
                    value = Operation(OperationCode::UBitwiseOr, value, std::move(converted_value));

                    // if we shifted enough for 1 byte -> we save it into temp
                    if (shifted_counter >= 32) {
                        SetTemporary(bb, indexer++, std::move(value));
                        // reset counter and value to prepare pack next byte
                        value = Immediate(0);
                        shifted_counter = 0;
                    }
                }
                for (u32 i = 0; i < indexer; ++i) {
                    SetRegister(bb, instr.gpr0.Value() + i, GetTemporary(i));
                }
                break;
            }
            default:
                UNREACHABLE();
                break;
            }
        }
        break;
    }
    case OpCode::Id::SUST: {
        UNIMPLEMENTED_IF(instr.suldst.mode != Tegra::Shader::SurfaceDataMode::P);
        UNIMPLEMENTED_IF(instr.suldst.out_of_bounds_store !=
                         Tegra::Shader::OutOfBoundsStore::Ignore);
        UNIMPLEMENTED_IF(instr.suldst.component_mask_selector != 0xf); // Ensure we have RGBA

        std::vector<Node> values;
        constexpr std::size_t hardcoded_size{4};
        for (std::size_t i = 0; i < hardcoded_size; ++i) {
            values.push_back(GetRegister(instr.gpr0.Value() + i));
        }

        const auto type{instr.suldst.image_type};
        auto& image{instr.suldst.is_immediate ? GetImage(instr.image, type)
                                              : GetBindlessImage(instr.gpr39, type)};
        image.MarkWrite();

        MetaImage meta{image, std::move(values)};
        bb.push_back(Operation(OperationCode::ImageStore, meta, GetCoordinates(type)));
        break;
    }
    case OpCode::Id::SUATOM: {
        UNIMPLEMENTED_IF(instr.suatom_d.is_ba != 0);

        const OperationCode operation_code = [instr] {
            switch (instr.suatom_d.operation_type) {
            case Tegra::Shader::ImageAtomicOperationType::S32:
            case Tegra::Shader::ImageAtomicOperationType::U32:
                switch (instr.suatom_d.operation) {
                case Tegra::Shader::ImageAtomicOperation::Add:
                    return OperationCode::AtomicImageAdd;
                case Tegra::Shader::ImageAtomicOperation::And:
                    return OperationCode::AtomicImageAnd;
                case Tegra::Shader::ImageAtomicOperation::Or:
                    return OperationCode::AtomicImageOr;
                case Tegra::Shader::ImageAtomicOperation::Xor:
                    return OperationCode::AtomicImageXor;
                case Tegra::Shader::ImageAtomicOperation::Exch:
                    return OperationCode::AtomicImageExchange;
                default:
                    break;
                }
                break;
            default:
                break;
            }
            UNIMPLEMENTED_MSG("Unimplemented operation={}, type={}",
                              static_cast<u64>(instr.suatom_d.operation.Value()),
                              static_cast<u64>(instr.suatom_d.operation_type.Value()));
            return OperationCode::AtomicImageAdd;
        }();

        Node value = GetRegister(instr.gpr0);

        const auto type = instr.suatom_d.image_type;
        auto& image = GetImage(instr.image, type);
        image.MarkAtomic();

        MetaImage meta{image, {std::move(value)}};
        SetRegister(bb, instr.gpr0, Operation(operation_code, meta, GetCoordinates(type)));
        break;
    }
    default:
        UNIMPLEMENTED_MSG("Unhandled image instruction: {}", opcode->get().GetName());
    }

    return pc;
}

ImageEntry& ShaderIR::GetImage(Tegra::Shader::Image image, Tegra::Shader::ImageType type) {
    const auto offset = static_cast<u32>(image.index.Value());

    const auto it =
        std::find_if(std::begin(used_images), std::end(used_images),
                     [offset](const ImageEntry& entry) { return entry.offset == offset; });
    if (it != std::end(used_images)) {
        ASSERT(!it->is_bindless && it->type == type);
        return *it;
    }

    const auto next_index = static_cast<u32>(used_images.size());
    return used_images.emplace_back(next_index, offset, type);
}

ImageEntry& ShaderIR::GetBindlessImage(Tegra::Shader::Register reg, Tegra::Shader::ImageType type) {
    const Node image_register = GetRegister(reg);
    const auto result =
        TrackCbuf(image_register, global_code, static_cast<s64>(global_code.size()));

    const auto buffer = std::get<1>(result);
    const auto offset = std::get<2>(result);

    const auto it = std::find_if(std::begin(used_images), std::end(used_images),
                                 [buffer, offset](const ImageEntry& entry) {
                                     return entry.buffer == buffer && entry.offset == offset;
                                 });
    if (it != std::end(used_images)) {
        ASSERT(it->is_bindless && it->type == type);
        return *it;
    }

    const auto next_index = static_cast<u32>(used_images.size());
    return used_images.emplace_back(next_index, offset, buffer, type);
}

} // namespace VideoCommon::Shader
