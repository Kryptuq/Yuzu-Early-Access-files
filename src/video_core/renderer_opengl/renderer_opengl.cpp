// SPDX-FileCopyrightText: 2014 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <memory>

#include <glad/glad.h>

#include "common/assert.h"
#include "common/logging/log.h"
#include "common/microprofile.h"
#include "common/settings.h"
#include "common/telemetry.h"
#include "core/core_timing.h"
#include "core/frontend/emu_window.h"
#include "core/memory.h"
#include "core/telemetry_session.h"
#include "video_core/host_shaders/ffx_a_h.h"
#include "video_core/host_shaders/ffx_fsr1_h.h"
#include "video_core/host_shaders/full_screen_triangle_vert.h"
#include "video_core/host_shaders/fxaa_frag.h"
#include "video_core/host_shaders/fxaa_vert.h"
#include "video_core/host_shaders/opengl_fidelityfx_fsr_easu_frag.h"
#include "video_core/host_shaders/opengl_fidelityfx_fsr_frag.h"
#include "video_core/host_shaders/opengl_fidelityfx_fsr_rcas_frag.h"
#include "video_core/host_shaders/opengl_present_frag.h"
#include "video_core/host_shaders/opengl_present_scaleforce_frag.h"
#include "video_core/host_shaders/opengl_present_vert.h"
#include "video_core/host_shaders/opengl_smaa_glsl.h"
#include "video_core/host_shaders/present_bicubic_frag.h"
#include "video_core/host_shaders/present_gaussian_frag.h"
#include "video_core/host_shaders/smaa_blending_weight_calculation_frag.h"
#include "video_core/host_shaders/smaa_blending_weight_calculation_vert.h"
#include "video_core/host_shaders/smaa_edge_detection_frag.h"
#include "video_core/host_shaders/smaa_edge_detection_vert.h"
#include "video_core/host_shaders/smaa_neighborhood_blending_frag.h"
#include "video_core/host_shaders/smaa_neighborhood_blending_vert.h"
#include "video_core/renderer_opengl/gl_fsr.h"
#include "video_core/renderer_opengl/gl_rasterizer.h"
#include "video_core/renderer_opengl/gl_shader_manager.h"
#include "video_core/renderer_opengl/gl_shader_util.h"
#include "video_core/renderer_opengl/renderer_opengl.h"
#include "video_core/smaa_area_tex.h"
#include "video_core/smaa_search_tex.h"
#include "video_core/textures/decoders.h"

namespace OpenGL {
namespace {
constexpr GLint PositionLocation = 0;
constexpr GLint TexCoordLocation = 1;
constexpr GLint ModelViewMatrixLocation = 0;

struct ScreenRectVertex {
    constexpr ScreenRectVertex(u32 x, u32 y, GLfloat u, GLfloat v)
        : position{{static_cast<GLfloat>(x), static_cast<GLfloat>(y)}}, tex_coord{{u, v}} {}

    std::array<GLfloat, 2> position;
    std::array<GLfloat, 2> tex_coord;
};

/**
 * Defines a 1:1 pixel ortographic projection matrix with (0,0) on the top-left
 * corner and (width, height) on the lower-bottom.
 *
 * The projection part of the matrix is trivial, hence these operations are represented
 * by a 3x2 matrix.
 */
std::array<GLfloat, 3 * 2> MakeOrthographicMatrix(float width, float height) {
    std::array<GLfloat, 3 * 2> matrix; // Laid out in column-major order

    // clang-format off
    matrix[0] = 2.f / width; matrix[2] =  0.f;          matrix[4] = -1.f;
    matrix[1] = 0.f;         matrix[3] = -2.f / height; matrix[5] =  1.f;
    // Last matrix row is implicitly assumed to be [0, 0, 1].
    // clang-format on

    return matrix;
}

const char* GetSource(GLenum source) {
    switch (source) {
    case GL_DEBUG_SOURCE_API:
        return "API";
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        return "WINDOW_SYSTEM";
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        return "SHADER_COMPILER";
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        return "THIRD_PARTY";
    case GL_DEBUG_SOURCE_APPLICATION:
        return "APPLICATION";
    case GL_DEBUG_SOURCE_OTHER:
        return "OTHER";
    default:
        ASSERT(false);
        return "Unknown source";
    }
}

const char* GetType(GLenum type) {
    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        return "ERROR";
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        return "DEPRECATED_BEHAVIOR";
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        return "UNDEFINED_BEHAVIOR";
    case GL_DEBUG_TYPE_PORTABILITY:
        return "PORTABILITY";
    case GL_DEBUG_TYPE_PERFORMANCE:
        return "PERFORMANCE";
    case GL_DEBUG_TYPE_OTHER:
        return "OTHER";
    case GL_DEBUG_TYPE_MARKER:
        return "MARKER";
    default:
        ASSERT(false);
        return "Unknown type";
    }
}

void APIENTRY DebugHandler(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                           const GLchar* message, const void* user_param) {
    const char format[] = "{} {} {}: {}";
    const char* const str_source = GetSource(source);
    const char* const str_type = GetType(type);

    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
        LOG_CRITICAL(Render_OpenGL, format, str_source, str_type, id, message);
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        LOG_WARNING(Render_OpenGL, format, str_source, str_type, id, message);
        break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
    case GL_DEBUG_SEVERITY_LOW:
        LOG_DEBUG(Render_OpenGL, format, str_source, str_type, id, message);
        break;
    }
}
} // Anonymous namespace

RendererOpenGL::RendererOpenGL(Core::TelemetrySession& telemetry_session_,
                               Core::Frontend::EmuWindow& emu_window_,
                               Core::Memory::Memory& cpu_memory_, Tegra::GPU& gpu_,
                               std::unique_ptr<Core::Frontend::GraphicsContext> context_)
    : RendererBase{emu_window_, std::move(context_)}, telemetry_session{telemetry_session_},
      emu_window{emu_window_}, cpu_memory{cpu_memory_}, gpu{gpu_}, device{emu_window_},
      state_tracker{}, program_manager{device},
      rasterizer(emu_window, gpu, cpu_memory, device, screen_info, program_manager, state_tracker) {
    if (Settings::values.renderer_debug && GLAD_GL_KHR_debug) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(DebugHandler, nullptr);
    }
    AddTelemetryFields();
    InitOpenGLObjects();

    // Initialize default attributes to match hardware's disabled attributes
    GLint max_attribs{};
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_attribs);
    for (GLint attrib = 0; attrib < max_attribs; ++attrib) {
        glVertexAttrib4f(attrib, 0.0f, 0.0f, 0.0f, 1.0f);
    }
    // Enable seamless cubemaps when per texture parameters are not available
    if (!GLAD_GL_ARB_seamless_cubemap_per_texture && !GLAD_GL_AMD_seamless_cubemap_per_texture) {
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    }
    // Enable unified vertex attributes and query vertex buffer address when the driver supports it
    if (device.HasVertexBufferUnifiedMemory()) {
        glEnableClientState(GL_VERTEX_ATTRIB_ARRAY_UNIFIED_NV);
        glEnableClientState(GL_ELEMENT_ARRAY_UNIFIED_NV);

        glMakeNamedBufferResidentNV(vertex_buffer.handle, GL_READ_ONLY);
        glGetNamedBufferParameterui64vNV(vertex_buffer.handle, GL_BUFFER_GPU_ADDRESS_NV,
                                         &vertex_buffer_address);
    }
}

RendererOpenGL::~RendererOpenGL() = default;

void RendererOpenGL::SwapBuffers(const Tegra::FramebufferConfig* framebuffer) {
    if (!framebuffer) {
        return;
    }
    PrepareRendertarget(framebuffer);
    RenderScreenshot();

    state_tracker.BindFramebuffer(0);
    DrawScreen(emu_window.GetFramebufferLayout());

    ++m_current_frame;

    gpu.RendererFrameEndNotify();
    rasterizer.TickFrame();

    context->SwapBuffers();
    render_window.OnFrameDisplayed();
}

void RendererOpenGL::PrepareRendertarget(const Tegra::FramebufferConfig* framebuffer) {
    if (!framebuffer) {
        return;
    }
    // If framebuffer is provided, reload it from memory to a texture
    if (screen_info.texture.width != static_cast<GLsizei>(framebuffer->width) ||
        screen_info.texture.height != static_cast<GLsizei>(framebuffer->height) ||
        screen_info.texture.pixel_format != framebuffer->pixel_format ||
        gl_framebuffer_data.empty()) {
        // Reallocate texture if the framebuffer size has changed.
        // This is expected to not happen very often and hence should not be a
        // performance problem.
        ConfigureFramebufferTexture(screen_info.texture, *framebuffer);
    }

    // Load the framebuffer from memory, draw it to the screen, and swap buffers
    LoadFBToScreenInfo(*framebuffer);
}

void RendererOpenGL::LoadFBToScreenInfo(const Tegra::FramebufferConfig& framebuffer) {
    // Framebuffer orientation handling
    framebuffer_transform_flags = framebuffer.transform_flags;
    framebuffer_crop_rect = framebuffer.crop_rect;
    framebuffer_width = framebuffer.width;
    framebuffer_height = framebuffer.height;

    const VAddr framebuffer_addr{framebuffer.address + framebuffer.offset};
    screen_info.was_accelerated =
        rasterizer.AccelerateDisplay(framebuffer, framebuffer_addr, framebuffer.stride);
    if (screen_info.was_accelerated) {
        return;
    }

    // Reset the screen info's display texture to its own permanent texture
    screen_info.display_texture = screen_info.texture.resource.handle;

    // TODO(Rodrigo): Read this from HLE
    constexpr u32 block_height_log2 = 4;
    const auto pixel_format{
        VideoCore::Surface::PixelFormatFromGPUPixelFormat(framebuffer.pixel_format)};
    const u32 bytes_per_pixel{VideoCore::Surface::BytesPerBlock(pixel_format)};
    const u64 size_in_bytes{Tegra::Texture::CalculateSize(
        true, bytes_per_pixel, framebuffer.stride, framebuffer.height, 1, block_height_log2, 0)};
    const u8* const host_ptr{cpu_memory.GetPointer(framebuffer_addr)};
    const std::span<const u8> input_data(host_ptr, size_in_bytes);
    Tegra::Texture::UnswizzleTexture(gl_framebuffer_data, input_data, bytes_per_pixel,
                                     framebuffer.width, framebuffer.height, 1, block_height_log2,
                                     0);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, static_cast<GLint>(framebuffer.stride));

    // Update existing texture
    // TODO: Test what happens on hardware when you change the framebuffer dimensions so that
    //       they differ from the LCD resolution.
    // TODO: Applications could theoretically crash yuzu here by specifying too large
    //       framebuffer sizes. We should make sure that this cannot happen.
    glTextureSubImage2D(screen_info.texture.resource.handle, 0, 0, 0, framebuffer.width,
                        framebuffer.height, screen_info.texture.gl_format,
                        screen_info.texture.gl_type, gl_framebuffer_data.data());

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}

void RendererOpenGL::LoadColorToActiveGLTexture(u8 color_r, u8 color_g, u8 color_b, u8 color_a,
                                                const TextureInfo& texture) {
    const u8 framebuffer_data[4] = {color_a, color_b, color_g, color_r};
    glClearTexImage(texture.resource.handle, 0, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer_data);
}

void RendererOpenGL::InitOpenGLObjects() {
    // Create shader programs
    fxaa_vertex = CreateProgram(HostShaders::FXAA_VERT, GL_VERTEX_SHADER);
    fxaa_fragment = CreateProgram(HostShaders::FXAA_FRAG, GL_FRAGMENT_SHADER);

    const auto replace_include = [](std::string& shader_source, std::string_view include_name,
                                    std::string_view include_content) {
        const std::string include_string = fmt::format("#include \"{}\"", include_name);
        const std::size_t pos = shader_source.find(include_string);
        ASSERT(pos != std::string::npos);
        shader_source.replace(pos, include_string.size(), include_content);
    };

    const auto SmaaShader = [&](std::string_view specialized_source, GLenum stage) {
        std::string shader_source{specialized_source};
        replace_include(shader_source, "opengl_smaa.glsl", HostShaders::OPENGL_SMAA_GLSL);
        return CreateProgram(shader_source, stage);
    };

    smaa_edge_detection_vert = SmaaShader(HostShaders::SMAA_EDGE_DETECTION_VERT, GL_VERTEX_SHADER);
    smaa_edge_detection_frag =
        SmaaShader(HostShaders::SMAA_EDGE_DETECTION_FRAG, GL_FRAGMENT_SHADER);
    smaa_blending_weight_calculation_vert =
        SmaaShader(HostShaders::SMAA_BLENDING_WEIGHT_CALCULATION_VERT, GL_VERTEX_SHADER);
    smaa_blending_weight_calculation_frag =
        SmaaShader(HostShaders::SMAA_BLENDING_WEIGHT_CALCULATION_FRAG, GL_FRAGMENT_SHADER);
    smaa_neighborhood_blending_vert =
        SmaaShader(HostShaders::SMAA_NEIGHBORHOOD_BLENDING_VERT, GL_VERTEX_SHADER);
    smaa_neighborhood_blending_frag =
        SmaaShader(HostShaders::SMAA_NEIGHBORHOOD_BLENDING_FRAG, GL_FRAGMENT_SHADER);

    present_vertex = CreateProgram(HostShaders::OPENGL_PRESENT_VERT, GL_VERTEX_SHADER);
    present_bilinear_fragment = CreateProgram(HostShaders::OPENGL_PRESENT_FRAG, GL_FRAGMENT_SHADER);
    present_bicubic_fragment = CreateProgram(HostShaders::PRESENT_BICUBIC_FRAG, GL_FRAGMENT_SHADER);
    present_gaussian_fragment =
        CreateProgram(HostShaders::PRESENT_GAUSSIAN_FRAG, GL_FRAGMENT_SHADER);
    present_scaleforce_fragment =
        CreateProgram(fmt::format("#version 460\n{}", HostShaders::OPENGL_PRESENT_SCALEFORCE_FRAG),
                      GL_FRAGMENT_SHADER);

    std::string fsr_source{HostShaders::OPENGL_FIDELITYFX_FSR_FRAG};
    replace_include(fsr_source, "ffx_a.h", HostShaders::FFX_A_H);
    replace_include(fsr_source, "ffx_fsr1.h", HostShaders::FFX_FSR1_H);

    std::string fsr_easu_frag_source{HostShaders::OPENGL_FIDELITYFX_FSR_EASU_FRAG};
    std::string fsr_rcas_frag_source{HostShaders::OPENGL_FIDELITYFX_FSR_RCAS_FRAG};
    replace_include(fsr_easu_frag_source, "opengl_fidelityfx_fsr.frag", fsr_source);
    replace_include(fsr_rcas_frag_source, "opengl_fidelityfx_fsr.frag", fsr_source);

    fsr = std::make_unique<FSR>(HostShaders::FULL_SCREEN_TRIANGLE_VERT, fsr_easu_frag_source,
                                fsr_rcas_frag_source);

    // Generate presentation sampler
    present_sampler.Create();
    glSamplerParameteri(present_sampler.handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glSamplerParameteri(present_sampler.handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(present_sampler.handle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(present_sampler.handle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(present_sampler.handle, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    present_sampler_nn.Create();
    glSamplerParameteri(present_sampler_nn.handle, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glSamplerParameteri(present_sampler_nn.handle, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glSamplerParameteri(present_sampler_nn.handle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(present_sampler_nn.handle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(present_sampler_nn.handle, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // Generate VBO handle for drawing
    vertex_buffer.Create();

    // Attach vertex data to VAO
    glNamedBufferData(vertex_buffer.handle, sizeof(ScreenRectVertex) * 4, nullptr, GL_STREAM_DRAW);

    // Allocate textures for the screen
    screen_info.texture.resource.Create(GL_TEXTURE_2D);

    const GLuint texture = screen_info.texture.resource.handle;
    glTextureStorage2D(texture, 1, GL_RGBA8, 1, 1);

    screen_info.display_texture = screen_info.texture.resource.handle;

    // Clear screen to black
    LoadColorToActiveGLTexture(0, 0, 0, 0, screen_info.texture);

    aa_framebuffer.Create();

    smaa_area_tex.Create(GL_TEXTURE_2D);
    glTextureStorage2D(smaa_area_tex.handle, 1, GL_RG8, AREATEX_WIDTH, AREATEX_HEIGHT);
    glTextureSubImage2D(smaa_area_tex.handle, 0, 0, 0, AREATEX_WIDTH, AREATEX_HEIGHT, GL_RG,
                        GL_UNSIGNED_BYTE, areaTexBytes);
    smaa_search_tex.Create(GL_TEXTURE_2D);
    glTextureStorage2D(smaa_search_tex.handle, 1, GL_R8, SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT);
    glTextureSubImage2D(smaa_search_tex.handle, 0, 0, 0, SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, GL_RED,
                        GL_UNSIGNED_BYTE, searchTexBytes);
}

void RendererOpenGL::AddTelemetryFields() {
    const char* const gl_version{reinterpret_cast<char const*>(glGetString(GL_VERSION))};
    const char* const gpu_vendor{reinterpret_cast<char const*>(glGetString(GL_VENDOR))};
    const char* const gpu_model{reinterpret_cast<char const*>(glGetString(GL_RENDERER))};

    LOG_INFO(Render_OpenGL, "GL_VERSION: {}", gl_version);
    LOG_INFO(Render_OpenGL, "GL_VENDOR: {}", gpu_vendor);
    LOG_INFO(Render_OpenGL, "GL_RENDERER: {}", gpu_model);

    constexpr auto user_system = Common::Telemetry::FieldType::UserSystem;
    telemetry_session.AddField(user_system, "GPU_Vendor", std::string(gpu_vendor));
    telemetry_session.AddField(user_system, "GPU_Model", std::string(gpu_model));
    telemetry_session.AddField(user_system, "GPU_OpenGL_Version", std::string(gl_version));
}

void RendererOpenGL::ConfigureFramebufferTexture(TextureInfo& texture,
                                                 const Tegra::FramebufferConfig& framebuffer) {
    texture.width = framebuffer.width;
    texture.height = framebuffer.height;
    texture.pixel_format = framebuffer.pixel_format;

    const auto pixel_format{
        VideoCore::Surface::PixelFormatFromGPUPixelFormat(framebuffer.pixel_format)};
    const u32 bytes_per_pixel{VideoCore::Surface::BytesPerBlock(pixel_format)};
    gl_framebuffer_data.resize(texture.width * texture.height * bytes_per_pixel);

    GLint internal_format;
    switch (framebuffer.pixel_format) {
    case Service::android::PixelFormat::Rgba8888:
        internal_format = GL_RGBA8;
        texture.gl_format = GL_RGBA;
        texture.gl_type = GL_UNSIGNED_INT_8_8_8_8_REV;
        break;
    case Service::android::PixelFormat::Rgb565:
        internal_format = GL_RGB565;
        texture.gl_format = GL_RGB;
        texture.gl_type = GL_UNSIGNED_SHORT_5_6_5;
        break;
    default:
        internal_format = GL_RGBA8;
        texture.gl_format = GL_RGBA;
        texture.gl_type = GL_UNSIGNED_INT_8_8_8_8_REV;
        // UNIMPLEMENTED_MSG("Unknown framebuffer pixel format: {}",
        //                   static_cast<u32>(framebuffer.pixel_format));
        break;
    }

    texture.resource.Release();
    texture.resource.Create(GL_TEXTURE_2D);
    glTextureStorage2D(texture.resource.handle, 1, internal_format, texture.width, texture.height);
    aa_texture.Release();
    aa_texture.Create(GL_TEXTURE_2D);
    glTextureStorage2D(aa_texture.handle, 1, GL_RGBA16F,
                       Settings::values.resolution_info.ScaleUp(screen_info.texture.width),
                       Settings::values.resolution_info.ScaleUp(screen_info.texture.height));
    glNamedFramebufferTexture(aa_framebuffer.handle, GL_COLOR_ATTACHMENT0, aa_texture.handle, 0);
    smaa_edges_tex.Release();
    smaa_edges_tex.Create(GL_TEXTURE_2D);
    glTextureStorage2D(smaa_edges_tex.handle, 1, GL_RG16F,
                       Settings::values.resolution_info.ScaleUp(screen_info.texture.width),
                       Settings::values.resolution_info.ScaleUp(screen_info.texture.height));
    smaa_blend_tex.Release();
    smaa_blend_tex.Create(GL_TEXTURE_2D);
    glTextureStorage2D(smaa_blend_tex.handle, 1, GL_RGBA16F,
                       Settings::values.resolution_info.ScaleUp(screen_info.texture.width),
                       Settings::values.resolution_info.ScaleUp(screen_info.texture.height));
}

void RendererOpenGL::DrawScreen(const Layout::FramebufferLayout& layout) {
    // TODO: Signal state tracker about these changes
    state_tracker.NotifyScreenDrawVertexArray();
    state_tracker.NotifyPolygonModes();
    state_tracker.NotifyViewport0();
    state_tracker.NotifyScissor0();
    state_tracker.NotifyColorMask(0);
    state_tracker.NotifyBlend0();
    state_tracker.NotifyFramebuffer();
    state_tracker.NotifyFrontFace();
    state_tracker.NotifyCullTest();
    state_tracker.NotifyDepthTest();
    state_tracker.NotifyStencilTest();
    state_tracker.NotifyPolygonOffset();
    state_tracker.NotifyRasterizeEnable();
    state_tracker.NotifyFramebufferSRGB();
    state_tracker.NotifyLogicOp();
    state_tracker.NotifyClipControl();
    state_tracker.NotifyAlphaTest();

    state_tracker.ClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);

    glEnable(GL_CULL_FACE);
    glDisable(GL_COLOR_LOGIC_OP);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_RASTERIZER_DISCARD);
    glDisable(GL_ALPHA_TEST);
    glDisablei(GL_BLEND, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);
    glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthRangeIndexed(0, 0.0, 0.0);

    glBindTextureUnit(0, screen_info.display_texture);

    auto anti_aliasing = Settings::values.anti_aliasing.GetValue();
    if (anti_aliasing > Settings::AntiAliasing::LastAA) {
        LOG_ERROR(Render_OpenGL, "Invalid antialiasing option selected {}", anti_aliasing);
        anti_aliasing = Settings::AntiAliasing::None;
        Settings::values.anti_aliasing.SetValue(anti_aliasing);
    }

    if (anti_aliasing != Settings::AntiAliasing::None) {
        glEnablei(GL_SCISSOR_TEST, 0);
        auto viewport_width = screen_info.texture.width;
        auto scissor_width = framebuffer_crop_rect.GetWidth();
        if (scissor_width <= 0) {
            scissor_width = viewport_width;
        }
        auto viewport_height = screen_info.texture.height;
        auto scissor_height = framebuffer_crop_rect.GetHeight();
        if (scissor_height <= 0) {
            scissor_height = viewport_height;
        }
        if (screen_info.was_accelerated) {
            viewport_width = Settings::values.resolution_info.ScaleUp(viewport_width);
            scissor_width = Settings::values.resolution_info.ScaleUp(scissor_width);
            viewport_height = Settings::values.resolution_info.ScaleUp(viewport_height);
            scissor_height = Settings::values.resolution_info.ScaleUp(scissor_height);
        }
        glScissorIndexed(0, 0, 0, scissor_width, scissor_height);
        glViewportIndexedf(0, 0.0f, 0.0f, static_cast<GLfloat>(viewport_width),
                           static_cast<GLfloat>(viewport_height));

        glBindSampler(0, present_sampler.handle);
        GLint old_read_fb;
        GLint old_draw_fb;
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &old_read_fb);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &old_draw_fb);

        switch (anti_aliasing) {
        case Settings::AntiAliasing::Fxaa: {
            program_manager.BindPresentPrograms(fxaa_vertex.handle, fxaa_fragment.handle);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, aa_framebuffer.handle);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        } break;
        case Settings::AntiAliasing::Smaa: {
            glClearColor(0, 0, 0, 0);
            glFrontFace(GL_CCW);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, aa_framebuffer.handle);
            glBindSampler(1, present_sampler.handle);
            glBindSampler(2, present_sampler.handle);

            glNamedFramebufferTexture(aa_framebuffer.handle, GL_COLOR_ATTACHMENT0,
                                      smaa_edges_tex.handle, 0);
            glClear(GL_COLOR_BUFFER_BIT);
            program_manager.BindPresentPrograms(smaa_edge_detection_vert.handle,
                                                smaa_edge_detection_frag.handle);
            glDrawArrays(GL_TRIANGLES, 0, 3);

            glBindTextureUnit(0, smaa_edges_tex.handle);
            glBindTextureUnit(1, smaa_area_tex.handle);
            glBindTextureUnit(2, smaa_search_tex.handle);
            glNamedFramebufferTexture(aa_framebuffer.handle, GL_COLOR_ATTACHMENT0,
                                      smaa_blend_tex.handle, 0);
            glClear(GL_COLOR_BUFFER_BIT);
            program_manager.BindPresentPrograms(smaa_blending_weight_calculation_vert.handle,
                                                smaa_blending_weight_calculation_frag.handle);
            glDrawArrays(GL_TRIANGLES, 0, 3);

            glBindTextureUnit(0, screen_info.display_texture);
            glBindTextureUnit(1, smaa_blend_tex.handle);
            glNamedFramebufferTexture(aa_framebuffer.handle, GL_COLOR_ATTACHMENT0,
                                      aa_texture.handle, 0);
            program_manager.BindPresentPrograms(smaa_neighborhood_blending_vert.handle,
                                                smaa_neighborhood_blending_frag.handle);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glFrontFace(GL_CW);
        } break;
        default:
            UNREACHABLE();
        }

        glBindFramebuffer(GL_READ_FRAMEBUFFER, old_read_fb);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, old_draw_fb);

        glBindTextureUnit(0, aa_texture.handle);
    }
    glDisablei(GL_SCISSOR_TEST, 0);

    if (Settings::values.scaling_filter.GetValue() == Settings::ScalingFilter::Fsr) {
        if (!fsr->AreBuffersInitialized()) {
            fsr->InitBuffers();
        }

        auto crop_rect = framebuffer_crop_rect;
        if (crop_rect.GetWidth() == 0) {
            crop_rect.right = framebuffer_width;
        }
        if (crop_rect.GetHeight() == 0) {
            crop_rect.bottom = framebuffer_height;
        }
        crop_rect = crop_rect.Scale(Settings::values.resolution_info.up_factor);
        const auto fsr_input_width = Settings::values.resolution_info.ScaleUp(framebuffer_width);
        const auto fsr_input_height = Settings::values.resolution_info.ScaleUp(framebuffer_height);
        glBindSampler(0, present_sampler.handle);
        fsr->Draw(program_manager, layout.screen, fsr_input_width, fsr_input_height, crop_rect);
    } else {
        if (fsr->AreBuffersInitialized()) {
            fsr->ReleaseBuffers();
        }
    }

    const std::array ortho_matrix =
        MakeOrthographicMatrix(static_cast<float>(layout.width), static_cast<float>(layout.height));

    const auto fragment_handle = [this]() {
        switch (Settings::values.scaling_filter.GetValue()) {
        case Settings::ScalingFilter::NearestNeighbor:
        case Settings::ScalingFilter::Bilinear:
            return present_bilinear_fragment.handle;
        case Settings::ScalingFilter::Bicubic:
            return present_bicubic_fragment.handle;
        case Settings::ScalingFilter::Gaussian:
            return present_gaussian_fragment.handle;
        case Settings::ScalingFilter::ScaleForce:
            return present_scaleforce_fragment.handle;
        case Settings::ScalingFilter::Fsr:
            return fsr->GetPresentFragmentProgram().handle;
        default:
            return present_bilinear_fragment.handle;
        }
    }();
    program_manager.BindPresentPrograms(present_vertex.handle, fragment_handle);
    glProgramUniformMatrix3x2fv(present_vertex.handle, ModelViewMatrixLocation, 1, GL_FALSE,
                                ortho_matrix.data());

    const auto& texcoords = screen_info.display_texcoords;
    auto left = texcoords.left;
    auto right = texcoords.right;
    if (framebuffer_transform_flags != Service::android::BufferTransformFlags::Unset) {
        if (framebuffer_transform_flags == Service::android::BufferTransformFlags::FlipV) {
            // Flip the framebuffer vertically
            left = texcoords.right;
            right = texcoords.left;
        } else {
            // Other transformations are unsupported
            LOG_CRITICAL(Render_OpenGL, "Unsupported framebuffer_transform_flags={}",
                         framebuffer_transform_flags);
            UNIMPLEMENTED();
        }
    }

    ASSERT_MSG(framebuffer_crop_rect.left == 0, "Unimplemented");

    f32 left_start{};
    if (framebuffer_crop_rect.Top() > 0) {
        left_start = static_cast<f32>(framebuffer_crop_rect.Top()) /
                     static_cast<f32>(framebuffer_crop_rect.Bottom());
    }
    f32 scale_u = static_cast<f32>(framebuffer_width) / static_cast<f32>(screen_info.texture.width);
    f32 scale_v =
        static_cast<f32>(framebuffer_height) / static_cast<f32>(screen_info.texture.height);

    if (Settings::values.scaling_filter.GetValue() != Settings::ScalingFilter::Fsr) {
        // Scale the output by the crop width/height. This is commonly used with 1280x720 rendering
        // (e.g. handheld mode) on a 1920x1080 framebuffer.
        if (framebuffer_crop_rect.GetWidth() > 0) {
            scale_u = static_cast<f32>(framebuffer_crop_rect.GetWidth()) /
                      static_cast<f32>(screen_info.texture.width);
        }
        if (framebuffer_crop_rect.GetHeight() > 0) {
            scale_v = static_cast<f32>(framebuffer_crop_rect.GetHeight()) /
                      static_cast<f32>(screen_info.texture.height);
        }
    }
    if (Settings::values.anti_aliasing.GetValue() == Settings::AntiAliasing::Fxaa &&
        !screen_info.was_accelerated) {
        scale_u /= Settings::values.resolution_info.up_factor;
        scale_v /= Settings::values.resolution_info.up_factor;
    }

    const auto& screen = layout.screen;
    const std::array vertices = {
        ScreenRectVertex(screen.left, screen.top, texcoords.top * scale_u,
                         left_start + left * scale_v),
        ScreenRectVertex(screen.right, screen.top, texcoords.bottom * scale_u,
                         left_start + left * scale_v),
        ScreenRectVertex(screen.left, screen.bottom, texcoords.top * scale_u,
                         left_start + right * scale_v),
        ScreenRectVertex(screen.right, screen.bottom, texcoords.bottom * scale_u,
                         left_start + right * scale_v),
    };
    glNamedBufferSubData(vertex_buffer.handle, 0, sizeof(vertices), std::data(vertices));

    if (screen_info.display_srgb) {
        glEnable(GL_FRAMEBUFFER_SRGB);
    } else {
        glDisable(GL_FRAMEBUFFER_SRGB);
    }
    glViewportIndexedf(0, 0.0f, 0.0f, static_cast<GLfloat>(layout.width),
                       static_cast<GLfloat>(layout.height));

    glEnableVertexAttribArray(PositionLocation);
    glEnableVertexAttribArray(TexCoordLocation);
    glVertexAttribDivisor(PositionLocation, 0);
    glVertexAttribDivisor(TexCoordLocation, 0);
    glVertexAttribFormat(PositionLocation, 2, GL_FLOAT, GL_FALSE,
                         offsetof(ScreenRectVertex, position));
    glVertexAttribFormat(TexCoordLocation, 2, GL_FLOAT, GL_FALSE,
                         offsetof(ScreenRectVertex, tex_coord));
    glVertexAttribBinding(PositionLocation, 0);
    glVertexAttribBinding(TexCoordLocation, 0);
    if (device.HasVertexBufferUnifiedMemory()) {
        glBindVertexBuffer(0, 0, 0, sizeof(ScreenRectVertex));
        glBufferAddressRangeNV(GL_VERTEX_ATTRIB_ARRAY_ADDRESS_NV, 0, vertex_buffer_address,
                               sizeof(vertices));
    } else {
        glBindVertexBuffer(0, vertex_buffer.handle, 0, sizeof(ScreenRectVertex));
    }

    if (Settings::values.scaling_filter.GetValue() != Settings::ScalingFilter::NearestNeighbor) {
        glBindSampler(0, present_sampler.handle);
    } else {
        glBindSampler(0, present_sampler_nn.handle);
    }

    // Update background color before drawing
    glClearColor(Settings::values.bg_red.GetValue() / 255.0f,
                 Settings::values.bg_green.GetValue() / 255.0f,
                 Settings::values.bg_blue.GetValue() / 255.0f, 1.0f);

    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // TODO
    // program_manager.RestoreGuestPipeline();
}

void RendererOpenGL::RenderScreenshot() {
    if (!renderer_settings.screenshot_requested) {
        return;
    }

    GLint old_read_fb;
    GLint old_draw_fb;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &old_read_fb);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &old_draw_fb);

    // Draw the current frame to the screenshot framebuffer
    screenshot_framebuffer.Create();
    glBindFramebuffer(GL_FRAMEBUFFER, screenshot_framebuffer.handle);

    const Layout::FramebufferLayout layout{renderer_settings.screenshot_framebuffer_layout};

    GLuint renderbuffer;
    glGenRenderbuffers(1, &renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, screen_info.display_srgb ? GL_SRGB8 : GL_RGB8,
                          layout.width, layout.height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderbuffer);

    DrawScreen(layout);

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glReadPixels(0, 0, layout.width, layout.height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
                 renderer_settings.screenshot_bits);

    screenshot_framebuffer.Release();
    glDeleteRenderbuffers(1, &renderbuffer);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, old_read_fb);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, old_draw_fb);

    renderer_settings.screenshot_complete_callback(true);
    renderer_settings.screenshot_requested = false;
}

} // namespace OpenGL
