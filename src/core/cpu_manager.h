// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <atomic>
#include <functional>
#include <memory>
#include <thread>

#include "common/fiber.h"
#include "common/thread.h"
#include "core/hardware_properties.h"

namespace Common {
class Event;
class Fiber;
} // namespace Common

namespace Core {

class System;

class CpuManager {
public:
    explicit CpuManager(System& system_);
    CpuManager(const CpuManager&) = delete;
    CpuManager(CpuManager&&) = delete;

    ~CpuManager();

    CpuManager& operator=(const CpuManager&) = delete;
    CpuManager& operator=(CpuManager&&) = delete;

    /// Sets if emulation is multicore or single core, must be set before Initialize
    void SetMulticore(bool is_multi) {
        is_multicore = is_multi;
    }

    /// Sets if emulation is using an asynchronous GPU.
    void SetAsyncGpu(bool is_async) {
        is_async_gpu = is_async;
    }

    void Initialize();
    void Shutdown();

    void Pause(bool paused);

    static std::function<void(void*)> GetGuestThreadStartFunc();
    static std::function<void(void*)> GetIdleThreadStartFunc();
    static std::function<void(void*)> GetSuspendThreadStartFunc();
    void* GetStartFuncParamater();

    void PreemptSingleCore(bool from_running_enviroment = true);

    std::size_t CurrentCore() const {
        return current_core.load();
    }

private:
    static void GuestThreadFunction(void* cpu_manager);
    static void GuestRewindFunction(void* cpu_manager);
    static void IdleThreadFunction(void* cpu_manager);
    static void SuspendThreadFunction(void* cpu_manager);

    void MultiCoreRunGuestThread();
    void MultiCoreRunGuestLoop();
    void MultiCoreRunIdleThread();
    void MultiCoreRunSuspendThread();
    void MultiCorePause(bool paused);

    void SingleCoreRunGuestThread();
    void SingleCoreRunGuestLoop();
    void SingleCoreRunIdleThread();
    void SingleCoreRunSuspendThread();
    void SingleCorePause(bool paused);

    static void ThreadStart(std::stop_token stop_token, CpuManager& cpu_manager, std::size_t core);

    void RunThread(std::stop_token stop_token, std::size_t core);

    struct CoreData {
        std::shared_ptr<Common::Fiber> host_context;
        std::unique_ptr<Common::Event> enter_barrier;
        std::unique_ptr<Common::Event> exit_barrier;
        std::atomic<bool> is_running;
        std::atomic<bool> is_paused;
        std::atomic<bool> initialized;
        std::jthread host_thread;
    };

    std::atomic<bool> running_mode{};
    std::atomic<bool> paused_state{};

    std::array<CoreData, Core::Hardware::NUM_CPU_CORES> core_data{};

    bool is_async_gpu{};
    bool is_multicore{};
    std::atomic<std::size_t> current_core{};
    std::size_t idle_count{};
    static constexpr std::size_t max_cycle_runs = 5;

    System& system;
};

} // namespace Core
