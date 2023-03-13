// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <mutex>

#include "common/assert.h"
#include "common/fiber.h"
#include "common/virtual_buffer.h"

#include <boost/context/detail/fcontext.hpp>

namespace Common {

constexpr std::size_t default_stack_size = 512 * 1024;

struct Fiber::FiberImpl {
    FiberImpl() : stack{default_stack_size}, rewind_stack{default_stack_size} {}

    VirtualBuffer<u8> stack;
    VirtualBuffer<u8> rewind_stack;

    std::mutex guard;
    std::function<void()> entry_point;
    std::function<void()> rewind_point;
    std::shared_ptr<Fiber> previous_fiber;
    bool is_thread_fiber{};
    bool released{};

    u8* stack_limit{};
    u8* rewind_stack_limit{};
    boost::context::detail::fcontext_t context{};
    boost::context::detail::fcontext_t rewind_context{};
};

void Fiber::SetRewindPoint(std::function<void()>&& rewind_func) {
    impl->rewind_point = std::move(rewind_func);
}

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
    ASSERT_MSG(impl->is_thread_fiber, "Exiting non main thread fiber");
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

void Fiber::YieldTo(std::weak_ptr<Fiber> weak_from, Fiber& to) {
    to.impl->guard.lock();
    to.impl->previous_fiber = weak_from.lock();

    auto transfer = boost::context::detail::jump_fcontext(to.impl->context, &to);

    // "from" might no longer be valid if the thread was killed
    if (auto from = weak_from.lock()) {
        if (from->impl->previous_fiber == nullptr) {
            ASSERT_MSG(false, "previous_fiber is nullptr!");
            return;
        }
        from->impl->previous_fiber->impl->context = transfer.fctx;
        from->impl->previous_fiber->impl->guard.unlock();
        from->impl->previous_fiber.reset();
    }
}

std::shared_ptr<Fiber> Fiber::ThreadToFiber() {
    std::shared_ptr<Fiber> fiber = std::shared_ptr<Fiber>{new Fiber()};
    fiber->impl->guard.lock();
    fiber->impl->is_thread_fiber = true;
    return fiber;
}

} // namespace Common
