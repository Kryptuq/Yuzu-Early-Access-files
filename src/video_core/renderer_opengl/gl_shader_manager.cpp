// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <glad/glad.h>

#include "common/common_types.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/renderer_opengl/gl_device.h"
#include "video_core/renderer_opengl/gl_shader_manager.h"

namespace OpenGL {

namespace {

void BindProgram(GLenum stage, GLuint current, GLuint old, bool& enabled) {
    if (current == old) {
        return;
    }
    if (current == 0) {
        if (enabled) {
            enabled = false;
            glDisable(stage);
        }
        return;
    }
    if (!enabled) {
        enabled = true;
        glEnable(stage);
    }
    glBindProgramARB(stage, current);
}

} // Anonymous namespace

ProgramManager::ProgramManager(const Device& device)
    : use_assembly_programs{device.UseAssemblyShaders()} {
    if (use_assembly_programs) {
        glEnable(GL_COMPUTE_PROGRAM_NV);
    } else {
        graphics_pipeline.Create();
        glBindProgramPipeline(graphics_pipeline.handle);
    }
}

ProgramManager::~ProgramManager() = default;

void ProgramManager::BindCompute(GLuint program) {
    if (use_assembly_programs) {
        glBindProgramARB(GL_COMPUTE_PROGRAM_NV, program);
    } else {
        is_graphics_bound = false;
        glUseProgram(program);
    }
}

void ProgramManager::BindGraphicsPipeline() {
    if (!use_assembly_programs) {
        UpdateSourcePrograms();
    }
}

void ProgramManager::BindHostPipeline(GLuint pipeline) {
    if (use_assembly_programs) {
        if (geometry_enabled) {
            geometry_enabled = false;
            old_state.geometry = 0;
            glDisable(GL_GEOMETRY_PROGRAM_NV);
        }
    } else {
        if (!is_graphics_bound) {
            glUseProgram(0);
        }
    }
    glBindProgramPipeline(pipeline);
}

void ProgramManager::RestoreGuestPipeline() {
    if (use_assembly_programs) {
        glBindProgramPipeline(0);
    } else {
        glBindProgramPipeline(graphics_pipeline.handle);
    }
}

void ProgramManager::BindHostCompute(GLuint program) {
    if (use_assembly_programs) {
        glDisable(GL_COMPUTE_PROGRAM_NV);
    }
    glUseProgram(program);
    is_graphics_bound = false;
}

void ProgramManager::RestoreGuestCompute() {
    if (use_assembly_programs) {
        glEnable(GL_COMPUTE_PROGRAM_NV);
        glUseProgram(0);
    }
}

void ProgramManager::UseVertexShader(GLuint program) {
    if (use_assembly_programs) {
        BindProgram(GL_VERTEX_PROGRAM_NV, program, current_state.vertex, vertex_enabled);
    }
    current_state.vertex = program;
}

void ProgramManager::UseGeometryShader(GLuint program) {
    if (use_assembly_programs) {
        BindProgram(GL_GEOMETRY_PROGRAM_NV, program, current_state.vertex, geometry_enabled);
    }
    current_state.geometry = program;
}

void ProgramManager::UseFragmentShader(GLuint program) {
    if (use_assembly_programs) {
        BindProgram(GL_FRAGMENT_PROGRAM_NV, program, current_state.vertex, fragment_enabled);
    }
    current_state.fragment = program;
}

void ProgramManager::UpdateSourcePrograms() {
    if (!is_graphics_bound) {
        is_graphics_bound = true;
        glUseProgram(0);
    }

    const GLuint handle = graphics_pipeline.handle;
    const auto update_state = [handle](GLenum stage, GLuint current, GLuint old) {
        if (current == old) {
            return;
        }
        glUseProgramStages(handle, stage, current);
    };
    update_state(GL_VERTEX_SHADER_BIT, current_state.vertex, old_state.vertex);
    update_state(GL_GEOMETRY_SHADER_BIT, current_state.geometry, old_state.geometry);
    update_state(GL_FRAGMENT_SHADER_BIT, current_state.fragment, old_state.fragment);

    old_state = current_state;
}

void MaxwellUniformData::SetFromRegs(const Tegra::Engines::Maxwell3D& maxwell) {
    const auto& regs = maxwell.regs;

    // Y_NEGATE controls what value S2R returns for the Y_DIRECTION system value.
    y_direction = regs.screen_y_control.y_negate == 0 ? 1.0f : -1.0f;
}

} // namespace OpenGL
