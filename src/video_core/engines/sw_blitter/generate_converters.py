# SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

import re

class Format:
    def __init__(self, string_value):
        self.name = string_value
        tmp = string_value.split('_')
        self.component_type = tmp[1]
        component_data = re.findall(r"\w\d+", tmp[0])
        self.num_components = len(component_data)
        sizes = []
        swizzle = []
        for data in component_data:
            swizzle.append(data[0])
            sizes.append(int(data[1:]))
        self.sizes = sizes
        self.swizzle = swizzle

    def build_component_type_array(self):
        result = "{ "
        b = False
        for i in range(0, self.num_components):
            if b:
                result += ", "
            b = True
            result += "ComponentType::" + self.component_type
        result += " }"
        return result

    def build_component_sizes_array(self):
        result = "{ "
        b = False
        for i in range(0, self.num_components):
            if b:
                result += ", "
            b = True
            result += str(self.sizes[i])
        result += " }"
        return result

    def build_component_swizzle_array(self):
        result = "{ "
        b = False
        for i in range(0, self.num_components):
            if b:
                result += ", "
            b = True
            swizzle = self.swizzle[i]
            if swizzle == "X":
                swizzle = "None"
            result += "Swizzle::" + swizzle
        result += " }"
        return result

    def print_declaration(self):
        print("struct " + self.name + "Traits {")
        print("  static constexpr size_t num_components = " + str(self.num_components) + ";")
        print("  static constexpr std::array<ComponentType, num_components> component_types = " + self.build_component_type_array() + ";")
        print("  static constexpr std::array<size_t, num_components> component_sizes = " + self.build_component_sizes_array() + ";")
        print("  static constexpr std::array<Swizzle, num_components> component_swizzle = " + self.build_component_swizzle_array() + ";")
        print("};\n")

    def print_case(self):
        print("case RenderTargetFormat::" + self.name + ":")
        print("  return impl->converters_cache")
        print("    .emplace(format, std::make_unique<ConverterImpl<" + self.name + "Traits>>())")
        print("    .first->second.get();")
        print("  break;")

txt = """
R32G32B32A32_FLOAT
R32G32B32A32_SINT
R32G32B32A32_UINT
R32G32B32X32_FLOAT
R32G32B32X32_SINT
R32G32B32X32_UINT
R16G16B16A16_UNORM
R16G16B16A16_SNORM
R16G16B16A16_SINT
R16G16B16A16_UINT
R16G16B16A16_FLOAT
R32G32_FLOAT
R32G32_SINT
R32G32_UINT
R16G16B16X16_FLOAT
A8R8G8B8_UNORM
A8R8G8B8_SRGB
A2B10G10R10_UNORM
A2B10G10R10_UINT
A2R10G10B10_UNORM
A8B8G8R8_UNORM
A8B8G8R8_SRGB
A8B8G8R8_SNORM
A8B8G8R8_SINT
A8B8G8R8_UINT
R16G16_UNORM
R16G16_SNORM
R16G16_SINT
R16G16_UINT
R16G16_FLOAT
B10G11R11_FLOAT
R32_SINT
R32_UINT
R32_FLOAT
X8R8G8B8_UNORM
X8R8G8B8_SRGB
R5G6B5_UNORM
A1R5G5B5_UNORM
R8G8_UNORM
R8G8_SNORM
R8G8_SINT
R8G8_UINT
R16_UNORM
R16_SNORM
R16_SINT
R16_UINT
R16_FLOAT
R8_UNORM
R8_SNORM
R8_SINT
R8_UINT
X1R5G5B5_UNORM
X8B8G8R8_UNORM
X8B8G8R8_SRGB
"""

x = txt.split()
y = list(map(lambda a: Format(a), x))
formats = list(y)
for format in formats:
  format.print_declaration()

for format in formats:
  format.print_case()
