// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <span>

#include <glad/glad.h>

#include "common/common_types.h"
#include "video_core/renderer_opengl/gl_resource_manager.h"
#include "video_core/texture_cache/types.h"

namespace OpenGL {

class Image;
class ProgramManager;

struct ImageBufferMap;

class UtilShaders {
public:
    explicit UtilShaders(ProgramManager& program_manager);
    ~UtilShaders();

    void ASTCDecode(Image& image, const ImageBufferMap& map,
                    std::span<const VideoCommon::SwizzleParameters> swizzles);

    void BlockLinearUpload2D(Image& image, const ImageBufferMap& map,
                             std::span<const VideoCommon::SwizzleParameters> swizzles);

    void BlockLinearUpload3D(Image& image, const ImageBufferMap& map,
                             std::span<const VideoCommon::SwizzleParameters> swizzles);

    void PitchUpload(Image& image, const ImageBufferMap& map,
                     std::span<const VideoCommon::SwizzleParameters> swizzles);

    void CopyBC4(Image& dst_image, Image& src_image,
                 std::span<const VideoCommon::ImageCopy> copies);

    void ConvertS8D24(Image& dst_image, std::span<const VideoCommon::ImageCopy> copies);

private:
    ProgramManager& program_manager;

    OGLBuffer swizzle_table_buffer;

    OGLProgram astc_decoder_program;
    OGLProgram block_linear_unswizzle_2d_program;
    OGLProgram block_linear_unswizzle_3d_program;
    OGLProgram pitch_unswizzle_program;
    OGLProgram copy_bc4_program;
    OGLProgram convert_s8d24_program;
};

GLenum StoreFormat(u32 bytes_per_block);

} // namespace OpenGL
