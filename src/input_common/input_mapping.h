// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included

#pragma once

#include "common/param_package.h"
#include "common/threadsafe_queue.h"

namespace InputCommon::Polling {
enum class InputType;
}

namespace InputCommon {
class InputEngine;
struct MappingData;

class MappingFactory {
public:
    MappingFactory();

    /**
     * Resets all variables to begin the mapping process
     * @param type type of input desired to be returned
     */
    void BeginMapping(Polling::InputType type);

    /// Returns an input event with mapping information from the input_queue
    [[nodiscard]] Common::ParamPackage GetNextInput();

    /**
     * Registers mapping input data from the driver
     * @param data A struct containing all the information needed to create a proper
     *             ParamPackage
     */
    void RegisterInput(const MappingData& data);

    /// Stop polling from all backends
    void StopMapping();

private:
    /**
     * If provided data satisfies the requirements it will push an element to the input_queue
     * Supported input:
     *     - Button: Creates a basic button ParamPackage
     *     - HatButton: Creates a basic hat button ParamPackage
     *     - Analog: Creates a basic analog ParamPackage
     * @param data A struct containing all the information needed to create a proper
     * ParamPackage
     */
    void RegisterButton(const MappingData& data);

    /**
     * If provided data satisfies the requirements it will push an element to the input_queue
     * Supported input:
     *     - Button, HatButton: Pass the data to RegisterButton
     *     - Analog: Stores the first axis and on the second axis creates a basic stick ParamPackage
     * @param data A struct containing all the information needed to create a proper
     *             ParamPackage
     */
    void RegisterStick(const MappingData& data);

    /**
     * If provided data satisfies the requirements it will push an element to the input_queue
     * Supported input:
     *     - Button, HatButton: Pass the data to RegisterButton
     *     - Analog: Stores the first two axis and on the third axis creates a basic Motion
     * ParamPackage
     *     - Motion: Creates a basic Motion ParamPackage
     * @param data A struct containing all the information needed to create a proper
     *             ParamPackage
     */
    void RegisterMotion(const MappingData& data);

    /**
     * Returns true if driver can be mapped
     * @param data A struct containing all the information needed to create a proper
     *             ParamPackage
     */
    bool IsDriverValid(const MappingData& data) const;

    Common::SPSCQueue<Common::ParamPackage> input_queue;
    Polling::InputType input_type{Polling::InputType::None};
    bool is_enabled{};
    int first_axis = -1;
    int second_axis = -1;
};

} // namespace InputCommon
