// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/assert.h"
#include "common/bit_field.h"
#include "common/common_types.h"
#include "common/expected.h"

// All the constants in this file come from http://switchbrew.org/index.php?title=Error_codes

/**
 * Identifies the module which caused the error. Error codes can be propagated through a call
 * chain, meaning that this doesn't always correspond to the module where the API call made is
 * contained.
 */
enum class ErrorModule : u32 {
    Common = 0,
    Kernel = 1,
    FS = 2,
    OS = 3, // used for Memory, Thread, Mutex, Nvidia
    HTCS = 4,
    NCM = 5,
    DD = 6,
    LR = 8,
    Loader = 9,
    CMIF = 10,
    HIPC = 11,
    PM = 15,
    NS = 16,
    HTC = 18,
    NCMContent = 20,
    SM = 21,
    RO = 22,
    SDMMC = 24,
    OVLN = 25,
    SPL = 26,
    ETHC = 100,
    I2C = 101,
    GPIO = 102,
    UART = 103,
    Settings = 105,
    WLAN = 107,
    XCD = 108,
    NIFM = 110,
    Hwopus = 111,
    Bluetooth = 113,
    VI = 114,
    NFP = 115,
    Time = 116,
    FGM = 117,
    OE = 118,
    PCIe = 120,
    Friends = 121,
    BCAT = 122,
    SSLSrv = 123,
    Account = 124,
    News = 125,
    Mii = 126,
    NFC = 127,
    AM = 128,
    PlayReport = 129,
    AHID = 130,
    Qlaunch = 132,
    PCV = 133,
    OMM = 134,
    BPC = 135,
    PSM = 136,
    NIM = 137,
    PSC = 138,
    TC = 139,
    USB = 140,
    NSD = 141,
    PCTL = 142,
    BTM = 143,
    ETicket = 145,
    NGC = 146,
    ERPT = 147,
    APM = 148,
    Profiler = 150,
    ErrorUpload = 151,
    Audio = 153,
    NPNS = 154,
    NPNSHTTPSTREAM = 155,
    ARP = 157,
    SWKBD = 158,
    BOOT = 159,
    NFCMifare = 161,
    UserlandAssert = 162,
    Fatal = 163,
    NIMShop = 164,
    SPSM = 165,
    BGTC = 167,
    UserlandCrash = 168,
    SREPO = 180,
    Dauth = 181,
    HID = 202,
    LDN = 203,
    Irsensor = 205,
    Capture = 206,
    Manu = 208,
    ATK = 209,
    GRC = 212,
    Migration = 216,
    MigrationLdcServ = 217,
    GeneralWebApplet = 800,
    WifiWebAuthApplet = 809,
    WhitelistedApplet = 810,
    ShopN = 811,
};

/// Encapsulates a Horizon OS error code, allowing it to be separated into its constituent fields.
union ResultCode {
    u32 raw;

    BitField<0, 9, ErrorModule> module;
    BitField<9, 13, u32> description;

    constexpr explicit ResultCode(u32 raw_) : raw(raw_) {}

    constexpr ResultCode(ErrorModule module_, u32 description_)
        : raw(module.FormatValue(module_) | description.FormatValue(description_)) {}

    [[nodiscard]] constexpr bool IsSuccess() const {
        return raw == 0;
    }

    [[nodiscard]] constexpr bool IsError() const {
        return !IsSuccess();
    }
};

[[nodiscard]] constexpr bool operator==(const ResultCode& a, const ResultCode& b) {
    return a.raw == b.raw;
}

[[nodiscard]] constexpr bool operator!=(const ResultCode& a, const ResultCode& b) {
    return !operator==(a, b);
}

// Convenience functions for creating some common kinds of errors:

/// The default success `ResultCode`.
constexpr ResultCode ResultSuccess(0);

/**
 * Placeholder result code used for unknown error codes.
 *
 * @note This should only be used when a particular error code
 *       is not known yet.
 */
constexpr ResultCode ResultUnknown(UINT32_MAX);

/**
 * A ResultRange defines an inclusive range of error descriptions within an error module.
 * This can be used to check whether the description of a given ResultCode falls within the range.
 * The conversion function returns a ResultCode with its description set to description_start.
 *
 * An example of how it could be used:
 * \code
 * constexpr ResultRange ResultCommonError{ErrorModule::Common, 0, 9999};
 *
 * ResultCode Example(int value) {
 *     const ResultCode result = OtherExample(value);
 *
 *     // This will only evaluate to true if result.module is ErrorModule::Common and
 *     // result.description is in between 0 and 9999 inclusive.
 *     if (ResultCommonError.Includes(result)) {
 *         // This returns ResultCode{ErrorModule::Common, 0};
 *         return ResultCommonError;
 *     }
 *
 *     return ResultSuccess;
 * }
 * \endcode
 */
class ResultRange {
public:
    consteval ResultRange(ErrorModule module, u32 description_start, u32 description_end_)
        : code{module, description_start}, description_end{description_end_} {}

    [[nodiscard]] consteval operator ResultCode() const {
        return code;
    }

    [[nodiscard]] constexpr bool Includes(ResultCode other) const {
        return code.module == other.module && code.description <= other.description &&
               other.description <= description_end;
    }

private:
    ResultCode code;
    u32 description_end;
};

/**
 * This is an optional value type. It holds a `ResultCode` and, if that code is ResultSuccess, it
 * also holds a result of type `T`. If the code is an error code (not ResultSuccess), then trying
 * to access the inner value with operator* is undefined behavior and will assert with Unwrap().
 * Users of this class must be cognizant to check the status of the ResultVal with operator bool(),
 * Code(), Succeeded() or Failed() prior to accessing the inner value.
 *
 * An example of how it could be used:
 * \code
 * ResultVal<int> Frobnicate(float strength) {
 *     if (strength < 0.f || strength > 1.0f) {
 *         // Can't frobnicate too weakly or too strongly
 *         return ResultCode{ErrorModule::Common, 1};
 *     } else {
 *         // Frobnicated! Give caller a cookie
 *         return 42;
 *     }
 * }
 * \endcode
 *
 * \code
 * auto frob_result = Frobnicate(0.75f);
 * if (frob_result) {
 *     // Frobbed ok
 *     printf("My cookie is %d\n", *frob_result);
 * } else {
 *     printf("Guess I overdid it. :( Error code: %ux\n", frob_result.Code().raw);
 * }
 * \endcode
 */
template <typename T>
class ResultVal {
public:
    constexpr ResultVal() : expected{} {}

    constexpr ResultVal(ResultCode code) : expected{Common::Unexpected(code)} {}

    template <typename U>
    constexpr ResultVal(U&& val) : expected{std::forward<U>(val)} {}

    template <typename... Args>
    constexpr ResultVal(Args&&... args) : expected{std::in_place, std::forward<Args>(args)...} {}

    ~ResultVal() = default;

    constexpr ResultVal(const ResultVal&) = default;
    constexpr ResultVal(ResultVal&&) = default;

    ResultVal& operator=(const ResultVal&) = default;
    ResultVal& operator=(ResultVal&&) = default;

    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return expected.has_value();
    }

    [[nodiscard]] constexpr ResultCode Code() const {
        return expected.has_value() ? ResultSuccess : expected.error();
    }

    [[nodiscard]] constexpr bool Succeeded() const {
        return expected.has_value();
    }

    [[nodiscard]] constexpr bool Failed() const {
        return !expected.has_value();
    }

    [[nodiscard]] constexpr T* operator->() {
        return std::addressof(expected.value());
    }

    [[nodiscard]] constexpr const T* operator->() const {
        return std::addressof(expected.value());
    }

    [[nodiscard]] constexpr T& operator*() & {
        return *expected;
    }

    [[nodiscard]] constexpr const T& operator*() const& {
        return *expected;
    }

    [[nodiscard]] constexpr T&& operator*() && {
        return *expected;
    }

    [[nodiscard]] constexpr const T&& operator*() const&& {
        return *expected;
    }

    [[nodiscard]] constexpr T& Unwrap() & {
        ASSERT_MSG(Succeeded(), "Tried to Unwrap empty ResultVal");
        return expected.value();
    }

    [[nodiscard]] constexpr const T& Unwrap() const& {
        ASSERT_MSG(Succeeded(), "Tried to Unwrap empty ResultVal");
        return expected.value();
    }

    [[nodiscard]] constexpr T&& Unwrap() && {
        ASSERT_MSG(Succeeded(), "Tried to Unwrap empty ResultVal");
        return std::move(expected.value());
    }

    [[nodiscard]] constexpr const T&& Unwrap() const&& {
        ASSERT_MSG(Succeeded(), "Tried to Unwrap empty ResultVal");
        return std::move(expected.value());
    }

    template <typename U>
    [[nodiscard]] constexpr T ValueOr(U&& v) const& {
        return expected.value_or(v);
    }

    template <typename U>
    [[nodiscard]] constexpr T ValueOr(U&& v) && {
        return expected.value_or(v);
    }

private:
    // TODO: Replace this with std::expected once it is standardized in the STL.
    Common::Expected<T, ResultCode> expected;
};

/**
 * Check for the success of `source` (which must evaluate to a ResultVal). If it succeeds, unwraps
 * the contained value and assigns it to `target`, which can be either an l-value expression or a
 * variable declaration. If it fails the return code is returned from the current function. Thus it
 * can be used to cascade errors out, achieving something akin to exception handling.
 */
#define CASCADE_RESULT(target, source)                                                             \
    auto CONCAT2(check_result_L, __LINE__) = source;                                               \
    if (CONCAT2(check_result_L, __LINE__).Failed()) {                                              \
        return CONCAT2(check_result_L, __LINE__).Code();                                           \
    }                                                                                              \
    target = std::move(*CONCAT2(check_result_L, __LINE__))

/**
 * Analogous to CASCADE_RESULT, but for a bare ResultCode. The code will be propagated if
 * non-success, or discarded otherwise.
 */
#define CASCADE_CODE(source)                                                                       \
    do {                                                                                           \
        auto CONCAT2(check_result_L, __LINE__) = source;                                           \
        if (CONCAT2(check_result_L, __LINE__).IsError()) {                                         \
            return CONCAT2(check_result_L, __LINE__);                                              \
        }                                                                                          \
    } while (false)

#define R_SUCCEEDED(res) (res.IsSuccess())

/// Evaluates a boolean expression, and succeeds if that expression is true.
#define R_SUCCEED_IF(expr) R_UNLESS(!(expr), ResultSuccess)

/// Evaluates a boolean expression, and returns a result unless that expression is true.
#define R_UNLESS(expr, res)                                                                        \
    {                                                                                              \
        if (!(expr)) {                                                                             \
            if (res.IsError()) {                                                                   \
                LOG_ERROR(Kernel, "Failed with result: {}", res.raw);                              \
            }                                                                                      \
            return res;                                                                            \
        }                                                                                          \
    }

/// Evaluates an expression that returns a result, and returns the result if it would fail.
#define R_TRY(res_expr)                                                                            \
    {                                                                                              \
        const auto _tmp_r_try_rc = (res_expr);                                                     \
        if (_tmp_r_try_rc.IsError()) {                                                             \
            return _tmp_r_try_rc;                                                                  \
        }                                                                                          \
    }
