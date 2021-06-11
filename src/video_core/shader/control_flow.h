// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <list>
#include <map>
#include <optional>
#include <set>
#include <variant>

#include "video_core/engines/shader_bytecode.h"
#include "video_core/shader/ast.h"
#include "video_core/shader/compiler_settings.h"
#include "video_core/shader/registry.h"
#include "video_core/shader/shader_ir.h"

namespace VideoCommon::Shader {

using Tegra::Shader::ConditionCode;
using Tegra::Shader::Pred;

constexpr s32 exit_branch = -1;

struct Condition {
    Pred predicate{Pred::UnusedIndex};
    ConditionCode cc{ConditionCode::T};

    bool IsUnconditional() const {
        return predicate == Pred::UnusedIndex && cc == ConditionCode::T;
    }

    bool operator==(const Condition& other) const {
        return std::tie(predicate, cc) == std::tie(other.predicate, other.cc);
    }

    bool operator!=(const Condition& other) const {
        return !operator==(other);
    }
};

class SingleBranch {
public:
    SingleBranch() = default;
    explicit SingleBranch(Condition condition_, s32 address_, bool kill_, bool is_sync_,
                          bool is_brk_, bool ignore_)
        : condition{condition_}, address{address_}, kill{kill_}, is_sync{is_sync_}, is_brk{is_brk_},
          ignore{ignore_} {}

    bool operator==(const SingleBranch& b) const {
        return std::tie(condition, address, kill, is_sync, is_brk, ignore) ==
               std::tie(b.condition, b.address, b.kill, b.is_sync, b.is_brk, b.ignore);
    }

    bool operator!=(const SingleBranch& b) const {
        return !operator==(b);
    }

    Condition condition{};
    s32 address{exit_branch};
    bool kill{};
    bool is_sync{};
    bool is_brk{};
    bool ignore{};
};

struct CaseBranch {
    explicit CaseBranch(u32 cmp_value_, u32 address_) : cmp_value{cmp_value_}, address{address_} {}
    u32 cmp_value;
    u32 address;
};

class MultiBranch {
public:
    explicit MultiBranch(u32 gpr_, std::vector<CaseBranch>&& branches_)
        : gpr{gpr_}, branches{std::move(branches_)} {}

    u32 gpr{};
    std::vector<CaseBranch> branches{};
};

using BranchData = std::variant<SingleBranch, MultiBranch>;
using BlockBranchInfo = std::shared_ptr<BranchData>;

bool BlockBranchInfoAreEqual(BlockBranchInfo first, BlockBranchInfo second);

struct ShaderBlock {
    u32 start{};
    u32 end{};
    bool ignore_branch{};
    BlockBranchInfo branch{};

    bool operator==(const ShaderBlock& sb) const {
        return std::tie(start, end, ignore_branch) ==
                   std::tie(sb.start, sb.end, sb.ignore_branch) &&
               BlockBranchInfoAreEqual(branch, sb.branch);
    }

    bool operator!=(const ShaderBlock& sb) const {
        return !operator==(sb);
    }
};

struct ShaderFunction {
    std::list<ShaderBlock> blocks{};
    std::set<u32> labels{};
    u32 start{};
    u32 end{};
    ASTManager manager{true, true};
    CompilerSettings settings{};
};

struct ShaderProgram {
    ShaderFunction main;
    std::map<u32, ShaderFunction> subfunctions;
};

std::unique_ptr<ShaderProgram> ScanFlow(const ProgramCode& program_code, u32 start_address,
                                        const CompilerSettings& settings, Registry& registry);

} // namespace VideoCommon::Shader
