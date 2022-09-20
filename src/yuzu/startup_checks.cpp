// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "video_core/vulkan_common/vulkan_wrapper.h"

#ifdef _WIN32
#include <cstring> // for memset, strncpy
#include <processthreadsapi.h>
#include <windows.h>
#elif defined(YUZU_UNIX)
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <cstdio>
#include "video_core/vulkan_common/vulkan_instance.h"
#include "video_core/vulkan_common/vulkan_library.h"
#include "yuzu/startup_checks.h"

void CheckVulkan() {
    // Just start the Vulkan loader, this will crash if something is wrong
    try {
        Vulkan::vk::InstanceDispatch dld;
        const Common::DynamicLibrary library = Vulkan::OpenLibrary();
        const Vulkan::vk::Instance instance =
            Vulkan::CreateInstance(library, dld, VK_API_VERSION_1_0);

    } catch (const Vulkan::vk::Exception& exception) {
        std::fprintf(stderr, "Failed to initialize Vulkan: %s\n", exception.what());
    }
}

bool CheckEnvVars(bool* is_child) {
#ifdef _WIN32
    // Check environment variable to see if we are the child
    char variable_contents[8];
    const DWORD startup_check_var =
        GetEnvironmentVariableA(STARTUP_CHECK_ENV_VAR, variable_contents, 8);
    if (startup_check_var > 0 && std::strncmp(variable_contents, ENV_VAR_ENABLED_TEXT, 8) == 0) {
        CheckVulkan();
        return true;
    }

    // Don't perform startup checks if we are a child process
    char is_child_s[8];
    const DWORD is_child_len = GetEnvironmentVariableA(IS_CHILD_ENV_VAR, is_child_s, 8);
    if (is_child_len > 0 && std::strncmp(is_child_s, ENV_VAR_ENABLED_TEXT, 8) == 0) {
        *is_child = true;
        return false;
    } else if (!SetEnvironmentVariableA(IS_CHILD_ENV_VAR, ENV_VAR_ENABLED_TEXT)) {
        std::fprintf(stderr, "SetEnvironmentVariableA failed to set %s with error %d\n",
                     IS_CHILD_ENV_VAR, GetLastError());
        return true;
    }
#endif
    return false;
}

bool StartupChecks(const char* arg0, bool* has_broken_vulkan, bool perform_vulkan_check) {
#ifdef _WIN32
    // Set the startup variable for child processes
    const bool env_var_set = SetEnvironmentVariableA(STARTUP_CHECK_ENV_VAR, ENV_VAR_ENABLED_TEXT);
    if (!env_var_set) {
        std::fprintf(stderr, "SetEnvironmentVariableA failed to set %s with error %d\n",
                     STARTUP_CHECK_ENV_VAR, GetLastError());
        return false;
    }

    if (perform_vulkan_check) {
        // Spawn child process that performs Vulkan check
        PROCESS_INFORMATION process_info;
        std::memset(&process_info, '\0', sizeof(process_info));

        if (!SpawnChild(arg0, &process_info, 0)) {
            return false;
        }

        // Wait until the processs exits and get exit code from it
        WaitForSingleObject(process_info.hProcess, INFINITE);
        DWORD exit_code = STILL_ACTIVE;
        const int err = GetExitCodeProcess(process_info.hProcess, &exit_code);
        if (err == 0) {
            std::fprintf(stderr, "GetExitCodeProcess failed with error %d\n", GetLastError());
        }

        // Vulkan is broken if the child crashed (return value is not zero)
        *has_broken_vulkan = (exit_code != 0);

        if (CloseHandle(process_info.hProcess) == 0) {
            std::fprintf(stderr, "CloseHandle failed with error %d\n", GetLastError());
        }
        if (CloseHandle(process_info.hThread) == 0) {
            std::fprintf(stderr, "CloseHandle failed with error %d\n", GetLastError());
        }
    }

    if (!SetEnvironmentVariableA(STARTUP_CHECK_ENV_VAR, nullptr)) {
        std::fprintf(stderr, "SetEnvironmentVariableA failed to clear %s with error %d\n",
                     STARTUP_CHECK_ENV_VAR, GetLastError());
    }

#elif defined(YUZU_UNIX)
    if (perform_vulkan_check) {
        const pid_t pid = fork();
        if (pid == 0) {
            CheckVulkan();
            return true;
        } else if (pid == -1) {
            const int err = errno;
            std::fprintf(stderr, "fork failed with error %d\n", err);
            return false;
        }

        // Get exit code from child process
        int status;
        const int r_val = wait(&status);
        if (r_val == -1) {
            const int err = errno;
            std::fprintf(stderr, "wait failed with error %d\n", err);
            return false;
        }
        // Vulkan is broken if the child crashed (return value is not zero)
        *has_broken_vulkan = (status != 0);
    }
#endif
    return false;
}

#ifdef _WIN32
bool SpawnChild(const char* arg0, PROCESS_INFORMATION* pi, int flags) {
    STARTUPINFOA startup_info;

    std::memset(&startup_info, '\0', sizeof(startup_info));
    startup_info.cb = sizeof(startup_info);

    char p_name[255];
    std::strncpy(p_name, arg0, 255);

    const bool process_created = CreateProcessA(nullptr,       // lpApplicationName
                                                p_name,        // lpCommandLine
                                                nullptr,       // lpProcessAttributes
                                                nullptr,       // lpThreadAttributes
                                                false,         // bInheritHandles
                                                flags,         // dwCreationFlags
                                                nullptr,       // lpEnvironment
                                                nullptr,       // lpCurrentDirectory
                                                &startup_info, // lpStartupInfo
                                                pi             // lpProcessInformation
    );
    if (!process_created) {
        std::fprintf(stderr, "CreateProcessA failed with error %d\n", GetLastError());
        return false;
    }

    return true;
}
#endif
