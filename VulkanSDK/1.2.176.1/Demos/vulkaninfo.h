/*
 * Copyright (c) 2015-2020 The Khronos Group Inc.
 * Copyright (c) 2015-2020 Valve Corporation
 * Copyright (c) 2015-2020 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
 * Author: David Pinedo <david@lunarg.com>
 * Author: Mark Lobodzinski <mark@lunarg.com>
 * Author: Rene Lindsay <rene@lunarg.com>
 * Author: Jeremy Kniager <jeremyk@lunarg.com>
 * Author: Shannon McPherson <shannon@lunarg.com>
 * Author: Bob Ellison <bob@lunarg.com>
 * Author: Charles Giessen <charles@lunarg.com>
 *
 */

#include <algorithm>
#include <array>
#include <exception>
#include <iostream>
#include <fstream>
#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <utility>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstring>

#ifdef __GNUC__
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#else
#define strndup(p, n) strdup(p)
#endif

#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#if _MSC_VER == 1900
#pragma warning(disable : 4800)
#endif
#endif  // _WIN32

#if defined(__linux__) || defined(__APPLE__)
#include <dlfcn.h>
#endif

#if defined(VK_USE_PLATFORM_XLIB_KHR) || defined(VK_USE_PLATFORM_XCB_KHR)
#include <X11/Xutil.h>
#endif

#if defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT)
#include "metal_view.h"
#endif

#include <vulkan/vulkan.h>

static const char *VkResultString(VkResult err);

// General error: Get file + line and a short message
struct FileLineException : std::runtime_error {
    FileLineException(const std::string &arg, const char *file, int line) : runtime_error(arg) {
        msg = std::string(file) + ":" + std::to_string(line) + ": " + arg;
    }
    ~FileLineException() throw() {}
    const char *what() const throw() { return msg.c_str(); }

  private:
    std::string msg;
};
#define THROW_ERR(arg) throw FileLineException(arg, __FILE__, __LINE__);

// Vulkan function error: Get name of function, file, line, and the error code returned by the function
struct VulkanException : std::runtime_error {
    VulkanException(const std::string &function, const char *file, int line, VkResult err) : runtime_error(function) {
        msg = std::string(file) + ":" + std::to_string(line) + ":" + function + " failed with " + VkResultString(err);
    }
    ~VulkanException() throw() {}
    const char *what() const throw() { return msg.c_str(); }

  private:
    std::string msg;
};
#define THROW_VK_ERR(func_name, err) throw VulkanException(func_name, __FILE__, __LINE__, err);

// Global configuration (Used by Windows WAIT_FOR_CONSOLE_DESTROY MACRO)
bool human_readable_output = true;
bool html_output = false;
bool json_output = false;
bool vkconfig_output = false;
bool portability_json = false;
bool summary = false;

#ifdef _WIN32

#define strdup _strdup

// Returns nonzero if the console is used only for this process. Will return
// zero if another process (such as cmd.exe) is also attached.
static int ConsoleIsExclusive(void) {
    DWORD pids[2];
    DWORD num_pids = GetConsoleProcessList(pids, ARRAYSIZE(pids));
    return num_pids <= 1;
}

#define WAIT_FOR_CONSOLE_DESTROY                                            \
    do {                                                                    \
        if (ConsoleIsExclusive() && human_readable_output) Sleep(INFINITE); \
    } while (0)
#else
#define WAIT_FOR_CONSOLE_DESTROY
#endif

#ifdef _WIN32

#define _CALL_PFN(pfn, ...) (pfn)
#define CALL_PFN(fncName) _CALL_PFN(User32Handles::pfn##fncName)

#define _CHECK_PFN(pfn, fncName)                                              \
    do {                                                                      \
        if (pfn == nullptr) {                                                 \
            fprintf(stderr, "Failed to get %s function address!\n", fncName); \
            WAIT_FOR_CONSOLE_DESTROY;                                         \
            exit(1);                                                          \
        }                                                                     \
    } while (false)

#define _SET_PFN(dllHandle, pfnType, pfn, fncName)                           \
    do {                                                                     \
        pfn = reinterpret_cast<pfnType>(GetProcAddress(dllHandle, fncName)); \
        _CHECK_PFN(pfn, fncName);                                            \
    } while (false)

#define SET_PFN(dllHandle, fncName) _SET_PFN(User32Handles::dllHandle, PFN_##fncName, User32Handles::pfn##fncName, #fncName)

// User32 function declarations
typedef WINUSERAPI BOOL(WINAPI *PFN_AdjustWindowRect)(_Inout_ LPRECT, _In_ DWORD, _In_ BOOL);
typedef WINUSERAPI HWND(WINAPI *PFN_CreateWindowExA)(_In_ DWORD, _In_opt_ LPCSTR, _In_opt_ LPCSTR, _In_ DWORD, _In_ int, _In_ int,
                                                     _In_ int, _In_ int, _In_opt_ HWND, _In_opt_ HMENU, _In_opt_ HINSTANCE,
                                                     _In_opt_ LPVOID);
typedef WINUSERAPI LRESULT(WINAPI *PFN_DefWindowProcA)(_In_ HWND, _In_ UINT, _In_ WPARAM, _In_ LPARAM);
typedef WINUSERAPI BOOL(WINAPI *PFN_DestroyWindow)(_In_ HWND);
typedef WINUSERAPI HICON(WINAPI *PFN_LoadIconA)(_In_opt_ HINSTANCE, _In_ LPCSTR);
typedef WINUSERAPI ATOM(WINAPI *PFN_RegisterClassExA)(_In_ CONST WNDCLASSEXA *);

struct User32Handles {
    // User32 function pointers
    static PFN_AdjustWindowRect pfnAdjustWindowRect;
    static PFN_CreateWindowExA pfnCreateWindowExA;
    static PFN_DefWindowProcA pfnDefWindowProcA;
    static PFN_DestroyWindow pfnDestroyWindow;
    static PFN_LoadIconA pfnLoadIconA;
    static PFN_RegisterClassExA pfnRegisterClassExA;

    // User32 dll handle
    static HMODULE user32DllHandle;
};

bool LoadUser32Dll() {
    User32Handles::user32DllHandle = LoadLibraryExA("user32.dll", nullptr, 0);
    if (User32Handles::user32DllHandle != NULL) {
        SET_PFN(user32DllHandle, AdjustWindowRect);
        SET_PFN(user32DllHandle, CreateWindowExA);
        SET_PFN(user32DllHandle, DefWindowProcA);
        SET_PFN(user32DllHandle, DestroyWindow);
        SET_PFN(user32DllHandle, LoadIconA);
        SET_PFN(user32DllHandle, RegisterClassExA);
        return true;
    }
    return false;
}

void FreeUser32Dll() {
    if (User32Handles::user32DllHandle != nullptr) {
        FreeLibrary(User32Handles::user32DllHandle);
        User32Handles::user32DllHandle = nullptr;
    }
}
#endif  // _WIN32

const char *app_short_name = "vulkaninfo";

std::vector<const char *> get_c_str_array(std::vector<std::string> const &vec) {
    std::vector<const char *> ret;
    for (auto &str : vec) ret.push_back(str.c_str());
    return ret;
}

static const char *VkDebugReportFlagsEXTString(const VkDebugReportFlagsEXT flags) {
    switch (flags) {
        case VK_DEBUG_REPORT_ERROR_BIT_EXT:
            return "ERROR";
        case VK_DEBUG_REPORT_WARNING_BIT_EXT:
            return "WARNING";
        case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
            return "PERF";
        case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
            return "INFO";
        case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
            return "DEBUG";
        default:
            return "UNKNOWN";
    }
}
static VKAPI_ATTR VkBool32 VKAPI_CALL DbgCallback(VkDebugReportFlagsEXT msgFlags, VkDebugReportObjectTypeEXT objType,
                                                  uint64_t srcObject, size_t location, int32_t msgCode, const char *pLayerPrefix,
                                                  const char *pMsg, void *pUserData) {
    std::cerr << VkDebugReportFlagsEXTString(msgFlags) << ": [" << pLayerPrefix << "] Code " << msgCode << " : " << pMsg << "\n";

    // True is reserved for layer developers, and MAY mean calls are not distributed down the layer chain after validation
    // error. False SHOULD always be returned by apps:
    return VK_FALSE;
}

// Helper for robustly executing the two-call pattern
template <typename T, typename F, typename... Ts>
auto GetVectorInit(const char *func_name, F &&f, T init, Ts &&... ts) -> std::vector<T> {
    uint32_t count = 0;
    std::vector<T> results;
    VkResult err;
    do {
        err = f(ts..., &count, nullptr);
        if (err) THROW_VK_ERR(func_name, err);
        results.resize(count, init);
        err = f(ts..., &count, results.data());
        results.resize(count);
    } while (err == VK_INCOMPLETE);
    if (err) THROW_VK_ERR(func_name, err);
    return results;
}

template <typename T, typename F, typename... Ts>
auto GetVector(const char *func_name, F &&f, Ts &&... ts) -> std::vector<T> {
    return GetVectorInit(func_name, f, T(), ts...);
}

// ----------- Instance Setup ------- //
struct VkDll {
    VkResult Initialize() {
#if defined(__linux__)
        library = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
        if (!library) library = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
#elif defined(_WIN32)
        library = LoadLibrary(TEXT("vulkan-1.dll"));
#endif
#if !defined(__APPLE__)
        if (library == 0) return VK_ERROR_INITIALIZATION_FAILED;
#endif
        return VK_SUCCESS;
    }
    void Close() {
#if defined(__linux__)
        dlclose(library);
#elif defined(_WIN32)
        FreeLibrary(library);
#endif
#if !defined(__APPLE__)
        library = 0;
#endif
    }

#if defined(__APPLE__)
#define APPLE_FP(name) name
#else
#define APPLE_FP(nama) nullptr
#endif

    // Function pointers, loaded from the dll
    PFN_vkCreateInstance fp_vkCreateInstance = APPLE_FP(vkCreateInstance);
    PFN_vkEnumerateInstanceExtensionProperties fp_vkEnumerateInstanceExtensionProperties =
        APPLE_FP(vkEnumerateInstanceExtensionProperties);
    PFN_vkEnumerateInstanceLayerProperties fp_vkEnumerateInstanceLayerProperties = APPLE_FP(vkEnumerateInstanceLayerProperties);
    PFN_vkDestroyInstance fp_vkDestroyInstance = APPLE_FP(vkDestroyInstance);
    PFN_vkEnumeratePhysicalDevices fp_vkEnumeratePhysicalDevices = APPLE_FP(vkEnumeratePhysicalDevices);
    PFN_vkGetPhysicalDeviceFeatures fp_vkGetPhysicalDeviceFeatures = APPLE_FP(vkGetPhysicalDeviceFeatures);
    PFN_vkGetPhysicalDeviceFormatProperties fp_vkGetPhysicalDeviceFormatProperties = APPLE_FP(vkGetPhysicalDeviceFormatProperties);
    PFN_vkGetPhysicalDeviceImageFormatProperties fp_vkGetPhysicalDeviceImageFormatProperties =
        APPLE_FP(vkGetPhysicalDeviceImageFormatProperties);
    PFN_vkGetPhysicalDeviceProperties fp_vkGetPhysicalDeviceProperties = APPLE_FP(vkGetPhysicalDeviceProperties);
    PFN_vkGetPhysicalDeviceQueueFamilyProperties fp_vkGetPhysicalDeviceQueueFamilyProperties =
        APPLE_FP(vkGetPhysicalDeviceQueueFamilyProperties);
    PFN_vkGetPhysicalDeviceMemoryProperties fp_vkGetPhysicalDeviceMemoryProperties = APPLE_FP(vkGetPhysicalDeviceMemoryProperties);
    PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr = APPLE_FP(vkGetInstanceProcAddr);
    PFN_vkGetDeviceProcAddr fp_vkGetDeviceProcAddr = APPLE_FP(vkGetDeviceProcAddr);
    PFN_vkCreateDevice fp_vkCreateDevice = APPLE_FP(vkCreateDevice);
    PFN_vkDestroyDevice fp_vkDestroyDevice = APPLE_FP(vkDestroyDevice);
    PFN_vkEnumerateDeviceExtensionProperties fp_vkEnumerateDeviceExtensionProperties =
        APPLE_FP(vkEnumerateDeviceExtensionProperties);
    PFN_vkGetDeviceQueue fp_vkGetDeviceQueue = APPLE_FP(vkGetDeviceQueue);
    PFN_vkCreateImage fp_vkCreateImage = APPLE_FP(vkCreateImage);
    PFN_vkDestroyImage fp_vkDestroyImage = APPLE_FP(vkDestroyImage);
    PFN_vkGetBufferMemoryRequirements fp_vkGetBufferMemoryRequirements = APPLE_FP(vkGetBufferMemoryRequirements);
    PFN_vkGetImageMemoryRequirements fp_vkGetImageMemoryRequirements = APPLE_FP(vkGetImageMemoryRequirements);
    PFN_vkGetImageSparseMemoryRequirements fp_vkGetImageSparseMemoryRequirements = APPLE_FP(vkGetImageSparseMemoryRequirements);
    PFN_vkEnumerateInstanceVersion fp_vkEnumerateInstanceVersion = APPLE_FP(vkEnumerateInstanceVersion);
    PFN_vkEnumeratePhysicalDeviceGroups fp_vkEnumeratePhysicalDeviceGroups = APPLE_FP(vkEnumeratePhysicalDeviceGroups);
    PFN_vkGetPhysicalDeviceFeatures2 fp_vkGetPhysicalDeviceFeatures2 = APPLE_FP(vkGetPhysicalDeviceFeatures2);
    PFN_vkGetPhysicalDeviceProperties2 fp_vkGetPhysicalDeviceProperties2 = APPLE_FP(vkGetPhysicalDeviceProperties2);
    PFN_vkGetPhysicalDeviceFormatProperties2 fp_vkGetPhysicalDeviceFormatProperties2 =
        APPLE_FP(vkGetPhysicalDeviceFormatProperties2);
    PFN_vkGetPhysicalDeviceQueueFamilyProperties2 fp_vkGetPhysicalDeviceQueueFamilyProperties2 =
        APPLE_FP(vkGetPhysicalDeviceQueueFamilyProperties2);
    PFN_vkGetPhysicalDeviceMemoryProperties2 fp_vkGetPhysicalDeviceMemoryProperties2 =
        APPLE_FP(vkGetPhysicalDeviceMemoryProperties2);
    PFN_vkDestroySurfaceKHR fp_vkDestroySurfaceKHR = APPLE_FP(vkDestroySurfaceKHR);

#ifdef VK_USE_PLATFORM_XLIB_KHR
    PFN_vkCreateXlibSurfaceKHR fp_vkCreateXlibSurfaceKHR = APPLE_FP(vkCreateXlibSurfaceKHR);
#endif  // VK_USE_PLATFORM_XLIB_KHR
#ifdef VK_USE_PLATFORM_XLIB_KHR
    PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR fp_vkGetPhysicalDeviceXlibPresentationSupportKHR =
        APPLE_FP(vkGetPhysicalDeviceXlibPresentationSupportKHR);
#endif  // VK_USE_PLATFORM_XLIB_KHR
#ifdef VK_USE_PLATFORM_XCB_KHR
    PFN_vkCreateXcbSurfaceKHR fp_vkCreateXcbSurfaceKHR = APPLE_FP(vkCreateXcbSurfaceKHR);
#endif  // VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_XCB_KHR
    PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR fp_vkGetPhysicalDeviceXcbPresentationSupportKHR =
        APPLE_FP(vkGetPhysicalDeviceXcbPresentationSupportKHR);
#endif  // VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    PFN_vkCreateWaylandSurfaceKHR fp_vkCreateWaylandSurfaceKHR = APPLE_FP(vkCreateWaylandSurfaceKHR);
#endif  // VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR fp_vkGetPhysicalDeviceWaylandPresentationSupportKHR =
        APPLE_FP(vkGetPhysicalDeviceWaylandPresentationSupportKHR);
#endif  // VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_DIRECTFB_EXT
    PFN_vkCreateDirectFBSurfaceEXT fp_vkCreateDirectFBSurfaceEXT = APPLE_FP(vkCreateDirectFBSurfaceEXT);
#endif  // VK_USE_PLATFORM_DIRECTFB_EXT
#ifdef VK_USE_PLATFORM_DIRECTFB_EXT
    PFN_vkGetPhysicalDeviceDirectFBPresentationSupportEXT fp_vkGetPhysicalDeviceDirectFBPresentationSupportEXT =
        APPLE_FP(vkGetPhysicalDeviceDirectFBPresentationSupportEXT);
#endif  // VK_USE_PLATFORM_DIRECTFB_EXT
#ifdef VK_USE_PLATFORM_ANDROID_KHR
    PFN_vkCreateAndroidSurfaceKHR fp_vkCreateAndroidSurfaceKHR = APPLE_FP(vkCreateAndroidSurfaceKHR);
#endif  // VK_USE_PLATFORM_ANDROID_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkCreateWin32SurfaceKHR fp_vkCreateWin32SurfaceKHR = APPLE_FP(vkCreateWin32SurfaceKHR);
#endif  // VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR fp_vkGetPhysicalDeviceWin32PresentationSupportKHR =
        APPLE_FP(vkGetPhysicalDeviceWin32PresentationSupportKHR);
#endif  // VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_MACOS_MVK
    PFN_vkCreateMacOSSurfaceMVK fp_vkCreateMacOSSurfaceMVK = APPLE_FP(vkCreateMacOSSurfaceMVK);
#endif  // VK_USE_PLATFORM_MACOS_MVK
#ifdef VK_USE_PLATFORM_METAL_EXT
    PFN_vkCreateMetalSurfaceEXT fp_vkCreateMetalSurfaceEXT = APPLE_FP(vkCreateMetalSurfaceEXT);
#endif  // VK_USE_PLATFORM_METAL_EXT
    void InitializeDispatchPointers() {
        Load(fp_vkCreateInstance, "vkCreateInstance");
        Load(fp_vkDestroyInstance, "vkDestroyInstance");
        Load(fp_vkEnumeratePhysicalDevices, "vkEnumeratePhysicalDevices");
        Load(fp_vkGetPhysicalDeviceFeatures, "vkGetPhysicalDeviceFeatures");
        Load(fp_vkGetPhysicalDeviceFormatProperties, "vkGetPhysicalDeviceFormatProperties");
        Load(fp_vkGetPhysicalDeviceImageFormatProperties, "vkGetPhysicalDeviceImageFormatProperties");
        Load(fp_vkGetPhysicalDeviceProperties, "vkGetPhysicalDeviceProperties");
        Load(fp_vkGetPhysicalDeviceQueueFamilyProperties, "vkGetPhysicalDeviceQueueFamilyProperties");
        Load(fp_vkGetPhysicalDeviceMemoryProperties, "vkGetPhysicalDeviceMemoryProperties");
        Load(fp_vkGetInstanceProcAddr, "vkGetInstanceProcAddr");
        Load(fp_vkGetDeviceProcAddr, "vkGetDeviceProcAddr");
        Load(fp_vkCreateDevice, "vkCreateDevice");
        Load(fp_vkDestroyDevice, "vkDestroyDevice");
        Load(fp_vkEnumerateInstanceExtensionProperties, "vkEnumerateInstanceExtensionProperties");
        Load(fp_vkEnumerateDeviceExtensionProperties, "vkEnumerateDeviceExtensionProperties");
        Load(fp_vkEnumerateInstanceLayerProperties, "vkEnumerateInstanceLayerProperties");
        Load(fp_vkGetDeviceQueue, "vkGetDeviceQueue");
        Load(fp_vkCreateImage, "vkCreateImage");
        Load(fp_vkDestroyImage, "vkDestroyImage");
        Load(fp_vkGetBufferMemoryRequirements, "vkGetBufferMemoryRequirements");
        Load(fp_vkGetImageMemoryRequirements, "vkGetImageMemoryRequirements");
        Load(fp_vkGetImageSparseMemoryRequirements, "vkGetImageSparseMemoryRequirements");
        Load(fp_vkEnumerateInstanceVersion, "vkEnumerateInstanceVersion");
        Load(fp_vkEnumeratePhysicalDeviceGroups, "vkEnumeratePhysicalDeviceGroups");
        Load(fp_vkGetPhysicalDeviceFeatures2, "vkGetPhysicalDeviceFeatures2");
        Load(fp_vkGetPhysicalDeviceProperties2, "vkGetPhysicalDeviceProperties2");
        Load(fp_vkGetPhysicalDeviceFormatProperties2, "vkGetPhysicalDeviceFormatProperties2");
        Load(fp_vkGetPhysicalDeviceQueueFamilyProperties2, "vkGetPhysicalDeviceQueueFamilyProperties2");
        Load(fp_vkGetPhysicalDeviceMemoryProperties2, "vkGetPhysicalDeviceMemoryProperties2");
        Load(fp_vkDestroySurfaceKHR, "vkDestroySurfaceKHR");

#ifdef VK_USE_PLATFORM_XLIB_KHR
        Load(fp_vkCreateXlibSurfaceKHR, "vkCreateXlibSurfaceKHR");
#endif  // VK_USE_PLATFORM_XLIB_KHR
#ifdef VK_USE_PLATFORM_XLIB_KHR
        Load(fp_vkGetPhysicalDeviceXlibPresentationSupportKHR, "vkGetPhysicalDeviceXlibPresentationSupportKHR");
#endif  // VK_USE_PLATFORM_XLIB_KHR
#ifdef VK_USE_PLATFORM_XCB_KHR
        Load(fp_vkCreateXcbSurfaceKHR, "vkCreateXcbSurfaceKHR");
#endif  // VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_XCB_KHR
        Load(fp_vkGetPhysicalDeviceXcbPresentationSupportKHR, "vkGetPhysicalDeviceXcbPresentationSupportKHR");
#endif  // VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        Load(fp_vkCreateWaylandSurfaceKHR, "vkCreateWaylandSurfaceKHR");
#endif  // VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        Load(fp_vkGetPhysicalDeviceWaylandPresentationSupportKHR, "vkGetPhysicalDeviceWaylandPresentationSupportKHR");
#endif  // VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_DIRECTFB_EXT
        Load(fp_vkCreateDirectFBSurfaceEXT, "vkCreateDirectFBSurfaceEXT");
#endif  // VK_USE_PLATFORM_DIRECTFB_EXT
#ifdef VK_USE_PLATFORM_DIRECTFB_EXT
        Load(fp_vkGetPhysicalDeviceDirectFBPresentationSupportEXT, "vkGetPhysicalDeviceDirectFBPresentationSupportEXT");
#endif  // VK_USE_PLATFORM_DIRECTFB_EXT
#ifdef VK_USE_PLATFORM_ANDROID_KHR
        Load(fp_vkCreateAndroidSurfaceKHR, "vkCreateAndroidSurfaceKHR");
#endif  // VK_USE_PLATFORM_ANDROID_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
        Load(fp_vkCreateWin32SurfaceKHR, "vkCreateWin32SurfaceKHR");
#endif  // VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
        Load(fp_vkGetPhysicalDeviceWin32PresentationSupportKHR, "vkGetPhysicalDeviceWin32PresentationSupportKHR");
#endif  // VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_MACOS_MVK
        Load(fp_vkCreateMacOSSurfaceMVK, "vkCreateMacOSSurfaceMVK");
#endif  // VK_USE_PLATFORM_MACOS_MVK
#ifdef VK_USE_PLATFORM_METAL_EXT
        Load(fp_vkCreateMetalSurfaceEXT, "vkCreateMetalSurfaceEXT");
#endif  // VK_USE_PLATFORM_METAL_EXT
    }

  private:
    template <typename T>
    void Load(T &func_dest, const char *func_name) {
#if defined(__linux__)
        func_dest = reinterpret_cast<T>(dlsym(library, func_name));
#elif defined(_WIN32)
        func_dest = reinterpret_cast<T>(GetProcAddress(library, func_name));
#endif
    }
#if defined(__linux__)
    void *library;
#elif defined(_WIN32)
    HMODULE library;
#endif
};

struct ExtensionFunctions {
    // Extension pointers, loaded after instance is created
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
    PFN_vkGetDeviceGroupPresentCapabilitiesKHR vkGetDeviceGroupPresentCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormats2KHR vkGetPhysicalDeviceSurfaceFormats2KHR;
    PFN_vkGetPhysicalDeviceProperties2KHR vkGetPhysicalDeviceProperties2KHR;
    PFN_vkGetPhysicalDeviceFormatProperties2KHR vkGetPhysicalDeviceFormatProperties2KHR;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR vkGetPhysicalDeviceQueueFamilyProperties2KHR;
    PFN_vkGetPhysicalDeviceFeatures2KHR vkGetPhysicalDeviceFeatures2KHR;
    PFN_vkGetPhysicalDeviceMemoryProperties2KHR vkGetPhysicalDeviceMemoryProperties2KHR;
    PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR vkGetPhysicalDeviceSurfaceCapabilities2KHR;
    PFN_vkGetPhysicalDeviceSurfaceCapabilities2EXT vkGetPhysicalDeviceSurfaceCapabilities2EXT;
    PFN_vkEnumeratePhysicalDeviceGroupsKHR vkEnumeratePhysicalDeviceGroupsKHR;
    PFN_vkGetPhysicalDeviceToolPropertiesEXT vkGetPhysicalDeviceToolPropertiesEXT;

    void LoadInstanceExtensionDispatchPointers(PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, VkInstance instance) {
        this->instance = instance;
        this->vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        Load(vkGetPhysicalDeviceSurfaceSupportKHR, "vkGetPhysicalDeviceSurfaceSupportKHR");
        Load(vkGetPhysicalDeviceSurfaceCapabilitiesKHR, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
        Load(vkGetPhysicalDeviceSurfaceFormatsKHR, "vkGetPhysicalDeviceSurfaceFormatsKHR");
        Load(vkGetPhysicalDeviceSurfaceFormats2KHR, "vkGetPhysicalDeviceSurfaceFormats2KHR");
        Load(vkGetPhysicalDeviceSurfacePresentModesKHR, "vkGetPhysicalDeviceSurfacePresentModesKHR");
        Load(vkGetDeviceGroupPresentCapabilitiesKHR, "vkGetDeviceGroupPresentCapabilitiesKHR");
        Load(vkGetPhysicalDeviceProperties2KHR, "vkGetPhysicalDeviceProperties2KHR");
        Load(vkGetPhysicalDeviceFormatProperties2KHR, "vkGetPhysicalDeviceFormatProperties2KHR");
        Load(vkGetPhysicalDeviceQueueFamilyProperties2KHR, "vkGetPhysicalDeviceQueueFamilyProperties2KHR");
        Load(vkGetPhysicalDeviceFeatures2KHR, "vkGetPhysicalDeviceFeatures2KHR");
        Load(vkGetPhysicalDeviceMemoryProperties2KHR, "vkGetPhysicalDeviceMemoryProperties2KHR");
        Load(vkGetPhysicalDeviceSurfaceCapabilities2KHR, "vkGetPhysicalDeviceSurfaceCapabilities2KHR");
        Load(vkGetPhysicalDeviceSurfaceCapabilities2EXT, "vkGetPhysicalDeviceSurfaceCapabilities2EXT");
        Load(vkEnumeratePhysicalDeviceGroupsKHR, "vkEnumeratePhysicalDeviceGroupsKHR");
        Load(vkGetPhysicalDeviceToolPropertiesEXT, "vkGetPhysicalDeviceToolPropertiesEXT");
    }

  private:
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
    VkInstance instance;
    template <typename T>
    void Load(T &dest, const char *name) {
        dest = reinterpret_cast<T>(vkGetInstanceProcAddr(instance, name));
    }
};
struct VkStructureHeader {
    VkStructureType sType;
    VkStructureHeader *pNext;
};

struct pNextChainBuildingBlockInfo {
    VkStructureType sType;
    uint32_t mem_size;
};

void buildpNextChain(VkStructureHeader *first, const std::vector<pNextChainBuildingBlockInfo> &chain_info) {
    VkStructureHeader *place = first;

    for (uint32_t i = 0; i < chain_info.size(); i++) {
        place->pNext = static_cast<VkStructureHeader *>(malloc(chain_info[i].mem_size));
        if (!place->pNext) {
            THROW_ERR("buildpNextChain's malloc failed to allocate");
        }
        std::memset(place->pNext, 0, chain_info[i].mem_size);
        place = place->pNext;
        place->sType = chain_info[i].sType;
    }

    place->pNext = nullptr;
}

void freepNextChain(VkStructureHeader *first) {
    VkStructureHeader *place = first;
    VkStructureHeader *next = nullptr;

    while (place) {
        next = place->pNext;
        free(place);
        place = next;
    }
}

struct LayerExtensionList {
    VkLayerProperties layer_properties{};
    std::vector<VkExtensionProperties> extension_properties;
};

struct AppInstance;

struct SurfaceExtension {
    std::string name;
    void (*create_window)(AppInstance &) = nullptr;
    VkSurfaceKHR (*create_surface)(AppInstance &) = nullptr;
    void (*destroy_window)(AppInstance &) = nullptr;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkBool32 supports_present = 0;

    bool operator==(const SurfaceExtension &other) {
        return name == other.name && surface == other.surface && supports_present == other.supports_present;
    }
};

struct VulkanVersion {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
};

struct AppInstance {
    VkDll dll;

    VkInstance instance;
    uint32_t instance_version;
    VulkanVersion vk_version;

    ExtensionFunctions ext_funcs;

    std::vector<LayerExtensionList> global_layers;

    std::vector<VkExtensionProperties> global_extensions;  // Instance Extensions

    std::vector<std::string> inst_extensions;

    std::vector<SurfaceExtension> surface_extensions;

    int width = 256, height = 256;

    VkSurfaceCapabilitiesKHR surface_capabilities;

#ifdef VK_USE_PLATFORM_WIN32_KHR
    HINSTANCE h_instance;  // Windows Instance
    HWND h_wnd;            // window handle
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
    xcb_connection_t *xcb_connection;
    xcb_screen_t *xcb_screen;
    xcb_window_t xcb_window;
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
    Display *xlib_display;
    Window xlib_window;
#endif
#ifdef VK_USE_PLATFORM_MACOS_MVK
    void *macos_window;
#endif
#ifdef VK_USE_PLATFORM_METAL_EXT
    void *metal_window;
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    wl_display *wayland_display;
    wl_surface *wayland_surface;
#endif
#ifdef VK_USE_PLATFORM_DIRECTFB_EXT
    IDirectFB *dfb;
    IDirectFBSurface *directfb_surface;
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR  // TODO
    ANativeWindow *window;
#endif
    AppInstance() {
        VkResult dllErr = dll.Initialize();
        if (dllErr != VK_SUCCESS) {
            THROW_ERR("Failed to initialize: Vulkan loader is not installed, not found, or failed to load.");
        }
        dll.InitializeDispatchPointers();

        if (!dll.fp_vkEnumerateInstanceVersion) {
            instance_version = VK_API_VERSION_1_0;
        } else {
            const VkResult err = dll.fp_vkEnumerateInstanceVersion(&instance_version);
            if (err) THROW_VK_ERR("vkEnumerateInstanceVersion", err);
        }

        // fallback to baked header version if loader returns 0 for the patch version
        uint32_t patch_version = VK_VERSION_PATCH(instance_version);
        if (patch_version == 0) patch_version = VK_VERSION_PATCH(VK_HEADER_VERSION);
        vk_version = {VK_VERSION_MAJOR(instance_version), VK_VERSION_MINOR(instance_version), patch_version};

        AppGetInstanceExtensions();

        const VkDebugReportCallbackCreateInfoEXT dbg_info = {VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT, nullptr,
                                                             VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT,
                                                             DbgCallback};

        const VkApplicationInfo app_info = {
            VK_STRUCTURE_TYPE_APPLICATION_INFO, nullptr, app_short_name, 1, nullptr, 0, instance_version};

        AppCompileInstanceExtensionsToEnable();

        std::vector<const char *> inst_exts;
        for (auto &ext : inst_extensions) inst_exts.push_back(ext.c_str());

        const VkInstanceCreateInfo inst_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,  &dbg_info,       0, &app_info, 0, nullptr,
                                                static_cast<uint32_t>(inst_exts.size()), inst_exts.data()};

        VkResult err = dll.fp_vkCreateInstance(&inst_info, nullptr, &instance);
        if (err == VK_ERROR_INCOMPATIBLE_DRIVER) {
            std::cerr << "Cannot create Vulkan instance.\n";
            std::cerr << "This problem is often caused by a faulty installation of the Vulkan driver or attempting to use a GPU "
                         "that does not support Vulkan.\n";
            THROW_VK_ERR("vkCreateInstance", err);
        } else if (err) {
            THROW_VK_ERR("vkCreateInstance", err);
        }
        ext_funcs.LoadInstanceExtensionDispatchPointers(dll.fp_vkGetInstanceProcAddr, instance);
    }

    ~AppInstance() {
        if (dll.fp_vkDestroyInstance) dll.fp_vkDestroyInstance(instance, nullptr);
        dll.Close();
    }

    AppInstance(const AppInstance &) = delete;
    const AppInstance &operator=(const AppInstance &) = delete;

    bool CheckExtensionEnabled(std::string extension_to_check) {
        for (auto &extension : inst_extensions) {
            if (extension_to_check == extension) {
                return true;
            }
        }
        return false;
    }

    /* Gets a list of layer and instance extensions */
    void AppGetInstanceExtensions() {
        /* Scan layers */
        auto global_layer_properties =
            GetVector<VkLayerProperties>("vkEnumerateInstanceLayerProperties", dll.fp_vkEnumerateInstanceLayerProperties);
        global_layers.resize(global_layer_properties.size());

        for (size_t i = 0; i < global_layer_properties.size(); i++) {
            global_layers[i].layer_properties = global_layer_properties[i];

            global_layers[i].extension_properties = AppGetGlobalLayerExtensions(global_layer_properties[i].layerName);
        }

        // Collect global extensions
        // Gets instance extensions, if no layer was specified in the first paramteter
        global_extensions = AppGetGlobalLayerExtensions(nullptr);
    }
    void AppCompileInstanceExtensionsToEnable() {
        // Get all supported Instance extensions (excl. layer-provided ones)
        for (auto &ext : global_extensions) {
            inst_extensions.push_back(ext.extensionName);
        }
    }

    void AddSurfaceExtension(SurfaceExtension ext) { surface_extensions.push_back(ext); }

    std::vector<VkExtensionProperties> AppGetGlobalLayerExtensions(char *layer_name) {
        return GetVector<VkExtensionProperties>("vkEnumerateInstanceExtensionProperties",
                                                dll.fp_vkEnumerateInstanceExtensionProperties, layer_name);
    }

    std::vector<VkPhysicalDevice> FindPhysicalDevices() {
        return GetVector<VkPhysicalDevice>("vkEnumeratePhysicalDevices", dll.fp_vkEnumeratePhysicalDevices, instance);
    }
};

// --------- Platform Specific Presentation Calls --------- //

//---------------------------Win32---------------------------
#ifdef VK_USE_PLATFORM_WIN32_KHR

// MS-Windows event handling function:
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return (CALL_PFN(DefWindowProcA)(hWnd, uMsg, wParam, lParam));
}

static void AppCreateWin32Window(AppInstance &inst) {
    inst.h_instance = GetModuleHandle(nullptr);

    WNDCLASSEX win_class;

    // Initialize the window class structure:
    win_class.cbSize = sizeof(WNDCLASSEX);
    win_class.style = CS_HREDRAW | CS_VREDRAW;
    win_class.lpfnWndProc = WndProc;
    win_class.cbClsExtra = 0;
    win_class.cbWndExtra = 0;
    win_class.hInstance = inst.h_instance;
    win_class.hIcon = CALL_PFN(LoadIconA)(nullptr, IDI_APPLICATION);
    win_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
    win_class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    win_class.lpszMenuName = nullptr;
    win_class.lpszClassName = app_short_name;
    win_class.hInstance = inst.h_instance;
    win_class.hIconSm = CALL_PFN(LoadIconA)(nullptr, IDI_WINLOGO);
    // Register window class:
    if (!CALL_PFN(RegisterClassExA)(&win_class)) {
        // It didn't work, so try to give a useful error:
        THROW_ERR("Failed to register the window class!");
    }
    // Create window with the registered class:
    RECT wr = {0, 0, inst.width, inst.height};
    CALL_PFN(AdjustWindowRect)(&wr, WS_OVERLAPPEDWINDOW, FALSE);
    inst.h_wnd = CALL_PFN(CreateWindowExA)(0,
                                           app_short_name,  // class name
                                           app_short_name,  // app name
                                           // WS_VISIBLE | WS_SYSMENU |
                                           WS_OVERLAPPEDWINDOW,  // window style
                                           100, 100,             // x/y coords
                                           wr.right - wr.left,   // width
                                           wr.bottom - wr.top,   // height
                                           nullptr,              // handle to parent
                                           nullptr,              // handle to menu
                                           inst.h_instance,      // hInstance
                                           nullptr);             // no extra parameters
    if (!inst.h_wnd) {
        // It didn't work, so try to give a useful error:
        THROW_ERR("Failed to create a window!");
    }
}

static VkSurfaceKHR AppCreateWin32Surface(AppInstance &inst) {
    VkWin32SurfaceCreateInfoKHR createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.hinstance = inst.h_instance;
    createInfo.hwnd = inst.h_wnd;

    VkSurfaceKHR surface;
    VkResult err = inst.dll.fp_vkCreateWin32SurfaceKHR(inst.instance, &createInfo, nullptr, &surface);
    if (err) THROW_VK_ERR("vkCreateWin32SurfaceKHR", err);
    return surface;
}

static void AppDestroyWin32Window(AppInstance &inst) { CALL_PFN(DestroyWindow)(inst.h_wnd); }
#endif  // VK_USE_PLATFORM_WIN32_KHR
//-----------------------------------------------------------

#if defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_XLIB_KHR) || defined(VK_USE_PLATFORM_WIN32_KHR) ||      \
    defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT) || defined(VK_USE_PLATFORM_WAYLAND_KHR) || \
    defined(VK_USE_PLATFORM_DIRECTFB_EXT) || defined(VK_USE_PLATFORM_ANDROID_KHR)
static void AppDestroySurface(AppInstance &inst, VkSurfaceKHR surface) {  // same for all platforms
    inst.dll.fp_vkDestroySurfaceKHR(inst.instance, surface, nullptr);
}
#endif

//----------------------------XCB----------------------------

#ifdef VK_USE_PLATFORM_XCB_KHR
static void AppCreateXcbWindow(AppInstance &inst) {
    //--Init Connection--
    const xcb_setup_t *setup;
    xcb_screen_iterator_t iter;
    int scr;

    // API guarantees non-null xcb_connection
    inst.xcb_connection = xcb_connect(nullptr, &scr);
    int conn_error = xcb_connection_has_error(inst.xcb_connection);
    if (conn_error) {
        fprintf(stderr, "XCB failed to connect to the X server due to error:%d.\n", conn_error);
        fflush(stderr);
        xcb_disconnect(inst.xcb_connection);
        inst.xcb_connection = nullptr;
        return;
    }

    setup = xcb_get_setup(inst.xcb_connection);
    iter = xcb_setup_roots_iterator(setup);
    while (scr-- > 0) {
        xcb_screen_next(&iter);
    }

    inst.xcb_screen = iter.data;
    //-------------------

    inst.xcb_window = xcb_generate_id(inst.xcb_connection);
    xcb_create_window(inst.xcb_connection, XCB_COPY_FROM_PARENT, inst.xcb_window, inst.xcb_screen->root, 0, 0, inst.width,
                      inst.height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, inst.xcb_screen->root_visual, 0, nullptr);

    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(inst.xcb_connection, 1, 12, "WM_PROTOCOLS");
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(inst.xcb_connection, cookie, 0);
    free(reply);
}

static VkSurfaceKHR AppCreateXcbSurface(AppInstance &inst) {
    if (!inst.xcb_connection) {
        THROW_ERR("AppCreateXcbSurface failed to establish connection");
    }

    VkXcbSurfaceCreateInfoKHR xcb_createInfo;
    xcb_createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    xcb_createInfo.pNext = nullptr;
    xcb_createInfo.flags = 0;
    xcb_createInfo.connection = inst.xcb_connection;
    xcb_createInfo.window = inst.xcb_window;

    VkSurfaceKHR surface;
    VkResult err = inst.dll.fp_vkCreateXcbSurfaceKHR(inst.instance, &xcb_createInfo, nullptr, &surface);
    if (err) THROW_VK_ERR("vkCreateXcbSurfaceKHR", err);
    return surface;
}

static void AppDestroyXcbWindow(AppInstance &inst) {
    if (!inst.xcb_connection) {
        return;  // Nothing to destroy
    }

    xcb_destroy_window(inst.xcb_connection, inst.xcb_window);
    xcb_disconnect(inst.xcb_connection);
}
#endif  // VK_USE_PLATFORM_XCB_KHR
//-----------------------------------------------------------

//----------------------------XLib---------------------------
#ifdef VK_USE_PLATFORM_XLIB_KHR
static void AppCreateXlibWindow(AppInstance &inst) {
    long visualMask = VisualScreenMask;
    int numberOfVisuals;

    inst.xlib_display = XOpenDisplay(nullptr);
    if (inst.xlib_display == nullptr) {
        THROW_ERR("XLib failed to connect to the X server.\nExiting...");
    }

    XVisualInfo vInfoTemplate = {};
    vInfoTemplate.screen = DefaultScreen(inst.xlib_display);
    XVisualInfo *visualInfo = XGetVisualInfo(inst.xlib_display, visualMask, &vInfoTemplate, &numberOfVisuals);
    inst.xlib_window = XCreateWindow(inst.xlib_display, RootWindow(inst.xlib_display, vInfoTemplate.screen), 0, 0, inst.width,
                                     inst.height, 0, visualInfo->depth, InputOutput, visualInfo->visual, 0, nullptr);

    XSync(inst.xlib_display, false);
    XFree(visualInfo);
}

static VkSurfaceKHR AppCreateXlibSurface(AppInstance &inst) {
    VkXlibSurfaceCreateInfoKHR createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.dpy = inst.xlib_display;
    createInfo.window = inst.xlib_window;

    VkSurfaceKHR surface;
    VkResult err = inst.dll.fp_vkCreateXlibSurfaceKHR(inst.instance, &createInfo, nullptr, &surface);
    if (err) THROW_VK_ERR("vkCreateXlibSurfaceKHR", err);
    return surface;
}

static void AppDestroyXlibWindow(AppInstance &inst) {
    XDestroyWindow(inst.xlib_display, inst.xlib_window);
    XCloseDisplay(inst.xlib_display);
}
#endif  // VK_USE_PLATFORM_XLIB_KHR
//-----------------------------------------------------------

//------------------------MACOS_MVK--------------------------
#ifdef VK_USE_PLATFORM_MACOS_MVK
static void AppCreateMacOSWindow(AppInstance &inst) {
    inst.macos_window = CreateMetalView(inst.width, inst.height);
    if (inst.macos_window == nullptr) {
        THROW_ERR("Could not create a native Metal view.\nExiting...");
    }
}

static VkSurfaceKHR AppCreateMacOSSurface(AppInstance &inst) {
    VkMacOSSurfaceCreateInfoMVK createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.pView = inst.macos_window;

    VkSurfaceKHR surface;
    VkResult err = inst.dll.fp_vkCreateMacOSSurfaceMVK(inst.instance, &createInfo, nullptr, &surface);
    if (err) THROW_VK_ERR("vkCreateMacOSSurfaceMVK", err);
    return surface;
}

static void AppDestroyMacOSWindow(AppInstance &inst) { DestroyMetalView(inst.macos_window); }
#endif  // VK_USE_PLATFORM_MACOS_MVK
//-----------------------------------------------------------

//------------------------METAL_EXT--------------------------
#ifdef VK_USE_PLATFORM_METAL_EXT
static void AppCreateMetalWindow(AppInstance &inst) {
    inst.metal_window = CreateMetalView(inst.width, inst.height);
    if (inst.metal_window == nullptr) {
        THROW_ERR("Could not create a native Metal view.\nExiting...");
    }
}

static VkSurfaceKHR AppCreateMetalSurface(AppInstance &inst) {
    VkMetalSurfaceCreateInfoEXT createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.pLayer = static_cast<CAMetalLayer *>(GetCAMetalLayerFromMetalView(inst.metal_window));

    VkSurfaceKHR surface;
    VkResult err = inst.dll.fp_vkCreateMetalSurfaceEXT(inst.instance, &createInfo, nullptr, &surface);
    if (err) THROW_VK_ERR("vkCreateMetalSurfaceEXT", err);
    return surface;
}

static void AppDestroyMetalWindow(AppInstance &inst) { DestroyMetalView(inst.metal_window); }
#endif  // VK_USE_PLATFORM_METAL_EXT
//-----------------------------------------------------------

//-------------------------WAYLAND---------------------------
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
static void wayland_registry_global(void *data, struct wl_registry *registry, uint32_t id, const char *interface,
                                    uint32_t version) {
    AppInstance &inst = *static_cast<AppInstance *>(data);
    if (strcmp(interface, "wl_compositor") == 0) {
        struct wl_compositor *compositor = (struct wl_compositor *)wl_registry_bind(registry, id, &wl_compositor_interface, 1);
        inst.wayland_surface = wl_compositor_create_surface(compositor);
    }
}
static void wayland_registry_global_remove(void *data, struct wl_registry *registry, uint32_t id) {}
static const struct wl_registry_listener wayland_registry_listener = {wayland_registry_global, wayland_registry_global_remove};

static void AppCreateWaylandWindow(AppInstance &inst) {
    inst.wayland_display = wl_display_connect(nullptr);
    struct wl_registry *registry = wl_display_get_registry(inst.wayland_display);
    wl_registry_add_listener(wl_display_get_registry(inst.wayland_display), &wayland_registry_listener, static_cast<void *>(&inst));
    wl_display_roundtrip(inst.wayland_display);
    wl_registry_destroy(registry);
}

static VkSurfaceKHR AppCreateWaylandSurface(AppInstance &inst) {
    VkWaylandSurfaceCreateInfoKHR createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.display = inst.wayland_display;
    createInfo.surface = inst.wayland_surface;

    VkSurfaceKHR surface;
    VkResult err = inst.dll.fp_vkCreateWaylandSurfaceKHR(inst.instance, &createInfo, nullptr, &surface);
    if (err) THROW_VK_ERR("vkCreateWaylandSurfaceKHR", err);
    return surface;
}

static void AppDestroyWaylandWindow(AppInstance &inst) { wl_display_disconnect(inst.wayland_display); }
#endif  // VK_USE_PLATFORM_WAYLAND_KHR
//-----------------------------------------------------------

//-------------------------DIRECTFB--------------------------
#ifdef VK_USE_PLATFORM_DIRECTFB_EXT
static void AppCreateDirectFBWindow(AppInstance &inst) {
    DFBResult ret;

    ret = DirectFBInit(NULL, NULL);
    if (ret) {
        THROW_ERR("DirectFBInit failed to initialize DirectFB.\nExiting...");
    }

    ret = DirectFBCreate(&inst.dfb);
    if (ret) {
        THROW_ERR("DirectFBCreate failed to create main interface of DirectFB.\nExiting...");
    }

    DFBSurfaceDescription desc;
    desc.flags = (DFBSurfaceDescriptionFlags)(DSDESC_CAPS | DSDESC_WIDTH | DSDESC_HEIGHT);
    desc.caps = DSCAPS_PRIMARY;
    desc.width = inst.width;
    desc.height = inst.height;
    ret = inst.dfb->CreateSurface(inst.dfb, &desc, &inst.directfb_surface);
    if (ret) {
        THROW_ERR("CreateSurface failed to create DirectFB surface interface.\nExiting...");
    }
}

static VkSurfaceKHR AppCreateDirectFBSurface(AppInstance &inst) {
    VkDirectFBSurfaceCreateInfoEXT createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_DIRECTFB_SURFACE_CREATE_INFO_EXT;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.dfb = inst.dfb;
    createInfo.surface = inst.directfb_surface;

    VkSurfaceKHR surface;
    VkResult err = inst.dll.fp_vkCreateDirectFBSurfaceEXT(inst.instance, &createInfo, nullptr, &surface);
    if (err) THROW_VK_ERR("vkCreateDirectFBSurfaceEXT", err);
    return surface;
}

static void AppDestroyDirectFBWindow(AppInstance &inst) {
    inst.directfb_surface->Release(inst.directfb_surface);
    inst.dfb->Release(inst.dfb);
}
#endif  // VK_USE_PLATFORM_DIRECTFB_EXT
//-----------------------------------------------------------

//-------------------------ANDROID---------------------------
#ifdef VK_USE_PLATFORM_ANDROID_KHR
static void AppCreateAndroidWindow(AppInstance &inst) {}
static VkSurfaceKHR AppCreateAndroidSurface(AppInstance &inst) {
    VkAndroidSurfaceCreateInfoKHR createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.window = (struct ANativeWindow *)(inst.window);

    err = inst.dll.fp_vkCreateAndroidSurfaceKHR(inst.inst, &createInfo, NULL, &inst.surface);
    THROW_VK_ERR("vkCreateAndroidSurfaceKHR", err);
}
static VkSurfaceKHR AppDestroyAndroidSurface(AppInstance &inst) {}
#endif
//-----------------------------------------------------------

// ------------ Setup Windows ------------- //

void SetupWindowExtensions(AppInstance &inst) {
#if defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_XLIB_KHR)
    bool has_display = true;
    const char *display_var = getenv("DISPLAY");
    if (display_var == nullptr || strlen(display_var) == 0) {
        has_display = false;
        std::cerr << "'DISPLAY' environment variable not set... skipping surface info\n";
    }
#endif

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    wl_display *wayland_display = wl_display_connect(nullptr);
    bool has_wayland_display = false;
    if (wayland_display != nullptr) {
        wl_display_disconnect(wayland_display);
        has_wayland_display = true;
    }
#endif

//--WIN32--
#ifdef VK_USE_PLATFORM_WIN32_KHR
    SurfaceExtension surface_ext_win32;
    if (inst.CheckExtensionEnabled(VK_KHR_WIN32_SURFACE_EXTENSION_NAME)) {
        surface_ext_win32.name = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
        surface_ext_win32.create_window = AppCreateWin32Window;
        surface_ext_win32.create_surface = AppCreateWin32Surface;
        surface_ext_win32.destroy_window = AppDestroyWin32Window;

        inst.AddSurfaceExtension(surface_ext_win32);
    }
#endif
//--XCB--
#ifdef VK_USE_PLATFORM_XCB_KHR
    SurfaceExtension surface_ext_xcb;
    if (inst.CheckExtensionEnabled(VK_KHR_XCB_SURFACE_EXTENSION_NAME)) {
        surface_ext_xcb.name = VK_KHR_XCB_SURFACE_EXTENSION_NAME;
        surface_ext_xcb.create_window = AppCreateXcbWindow;
        surface_ext_xcb.create_surface = AppCreateXcbSurface;
        surface_ext_xcb.destroy_window = AppDestroyXcbWindow;
        if (has_display) {
            inst.AddSurfaceExtension(surface_ext_xcb);
        }
    }
#endif
//--XLIB--
#ifdef VK_USE_PLATFORM_XLIB_KHR
    SurfaceExtension surface_ext_xlib;
    if (inst.CheckExtensionEnabled(VK_KHR_XLIB_SURFACE_EXTENSION_NAME)) {
        surface_ext_xlib.name = VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
        surface_ext_xlib.create_window = AppCreateXlibWindow;
        surface_ext_xlib.create_surface = AppCreateXlibSurface;
        surface_ext_xlib.destroy_window = AppDestroyXlibWindow;
        if (has_display) {
            inst.AddSurfaceExtension(surface_ext_xlib);
        }
    }
#endif
//--MACOS--
#ifdef VK_USE_PLATFORM_MACOS_MVK
    SurfaceExtension surface_ext_macos;
    if (inst.CheckExtensionEnabled(VK_MVK_MACOS_SURFACE_EXTENSION_NAME)) {
        surface_ext_macos.name = VK_MVK_MACOS_SURFACE_EXTENSION_NAME;
        surface_ext_macos.create_window = AppCreateMacOSWindow;
        surface_ext_macos.create_surface = AppCreateMacOSSurface;
        surface_ext_macos.destroy_window = AppDestroyMacOSWindow;

        inst.AddSurfaceExtension(surface_ext_macos);
    }
#endif

#ifdef VK_USE_PLATFORM_METAL_EXT
    SurfaceExtension surface_ext_metal;
    if (inst.CheckExtensionEnabled(VK_EXT_METAL_SURFACE_EXTENSION_NAME)) {
        surface_ext_metal.name = VK_EXT_METAL_SURFACE_EXTENSION_NAME;
        surface_ext_metal.create_window = AppCreateMetalWindow;
        surface_ext_metal.create_surface = AppCreateMetalSurface;
        surface_ext_metal.destroy_window = AppDestroyMetalWindow;

        inst.AddSurfaceExtension(surface_ext_metal);
    }
#endif
//--WAYLAND--
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    SurfaceExtension surface_ext_wayland;
    if (inst.CheckExtensionEnabled(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME)) {
        surface_ext_wayland.name = VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
        surface_ext_wayland.create_window = AppCreateWaylandWindow;
        surface_ext_wayland.create_surface = AppCreateWaylandSurface;
        surface_ext_wayland.destroy_window = AppDestroyWaylandWindow;
        if (has_wayland_display) {
            inst.AddSurfaceExtension(surface_ext_wayland);
        }
    }
#endif
//--DIRECTFB--
#ifdef VK_USE_PLATFORM_DIRECTFB_EXT
    SurfaceExtension surface_ext_directfb;
    if (inst.CheckExtensionEnabled(VK_EXT_DIRECTFB_SURFACE_EXTENSION_NAME)) {
        surface_ext_directfb.name = VK_EXT_DIRECTFB_SURFACE_EXTENSION_NAME;
        surface_ext_directfb.create_window = AppCreateDirectFBWindow;
        surface_ext_directfb.create_surface = AppCreateDirectFBSurface;
        surface_ext_directfb.destroy_window = AppDestroyDirectFBWindow;

        inst.AddSurfaceExtension(surface_ext_directfb);
    }
#endif
//--ANDROID--
#ifdef VK_USE_PLATFORM_ANDROID_KHR
    SurfaceExtension surface_ext_android;
    if (inst.CheckExtensionEnabled(VK_ANDROID_SURFACE_EXTENSION_NAME)) {
        surface_ext_android.name = VK_ANDROID_SURFACE_EXTENSION_NAME;
        surface_ext_android.create_window = AppCreateAndroidWindow;
        surface_ext_android.create_surface = AppCreateAndroidSurface;
        surface_ext_android.destroy_window = AppDestroyAndroidWindow;

        inst.AddSurfaceExtension(surface_ext_android);
    }
#endif
}

// ---------- Surfaces -------------- //

class AppSurface {
  public:
    AppInstance &inst;
    VkPhysicalDevice phys_device;
    SurfaceExtension surface_extension;

    std::vector<VkPresentModeKHR> surf_present_modes;

    std::vector<VkSurfaceFormatKHR> surf_formats;
    std::vector<VkSurfaceFormat2KHR> surf_formats2;

    VkSurfaceCapabilitiesKHR surface_capabilities{};
    VkSurfaceCapabilities2KHR surface_capabilities2_khr{};
    VkSurfaceCapabilities2EXT surface_capabilities2_ext{};

    AppSurface(AppInstance &inst, VkPhysicalDevice phys_device, SurfaceExtension surface_extension,
               std::vector<pNextChainBuildingBlockInfo> &sur_extension_pNextChain)
        : inst(inst),
          phys_device(phys_device),
          surface_extension(surface_extension),
          surf_present_modes(GetVector<VkPresentModeKHR>("vkGetPhysicalDeviceSurfacePresentModesKHR",
                                                         inst.ext_funcs.vkGetPhysicalDeviceSurfacePresentModesKHR, phys_device,
                                                         surface_extension.surface)) {
        const VkPhysicalDeviceSurfaceInfo2KHR surface_info2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR, nullptr,
                                                               surface_extension.surface};

        if (inst.CheckExtensionEnabled(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME)) {
            VkSurfaceFormat2KHR init{};
            init.sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR;
            surf_formats2 = GetVectorInit<VkSurfaceFormat2KHR>("vkGetPhysicalDeviceSurfaceFormats2KHR",
                                                               inst.ext_funcs.vkGetPhysicalDeviceSurfaceFormats2KHR, init,
                                                               phys_device, &surface_info2);
        } else {
            surf_formats = GetVector<VkSurfaceFormatKHR>("vkGetPhysicalDeviceSurfaceFormatsKHR",
                                                         inst.ext_funcs.vkGetPhysicalDeviceSurfaceFormatsKHR, phys_device,
                                                         surface_extension.surface);
        }

        if (inst.CheckExtensionEnabled(VK_KHR_SURFACE_EXTENSION_NAME)) {
            VkResult err = inst.ext_funcs.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys_device, surface_extension.surface,
                                                                                    &surface_capabilities);
            if (err) THROW_VK_ERR("vkGetPhysicalDeviceSurfaceCapabilitiesKHR", err);
        }

        if (inst.CheckExtensionEnabled(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME)) {
            surface_capabilities2_khr.sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR;
            buildpNextChain((VkStructureHeader *)&surface_capabilities2_khr, sur_extension_pNextChain);

            VkPhysicalDeviceSurfaceInfo2KHR surface_info{};
            surface_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
            surface_info.surface = surface_extension.surface;
#if defined(WIN32)
            VkSurfaceFullScreenExclusiveWin32InfoEXT win32_fullscreen_exclusive_info{};
            win32_fullscreen_exclusive_info.sType = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_WIN32_INFO_EXT;
            win32_fullscreen_exclusive_info.hmonitor = MonitorFromWindow(inst.h_wnd, MONITOR_DEFAULTTOPRIMARY);

            surface_info.pNext = reinterpret_cast<void *>(&win32_fullscreen_exclusive_info);
#endif  // defined(WIN32)
            VkResult err =
                inst.ext_funcs.vkGetPhysicalDeviceSurfaceCapabilities2KHR(phys_device, &surface_info, &surface_capabilities2_khr);
            if (err) THROW_VK_ERR("vkGetPhysicalDeviceSurfaceCapabilities2KHR", err);
        }

        if (inst.CheckExtensionEnabled(VK_EXT_DISPLAY_SURFACE_COUNTER_EXTENSION_NAME)) {
            surface_capabilities2_ext.sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_EXT;
            surface_capabilities2_ext.pNext = nullptr;
            VkResult err = inst.ext_funcs.vkGetPhysicalDeviceSurfaceCapabilities2EXT(phys_device, surface_extension.surface,
                                                                                     &surface_capabilities2_ext);
            if (err) THROW_VK_ERR("vkGetPhysicalDeviceSurfaceCapabilities2EXT", err);
        }
    }

    ~AppSurface() {
        if (inst.CheckExtensionEnabled(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME)) {
            freepNextChain(static_cast<VkStructureHeader *>(surface_capabilities2_khr.pNext));
        }
    }

    AppSurface(const AppSurface &) = delete;
    const AppSurface &operator=(const AppSurface &) = delete;
};

// -------------------- Device Groups ------------------------//

std::vector<VkPhysicalDeviceGroupProperties> GetGroups(AppInstance &inst) {
    if (inst.CheckExtensionEnabled(VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME)) {
        return GetVector<VkPhysicalDeviceGroupProperties>("vkEnumeratePhysicalDeviceGroupsKHR",
                                                          inst.ext_funcs.vkEnumeratePhysicalDeviceGroupsKHR, inst.instance);
    }
    return {};
}

std::vector<VkPhysicalDeviceProperties> GetGroupProps(AppInstance &inst, VkPhysicalDeviceGroupProperties group) {
    std::vector<VkPhysicalDeviceProperties> props(group.physicalDeviceCount);

    for (uint32_t i = 0; i < group.physicalDeviceCount; ++i) {
        inst.dll.fp_vkGetPhysicalDeviceProperties(group.physicalDevices[i], &props[i]);
    }

    return props;
}

// The bool of the pair returns true if the extension VK_KHR_device_group is present
std::pair<bool, VkDeviceGroupPresentCapabilitiesKHR> GetGroupCapabilities(AppInstance &inst,
                                                                          VkPhysicalDeviceGroupProperties group) {
    // Build create info for logical device made from all physical devices in this group.
    std::vector<std::string> extensions_list = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DEVICE_GROUP_EXTENSION_NAME};
    VkDeviceGroupDeviceCreateInfoKHR dg_ci = {VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO_KHR, nullptr,
                                              group.physicalDeviceCount, group.physicalDevices};

    float queue_priority = 1.0f;

    auto ext_list = get_c_str_array(extensions_list);

    VkDeviceQueueCreateInfo q_ci = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0, 0, 1, &queue_priority};
    VkDeviceCreateInfo device_ci = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,   &dg_ci,         0, 1, &q_ci, 0, nullptr,
                                    static_cast<uint32_t>(ext_list.size()), ext_list.data()};

    VkDevice logical_device = VK_NULL_HANDLE;

    VkResult err = inst.dll.fp_vkCreateDevice(group.physicalDevices[0], &device_ci, nullptr, &logical_device);
    if (err != VK_SUCCESS && err != VK_ERROR_EXTENSION_NOT_PRESENT) THROW_VK_ERR("vkCreateDevice", err);

    if (err == VK_ERROR_EXTENSION_NOT_PRESENT) {
        VkDeviceGroupPresentCapabilitiesKHR group_capabilities = {VK_STRUCTURE_TYPE_DEVICE_GROUP_PRESENT_CAPABILITIES_KHR, nullptr};
        inst.dll.fp_vkDestroyDevice(logical_device, nullptr);
        return std::pair<bool, VkDeviceGroupPresentCapabilitiesKHR>(false, group_capabilities);
    }

    VkDeviceGroupPresentCapabilitiesKHR group_capabilities = {VK_STRUCTURE_TYPE_DEVICE_GROUP_PRESENT_CAPABILITIES_KHR, nullptr};

    // If the KHR_device_group extension is present, write the capabilities of the logical device into a struct for later
    // output to user.
    err = inst.ext_funcs.vkGetDeviceGroupPresentCapabilitiesKHR(logical_device, &group_capabilities);
    if (err) THROW_VK_ERR("vkGetDeviceGroupPresentCapabilitiesKHR", err);

    inst.dll.fp_vkDestroyDevice(logical_device, nullptr);

    return std::pair<bool, VkDeviceGroupPresentCapabilitiesKHR>(true, group_capabilities);
}

// -------------------- Device Setup ------------------- //

const VkFormat color_format = VK_FORMAT_R8G8B8A8_UNORM;

struct ImageTypeSupport {
    enum class Type { regular, sparse, transient } type = Type::regular;
    bool supported = false;
    uint32_t memoryTypeBits = 0;

    bool Compatible(uint32_t memtype_bit) { return supported && (memoryTypeBits & memtype_bit); }
};

struct ImageTypeFormatInfo {
    VkFormat format;
    std::vector<ImageTypeSupport> type_support;
};

struct ImageTypeInfos {
    VkImageTiling tiling;
    std::vector<ImageTypeFormatInfo> formats;
};

VkImageCreateInfo GetImageCreateInfo(VkFormat format, VkImageTiling tiling, VkImageCreateFlags flags, VkImageUsageFlags usages) {
    return {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            nullptr,
            flags,
            VK_IMAGE_TYPE_2D,
            format,
            {8, 8, 1},
            1,
            1,
            VK_SAMPLE_COUNT_1_BIT,
            tiling,
            usages,
            VK_SHARING_MODE_EXCLUSIVE,
            0,
            nullptr,
            VK_IMAGE_LAYOUT_UNDEFINED};
}

ImageTypeSupport FillImageTypeSupport(AppInstance &inst, VkPhysicalDevice phys_device, VkDevice device,
                                      ImageTypeSupport::Type img_type, VkImageCreateInfo image_ci) {
    VkImageFormatProperties img_props;
    VkResult res = inst.dll.fp_vkGetPhysicalDeviceImageFormatProperties(
        phys_device, image_ci.format, image_ci.imageType, image_ci.tiling, image_ci.usage, image_ci.flags, &img_props);

    if (res == VK_SUCCESS) {
        ImageTypeSupport img_type_support{};
        img_type_support.type = img_type;
        img_type_support.supported = true;

        VkImage dummy_img;
        res = inst.dll.fp_vkCreateImage(device, &image_ci, nullptr, &dummy_img);
        if (res) THROW_VK_ERR("vkCreateImage", res);

        VkMemoryRequirements mem_req;
        inst.dll.fp_vkGetImageMemoryRequirements(device, dummy_img, &mem_req);
        img_type_support.memoryTypeBits = mem_req.memoryTypeBits;

        inst.dll.fp_vkDestroyImage(device, dummy_img, nullptr);
        return img_type_support;
    } else if (res == VK_ERROR_FORMAT_NOT_SUPPORTED) {
        return {};  // default initialization has supported being false
    }
    THROW_VK_ERR("vkGetPhysicalDeviceImageFormatProperties", res);
    return {};
}

struct pNextChainInfos {
    std::vector<pNextChainBuildingBlockInfo> phys_device_props2;
    std::vector<pNextChainBuildingBlockInfo> phys_device_mem_props2;
    std::vector<pNextChainBuildingBlockInfo> phys_device_features2;
    std::vector<pNextChainBuildingBlockInfo> surface_capabilities2;
    std::vector<pNextChainBuildingBlockInfo> format_properties2;
};

struct FormatRange {
    // the Vulkan standard version that supports this format range, or 0 if non-standard
    uint32_t minimum_instance_version;

    // The name of the extension that supports this format range, or NULL if the range
    // is only part of the standard
    const char *extension_name;

    // The first and last supported formats within this range.
    VkFormat first_format;
    VkFormat last_format;
};

struct AppGpu {
    AppInstance &inst;
    uint32_t id;
    VkPhysicalDevice phys_device;
    VulkanVersion api_version;

    VkPhysicalDeviceProperties props;
    VkPhysicalDeviceProperties2KHR props2;

    uint32_t queue_count;
    std::vector<VkQueueFamilyProperties> queue_props;
    std::vector<VkQueueFamilyProperties2KHR> queue_props2;

    VkPhysicalDeviceMemoryProperties memory_props;
    VkPhysicalDeviceMemoryProperties2KHR memory_props2;

    std::vector<ImageTypeInfos> memory_image_support_types;

    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceFeatures2KHR features2;
    VkPhysicalDevice limits;

    std::vector<VkExtensionProperties> device_extensions;

    VkDevice dev;
    VkPhysicalDeviceFeatures enabled_features;

    std::array<VkDeviceSize, VK_MAX_MEMORY_HEAPS> heapBudget;
    std::array<VkDeviceSize, VK_MAX_MEMORY_HEAPS> heapUsage;

    std::vector<FormatRange> supported_format_ranges;

    AppGpu(AppInstance &inst, uint32_t id, VkPhysicalDevice phys_device, pNextChainInfos chainInfos)
        : inst(inst), id(id), phys_device(phys_device) {
        inst.dll.fp_vkGetPhysicalDeviceProperties(phys_device, &props);

        // needs to find the minimum of the instance and device version, and use that to print the device info
        uint32_t gpu_version = props.apiVersion < inst.instance_version ? props.apiVersion : inst.instance_version;
        api_version = {VK_VERSION_MAJOR(gpu_version), VK_VERSION_MINOR(gpu_version), VK_VERSION_PATCH(gpu_version)};

        if (inst.CheckExtensionEnabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
            props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
            buildpNextChain((VkStructureHeader *)&props2, chainInfos.phys_device_props2);

            inst.ext_funcs.vkGetPhysicalDeviceProperties2KHR(phys_device, &props2);
        }
        /* get queue count */
        inst.dll.fp_vkGetPhysicalDeviceQueueFamilyProperties(phys_device, &queue_count, nullptr);

        queue_props.resize(queue_count);

        inst.dll.fp_vkGetPhysicalDeviceQueueFamilyProperties(phys_device, &queue_count, queue_props.data());

        if (inst.CheckExtensionEnabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
            queue_props2.resize(queue_count);

            for (size_t i = 0; i < queue_count; ++i) {
                queue_props2[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2_KHR;
                queue_props2[i].pNext = nullptr;
            }

            inst.ext_funcs.vkGetPhysicalDeviceQueueFamilyProperties2KHR(phys_device, &queue_count, queue_props2.data());
        }

        inst.dll.fp_vkGetPhysicalDeviceMemoryProperties(phys_device, &memory_props);

        inst.dll.fp_vkGetPhysicalDeviceFeatures(phys_device, &features);

        if (inst.CheckExtensionEnabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
            memory_props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2_KHR;
            buildpNextChain((VkStructureHeader *)&memory_props2, chainInfos.phys_device_mem_props2);

            inst.ext_funcs.vkGetPhysicalDeviceMemoryProperties2KHR(phys_device, &memory_props2);

            features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
            buildpNextChain((VkStructureHeader *)&features2, chainInfos.phys_device_features2);

            inst.ext_funcs.vkGetPhysicalDeviceFeatures2KHR(phys_device, &features2);
        }

        device_extensions = AppGetPhysicalDeviceLayerExtensions(nullptr);

        const float queue_priority = 1.0f;
        const VkDeviceQueueCreateInfo q_ci = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                              nullptr,
                                              0,
                                              0,  // just pick the first one and hope for the best
                                              1,
                                              &queue_priority};
        enabled_features = VkPhysicalDeviceFeatures{0};
        const VkDeviceCreateInfo device_ci = {
            VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, nullptr, 0, 1, &q_ci, 0, nullptr, 0, nullptr, &enabled_features};

        VkResult err = inst.dll.fp_vkCreateDevice(phys_device, &device_ci, nullptr, &dev);
        if (err) THROW_VK_ERR("vkCreateDevice", err);

        const std::vector<VkImageTiling> tilings = {VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_TILING_LINEAR};
        const std::vector<VkFormat> formats = {
            color_format,      VK_FORMAT_D16_UNORM,         VK_FORMAT_X8_D24_UNORM_PACK32, VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT,   VK_FORMAT_D32_SFLOAT_S8_UINT};

        for (VkImageTiling tiling : tilings) {
            ImageTypeInfos image_type_infos;
            image_type_infos.tiling = tiling;

            for (VkFormat format : formats) {
                ImageTypeFormatInfo image_type_format_info;
                image_type_format_info.format = format;

                VkFormatProperties fmt_props;
                inst.dll.fp_vkGetPhysicalDeviceFormatProperties(phys_device, format, &fmt_props);
                if ((tiling == VK_IMAGE_TILING_OPTIMAL && fmt_props.optimalTilingFeatures == 0) ||
                    (tiling == VK_IMAGE_TILING_LINEAR && fmt_props.linearTilingFeatures == 0)) {
                    continue;
                }

                VkImageCreateInfo image_ci_regular = GetImageCreateInfo(format, tiling, 0, 0);
                VkImageCreateInfo image_ci_transient =
                    GetImageCreateInfo(format, tiling, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, 0);
                VkImageCreateInfo image_ci_sparse = GetImageCreateInfo(format, tiling, 0, VK_IMAGE_CREATE_SPARSE_BINDING_BIT);

                if (tiling == VK_IMAGE_TILING_LINEAR) {
                    if (format == color_format) {
                        image_ci_regular.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
                        image_ci_transient.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
                    } else {
                        // linear tiling is only applicable to color image types
                        continue;
                    }
                } else {
                    if (format == color_format) {
                        image_ci_regular.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                        image_ci_transient.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

                    } else {
                        image_ci_regular.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                        image_ci_transient.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                    }
                }

                auto image_ts_regular =
                    FillImageTypeSupport(inst, phys_device, dev, ImageTypeSupport::Type::regular, image_ci_regular);
                if (image_ts_regular.supported) {
                    image_type_format_info.type_support.push_back(image_ts_regular);
                }
                auto image_ts_transient =
                    FillImageTypeSupport(inst, phys_device, dev, ImageTypeSupport::Type::transient, image_ci_transient);
                if (image_ts_transient.supported) {
                    image_type_format_info.type_support.push_back(image_ts_transient);
                }

                if (enabled_features.sparseBinding) {
                    auto image_ts_sparse =
                        FillImageTypeSupport(inst, phys_device, dev, ImageTypeSupport::Type::sparse, image_ci_sparse);
                    if (image_ts_sparse.supported) {
                        image_type_format_info.type_support.push_back(image_ts_sparse);
                    }
                }
                image_type_infos.formats.push_back(image_type_format_info);
            }
            memory_image_support_types.push_back(image_type_infos);
        }

        // Memory //

        struct VkStructureHeader *structure = NULL;
        if (inst.CheckExtensionEnabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
            structure = (struct VkStructureHeader *)memory_props2.pNext;

            while (structure) {
                if (structure->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT &&
                    CheckPhysicalDeviceExtensionIncluded(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME)) {
                    VkPhysicalDeviceMemoryBudgetPropertiesEXT *mem_budget_props =
                        (VkPhysicalDeviceMemoryBudgetPropertiesEXT *)structure;
                    for (uint32_t i = 0; i < VK_MAX_MEMORY_HEAPS; i++) {
                        heapBudget[i] = mem_budget_props->heapBudget[i];
                        heapUsage[i] = mem_budget_props->heapUsage[i];
                    }
                }

                structure = (struct VkStructureHeader *)structure->pNext;
            }
        }
        // TODO buffer - memory type compatibility

        supported_format_ranges = {
            {
                // Standard formats in Vulkan 1.0
                VK_MAKE_VERSION(1, 0, 0), NULL,
                static_cast<VkFormat>(0),   // first core VkFormat
                static_cast<VkFormat>(184)  // last core VkFormat
            },
            {
                // YCBCR extension, standard in Vulkan 1.1
                VK_MAKE_VERSION(1, 1, 0),
                VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME,
                VK_FORMAT_G8B8G8R8_422_UNORM,
                VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM,
            },
            {
                // PVRTC extension, not standardized
                0,
                VK_IMG_FORMAT_PVRTC_EXTENSION_NAME,
                VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG,
                VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG,
            },
            {
                // ASTC extension, not standardized
                0,
                VK_EXT_TEXTURE_COMPRESSION_ASTC_HDR_EXTENSION_NAME,
                VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT,
                VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT,
            },
        };
    }
    ~AppGpu() {
        inst.dll.fp_vkDestroyDevice(dev, nullptr);

        if (inst.CheckExtensionEnabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
            freepNextChain(static_cast<VkStructureHeader *>(features2.pNext));
            freepNextChain(static_cast<VkStructureHeader *>(props2.pNext));
            freepNextChain(static_cast<VkStructureHeader *>(memory_props2.pNext));
        }
    }

    AppGpu(const AppGpu &) = delete;
    const AppGpu &operator=(const AppGpu &) = delete;

    bool CheckPhysicalDeviceExtensionIncluded(std::string extension_to_check) {
        for (auto &extension : device_extensions) {
            if (extension_to_check == std::string(extension.extensionName)) {
                return true;
            }
        }
        return false;
    }

    std::vector<VkExtensionProperties> AppGetPhysicalDeviceLayerExtensions(char *layer_name) {
        return GetVector<VkExtensionProperties>("vkEnumerateDeviceExtensionProperties",
                                                inst.dll.fp_vkEnumerateDeviceExtensionProperties, phys_device, layer_name);
    }

    // Helper function to determine whether a format range is currently supported.
    bool FormatRangeSupported(FormatRange &format_range) {
        // True if standard and supported by both this instance and this GPU
        if (format_range.minimum_instance_version > 0 && inst.instance_version >= format_range.minimum_instance_version &&
            props.apiVersion >= format_range.minimum_instance_version) {
            return true;
        }

        // True if this extension is present
        if (format_range.extension_name != nullptr) {
            return inst.CheckExtensionEnabled(format_range.extension_name);
        }

        // Otherwise, not supported.
        return false;
    }

    VkPhysicalDeviceProperties GetDeviceProperties() {
        if (inst.CheckExtensionEnabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
            return props2.properties;
        } else {
            return props;
        }
    }
};
struct AppQueueFamilyProperties {
    VkQueueFamilyProperties props;
    uint32_t queue_index;
    bool is_present_platform_agnostic = true;
    VkBool32 platforms_support_present = VK_FALSE;

    AppQueueFamilyProperties(AppGpu &gpu, uint32_t queue_index) : queue_index(queue_index) {
        if (gpu.inst.CheckExtensionEnabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
            props = gpu.queue_props2[queue_index].queueFamilyProperties;
        } else {
            props = gpu.queue_props[queue_index];
        }

        for (auto &surface_ext : gpu.inst.surface_extensions) {
            VkResult err = gpu.inst.ext_funcs.vkGetPhysicalDeviceSurfaceSupportKHR(
                gpu.phys_device, queue_index, surface_ext.surface, &surface_ext.supports_present);
            if (err) THROW_VK_ERR("vkGetPhysicalDeviceSurfaceSupportKHR", err);

            const bool first = (surface_ext == gpu.inst.surface_extensions.at(0));
            if (!first && platforms_support_present != surface_ext.supports_present) {
                is_present_platform_agnostic = false;
            }
            platforms_support_present = surface_ext.supports_present;
        }
    }
};

std::vector<VkPhysicalDeviceToolPropertiesEXT> GetToolingInfo(AppGpu &gpu) {
    if (gpu.inst.ext_funcs.vkGetPhysicalDeviceToolPropertiesEXT == nullptr) return {};
    return GetVector<VkPhysicalDeviceToolPropertiesEXT>("vkGetPhysicalDeviceToolPropertiesEXT",
                                                        gpu.inst.ext_funcs.vkGetPhysicalDeviceToolPropertiesEXT, gpu.phys_device);
}

// --------- Format Properties ----------//

struct PropFlags {
    uint32_t linear;
    uint32_t optimal;
    uint32_t buffer;

    bool operator==(const PropFlags &other) const {
        return (linear == other.linear && optimal == other.optimal && buffer == other.buffer);
    }
};

namespace std {
template <>
struct hash<PropFlags> {
    std::size_t operator()(const PropFlags &k) const {
        return ((std::hash<uint32_t>()(k.linear) ^ (std::hash<uint32_t>()(k.optimal) << 1)) >> 1) ^
               (std::hash<uint32_t>()(k.buffer) << 1);
    }
};
}  // namespace std

// Used to sort the formats into buckets by their properties.
std::unordered_map<PropFlags, std::vector<VkFormat>> FormatPropMap(AppGpu &gpu) {
    std::unordered_map<PropFlags, std::vector<VkFormat>> map;
    for (auto fmtRange : gpu.supported_format_ranges) {
        for (int32_t fmt = fmtRange.first_format; fmt <= fmtRange.last_format; ++fmt) {
            VkFormatProperties props;
            gpu.inst.dll.fp_vkGetPhysicalDeviceFormatProperties(gpu.phys_device, static_cast<VkFormat>(fmt), &props);

            PropFlags pf = {props.linearTilingFeatures, props.optimalTilingFeatures, props.bufferFeatures};

            map[pf].push_back(static_cast<VkFormat>(fmt));
        }
    }
    return map;
}

VkFormatProperties2 GetFormatProperties2(AppGpu &gpu, VkFormat format, pNextChainInfos &chainInfos) {
    VkFormatProperties2 props;
    props.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
    buildpNextChain((VkStructureHeader *)&props, chainInfos.format_properties2);
    gpu.inst.ext_funcs.vkGetPhysicalDeviceFormatProperties2KHR(gpu.phys_device, format, &props);
    return props;
}
