// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <string>
#include <string_view>

#include <fmt/format.h>

#include "common/assert.h"
#include "common/common_types.h"
#include "video_core/shader/ast.h"
#include "video_core/shader/expr.h"

namespace VideoCommon::Shader {

ASTZipper::ASTZipper() = default;

void ASTZipper::Init(const ASTNode new_first, const ASTNode parent) {
    ASSERT(new_first->manager == nullptr);
    first = new_first;
    last = new_first;

    ASTNode current = first;
    while (current) {
        current->manager = this;
        current->parent = parent;
        last = current;
        current = current->next;
    }
}

void ASTZipper::PushBack(const ASTNode new_node) {
    ASSERT(new_node->manager == nullptr);
    new_node->previous = last;
    if (last) {
        last->next = new_node;
    }
    new_node->next.reset();
    last = new_node;
    if (!first) {
        first = new_node;
    }
    new_node->manager = this;
}

void ASTZipper::PushFront(const ASTNode new_node) {
    ASSERT(new_node->manager == nullptr);
    new_node->previous.reset();
    new_node->next = first;
    if (first) {
        first->previous = new_node;
    }
    if (last == first) {
        last = new_node;
    }
    first = new_node;
    new_node->manager = this;
}

void ASTZipper::InsertAfter(const ASTNode new_node, const ASTNode at_node) {
    ASSERT(new_node->manager == nullptr);
    if (!at_node) {
        PushFront(new_node);
        return;
    }
    const ASTNode next = at_node->next;
    if (next) {
        next->previous = new_node;
    }
    new_node->previous = at_node;
    if (at_node == last) {
        last = new_node;
    }
    new_node->next = next;
    at_node->next = new_node;
    new_node->manager = this;
}

void ASTZipper::InsertBefore(const ASTNode new_node, const ASTNode at_node) {
    ASSERT(new_node->manager == nullptr);
    if (!at_node) {
        PushBack(new_node);
        return;
    }
    const ASTNode previous = at_node->previous;
    if (previous) {
        previous->next = new_node;
    }
    new_node->next = at_node;
    if (at_node == first) {
        first = new_node;
    }
    new_node->previous = previous;
    at_node->previous = new_node;
    new_node->manager = this;
}

void ASTZipper::DetachTail(ASTNode node) {
    ASSERT(node->manager == this);
    if (node == first) {
        first.reset();
        last.reset();
        return;
    }

    last = node->previous;
    last->next.reset();
    node->previous.reset();

    ASTNode current = std::move(node);
    while (current) {
        current->manager = nullptr;
        current->parent.reset();
        current = current->next;
    }
}

void ASTZipper::DetachSegment(const ASTNode start, const ASTNode end) {
    ASSERT(start->manager == this && end->manager == this);
    if (start == end) {
        DetachSingle(start);
        return;
    }
    const ASTNode prev = start->previous;
    const ASTNode post = end->next;
    if (!prev) {
        first = post;
    } else {
        prev->next = post;
    }
    if (!post) {
        last = prev;
    } else {
        post->previous = prev;
    }
    start->previous.reset();
    end->next.reset();
    ASTNode current = start;
    bool found = false;
    while (current) {
        current->manager = nullptr;
        current->parent.reset();
        found |= current == end;
        current = current->next;
    }
    ASSERT(found);
}

void ASTZipper::DetachSingle(const ASTNode node) {
    ASSERT(node->manager == this);
    const ASTNode prev = node->previous;
    const ASTNode post = node->next;
    node->previous.reset();
    node->next.reset();
    if (!prev) {
        first = post;
    } else {
        prev->next = post;
    }
    if (!post) {
        last = prev;
    } else {
        post->previous = prev;
    }

    node->manager = nullptr;
    node->parent.reset();
}

void ASTZipper::Remove(const ASTNode node) {
    ASSERT(node->manager == this);
    const ASTNode next = node->next;
    const ASTNode previous = node->previous;
    if (previous) {
        previous->next = next;
    }
    if (next) {
        next->previous = previous;
    }
    node->parent.reset();
    node->manager = nullptr;
    if (node == last) {
        last = previous;
    }
    if (node == first) {
        first = next;
    }
}

class ExprPrinter final {
public:
    void operator()(const ExprAnd& expr) {
        inner += "( ";
        std::visit(*this, *expr.operand1);
        inner += " && ";
        std::visit(*this, *expr.operand2);
        inner += ')';
    }

    void operator()(const ExprOr& expr) {
        inner += "( ";
        std::visit(*this, *expr.operand1);
        inner += " || ";
        std::visit(*this, *expr.operand2);
        inner += ')';
    }

    void operator()(const ExprNot& expr) {
        inner += "!";
        std::visit(*this, *expr.operand1);
    }

    void operator()(const ExprPredicate& expr) {
        inner += fmt::format("P{}", expr.predicate);
    }

    void operator()(const ExprCondCode& expr) {
        inner += fmt::format("CC{}", expr.cc);
    }

    void operator()(const ExprVar& expr) {
        inner += fmt::format("V{}", expr.var_index);
    }

    void operator()(const ExprBoolean& expr) {
        inner += expr.value ? "true" : "false";
    }

    void operator()(const ExprGprEqual& expr) {
        inner += fmt::format("(gpr_{} == {})", expr.gpr, expr.value);
    }

    const std::string& GetResult() const {
        return inner;
    }

private:
    std::string inner;
};

class ASTPrinter {
public:
    void operator()(const ASTProgram& ast) {
        scope++;
        inner += "program {\n";
        ASTNode current = ast.nodes.GetFirst();
        while (current) {
            Visit(current);
            current = current->GetNext();
        }
        inner += "}\n";
        scope--;
    }

    void operator()(const ASTIfThen& ast) {
        ExprPrinter expr_parser{};
        std::visit(expr_parser, *ast.condition);
        inner += fmt::format("{}if ({}) {{\n", Indent(), expr_parser.GetResult());
        scope++;
        ASTNode current = ast.nodes.GetFirst();
        while (current) {
            Visit(current);
            current = current->GetNext();
        }
        scope--;
        inner += fmt::format("{}}}\n", Indent());
    }

    void operator()(const ASTIfElse& ast) {
        inner += Indent();
        inner += "else {\n";

        scope++;
        ASTNode current = ast.nodes.GetFirst();
        while (current) {
            Visit(current);
            current = current->GetNext();
        }
        scope--;

        inner += Indent();
        inner += "}\n";
    }

    void operator()(const ASTBlockEncoded& ast) {
        inner += fmt::format("{}Block({}, {});\n", Indent(), ast.start, ast.end);
    }

    void operator()([[maybe_unused]] const ASTBlockDecoded& ast) {
        inner += Indent();
        inner += "Block;\n";
    }

    void operator()(const ASTVarSet& ast) {
        ExprPrinter expr_parser{};
        std::visit(expr_parser, *ast.condition);
        inner += fmt::format("{}V{} := {};\n", Indent(), ast.index, expr_parser.GetResult());
    }

    void operator()(const ASTLabel& ast) {
        inner += fmt::format("Label_{}:\n", ast.index);
    }

    void operator()(const ASTGoto& ast) {
        ExprPrinter expr_parser{};
        std::visit(expr_parser, *ast.condition);
        inner +=
            fmt::format("{}({}) -> goto Label_{};\n", Indent(), expr_parser.GetResult(), ast.label);
    }

    void operator()(const ASTDoWhile& ast) {
        ExprPrinter expr_parser{};
        std::visit(expr_parser, *ast.condition);
        inner += fmt::format("{}do {{\n", Indent());
        scope++;
        ASTNode current = ast.nodes.GetFirst();
        while (current) {
            Visit(current);
            current = current->GetNext();
        }
        scope--;
        inner += fmt::format("{}}} while ({});\n", Indent(), expr_parser.GetResult());
    }

    void operator()(const ASTReturn& ast) {
        ExprPrinter expr_parser{};
        std::visit(expr_parser, *ast.condition);
        inner += fmt::format("{}({}) -> {};\n", Indent(), expr_parser.GetResult(),
                             ast.kills ? "discard" : "exit");
    }

    void operator()(const ASTBreak& ast) {
        ExprPrinter expr_parser{};
        std::visit(expr_parser, *ast.condition);
        inner += fmt::format("{}({}) -> break;\n", Indent(), expr_parser.GetResult());
    }

    void Visit(const ASTNode& node) {
        std::visit(*this, *node->GetInnerData());
    }

    const std::string& GetResult() const {
        return inner;
    }

private:
    std::string_view Indent() {
        if (space_segment_scope == scope) {
            return space_segment;
        }

        // Ensure that we don't exceed our view.
        ASSERT(scope * 2 < spaces.size());

        space_segment = spaces.substr(0, scope * 2);
        space_segment_scope = scope;
        return space_segment;
    }

    std::string inner{};
    std::string_view space_segment;

    u32 scope{};
    u32 space_segment_scope{};

    static constexpr std::string_view spaces{"                                    "};
};

std::string ASTManager::Print() const {
    ASTPrinter printer{};
    printer.Visit(main_node);
    return printer.GetResult();
}

ASTManager::ASTManager(bool do_full_decompile, bool disable_else_derivation_)
    : full_decompile{do_full_decompile}, disable_else_derivation{disable_else_derivation_} {}

ASTManager::~ASTManager() {
    Clear();
}

void ASTManager::Init() {
    main_node = ASTBase::Make<ASTProgram>(ASTNode{});
    program = std::get_if<ASTProgram>(main_node->GetInnerData());
    false_condition = MakeExpr<ExprBoolean>(false);
}

void ASTManager::DeclareLabel(u32 address) {
    const auto pair = labels_map.emplace(address, labels_count);
    if (pair.second) {
        labels_count++;
        labels.resize(labels_count);
    }
}

void ASTManager::InsertLabel(u32 address) {
    const u32 index = labels_map[address];
    const ASTNode label = ASTBase::Make<ASTLabel>(main_node, index);
    labels[index] = label;
    program->nodes.PushBack(label);
}

void ASTManager::InsertGoto(Expr condition, u32 address) {
    const u32 index = labels_map[address];
    const ASTNode goto_node = ASTBase::Make<ASTGoto>(main_node, std::move(condition), index);
    gotos.push_back(goto_node);
    program->nodes.PushBack(goto_node);
}

void ASTManager::InsertBlock(u32 start_address, u32 end_address) {
    ASTNode block = ASTBase::Make<ASTBlockEncoded>(main_node, start_address, end_address);
    program->nodes.PushBack(std::move(block));
}

void ASTManager::InsertReturn(Expr condition, bool kills) {
    ASTNode node = ASTBase::Make<ASTReturn>(main_node, std::move(condition), kills);
    program->nodes.PushBack(std::move(node));
}

// The decompile algorithm is based on
// "Taming control flow: A structured approach to eliminating goto statements"
// by AM Erosa, LJ Hendren 1994. In general, the idea is to get gotos to be
// on the same structured level as the label which they jump to. This is done,
// through outward/inward movements and lifting. Once they are at the same
// level, you can enclose them in an "if" structure or a "do-while" structure.
void ASTManager::Decompile() {
    auto it = gotos.begin();
    while (it != gotos.end()) {
        const ASTNode goto_node = *it;
        const auto label_index = goto_node->GetGotoLabel();
        if (!label_index) {
            return;
        }
        const ASTNode label = labels[*label_index];
        if (!full_decompile) {
            // We only decompile backward jumps
            if (!IsBackwardsJump(goto_node, label)) {
                it++;
                continue;
            }
        }
        if (IndirectlyRelated(goto_node, label)) {
            while (!DirectlyRelated(goto_node, label)) {
                MoveOutward(goto_node);
            }
        }
        if (DirectlyRelated(goto_node, label)) {
            u32 goto_level = goto_node->GetLevel();
            const u32 label_level = label->GetLevel();
            while (label_level < goto_level) {
                MoveOutward(goto_node);
                goto_level--;
            }
            // TODO(Blinkhawk): Implement Lifting and Inward Movements
        }
        if (label->GetParent() == goto_node->GetParent()) {
            bool is_loop = false;
            ASTNode current = goto_node->GetPrevious();
            while (current) {
                if (current == label) {
                    is_loop = true;
                    break;
                }
                current = current->GetPrevious();
            }

            if (is_loop) {
                EncloseDoWhile(goto_node, label);
            } else {
                EncloseIfThen(goto_node, label);
            }
            it = gotos.erase(it);
            continue;
        }
        it++;
    }
    if (full_decompile) {
        for (const ASTNode& label : labels) {
            auto& manager = label->GetManager();
            manager.Remove(label);
        }
        labels.clear();
    } else {
        auto label_it = labels.begin();
        while (label_it != labels.end()) {
            bool can_remove = true;
            ASTNode label = *label_it;
            for (const ASTNode& goto_node : gotos) {
                const auto label_index = goto_node->GetGotoLabel();
                if (!label_index) {
                    return;
                }
                ASTNode& glabel = labels[*label_index];
                if (glabel == label) {
                    can_remove = false;
                    break;
                }
            }
            if (can_remove) {
                label->MarkLabelUnused();
            }
        }
    }
}

bool ASTManager::IsBackwardsJump(ASTNode goto_node, ASTNode label_node) const {
    u32 goto_level = goto_node->GetLevel();
    u32 label_level = label_node->GetLevel();
    while (goto_level > label_level) {
        goto_level--;
        goto_node = goto_node->GetParent();
    }
    while (label_level > goto_level) {
        label_level--;
        label_node = label_node->GetParent();
    }
    while (goto_node->GetParent() != label_node->GetParent()) {
        goto_node = goto_node->GetParent();
        label_node = label_node->GetParent();
    }
    ASTNode current = goto_node->GetPrevious();
    while (current) {
        if (current == label_node) {
            return true;
        }
        current = current->GetPrevious();
    }
    return false;
}

bool ASTManager::IndirectlyRelated(const ASTNode& first, const ASTNode& second) const {
    return !(first->GetParent() == second->GetParent() || DirectlyRelated(first, second));
}

bool ASTManager::DirectlyRelated(const ASTNode& first, const ASTNode& second) const {
    if (first->GetParent() == second->GetParent()) {
        return false;
    }
    const u32 first_level = first->GetLevel();
    const u32 second_level = second->GetLevel();
    u32 min_level;
    u32 max_level;
    ASTNode max;
    ASTNode min;
    if (first_level > second_level) {
        min_level = second_level;
        min = second;
        max_level = first_level;
        max = first;
    } else {
        min_level = first_level;
        min = first;
        max_level = second_level;
        max = second;
    }

    while (max_level > min_level) {
        max_level--;
        max = max->GetParent();
    }

    return min->GetParent() == max->GetParent();
}

void ASTManager::ShowCurrentState(std::string_view state) const {
    LOG_CRITICAL(HW_GPU, "\nState {}:\n\n{}\n", state, Print());
    SanityCheck();
}

void ASTManager::SanityCheck() const {
    for (const auto& label : labels) {
        if (!label->GetParent()) {
            LOG_CRITICAL(HW_GPU, "Sanity Check Failed");
        }
    }
}

void ASTManager::EncloseDoWhile(ASTNode goto_node, ASTNode label) {
    ASTZipper& zipper = goto_node->GetManager();
    const ASTNode loop_start = label->GetNext();
    if (loop_start == goto_node) {
        zipper.Remove(goto_node);
        return;
    }
    const ASTNode parent = label->GetParent();
    const Expr condition = goto_node->GetGotoCondition();
    zipper.DetachSegment(loop_start, goto_node);
    const ASTNode do_while_node = ASTBase::Make<ASTDoWhile>(parent, condition);
    ASTZipper* sub_zipper = do_while_node->GetSubNodes();
    sub_zipper->Init(loop_start, do_while_node);
    zipper.InsertAfter(do_while_node, label);
    sub_zipper->Remove(goto_node);
}

void ASTManager::EncloseIfThen(ASTNode goto_node, ASTNode label) {
    ASTZipper& zipper = goto_node->GetManager();
    const ASTNode if_end = label->GetPrevious();
    if (if_end == goto_node) {
        zipper.Remove(goto_node);
        return;
    }
    const ASTNode prev = goto_node->GetPrevious();
    const Expr condition = goto_node->GetGotoCondition();
    bool do_else = false;
    if (!disable_else_derivation && prev->IsIfThen()) {
        const Expr if_condition = prev->GetIfCondition();
        do_else = ExprAreEqual(if_condition, condition);
    }
    const ASTNode parent = label->GetParent();
    zipper.DetachSegment(goto_node, if_end);
    ASTNode if_node;
    if (do_else) {
        if_node = ASTBase::Make<ASTIfElse>(parent);
    } else {
        Expr neg_condition = MakeExprNot(condition);
        if_node = ASTBase::Make<ASTIfThen>(parent, neg_condition);
    }
    ASTZipper* sub_zipper = if_node->GetSubNodes();
    sub_zipper->Init(goto_node, if_node);
    zipper.InsertAfter(if_node, prev);
    sub_zipper->Remove(goto_node);
}

void ASTManager::MoveOutward(ASTNode goto_node) {
    ASTZipper& zipper = goto_node->GetManager();
    const ASTNode parent = goto_node->GetParent();
    ASTZipper& zipper2 = parent->GetManager();
    const ASTNode grandpa = parent->GetParent();
    const bool is_loop = parent->IsLoop();
    const bool is_else = parent->IsIfElse();
    const bool is_if = parent->IsIfThen();

    const ASTNode prev = goto_node->GetPrevious();
    const ASTNode post = goto_node->GetNext();

    const Expr condition = goto_node->GetGotoCondition();
    zipper.DetachSingle(goto_node);
    if (is_loop) {
        const u32 var_index = NewVariable();
        const Expr var_condition = MakeExpr<ExprVar>(var_index);
        const ASTNode var_node = ASTBase::Make<ASTVarSet>(parent, var_index, condition);
        const ASTNode var_node_init = ASTBase::Make<ASTVarSet>(parent, var_index, false_condition);
        zipper2.InsertBefore(var_node_init, parent);
        zipper.InsertAfter(var_node, prev);
        goto_node->SetGotoCondition(var_condition);
        const ASTNode break_node = ASTBase::Make<ASTBreak>(parent, var_condition);
        zipper.InsertAfter(break_node, var_node);
    } else if (is_if || is_else) {
        const u32 var_index = NewVariable();
        const Expr var_condition = MakeExpr<ExprVar>(var_index);
        const ASTNode var_node = ASTBase::Make<ASTVarSet>(parent, var_index, condition);
        const ASTNode var_node_init = ASTBase::Make<ASTVarSet>(parent, var_index, false_condition);
        if (is_if) {
            zipper2.InsertBefore(var_node_init, parent);
        } else {
            zipper2.InsertBefore(var_node_init, parent->GetPrevious());
        }
        zipper.InsertAfter(var_node, prev);
        goto_node->SetGotoCondition(var_condition);
        if (post) {
            zipper.DetachTail(post);
            const ASTNode if_node = ASTBase::Make<ASTIfThen>(parent, MakeExprNot(var_condition));
            ASTZipper* sub_zipper = if_node->GetSubNodes();
            sub_zipper->Init(post, if_node);
            zipper.InsertAfter(if_node, var_node);
        }
    } else {
        UNREACHABLE();
    }
    const ASTNode next = parent->GetNext();
    if (is_if && next && next->IsIfElse()) {
        zipper2.InsertAfter(goto_node, next);
        goto_node->SetParent(grandpa);
        return;
    }
    zipper2.InsertAfter(goto_node, parent);
    goto_node->SetParent(grandpa);
}

class ASTClearer {
public:
    ASTClearer() = default;

    void operator()(const ASTProgram& ast) {
        ASTNode current = ast.nodes.GetFirst();
        while (current) {
            Visit(current);
            current = current->GetNext();
        }
    }

    void operator()(const ASTIfThen& ast) {
        ASTNode current = ast.nodes.GetFirst();
        while (current) {
            Visit(current);
            current = current->GetNext();
        }
    }

    void operator()(const ASTIfElse& ast) {
        ASTNode current = ast.nodes.GetFirst();
        while (current) {
            Visit(current);
            current = current->GetNext();
        }
    }

    void operator()([[maybe_unused]] const ASTBlockEncoded& ast) {}

    void operator()(ASTBlockDecoded& ast) {
        ast.nodes.clear();
    }

    void operator()([[maybe_unused]] const ASTVarSet& ast) {}

    void operator()([[maybe_unused]] const ASTLabel& ast) {}

    void operator()([[maybe_unused]] const ASTGoto& ast) {}

    void operator()(const ASTDoWhile& ast) {
        ASTNode current = ast.nodes.GetFirst();
        while (current) {
            Visit(current);
            current = current->GetNext();
        }
    }

    void operator()([[maybe_unused]] const ASTReturn& ast) {}

    void operator()([[maybe_unused]] const ASTBreak& ast) {}

    void Visit(const ASTNode& node) {
        std::visit(*this, *node->GetInnerData());
        node->Clear();
    }
};

void ASTManager::Clear() {
    if (!main_node) {
        return;
    }
    ASTClearer clearer{};
    clearer.Visit(main_node);
    main_node.reset();
    program = nullptr;
    labels_map.clear();
    labels.clear();
    gotos.clear();
}

} // namespace VideoCommon::Shader
