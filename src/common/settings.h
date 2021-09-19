// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "common/common_types.h"
#include "common/settings_input.h"

namespace Settings {

enum class RendererBackend : u32 {
    OpenGL = 0,
    Vulkan = 1,
};

enum class ShaderBackend : u32 {
    GLSL = 0,
    GLASM = 1,
    SPIRV = 2,
};

enum class GPUAccuracy : u32 {
    Normal = 0,
    High = 1,
    Extreme = 2,
};

enum class CPUAccuracy : u32 {
    Auto = 0,
    Accurate = 1,
    Unsafe = 2,
};

enum class FullscreenMode : u32 {
    Borderless = 0,
    Exclusive = 1,
};

enum class NvdecEmulation : u32 {
    Off = 0,
    CPU = 1,
    GPU = 2,
};

/** The BasicSetting class is a simple resource manager. It defines a label and default value
 * alongside the actual value of the setting for simpler and less-error prone use with frontend
 * configurations. Setting a default value and label is required, though subclasses may deviate from
 * this requirement.
 */
template <typename Type>
class BasicSetting {
protected:
    BasicSetting() = default;

    /**
     * Only sets the setting to the given initializer, leaving the other members to their default
     * initializers.
     *
     * @param global_val Initial value of the setting
     */
    explicit BasicSetting(const Type& global_val) : global{global_val} {}

public:
    /**
     * Sets a default value, label, and setting value.
     *
     * @param default_val Intial value of the setting, and default value of the setting
     * @param name Label for the setting
     */
    explicit BasicSetting(const Type& default_val, const std::string& name)
        : default_value{default_val}, global{default_val}, label{name} {}
    virtual ~BasicSetting() = default;

    /**
     *  Returns a reference to the setting's value.
     *
     * @returns A reference to the setting
     */
    [[nodiscard]] virtual const Type& GetValue() const {
        return global;
    }

    /**
     * Sets the setting to the given value.
     *
     * @param value The desired value
     */
    virtual void SetValue(const Type& value) {
        Type temp{value};
        std::swap(global, temp);
    }

    /**
     * Returns the value that this setting was created with.
     *
     * @returns A reference to the default value
     */
    [[nodiscard]] const Type& GetDefault() const {
        return default_value;
    }

    /**
     * Returns the label this setting was created with.
     *
     * @returns A reference to the label
     */
    [[nodiscard]] const std::string& GetLabel() const {
        return label;
    }

    /**
     * Assigns a value to the setting.
     *
     * @param value The desired setting value
     *
     * @returns A reference to the setting
     */
    virtual const Type& operator=(const Type& value) {
        Type temp{value};
        std::swap(global, temp);
        return global;
    }

    /**
     * Returns a reference to the setting.
     *
     * @returns A reference to the setting
     */
    explicit virtual operator const Type&() const {
        return global;
    }

protected:
    const Type default_value{}; ///< The default value
    Type global{};              ///< The setting
    const std::string label{};  ///< The setting's label
};

/**
 * BasicRangedSetting class is intended for use with quantifiable settings that need a more
 * restrictive range than implicitly defined by its type. Implements a minimum and maximum that is
 * simply used to sanitize SetValue and the assignment overload.
 */
template <typename Type>
class BasicRangedSetting : virtual public BasicSetting<Type> {
public:
    /**
     * Sets a default value, minimum value, maximum value, and label.
     *
     * @param default_val Intial value of the setting, and default value of the setting
     * @param min_val Sets the minimum allowed value of the setting
     * @param max_val Sets the maximum allowed value of the setting
     * @param name Label for the setting
     */
    explicit BasicRangedSetting(const Type& default_val, const Type& min_val, const Type& max_val,
                                const std::string& name)
        : BasicSetting<Type>{default_val, name}, minimum{min_val}, maximum{max_val} {}
    virtual ~BasicRangedSetting() = default;

    /**
     * Like BasicSetting's SetValue, except value is clamped to the range of the setting.
     *
     * @param value The desired value
     */
    void SetValue(const Type& value) override {
        this->global = std::clamp(value, minimum, maximum);
    }

    /**
     * Like BasicSetting's assignment overload, except value is clamped to the range of the setting.
     *
     * @param value The desired value
     * @returns A reference to the setting's value
     */
    const Type& operator=(const Type& value) override {
        this->global = std::clamp(value, minimum, maximum);
        return this->global;
    }

    const Type minimum; ///< Minimum allowed value of the setting
    const Type maximum; ///< Maximum allowed value of the setting
};

/**
 * The Setting class is a slightly more complex version of the BasicSetting class. This adds a
 * custom setting to switch to when a guest application specifically requires it. The effect is that
 * other components of the emulator can access the setting's intended value without any need for the
 * component to ask whether the custom or global setting is needed at the moment.
 *
 * By default, the global setting is used.
 *
 * Like the BasicSetting, this requires setting a default value and label to use.
 */
template <typename Type>
class Setting : virtual public BasicSetting<Type> {
public:
    /**
     * Sets a default value, label, and setting value.
     *
     * @param default_val Intial value of the setting, and default value of the setting
     * @param name Label for the setting
     */
    explicit Setting(const Type& default_val, const std::string& name)
        : BasicSetting<Type>(default_val, name) {}
    virtual ~Setting() = default;

    /**
     * Tells this setting to represent either the global or custom setting when other member
     * functions are used.
     *
     * @param to_global Whether to use the global or custom setting.
     */
    void SetGlobal(bool to_global) {
        use_global = to_global;
    }

    /**
     * Returns whether this setting is using the global setting or not.
     *
     * @returns The global state
     */
    [[nodiscard]] bool UsingGlobal() const {
        return use_global;
    }

    /**
     * Returns either the global or custom setting depending on the values of this setting's global
     * state or if the global value was specifically requested.
     *
     * @param need_global Request global value regardless of setting's state; defaults to false
     *
     * @returns The required value of the setting
     */
    [[nodiscard]] virtual const Type& GetValue() const override {
        if (use_global) {
            return this->global;
        }
        return custom;
    }
    [[nodiscard]] virtual const Type& GetValue(bool need_global) const {
        if (use_global || need_global) {
            return this->global;
        }
        return custom;
    }

    /**
     * Sets the current setting value depending on the global state.
     *
     * @param value The new value
     */
    void SetValue(const Type& value) override {
        Type temp{value};
        if (use_global) {
            std::swap(this->global, temp);
        } else {
            std::swap(custom, temp);
        }
    }

    /**
     * Assigns the current setting value depending on the global state.
     *
     * @param value The new value
     *
     * @returns A reference to the current setting value
     */
    const Type& operator=(const Type& value) override {
        Type temp{value};
        if (use_global) {
            std::swap(this->global, temp);
            return this->global;
        }
        std::swap(custom, temp);
        return custom;
    }

    /**
     * Returns the current setting value depending on the global state.
     *
     * @returns A reference to the current setting value
     */
    virtual explicit operator const Type&() const override {
        if (use_global) {
            return this->global;
        }
        return custom;
    }

protected:
    bool use_global{true}; ///< The setting's global state
    Type custom{};         ///< The custom value of the setting
};

/**
 * RangedSetting is a Setting that implements a maximum and minimum value for its setting. Intended
 * for use with quantifiable settings.
 */
template <typename Type>
class RangedSetting final : public BasicRangedSetting<Type>, public Setting<Type> {
public:
    /**
     * Sets a default value, minimum value, maximum value, and label.
     *
     * @param default_val Intial value of the setting, and default value of the setting
     * @param min_val Sets the minimum allowed value of the setting
     * @param max_val Sets the maximum allowed value of the setting
     * @param name Label for the setting
     */
    explicit RangedSetting(const Type& default_val, const Type& min_val, const Type& max_val,
                           const std::string& name)
        : BasicSetting<Type>{default_val, name},
          BasicRangedSetting<Type>{default_val, min_val, max_val, name}, Setting<Type>{default_val,
                                                                                       name} {}
    virtual ~RangedSetting() = default;

    // The following are needed to avoid a MSVC bug
    // (source: https://stackoverflow.com/questions/469508)
    [[nodiscard]] const Type& GetValue() const override {
        return Setting<Type>::GetValue();
    }
    [[nodiscard]] const Type& GetValue(bool need_global) const override {
        return Setting<Type>::GetValue(need_global);
    }
    explicit operator const Type&() const override {
        if (this->use_global) {
            return this->global;
        }
        return this->custom;
    }

    /**
     * Like BasicSetting's SetValue, except value is clamped to the range of the setting. Sets the
     * appropriate value depending on the global state.
     *
     * @param value The desired value
     */
    void SetValue(const Type& value) override {
        const Type temp = std::clamp(value, this->minimum, this->maximum);
        if (this->use_global) {
            this->global = temp;
        }
        this->custom = temp;
    }

    /**
     * Like BasicSetting's assignment overload, except value is clamped to the range of the setting.
     * Uses the appropriate value depending on the global state.
     *
     * @param value The desired value
     * @returns A reference to the setting's value
     */
    const Type& operator=(const Type& value) override {
        const Type temp = std::clamp(value, this->minimum, this->maximum);
        if (this->use_global) {
            this->global = temp;
            return this->global;
        }
        this->custom = temp;
        return this->custom;
    }
};

/**
 * The InputSetting class allows for getting a reference to either the global or custom members.
 * This is required as we cannot easily modify the values of user-defined types within containers
 * using the SetValue() member function found in the Setting class. The primary purpose of this
 * class is to store an array of 10 PlayerInput structs for both the global and custom setting and
 * allows for easily accessing and modifying both settings.
 */
template <typename Type>
class InputSetting final {
public:
    InputSetting() = default;
    explicit InputSetting(Type val) : BasicSetting<Type>(val) {}
    ~InputSetting() = default;
    void SetGlobal(bool to_global) {
        use_global = to_global;
    }
    [[nodiscard]] bool UsingGlobal() const {
        return use_global;
    }
    [[nodiscard]] Type& GetValue(bool need_global = false) {
        if (use_global || need_global) {
            return global;
        }
        return custom;
    }

private:
    bool use_global{true}; ///< The setting's global state
    Type global{};         ///< The setting
    Type custom{};         ///< The custom setting value
};

struct TouchFromButtonMap {
    std::string name;
    std::vector<std::string> buttons;
};

struct Values {
    // Audio
    BasicSetting<std::string> audio_device_id{"auto", "output_device"};
    BasicSetting<std::string> sink_id{"auto", "output_engine"};
    BasicSetting<bool> audio_muted{false, "audio_muted"};
    RangedSetting<u8> volume{100, 0, 100, "volume"};

    // Core
    Setting<bool> use_multi_core{true, "use_multi_core"};

    // Cpu
    RangedSetting<CPUAccuracy> cpu_accuracy{CPUAccuracy::Auto, CPUAccuracy::Auto,
                                            CPUAccuracy::Unsafe, "cpu_accuracy"};
    // TODO: remove cpu_accuracy_first_time, migration setting added 8 July 2021
    BasicSetting<bool> cpu_accuracy_first_time{true, "cpu_accuracy_first_time"};
    BasicSetting<bool> cpu_debug_mode{false, "cpu_debug_mode"};

    BasicSetting<bool> cpuopt_page_tables{true, "cpuopt_page_tables"};
    BasicSetting<bool> cpuopt_block_linking{true, "cpuopt_block_linking"};
    BasicSetting<bool> cpuopt_return_stack_buffer{true, "cpuopt_return_stack_buffer"};
    BasicSetting<bool> cpuopt_fast_dispatcher{true, "cpuopt_fast_dispatcher"};
    BasicSetting<bool> cpuopt_context_elimination{true, "cpuopt_context_elimination"};
    BasicSetting<bool> cpuopt_const_prop{true, "cpuopt_const_prop"};
    BasicSetting<bool> cpuopt_misc_ir{true, "cpuopt_misc_ir"};
    BasicSetting<bool> cpuopt_reduce_misalign_checks{true, "cpuopt_reduce_misalign_checks"};
    BasicSetting<bool> cpuopt_fastmem{true, "cpuopt_fastmem"};

    Setting<bool> cpuopt_unsafe_unfuse_fma{true, "cpuopt_unsafe_unfuse_fma"};
    Setting<bool> cpuopt_unsafe_reduce_fp_error{true, "cpuopt_unsafe_reduce_fp_error"};
    Setting<bool> cpuopt_unsafe_ignore_standard_fpcr{true, "cpuopt_unsafe_ignore_standard_fpcr"};
    Setting<bool> cpuopt_unsafe_inaccurate_nan{true, "cpuopt_unsafe_inaccurate_nan"};
    Setting<bool> cpuopt_unsafe_fastmem_check{true, "cpuopt_unsafe_fastmem_check"};

    // Renderer
    RangedSetting<RendererBackend> renderer_backend{
        RendererBackend::OpenGL, RendererBackend::OpenGL, RendererBackend::Vulkan, "backend"};
    BasicSetting<bool> renderer_debug{false, "debug"};
    BasicSetting<bool> renderer_shader_feedback{false, "shader_feedback"};
    BasicSetting<bool> enable_nsight_aftermath{false, "nsight_aftermath"};
    BasicSetting<bool> disable_shader_loop_safety_checks{false,
                                                         "disable_shader_loop_safety_checks"};
    Setting<int> vulkan_device{0, "vulkan_device"};

    Setting<u16> resolution_factor{1, "resolution_factor"};
    // *nix platforms may have issues with the borderless windowed fullscreen mode.
    // Default to exclusive fullscreen on these platforms for now.
    RangedSetting<FullscreenMode> fullscreen_mode{
#ifdef _WIN32
        FullscreenMode::Borderless,
#else
        FullscreenMode::Exclusive,
#endif
        FullscreenMode::Borderless, FullscreenMode::Exclusive, "fullscreen_mode"};
    RangedSetting<int> aspect_ratio{0, 0, 3, "aspect_ratio"};
    RangedSetting<int> max_anisotropy{0, 0, 4, "max_anisotropy"};
    Setting<bool> use_speed_limit{true, "use_speed_limit"};
    RangedSetting<u16> speed_limit{100, 0, 9999, "speed_limit"};
    Setting<bool> use_disk_shader_cache{true, "use_disk_shader_cache"};
    RangedSetting<GPUAccuracy> gpu_accuracy{GPUAccuracy::High, GPUAccuracy::Normal,
                                            GPUAccuracy::Extreme, "gpu_accuracy"};
    Setting<bool> use_asynchronous_gpu_emulation{true, "use_asynchronous_gpu_emulation"};
    Setting<NvdecEmulation> nvdec_emulation{NvdecEmulation::GPU, "nvdec_emulation"};
    Setting<bool> accelerate_astc{true, "accelerate_astc"};
    Setting<bool> use_vsync{true, "use_vsync"};
    BasicRangedSetting<u16> fps_cap{1000, 1, 1000, "fps_cap"};
    BasicSetting<bool> disable_fps_limit{false, "disable_fps_limit"};
    RangedSetting<ShaderBackend> shader_backend{ShaderBackend::GLASM, ShaderBackend::GLSL,
                                                ShaderBackend::SPIRV, "shader_backend"};
    Setting<bool> use_asynchronous_shaders{false, "use_asynchronous_shaders"};
    Setting<bool> use_fast_gpu_time{true, "use_fast_gpu_time"};

    Setting<u8> bg_red{0, "bg_red"};
    Setting<u8> bg_green{0, "bg_green"};
    Setting<u8> bg_blue{0, "bg_blue"};

    // System
    Setting<std::optional<u32>> rng_seed{std::optional<u32>(), "rng_seed"};
    // Measured in seconds since epoch
    std::optional<std::chrono::seconds> custom_rtc;
    // Set on game boot, reset on stop. Seconds difference between current time and `custom_rtc`
    std::chrono::seconds custom_rtc_differential;

    BasicSetting<s32> current_user{0, "current_user"};
    RangedSetting<s32> language_index{1, 0, 17, "language_index"};
    RangedSetting<s32> region_index{1, 0, 6, "region_index"};
    RangedSetting<s32> time_zone_index{0, 0, 45, "time_zone_index"};
    RangedSetting<s32> sound_index{1, 0, 2, "sound_index"};

    // Controls
    InputSetting<std::array<PlayerInput, 10>> players;

    Setting<bool> use_docked_mode{true, "use_docked_mode"};

    BasicSetting<bool> enable_raw_input{false, "enable_raw_input"};

    Setting<bool> vibration_enabled{true, "vibration_enabled"};
    Setting<bool> enable_accurate_vibrations{false, "enable_accurate_vibrations"};

    Setting<bool> motion_enabled{true, "motion_enabled"};
    BasicSetting<std::string> motion_device{"engine:motion_emu,update_period:100,sensitivity:0.01",
                                            "motion_device"};
    BasicSetting<std::string> udp_input_servers{"127.0.0.1:26760", "udp_input_servers"};

    BasicSetting<bool> pause_tas_on_load{true, "pause_tas_on_load"};
    BasicSetting<bool> tas_enable{false, "tas_enable"};
    BasicSetting<bool> tas_loop{false, "tas_loop"};
    BasicSetting<bool> tas_swap_controllers{true, "tas_swap_controllers"};

    BasicSetting<bool> mouse_panning{false, "mouse_panning"};
    BasicRangedSetting<u8> mouse_panning_sensitivity{10, 1, 100, "mouse_panning_sensitivity"};
    BasicSetting<bool> mouse_enabled{false, "mouse_enabled"};
    std::string mouse_device;
    MouseButtonsRaw mouse_buttons;

    BasicSetting<bool> emulate_analog_keyboard{false, "emulate_analog_keyboard"};
    BasicSetting<bool> keyboard_enabled{false, "keyboard_enabled"};
    KeyboardKeysRaw keyboard_keys;
    KeyboardModsRaw keyboard_mods;

    BasicSetting<bool> debug_pad_enabled{false, "debug_pad_enabled"};
    ButtonsRaw debug_pad_buttons;
    AnalogsRaw debug_pad_analogs;

    TouchscreenInput touchscreen;

    BasicSetting<bool> use_touch_from_button{false, "use_touch_from_button"};
    BasicSetting<std::string> touch_device{"min_x:100,min_y:50,max_x:1800,max_y:850",
                                           "touch_device"};
    BasicSetting<int> touch_from_button_map_index{0, "touch_from_button_map"};
    std::vector<TouchFromButtonMap> touch_from_button_maps;

    std::atomic_bool is_device_reload_pending{true};

    // Data Storage
    BasicSetting<bool> use_virtual_sd{true, "use_virtual_sd"};
    BasicSetting<bool> gamecard_inserted{false, "gamecard_inserted"};
    BasicSetting<bool> gamecard_current_game{false, "gamecard_current_game"};
    BasicSetting<std::string> gamecard_path{std::string(), "gamecard_path"};

    // Debugging
    bool record_frame_times;
    BasicSetting<bool> use_gdbstub{false, "use_gdbstub"};
    BasicSetting<u16> gdbstub_port{0, "gdbstub_port"};
    BasicSetting<std::string> program_args{std::string(), "program_args"};
    BasicSetting<bool> dump_exefs{false, "dump_exefs"};
    BasicSetting<bool> dump_nso{false, "dump_nso"};
    BasicSetting<bool> enable_fs_access_log{false, "enable_fs_access_log"};
    BasicSetting<bool> reporting_services{false, "reporting_services"};
    BasicSetting<bool> quest_flag{false, "quest_flag"};
    BasicSetting<bool> disable_macro_jit{false, "disable_macro_jit"};
    BasicSetting<bool> extended_logging{false, "extended_logging"};
    BasicSetting<bool> use_debug_asserts{false, "use_debug_asserts"};
    BasicSetting<bool> use_auto_stub{false, "use_auto_stub"};

    // Miscellaneous
    BasicSetting<std::string> log_filter{"*:Info", "log_filter"};
    BasicSetting<bool> use_dev_keys{false, "use_dev_keys"};

    // Network
    BasicSetting<std::string> bcat_backend{"none", "bcat_backend"};
    BasicSetting<bool> bcat_boxcat_local{false, "bcat_boxcat_local"};
    BasicSetting<std::string> network_interface{std::string(), "network_interface"};

    // WebService
    BasicSetting<bool> enable_telemetry{true, "enable_telemetry"};
    BasicSetting<std::string> web_api_url{"https://api.yuzu-emu.org", "web_api_url"};
    BasicSetting<std::string> yuzu_username{std::string(), "yuzu_username"};
    BasicSetting<std::string> yuzu_token{std::string(), "yuzu_token"};

    // Add-Ons
    std::map<u64, std::vector<std::string>> disabled_addons;
};

extern Values values;

bool IsConfiguringGlobal();
void SetConfiguringGlobal(bool is_global);

bool IsGPULevelExtreme();
bool IsGPULevelHigh();

bool IsFastmemEnabled();

float Volume();

std::string GetTimeZoneString();

void LogSettings();

// Restore the global state of all applicable settings in the Values struct
void RestoreGlobalState(bool is_powered_on);

} // namespace Settings
