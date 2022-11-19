// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/fiber.h"
#include "common/spin_lock.h"

#if defined(_WIN32) || defined(WIN32)
#include <windows.h>
#else
#include <boost/context/detail/fcontext.hpp>
#endif

namespace Common {

constexpr std::size_t default_stack_size = 512 * 1024;

struct Fiber::FiberImpl {
    SpinLock guard{};
    std::function<void()> entry_point;
    std::function<void()> rewind_point;
    void* rewind_parameter{};
    std::shared_ptr<Fiber> previous_fiber;
    bool is_thread_fiber{};
    bool released{};

#if defined(_WIN32) || defined(WIN32)
    LPVOID handle = nullptr;
    LPVOID rewind_handle = nullptr;
#else
    alignas(64) std::array<u8, default_stack_size> stack;
    alignas(64) std::array<u8, default_stack_size> rewind_stack;
    u8* stack_limit;
    u8* rewind_stack_limit;
    boost::context::detail::fcontext_t context;
    boost::context::detail::fcontext_t rewind_context;
#endif
};

void Fiber::SetRewindPoint(std::function<void()>&& rewind_func) {
    impl->rewind_point = std::move(rewind_func);
}

#if defined(_WIN32) || defined(WIN32)

void Fiber::Start() {
    ASSERT(impl->previous_fiber != nullptr);
    impl->previous_fiber->impl->guard.unlock();
    impl->previous_fiber.reset();
    impl->entry_point();
    UNREACHABLE();
}

void Fiber::OnRewind() {
    ASSERT(impl->handle != nullptr);
    DeleteFiber(impl->handle);
    impl->handle = impl->rewind_handle;
    impl->rewind_handle = nullptr;
    impl->rewind_point();
    UNREACHABLE();
}

void Fiber::FiberStartFunc(void* fiber_parameter) {
    auto* fiber = static_cast<Fiber*>(fiber_parameter);
    fiber->Start();
}

void Fiber::RewindStartFunc(void* fiber_parameter) {
    auto* fiber = static_cast<Fiber*>(fiber_parameter);
    fiber->OnRewind();
}

Fiber::Fiber(std::function<void()>&& entry_point_func) : impl{std::make_unique<FiberImpl>()} {
    impl->entry_point = std::move(entry_point_func);
    impl->handle = CreateFiber(default_stack_size, &FiberStartFunc, this);
}

Fiber::Fiber() : impl{std::make_unique<FiberImpl>()} {}

Fiber::~Fiber() {
    if (impl->released) {
        return;
    }
    // Make sure the Fiber is not being used
    const bool locked = impl->guard.try_lock();
    ASSERT_MSG(locked, "Destroying a fiber that's still running");
    if (locked) {
        impl->guard.unlock();
    }
    DeleteFiber(impl->handle);
}

void Fiber::Exit() {
    ASSERT_MSG(impl->is_thread_fiber, "Exitting non main thread fiber");
    if (!impl->is_thread_fiber) {
        return;
    }
    ConvertFiberToThread();
    impl->guard.unlock();
    impl->released = true;
}

void Fiber::Rewind() {
    ASSERT(impl->rewind_point);
    ASSERT(impl->rewind_handle == nullptr);
    impl->rewind_handle = CreateFiber(default_stack_size, &RewindStartFunc, this);
    SwitchToFiber(impl->rewind_handle);
}

void Fiber::YieldTo(std::shared_ptr<Fiber> from, std::shared_ptr<Fiber> to) {
    ASSERT_MSG(from != nullptr, "Yielding fiber is null!");
    ASSERT_MSG(to != nullptr, "Next fiber is null!");
    to->impl->guard.lock();
    to->impl->previous_fiber = from;
    SwitchToFiber(to->impl->handle);
    ASSERT(from->impl->previous_fiber != nullptr);
    from->impl->previous_fiber->impl->guard.unlock();
    from->impl->previous_fiber.reset();
}

std::shared_ptr<Fiber> Fiber::ThreadToFiber() {
    std::shared_ptr<Fiber> fiber = std::shared_ptr<Fiber>{new Fiber()};
    fiber->impl->guard.lock();
    fiber->impl->handle = ConvertThreadToFiber(nullptr);
    fiber->impl->is_thread_fiber = true;
    return fiber;
}

#else

void Fiber::Start(boost::context::detail::transfer_t& transfer) {
    ASSERT(impl->previous_fiber != nullptr);
    impl->previous_fiber->impl->context = transfer.fctx;
    impl->previous_fiber->impl->guard.unlock();
    impl->previous_fiber.reset();
    impl->entry_point();
    UNREACHABLE();
}

void Fiber::OnRewind([[maybe_unused]] boost::context::detail::transfer_t& transfer) {
    ASSERT(impl->context != nullptr);
    impl->context = impl->rewind_context;
    impl->rewind_context = nullptr;
    u8* tmp = impl->stack_limit;
    impl->stack_limit = impl->rewind_stack_limit;
    impl->rewind_stack_limit = tmp;
    impl->rewind_point();
    UNREACHABLE();
}

void Fiber::FiberStartFunc(boost::context::detail::transfer_t transfer) {
    auto* fiber = static_cast<Fiber*>(transfer.data);
    fiber->Start(transfer);
}

void Fiber::RewindStartFunc(boost::context::detail::transfer_t transfer) {
    auto* fiber = static_cast<Fiber*>(transfer.data);
    fiber->OnRewind(transfer);
}

Fiber::Fiber(std::function<void()>&& entry_point_func) : impl{std::make_unique<FiberImpl>()} {
    impl->entry_point = std::move(entry_point_func);
    impl->stack_limit = impl->stack.data();
    impl->rewind_stack_limit = impl->rewind_stack.data();
    u8* stack_base = impl->stack_limit + default_stack_size;
    impl->context =
        boost::context::detail::make_fcontext(stack_base, impl->stack.size(), FiberStartFunc);
}

Fiber::Fiber() : impl{std::make_unique<FiberImpl>()} {}

Fiber::~Fiber() {
    if (impl->released) {
        return;
    }
    // Make sure the Fiber is not being used
    const bool locked = impl->guard.try_lock();
    ASSERT_MSG(locked, "Destroying a fiber that's still running");
    if (locked) {
        impl->guard.unlock();
    }
}

void Fiber::Exit() {
    ASSERT_MSG(impl->is_thread_fiber, "Exitting non main thread fiber");
    if (!impl->is_thread_fiber) {
        return;
    }
    impl->guard.unlock();
    impl->released = true;
}

void Fiber::Rewind() {
    ASSERT(impl->rewind_point);
    ASSERT(impl->rewind_context == nullptr);
    u8* stack_base = impl->rewind_stack_limit + default_stack_size;
    impl->rewind_context =
        boost::context::detail::make_fcontext(stack_base, impl->stack.size(), RewindStartFunc);
    boost::context::detail::jump_fcontext(impl->rewind_context, this);
}

void Fiber::YieldTo(std::shared_ptr<Fiber> from, std::shared_ptr<Fiber> to) {
    ASSERT_MSG(from != nullptr, "Yielding fiber is null!");
    ASSERT_MSG(to != nullptr, "Next fiber is null!");
    to->impl->guard.lock();
    to->impl->previous_fiber = from;
    auto transfer = boost::context::detail::jump_fcontext(to->impl->context, to.get());
    ASSERT(from->impl->previous_fiber != nullptr);
    from->impl->previous_fiber->impl->context = transfer.fctx;
    from->impl->previous_fiber->impl->guard.unlock();
    from->impl->previous_fiber.reset();
}

std::shared_ptr<Fiber> Fiber::ThreadToFiber() {
    std::shared_ptr<Fiber> fiber = std::shared_ptr<Fiber>{new Fiber()};
    fiber->impl->guard.lock();
    fiber->impl->is_thread_fiber = true;
    return fiber;
}

#endif
} // namespace Common