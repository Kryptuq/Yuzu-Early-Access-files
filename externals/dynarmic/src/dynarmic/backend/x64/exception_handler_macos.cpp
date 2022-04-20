/* This file is part of the dynarmic project.
 * Copyright (c) 2019 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <mach/mach.h>
#include <mach/message.h>

#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <fmt/format.h>
#include <mcl/assert.hpp>
#include <mcl/bit_cast.hpp>
#include <mcl/stdint.hpp>

#include "dynarmic/backend/x64/block_of_code.h"
#include "dynarmic/backend/x64/exception_handler.h"

#define mig_external extern "C"
#include "dynarmic/backend/x64/mig/mach_exc_server.h"

namespace Dynarmic::Backend::X64 {

namespace {

struct CodeBlockInfo {
    u64 code_begin, code_end;
    std::function<FakeCall(u64)> cb;
};

struct MachMessage {
    mach_msg_header_t head;
    char data[2048];  ///< Arbitrary size
};

class MachHandler final {
public:
    MachHandler();
    ~MachHandler();

    kern_return_t HandleRequest(x86_thread_state64_t* thread_state);

    void AddCodeBlock(CodeBlockInfo info);
    void RemoveCodeBlock(u64 rip);

private:
    auto FindCodeBlockInfo(u64 rip) {
        return std::find_if(code_block_infos.begin(), code_block_infos.end(), [&](const auto& x) { return x.code_begin <= rip && x.code_end > rip; });
    }

    std::vector<CodeBlockInfo> code_block_infos;
    std::mutex code_block_infos_mutex;

    std::thread thread;
    mach_port_t server_port;

    void MessagePump();
};

MachHandler::MachHandler() {
#define KCHECK(x) ASSERT_MSG((x) == KERN_SUCCESS, "dynarmic: macOS MachHandler: init failure at {}", #x)

    KCHECK(mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &server_port));
    KCHECK(mach_port_insert_right(mach_task_self(), server_port, server_port, MACH_MSG_TYPE_MAKE_SEND));
    KCHECK(task_set_exception_ports(mach_task_self(), EXC_MASK_BAD_ACCESS, server_port, EXCEPTION_STATE | MACH_EXCEPTION_CODES, x86_THREAD_STATE64));

    // The below doesn't actually work, and I'm not sure why; since this doesn't work we'll have a spurious error message upon shutdown.
    mach_port_t prev;
    KCHECK(mach_port_request_notification(mach_task_self(), server_port, MACH_NOTIFY_PORT_DESTROYED, 0, server_port, MACH_MSG_TYPE_MAKE_SEND_ONCE, &prev));

#undef KCHECK

    thread = std::thread(&MachHandler::MessagePump, this);
}

MachHandler::~MachHandler() {
    mach_port_destroy(mach_task_self(), server_port);
    thread.join();
}

void MachHandler::MessagePump() {
    mach_msg_return_t mr;
    MachMessage request;
    MachMessage reply;

    while (true) {
        mr = mach_msg(&request.head, MACH_RCV_MSG | MACH_RCV_LARGE, 0, sizeof(request), server_port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
        if (mr != MACH_MSG_SUCCESS) {
            fmt::print(stderr, "dynarmic: macOS MachHandler: Failed to receive mach message. error: {:#08x} ({})\n", mr, mach_error_string(mr));
            return;
        }

        if (!mach_exc_server(&request.head, &reply.head)) {
            fmt::print(stderr, "dynarmic: macOS MachHandler: Unexpected mach message\n");
            return;
        }

        mr = mach_msg(&reply.head, MACH_SEND_MSG, reply.head.msgh_size, 0, MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
        if (mr != MACH_MSG_SUCCESS) {
            fmt::print(stderr, "dynarmic: macOS MachHandler: Failed to send mach message. error: {:#08x} ({})\n", mr, mach_error_string(mr));
            return;
        }
    }
}

kern_return_t MachHandler::HandleRequest(x86_thread_state64_t* ts) {
    std::lock_guard<std::mutex> guard(code_block_infos_mutex);

    const auto iter = FindCodeBlockInfo(ts->__rip);
    if (iter == code_block_infos.end()) {
        fmt::print(stderr, "Unhandled EXC_BAD_ACCESS at rip {:#016x}\n", ts->__rip);
        return KERN_FAILURE;
    }

    FakeCall fc = iter->cb(ts->__rip);

    ts->__rsp -= sizeof(u64);
    *mcl::bit_cast<u64*>(ts->__rsp) = fc.ret_rip;
    ts->__rip = fc.call_rip;

    return KERN_SUCCESS;
}

void MachHandler::AddCodeBlock(CodeBlockInfo cbi) {
    std::lock_guard<std::mutex> guard(code_block_infos_mutex);
    if (auto iter = FindCodeBlockInfo(cbi.code_begin); iter != code_block_infos.end()) {
        code_block_infos.erase(iter);
    }
    code_block_infos.push_back(cbi);
}

void MachHandler::RemoveCodeBlock(u64 rip) {
    std::lock_guard<std::mutex> guard(code_block_infos_mutex);
    const auto iter = FindCodeBlockInfo(rip);
    if (iter == code_block_infos.end()) {
        return;
    }
    code_block_infos.erase(iter);
}

MachHandler mach_handler;

}  // anonymous namespace

mig_external kern_return_t catch_mach_exception_raise(mach_port_t, mach_port_t, mach_port_t, exception_type_t, mach_exception_data_t, mach_msg_type_number_t) {
    fmt::print(stderr, "dynarmic: Unexpected mach message: mach_exception_raise\n");
    return KERN_FAILURE;
}

mig_external kern_return_t catch_mach_exception_raise_state_identity(mach_port_t, mach_port_t, mach_port_t, exception_type_t, mach_exception_data_t, mach_msg_type_number_t, int*, thread_state_t, mach_msg_type_number_t, thread_state_t, mach_msg_type_number_t*) {
    fmt::print(stderr, "dynarmic: Unexpected mach message: mach_exception_raise_state_identity\n");
    return KERN_FAILURE;
}

mig_external kern_return_t catch_mach_exception_raise_state(
    mach_port_t /*exception_port*/,
    exception_type_t exception,
    const mach_exception_data_t /*code*/,  // code[0] is as per kern_return.h, code[1] is rip.
    mach_msg_type_number_t /*codeCnt*/,
    int* flavor,
    const thread_state_t old_state,
    mach_msg_type_number_t old_stateCnt,
    thread_state_t new_state,
    mach_msg_type_number_t* new_stateCnt) {
    if (!flavor || !new_stateCnt) {
        fmt::print(stderr, "dynarmic: catch_mach_exception_raise_state: Invalid arguments.\n");
        return KERN_INVALID_ARGUMENT;
    }
    if (*flavor != x86_THREAD_STATE64 || old_stateCnt != x86_THREAD_STATE64_COUNT || *new_stateCnt < x86_THREAD_STATE64_COUNT) {
        fmt::print(stderr, "dynarmic: catch_mach_exception_raise_state: Unexpected flavor.\n");
        return KERN_INVALID_ARGUMENT;
    }
    if (exception != EXC_BAD_ACCESS) {
        fmt::print(stderr, "dynarmic: catch_mach_exception_raise_state: Unexpected exception type.\n");
        return KERN_FAILURE;
    }

    x86_thread_state64_t* ts = reinterpret_cast<x86_thread_state64_t*>(new_state);
    std::memcpy(ts, reinterpret_cast<const x86_thread_state64_t*>(old_state), sizeof(x86_thread_state64_t));
    *new_stateCnt = x86_THREAD_STATE64_COUNT;

    return mach_handler.HandleRequest(ts);
}

struct ExceptionHandler::Impl final {
    Impl(BlockOfCode& code)
            : code_begin(mcl::bit_cast<u64>(code.getCode()))
            , code_end(code_begin + code.GetTotalCodeSize()) {}

    void SetCallback(std::function<FakeCall(u64)> cb) {
        CodeBlockInfo cbi;
        cbi.code_begin = code_begin;
        cbi.code_end = code_end;
        cbi.cb = cb;
        mach_handler.AddCodeBlock(cbi);
    }

    ~Impl() {
        mach_handler.RemoveCodeBlock(code_begin);
    }

private:
    u64 code_begin, code_end;
};

ExceptionHandler::ExceptionHandler() = default;

ExceptionHandler::~ExceptionHandler() = default;

void ExceptionHandler::Register(BlockOfCode& code) {
    impl = std::make_unique<Impl>(code);
}

bool ExceptionHandler::SupportsFastmem() const noexcept {
    return static_cast<bool>(impl);
}

void ExceptionHandler::SetFastmemCallback(std::function<FakeCall(u64)> cb) {
    impl->SetCallback(cb);
}

}  // namespace Dynarmic::Backend::X64
