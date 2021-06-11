// Copyright 2020 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <atomic>
#include <memory>

namespace Common {
class Event;
}

namespace Core {

class CPUInterruptHandler {
public:
    CPUInterruptHandler();
    ~CPUInterruptHandler();

    CPUInterruptHandler(const CPUInterruptHandler&) = delete;
    CPUInterruptHandler& operator=(const CPUInterruptHandler&) = delete;

    CPUInterruptHandler(CPUInterruptHandler&&) = delete;
    CPUInterruptHandler& operator=(CPUInterruptHandler&&) = delete;

    bool IsInterrupted() const {
        return is_interrupted;
    }

    void SetInterrupt(bool is_interrupted);

    void AwaitInterrupt();

private:
    std::unique_ptr<Common::Event> interrupt_event;
    std::atomic_bool is_interrupted{false};
};

} // namespace Core
