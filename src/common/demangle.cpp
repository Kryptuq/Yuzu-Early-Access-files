// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <llvm/Demangle/Demangle.h>

#include "common/demangle.h"
#include "common/scope_exit.h"

namespace Common {

std::string DemangleSymbol(const std::string& mangled) {
    auto is_itanium = [](const std::string& name) -> bool {
        // A valid Itanium encoding requires 1-4 leading underscores, followed by 'Z'.
        auto pos = name.find_first_not_of('_');
        return pos > 0 && pos <= 4 && pos < name.size() && name[pos] == 'Z';
    };

    if (mangled.empty()) {
        return mangled;
    }

    char* demangled = nullptr;
    SCOPE_EXIT({ std::free(demangled); });

    if (is_itanium(mangled)) {
        demangled = llvm::itaniumDemangle(mangled.c_str(), nullptr, nullptr, nullptr);
    }

    if (!demangled) {
        return mangled;
    }
    return demangled;
}

} // namespace Common
