#pragma once

#include <array>
#include "common/bit_field.h"
#include "common/common_types.h"

namespace Service::Nvidia {

constexpr u32 MaxSyncPoints = 192;
constexpr u32 MaxNvEvents = 64;
using DeviceFD = s32;

constexpr DeviceFD INVALID_NVDRV_FD = -1;

struct Fence {
    s32 id;
    u32 value;
};

static_assert(sizeof(Fence) == 8, "Fence has wrong size");

struct MultiFence {
    u32 num_fences;
    std::array<Fence, 4> fences;
};

enum class NvResult : u32 {
    Success = 0x0,
    NotImplemented = 0x1,
    NotSupported = 0x2,
    NotInitialized = 0x3,
    BadParameter = 0x4,
    Timeout = 0x5,
    InsufficientMemory = 0x6,
    ReadOnlyAttribute = 0x7,
    InvalidState = 0x8,
    InvalidAddress = 0x9,
    InvalidSize = 0xA,
    BadValue = 0xB,
    AlreadyAllocated = 0xD,
    Busy = 0xE,
    ResourceError = 0xF,
    CountMismatch = 0x10,
    OverFlow = 0x11,
    InsufficientTransferMemory = 0x1000,
    InsufficientVideoMemory = 0x10000,
    BadSurfaceColorScheme = 0x10001,
    InvalidSurface = 0x10002,
    SurfaceNotSupported = 0x10003,
    DispInitFailed = 0x20000,
    DispAlreadyAttached = 0x20001,
    DispTooManyDisplays = 0x20002,
    DispNoDisplaysAttached = 0x20003,
    DispModeNotSupported = 0x20004,
    DispNotFound = 0x20005,
    DispAttachDissallowed = 0x20006,
    DispTypeNotSupported = 0x20007,
    DispAuthenticationFailed = 0x20008,
    DispNotAttached = 0x20009,
    DispSamePwrState = 0x2000A,
    DispEdidFailure = 0x2000B,
    DispDsiReadAckError = 0x2000C,
    DispDsiReadInvalidResp = 0x2000D,
    FileWriteFailed = 0x30000,
    FileReadFailed = 0x30001,
    EndOfFile = 0x30002,
    FileOperationFailed = 0x30003,
    DirOperationFailed = 0x30004,
    EndOfDirList = 0x30005,
    ConfigVarNotFound = 0x30006,
    InvalidConfigVar = 0x30007,
    LibraryNotFound = 0x30008,
    SymbolNotFound = 0x30009,
    MemoryMapFailed = 0x3000A,
    IoctlFailed = 0x3000F,
    AccessDenied = 0x30010,
    DeviceNotFound = 0x30011,
    KernelDriverNotFound = 0x30012,
    FileNotFound = 0x30013,
    PathAlreadyExists = 0x30014,
    ModuleNotPresent = 0xA000E,
};

enum class EventState {
    Free = 0,
    Registered = 1,
    Waiting = 2,
    Busy = 3,
};

union Ioctl {
    u32_le raw;
    BitField<0, 8, u32> cmd;
    BitField<8, 8, u32> group;
    BitField<16, 14, u32> length;
    BitField<30, 1, u32> is_in;
    BitField<31, 1, u32> is_out;
};

} // namespace Service::Nvidia
