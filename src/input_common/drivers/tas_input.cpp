// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <cstring>
#include <regex>
#include <fmt/format.h>

#include "common/fs/file.h"
#include "common/fs/fs_types.h"
#include "common/fs/path_util.h"
#include "common/logging/log.h"
#include "common/settings.h"
#include "input_common/drivers/tas_input.h"

namespace InputCommon::TasInput {

enum TasAxes : u8 {
    StickX,
    StickY,
    SubstickX,
    SubstickY,
    Undefined,
};

// Supported keywords and buttons from a TAS file
constexpr std::array<std::pair<std::string_view, TasButton>, 20> text_to_tas_button = {
    std::pair{"KEY_A", TasButton::BUTTON_A},
    {"KEY_B", TasButton::BUTTON_B},
    {"KEY_X", TasButton::BUTTON_X},
    {"KEY_Y", TasButton::BUTTON_Y},
    {"KEY_LSTICK", TasButton::STICK_L},
    {"KEY_RSTICK", TasButton::STICK_R},
    {"KEY_L", TasButton::TRIGGER_L},
    {"KEY_R", TasButton::TRIGGER_R},
    {"KEY_PLUS", TasButton::BUTTON_PLUS},
    {"KEY_MINUS", TasButton::BUTTON_MINUS},
    {"KEY_DLEFT", TasButton::BUTTON_LEFT},
    {"KEY_DUP", TasButton::BUTTON_UP},
    {"KEY_DRIGHT", TasButton::BUTTON_RIGHT},
    {"KEY_DDOWN", TasButton::BUTTON_DOWN},
    {"KEY_SL", TasButton::BUTTON_SL},
    {"KEY_SR", TasButton::BUTTON_SR},
    {"KEY_CAPTURE", TasButton::BUTTON_CAPTURE},
    {"KEY_HOME", TasButton::BUTTON_HOME},
    {"KEY_ZL", TasButton::TRIGGER_ZL},
    {"KEY_ZR", TasButton::TRIGGER_ZR},
};

Tas::Tas(const std::string& input_engine_) : InputCommon::InputEngine(input_engine_) {
    for (size_t player_index = 0; player_index < PLAYER_NUMBER; player_index++) {
        PadIdentifier identifier{
            .guid = Common::UUID{},
            .port = player_index,
            .pad = 0,
        };
        PreSetController(identifier);
    }
    ClearInput();
    if (!Settings::values.tas_enable) {
        needs_reset = true;
        return;
    }
    LoadTasFiles();
}

Tas::~Tas() {
    Stop();
};

void Tas::LoadTasFiles() {
    script_length = 0;
    for (size_t i = 0; i < commands.size(); i++) {
        LoadTasFile(i, 0);
        if (commands[i].size() > script_length) {
            script_length = commands[i].size();
        }
    }
}

void Tas::LoadTasFile(size_t player_index, size_t file_index) {
    if (!commands[player_index].empty()) {
        commands[player_index].clear();
    }
    std::string file = Common::FS::ReadStringFromFile(
        Common::FS::GetYuzuPath(Common::FS::YuzuPath::TASDir) /
            fmt::format("script{}-{}.txt", file_index, player_index + 1),
        Common::FS::FileType::BinaryFile);
    std::stringstream command_line(file);
    std::string line;
    int frame_no = 0;
    while (std::getline(command_line, line, '\n')) {
        if (line.empty()) {
            continue;
        }
        std::smatch m;

        std::stringstream linestream(line);
        std::string segment;
        std::vector<std::string> seglist;

        while (std::getline(linestream, segment, ' ')) {
            seglist.push_back(segment);
        }

        if (seglist.size() < 4) {
            continue;
        }

        while (frame_no < std::stoi(seglist.at(0))) {
            commands[player_index].push_back({});
            frame_no++;
        }

        TASCommand command = {
            .buttons = ReadCommandButtons(seglist.at(1)),
            .l_axis = ReadCommandAxis(seglist.at(2)),
            .r_axis = ReadCommandAxis(seglist.at(3)),
        };
        commands[player_index].push_back(command);
        frame_no++;
    }
    LOG_INFO(Input, "TAS file loaded! {} frames", frame_no);
}

void Tas::WriteTasFile(std::u8string file_name) {
    std::string output_text;
    for (size_t frame = 0; frame < record_commands.size(); frame++) {
        const TASCommand& line = record_commands[frame];
        output_text += fmt::format("{} {} {} {}\n", frame, WriteCommandButtons(line.buttons),
                                   WriteCommandAxis(line.l_axis), WriteCommandAxis(line.r_axis));
    }
    const auto bytes_written = Common::FS::WriteStringToFile(
        Common::FS::GetYuzuPath(Common::FS::YuzuPath::TASDir) / file_name,
        Common::FS::FileType::TextFile, output_text);
    if (bytes_written == output_text.size()) {
        LOG_INFO(Input, "TAS file written to file!");
    } else {
        LOG_ERROR(Input, "Writing the TAS-file has failed! {} / {} bytes written", bytes_written,
                  output_text.size());
    }
}

void Tas::RecordInput(u64 buttons, TasAnalog left_axis, TasAnalog right_axis) {
    last_input = {
        .buttons = buttons,
        .l_axis = left_axis,
        .r_axis = right_axis,
    };
}

std::tuple<TasState, size_t, size_t> Tas::GetStatus() const {
    TasState state;
    if (is_recording) {
        return {TasState::Recording, 0, record_commands.size()};
    }

    if (is_running) {
        state = TasState::Running;
    } else {
        state = TasState::Stopped;
    }

    return {state, current_command, script_length};
}

void Tas::UpdateThread() {
    if (!Settings::values.tas_enable) {
        if (is_running) {
            Stop();
        }
        return;
    }

    if (is_recording) {
        record_commands.push_back(last_input);
    }
    if (needs_reset) {
        current_command = 0;
        needs_reset = false;
        LoadTasFiles();
        LOG_DEBUG(Input, "tas_reset done");
    }

    if (!is_running) {
        ClearInput();
        return;
    }
    if (current_command < script_length) {
        LOG_DEBUG(Input, "Playing TAS {}/{}", current_command, script_length);
        const size_t frame = current_command++;
        for (size_t player_index = 0; player_index < commands.size(); player_index++) {
            TASCommand command{};
            if (frame < commands[player_index].size()) {
                command = commands[player_index][frame];
            }

            PadIdentifier identifier{
                .guid = Common::UUID{},
                .port = player_index,
                .pad = 0,
            };
            for (std::size_t i = 0; i < sizeof(command.buttons) * 8; ++i) {
                const bool button_status = (command.buttons & (1LLU << i)) != 0;
                const int button = static_cast<int>(i);
                SetButton(identifier, button, button_status);
            }
            SetAxis(identifier, TasAxes::StickX, command.l_axis.x);
            SetAxis(identifier, TasAxes::StickY, command.l_axis.y);
            SetAxis(identifier, TasAxes::SubstickX, command.r_axis.x);
            SetAxis(identifier, TasAxes::SubstickY, command.r_axis.y);
        }
    } else {
        is_running = Settings::values.tas_loop.GetValue();
        LoadTasFiles();
        current_command = 0;
        ClearInput();
    }
}

void Tas::ClearInput() {
    ResetButtonState();
    ResetAnalogState();
}

TasAnalog Tas::ReadCommandAxis(const std::string& line) const {
    std::stringstream linestream(line);
    std::string segment;
    std::vector<std::string> seglist;

    while (std::getline(linestream, segment, ';')) {
        seglist.push_back(segment);
    }

    const float x = std::stof(seglist.at(0)) / 32767.0f;
    const float y = std::stof(seglist.at(1)) / 32767.0f;

    return {x, y};
}

u64 Tas::ReadCommandButtons(const std::string& data) const {
    std::stringstream button_text(data);
    std::string line;
    u64 buttons = 0;
    while (std::getline(button_text, line, ';')) {
        for (auto [text, tas_button] : text_to_tas_button) {
            if (text == line) {
                buttons |= static_cast<u64>(tas_button);
                break;
            }
        }
    }
    return buttons;
}

std::string Tas::WriteCommandButtons(u64 buttons) const {
    std::string returns = "";
    for (auto [text_button, tas_button] : text_to_tas_button) {
        if ((buttons & static_cast<u64>(tas_button)) != 0) {
            returns += fmt::format("{};", text_button);
        }
    }
    return returns.empty() ? "NONE" : returns;
}

std::string Tas::WriteCommandAxis(TasAnalog analog) const {
    return fmt::format("{};{}", analog.x * 32767, analog.y * 32767);
}

void Tas::StartStop() {
    if (!Settings::values.tas_enable) {
        return;
    }
    if (is_running) {
        Stop();
    } else {
        is_running = true;
    }
}

void Tas::Stop() {
    is_running = false;
}

void Tas::Reset() {
    if (!Settings::values.tas_enable) {
        return;
    }
    needs_reset = true;
}

bool Tas::Record() {
    if (!Settings::values.tas_enable) {
        return true;
    }
    is_recording = !is_recording;
    return is_recording;
}

void Tas::SaveRecording(bool overwrite_file) {
    if (is_recording) {
        return;
    }
    if (record_commands.empty()) {
        return;
    }
    WriteTasFile(u8"record.txt");
    if (overwrite_file) {
        WriteTasFile(u8"script0-1.txt");
    }
    needs_reset = true;
    record_commands.clear();
}

} // namespace InputCommon::TasInput
