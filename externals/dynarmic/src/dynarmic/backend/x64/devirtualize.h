/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <cstring>
#include <utility>

#include <mp/traits/function_info.h>

#include "dynarmic/backend/x64/callback.h"
#include "dynarmic/common/cast_util.h"
#include "dynarmic/common/common_types.h"

namespace Dynarmic {
namespace Backend::X64 {

namespace impl {

template<typename FunctionType, FunctionType mfp>
struct ThunkBuilder;

template<typename C, typename R, typename... Args, R (C::*mfp)(Args...)>
struct ThunkBuilder<R (C::*)(Args...), mfp> {
    static R Thunk(C* this_, Args... args) {
        return (this_->*mfp)(std::forward<Args>(args)...);
    }
};

}  // namespace impl

template<auto mfp>
ArgCallback DevirtualizeGeneric(mp::class_type<decltype(mfp)>* this_) {
    return ArgCallback{&impl::ThunkBuilder<decltype(mfp), mfp>::Thunk, reinterpret_cast<u64>(this_)};
}

template<auto mfp>
ArgCallback DevirtualizeWindows(mp::class_type<decltype(mfp)>* this_) {
    static_assert(sizeof(mfp) == 8);
    return ArgCallback{Common::BitCast<u64>(mfp), reinterpret_cast<u64>(this_)};
}

template<auto mfp>
ArgCallback DevirtualizeItanium(mp::class_type<decltype(mfp)>* this_) {
    struct MemberFunctionPointer {
        /// For a non-virtual function, this is a simple function pointer.
        /// For a virtual function, it is (1 + virtual table offset in bytes).
        u64 ptr;
        /// The required adjustment to `this`, prior to the call.
        u64 adj;
    } mfp_struct = Common::BitCast<MemberFunctionPointer>(mfp);

    static_assert(sizeof(MemberFunctionPointer) == 16);
    static_assert(sizeof(MemberFunctionPointer) == sizeof(mfp));

    u64 fn_ptr = mfp_struct.ptr;
    u64 this_ptr = reinterpret_cast<u64>(this_) + mfp_struct.adj;
    if (mfp_struct.ptr & 1) {
        u64 vtable = Common::BitCastPointee<u64>(this_ptr);
        fn_ptr = Common::BitCastPointee<u64>(vtable + fn_ptr - 1);
    }
    return ArgCallback{fn_ptr, this_ptr};
}

template<auto mfp>
ArgCallback Devirtualize(mp::class_type<decltype(mfp)>* this_) {
#if defined(__APPLE__) || defined(linux) || defined(__linux) || defined(__linux__)
    return DevirtualizeItanium<mfp>(this_);
#elif defined(__MINGW64__)
    return DevirtualizeItanium<mfp>(this_);
#elif defined(_WIN32)
    return DevirtualizeWindows<mfp>(this_);
#else
    return DevirtualizeGeneric<mfp>(this_);
#endif
}

}  // namespace Backend::X64
}  // namespace Dynarmic
