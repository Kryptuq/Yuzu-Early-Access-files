/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

namespace Dynarmic::A32 {
struct UserCallbacks;
}

namespace Dynarmic::A64 {
struct UserCallbacks;
struct UserConfig;
}  // namespace Dynarmic::A64

namespace Dynarmic::IR {
class Block;
}

namespace Dynarmic::Optimization {

struct PolyfillOptions {
    bool sha256 = false;

    bool operator==(const PolyfillOptions&) const = default;
};

void PolyfillPass(IR::Block& block, const PolyfillOptions& opt);
void A32ConstantMemoryReads(IR::Block& block, A32::UserCallbacks* cb);
void A32GetSetElimination(IR::Block& block);
void A64CallbackConfigPass(IR::Block& block, const A64::UserConfig& conf);
void A64GetSetElimination(IR::Block& block);
void A64MergeInterpretBlocksPass(IR::Block& block, A64::UserCallbacks* cb);
void ConstantPropagation(IR::Block& block);
void DeadCodeElimination(IR::Block& block);
void IdentityRemovalPass(IR::Block& block);
void VerificationPass(const IR::Block& block);

}  // namespace Dynarmic::Optimization
