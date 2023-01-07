// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <span>

#include "video_core/renderer_opengl/gl_device.h"
#include "video_core/renderer_opengl/gl_resource_manager.h"

namespace OpenGL {

class ProgramManager {
    static constexpr size_t NUM_STAGES = 5;

public:
    explicit ProgramManager(const Device& device);

    void BindComputeProgram(GLuint program);

    void BindComputeAssemblyProgram(GLuint program);

    void BindSourcePrograms(std::span<const OGLProgram, NUM_STAGES> programs);

    void BindPresentPrograms(GLuint vertex, GLuint fragment);

    void BindAssemblyPrograms(std::span<const OGLAssemblyProgram, NUM_STAGES> programs,
                              u32 stage_mask);

    void RestoreGuestCompute();

private:
    void BindPipeline();

    void UnbindPipeline();

    void UnbindCompute();

    OGLPipeline pipeline;
    bool is_pipeline_bound{};
    bool is_compute_bound{};

    u32 current_stage_mask = 0;
    std::array<GLuint, NUM_STAGES> current_programs{};
    GLuint current_assembly_compute_program = 0;
};

} // namespace OpenGL
