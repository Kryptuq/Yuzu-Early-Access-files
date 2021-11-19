// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>

#include "common/common_types.h"
#include "common/settings_input.h"
#include "input_common/input_engine.h"
#include "input_common/main.h"

/*
To play back TAS scripts on Yuzu, select the folder with scripts in the configuration menu below
Tools -> Configure TAS. The file itself has normal text format and has to be called script0-1.txt
for controller 1, script0-2.txt for controller 2 and so forth (with max. 8 players).

A script file has the same format as TAS-nx uses, so final files will look like this:

1 KEY_B 0;0 0;0
6 KEY_ZL 0;0 0;0
41 KEY_ZL;KEY_Y 0;0 0;0
43 KEY_X;KEY_A 32767;0 0;0
44 KEY_A 32767;0 0;0
45 KEY_A 32767;0 0;0
46 KEY_A 32767;0 0;0
47 KEY_A 32767;0 0;0

After placing the file at the correct location, it can be read into Yuzu with the (default) hotkey
CTRL+F6 (refresh). In the bottom left corner, it will display the amount of frames the script file
has. Playback can be started or stopped using CTRL+F5.

However, for playback to actually work, the correct input device has to be selected: In the Controls
menu, select TAS from the device list for the controller that the script should be played on.

Recording a new script file is really simple: Just make sure that the proper device (not TAS) is
connected on P1, and press CTRL+F7 to start recording. When done, just press the same keystroke
again (CTRL+F7). The new script will be saved at the location previously selected, as the filename
record.txt.

For debugging purposes, the common controller debugger can be used (View -> Debugging -> Controller
P1).
*/

namespace InputCommon::TasInput {

constexpr size_t PLAYER_NUMBER = 10;

enum class TasButton : u64 {
    BUTTON_A = 1U << 0,
    BUTTON_B = 1U << 1,
    BUTTON_X = 1U << 2,
    BUTTON_Y = 1U << 3,
    STICK_L = 1U << 4,
    STICK_R = 1U << 5,
    TRIGGER_L = 1U << 6,
    TRIGGER_R = 1U << 7,
    TRIGGER_ZL = 1U << 8,
    TRIGGER_ZR = 1U << 9,
    BUTTON_PLUS = 1U << 10,
    BUTTON_MINUS = 1U << 11,
    BUTTON_LEFT = 1U << 12,
    BUTTON_UP = 1U << 13,
    BUTTON_RIGHT = 1U << 14,
    BUTTON_DOWN = 1U << 15,
    BUTTON_SL = 1U << 16,
    BUTTON_SR = 1U << 17,
    BUTTON_HOME = 1U << 18,
    BUTTON_CAPTURE = 1U << 19,
};

struct TasAnalog {
    float x{};
    float y{};
};

enum class TasState {
    Running,
    Recording,
    Stopped,
};

class Tas final : public InputCommon::InputEngine {
public:
    explicit Tas(const std::string& input_engine_);
    ~Tas();

    /**
     * Changes the input status that will be stored in each frame
     * @param buttons: bitfield with the status of the buttons
     * @param left_axis: value of the left axis
     * @param right_axis: value of the right axis
     */
    void RecordInput(u64 buttons, TasAnalog left_axis, TasAnalog right_axis);

    // Main loop that records or executes input
    void UpdateThread();

    //  Sets the flag to start or stop the TAS command excecution and swaps controllers profiles
    void StartStop();

    //  Stop the TAS and reverts any controller profile
    void Stop();

    // Sets the flag to reload the file and start from the begining in the next update
    void Reset();

    /**
     * Sets the flag to enable or disable recording of inputs
     * @return Returns true if the current recording status is enabled
     */
    bool Record();

    /**
     * Saves contents of record_commands on a file
     * @param overwrite_file: Indicates if player 1 should be overwritten
     */
    void SaveRecording(bool overwrite_file);

    /**
     * Returns the current status values of TAS playback/recording
     * @return Tuple of
     * TasState indicating the current state out of Running ;
     * Current playback progress ;
     * Total length of script file currently loaded or being recorded
     */
    std::tuple<TasState, size_t, size_t> GetStatus() const;

private:
    struct TASCommand {
        u64 buttons{};
        TasAnalog l_axis{};
        TasAnalog r_axis{};
    };

    /// Loads TAS files from all players
    void LoadTasFiles();

    /** Loads TAS file from the specified player
     * @param player_index: player number to save the script
     * @param file_index: script number of the file
     */
    void LoadTasFile(size_t player_index, size_t file_index);

    /** Writes a TAS file from the recorded commands
     * @param file_name: name of the file to be written
     */
    void WriteTasFile(std::u8string file_name);

    /**
     * Parses a string containing the axis values. X and Y have a range from -32767 to 32767
     * @param line: string containing axis values with the following format "x;y"
     * @return Returns a TAS analog object with axis values with range from -1.0 to 1.0
     */
    TasAnalog ReadCommandAxis(const std::string& line) const;

    /**
     * Parses a string containing the button values. Each button is represented by it's text format
     * specified in text_to_tas_button array
     * @param line: string containing button name with the following format "a;b;c;d..."
     * @return Returns a u64 with each bit representing the status of a button
     */
    u64 ReadCommandButtons(const std::string& line) const;

    /**
     * Reset state of all players
     */
    void ClearInput();

    /**
     * Converts an u64 containing the button status into the text equivalent
     * @param buttons: bitfield with the status of the buttons
     * @return Returns a string with the name of the buttons to be written to the file
     */
    std::string WriteCommandButtons(u64 buttons) const;

    /**
     * Converts an TAS analog object containing the axis status into the text equivalent
     * @param data: value of the axis
     * @return A string with the value of the axis to be written to the file
     */
    std::string WriteCommandAxis(TasAnalog data) const;

    size_t script_length{0};
    bool is_recording{false};
    bool is_running{false};
    bool needs_reset{false};
    std::array<std::vector<TASCommand>, PLAYER_NUMBER> commands{};
    std::vector<TASCommand> record_commands{};
    size_t current_command{0};
    TASCommand last_input{}; // only used for recording
};
} // namespace InputCommon::TasInput
