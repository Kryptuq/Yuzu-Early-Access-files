/* $NetBSD: tree.h,v 1.8 2004/03/28 19:38:30 provos Exp $ */
/* $OpenBSD: tree.h,v 1.7 2002/10/17 21:51:54 art Exp $ */
/* $FreeBSD$ */

/*-
 * Copyright 2002 Niels Provos <provos@citi.umich.edu>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

/*
 * This file defines data structures for red-black trees.
 *
 * A red-black tree is a binary search tree with the node color as an
 * extra attribute.  It fulfills a set of conditions:
 * - every search path from the root to a leaf consists of the
 *   same number of black nodes,
 * - each red node (except for the root) has a black parent,
 * - each leaf node is black.
 *
 * Every operation on a red-black tree is bounded as O(lg n).
 * The maximum height of a red-black tree is 2lg (n+1).
 */

#include "common/assert.h"

namespace Common {
template <typename T>
class RBHead {
public:
    [[nodiscard]] T* Root() {
        return rbh_root;
    }

    [[nodiscard]] const T* Root() const {
        return rbh_root;
    }

    void SetRoot(T* root) {
        rbh_root = root;
    }

    [[nodiscard]] bool IsEmpty() const {
        return Root() == nullptr;
    }

private:
    T* rbh_root = nullptr;
};

enum class EntryColor {
    Black,
    Red,
};

template <typename T>
class RBEntry {
public:
    [[nodiscard]] T* Left() {
        return rbe_left;
    }

    [[nodiscard]] const T* Left() const {
        return rbe_left;
    }

    void SetLeft(T* left) {
        rbe_left = left;
    }

    [[nodiscard]] T* Right() {
        return rbe_right;
    }

    [[nodiscard]] const T* Right() const {
        return rbe_right;
    }

    void SetRight(T* right) {
        rbe_right = right;
    }

    [[nodiscard]] T* Parent() {
        return rbe_parent;
    }

    [[nodiscard]] const T* Parent() const {
        return rbe_parent;
    }

    void SetParent(T* parent) {
        rbe_parent = parent;
    }

    [[nodiscard]] bool IsBlack() const {
        return rbe_color == EntryColor::Black;
    }

    [[nodiscard]] bool IsRed() const {
        return rbe_color == EntryColor::Red;
    }

    [[nodiscard]] EntryColor Color() const {
        return rbe_color;
    }

    void SetColor(EntryColor color) {
        rbe_color = color;
    }

private:
    T* rbe_left = nullptr;
    T* rbe_right = nullptr;
    T* rbe_parent = nullptr;
    EntryColor rbe_color{};
};

template <typename Node>
[[nodiscard]] RBEntry<Node>& RB_ENTRY(Node* node) {
    return node->GetEntry();
}

template <typename Node>
[[nodiscard]] const RBEntry<Node>& RB_ENTRY(const Node* node) {
    return node->GetEntry();
}

template <typename Node>
[[nodiscard]] Node* RB_PARENT(Node* node) {
    return RB_ENTRY(node).Parent();
}

template <typename Node>
[[nodiscard]] const Node* RB_PARENT(const Node* node) {
    return RB_ENTRY(node).Parent();
}

template <typename Node>
void RB_SET_PARENT(Node* node, Node* parent) {
    return RB_ENTRY(node).SetParent(parent);
}

template <typename Node>
[[nodiscard]] Node* RB_LEFT(Node* node) {
    return RB_ENTRY(node).Left();
}

template <typename Node>
[[nodiscard]] const Node* RB_LEFT(const Node* node) {
    return RB_ENTRY(node).Left();
}

template <typename Node>
void RB_SET_LEFT(Node* node, Node* left) {
    return RB_ENTRY(node).SetLeft(left);
}

template <typename Node>
[[nodiscard]] Node* RB_RIGHT(Node* node) {
    return RB_ENTRY(node).Right();
}

template <typename Node>
[[nodiscard]] const Node* RB_RIGHT(const Node* node) {
    return RB_ENTRY(node).Right();
}

template <typename Node>
void RB_SET_RIGHT(Node* node, Node* right) {
    return RB_ENTRY(node).SetRight(right);
}

template <typename Node>
[[nodiscard]] bool RB_IS_BLACK(const Node* node) {
    return RB_ENTRY(node).IsBlack();
}

template <typename Node>
[[nodiscard]] bool RB_IS_RED(const Node* node) {
    return RB_ENTRY(node).IsRed();
}

template <typename Node>
[[nodiscard]] EntryColor RB_COLOR(const Node* node) {
    return RB_ENTRY(node).Color();
}

template <typename Node>
void RB_SET_COLOR(Node* node, EntryColor color) {
    return RB_ENTRY(node).SetColor(color);
}

template <typename Node>
void RB_SET(Node* node, Node* parent) {
    auto& entry = RB_ENTRY(node);
    entry.SetParent(parent);
    entry.SetLeft(nullptr);
    entry.SetRight(nullptr);
    entry.SetColor(EntryColor::Red);
}

template <typename Node>
void RB_SET_BLACKRED(Node* black, Node* red) {
    RB_SET_COLOR(black, EntryColor::Black);
    RB_SET_COLOR(red, EntryColor::Red);
}

template <typename Node>
void RB_ROTATE_LEFT(RBHead<Node>* head, Node* elm, Node*& tmp) {
    tmp = RB_RIGHT(elm);
    RB_SET_RIGHT(elm, RB_LEFT(tmp));
    if (RB_RIGHT(elm) != nullptr) {
        RB_SET_PARENT(RB_LEFT(tmp), elm);
    }

    RB_SET_PARENT(tmp, RB_PARENT(elm));
    if (RB_PARENT(tmp) != nullptr) {
        if (elm == RB_LEFT(RB_PARENT(elm))) {
            RB_SET_LEFT(RB_PARENT(elm), tmp);
        } else {
            RB_SET_RIGHT(RB_PARENT(elm), tmp);
        }
    } else {
        head->SetRoot(tmp);
    }

    RB_SET_LEFT(tmp, elm);
    RB_SET_PARENT(elm, tmp);
}

template <typename Node>
void RB_ROTATE_RIGHT(RBHead<Node>* head, Node* elm, Node*& tmp) {
    tmp = RB_LEFT(elm);
    RB_SET_LEFT(elm, RB_RIGHT(tmp));
    if (RB_LEFT(elm) != nullptr) {
        RB_SET_PARENT(RB_RIGHT(tmp), elm);
    }

    RB_SET_PARENT(tmp, RB_PARENT(elm));
    if (RB_PARENT(tmp) != nullptr) {
        if (elm == RB_LEFT(RB_PARENT(elm))) {
            RB_SET_LEFT(RB_PARENT(elm), tmp);
        } else {
            RB_SET_RIGHT(RB_PARENT(elm), tmp);
        }
    } else {
        head->SetRoot(tmp);
    }

    RB_SET_RIGHT(tmp, elm);
    RB_SET_PARENT(elm, tmp);
}

template <typename Node>
void RB_INSERT_COLOR(RBHead<Node>* head, Node* elm) {
    Node* parent = nullptr;
    Node* tmp = nullptr;

    while ((parent = RB_PARENT(elm)) != nullptr && RB_IS_RED(parent)) {
        Node* gparent = RB_PARENT(parent);
        if (parent == RB_LEFT(gparent)) {
            tmp = RB_RIGHT(gparent);
            if (tmp && RB_IS_RED(tmp)) {
                RB_SET_COLOR(tmp, EntryColor::Black);
                RB_SET_BLACKRED(parent, gparent);
                elm = gparent;
                continue;
            }

            if (RB_RIGHT(parent) == elm) {
                RB_ROTATE_LEFT(head, parent, tmp);
                tmp = parent;
                parent = elm;
                elm = tmp;
            }

            RB_SET_BLACKRED(parent, gparent);
            RB_ROTATE_RIGHT(head, gparent, tmp);
        } else {
            tmp = RB_LEFT(gparent);
            if (tmp && RB_IS_RED(tmp)) {
                RB_SET_COLOR(tmp, EntryColor::Black);
                RB_SET_BLACKRED(parent, gparent);
                elm = gparent;
                continue;
            }

            if (RB_LEFT(parent) == elm) {
                RB_ROTATE_RIGHT(head, parent, tmp);
                tmp = parent;
                parent = elm;
                elm = tmp;
            }

            RB_SET_BLACKRED(parent, gparent);
            RB_ROTATE_LEFT(head, gparent, tmp);
        }
    }

    RB_SET_COLOR(head->Root(), EntryColor::Black);
}

template <typename Node>
void RB_REMOVE_COLOR(RBHead<Node>* head, Node* parent, Node* elm) {
    Node* tmp;
    while ((elm == nullptr || RB_IS_BLACK(elm)) && elm != head->Root() && parent != nullptr) {
        if (RB_LEFT(parent) == elm) {
            tmp = RB_RIGHT(parent);
            if (!tmp) {
                ASSERT_MSG(false, "tmp is invalid!");
                break;
            }
            if (RB_IS_RED(tmp)) {
                RB_SET_BLACKRED(tmp, parent);
                RB_ROTATE_LEFT(head, parent, tmp);
                tmp = RB_RIGHT(parent);
            }

            if ((RB_LEFT(tmp) == nullptr || RB_IS_BLACK(RB_LEFT(tmp))) &&
                (RB_RIGHT(tmp) == nullptr || RB_IS_BLACK(RB_RIGHT(tmp)))) {
                RB_SET_COLOR(tmp, EntryColor::Red);
                elm = parent;
                parent = RB_PARENT(elm);
            } else {
                if (RB_RIGHT(tmp) == nullptr || RB_IS_BLACK(RB_RIGHT(tmp))) {
                    Node* oleft;
                    if ((oleft = RB_LEFT(tmp)) != nullptr) {
                        RB_SET_COLOR(oleft, EntryColor::Black);
                    }

                    RB_SET_COLOR(tmp, EntryColor::Red);
                    RB_ROTATE_RIGHT(head, tmp, oleft);
                    tmp = RB_RIGHT(parent);
                }

                RB_SET_COLOR(tmp, RB_COLOR(parent));
                RB_SET_COLOR(parent, EntryColor::Black);
                if (RB_RIGHT(tmp)) {
                    RB_SET_COLOR(RB_RIGHT(tmp), EntryColor::Black);
                }

                RB_ROTATE_LEFT(head, parent, tmp);
                elm = head->Root();
                break;
            }
        } else {
            tmp = RB_LEFT(parent);
            if (RB_IS_RED(tmp)) {
                RB_SET_BLACKRED(tmp, parent);
                RB_ROTATE_RIGHT(head, parent, tmp);
                tmp = RB_LEFT(parent);
            }

            if (!tmp) {
                ASSERT_MSG(false, "tmp is invalid!");
                break;
            }

            if ((RB_LEFT(tmp) == nullptr || RB_IS_BLACK(RB_LEFT(tmp))) &&
                (RB_RIGHT(tmp) == nullptr || RB_IS_BLACK(RB_RIGHT(tmp)))) {
                RB_SET_COLOR(tmp, EntryColor::Red);
                elm = parent;
                parent = RB_PARENT(elm);
            } else {
                if (RB_LEFT(tmp) == nullptr || RB_IS_BLACK(RB_LEFT(tmp))) {
                    Node* oright;
                    if ((oright = RB_RIGHT(tmp)) != nullptr) {
                        RB_SET_COLOR(oright, EntryColor::Black);
                    }

                    RB_SET_COLOR(tmp, EntryColor::Red);
                    RB_ROTATE_LEFT(head, tmp, oright);
                    tmp = RB_LEFT(parent);
                }

                RB_SET_COLOR(tmp, RB_COLOR(parent));
                RB_SET_COLOR(parent, EntryColor::Black);

                if (RB_LEFT(tmp)) {
                    RB_SET_COLOR(RB_LEFT(tmp), EntryColor::Black);
                }

                RB_ROTATE_RIGHT(head, parent, tmp);
                elm = head->Root();
                break;
            }
        }
    }

    if (elm) {
        RB_SET_COLOR(elm, EntryColor::Black);
    }
}

template <typename Node>
Node* RB_REMOVE(RBHead<Node>* head, Node* elm) {
    Node* child = nullptr;
    Node* parent = nullptr;
    Node* old = elm;
    EntryColor color{};

    const auto finalize = [&] {
        if (color == EntryColor::Black) {
            RB_REMOVE_COLOR(head, parent, child);
        }

        return old;
    };

    if (RB_LEFT(elm) == nullptr) {
        child = RB_RIGHT(elm);
    } else if (RB_RIGHT(elm) == nullptr) {
        child = RB_LEFT(elm);
    } else {
        Node* left;
        elm = RB_RIGHT(elm);
        while ((left = RB_LEFT(elm)) != nullptr) {
            elm = left;
        }

        child = RB_RIGHT(elm);
        parent = RB_PARENT(elm);
        color = RB_COLOR(elm);

        if (child) {
            RB_SET_PARENT(child, parent);
        }
        if (parent) {
            if (RB_LEFT(parent) == elm) {
                RB_SET_LEFT(parent, child);
            } else {
                RB_SET_RIGHT(parent, child);
            }
        } else {
            head->SetRoot(child);
        }

        if (RB_PARENT(elm) == old) {
            parent = elm;
        }

        elm->SetEntry(old->GetEntry());

        if (RB_PARENT(old)) {
            if (RB_LEFT(RB_PARENT(old)) == old) {
                RB_SET_LEFT(RB_PARENT(old), elm);
            } else {
                RB_SET_RIGHT(RB_PARENT(old), elm);
            }
        } else {
            head->SetRoot(elm);
        }
        RB_SET_PARENT(RB_LEFT(old), elm);
        if (RB_RIGHT(old)) {
            RB_SET_PARENT(RB_RIGHT(old), elm);
        }
        if (parent) {
            left = parent;
        }

        return finalize();
    }

    parent = RB_PARENT(elm);
    color = RB_COLOR(elm);

    if (child) {
        RB_SET_PARENT(child, parent);
    }
    if (parent) {
        if (RB_LEFT(parent) == elm) {
            RB_SET_LEFT(parent, child);
        } else {
            RB_SET_RIGHT(parent, child);
        }
    } else {
        head->SetRoot(child);
    }

    return finalize();
}

// Inserts a node into the RB tree
template <typename Node, typename CompareFunction>
Node* RB_INSERT(RBHead<Node>* head, Node* elm, CompareFunction cmp) {
    Node* parent = nullptr;
    Node* tmp = head->Root();
    int comp = 0;

    while (tmp) {
        parent = tmp;
        comp = cmp(elm, parent);
        if (comp < 0) {
            tmp = RB_LEFT(tmp);
        } else if (comp > 0) {
            tmp = RB_RIGHT(tmp);
        } else {
            return tmp;
        }
    }

    RB_SET(elm, parent);

    if (parent != nullptr) {
        if (comp < 0) {
            RB_SET_LEFT(parent, elm);
        } else {
            RB_SET_RIGHT(parent, elm);
        }
    } else {
        head->SetRoot(elm);
    }

    RB_INSERT_COLOR(head, elm);
    return nullptr;
}

// Finds the node with the same key as elm
template <typename Node, typename CompareFunction>
Node* RB_FIND(RBHead<Node>* head, Node* elm, CompareFunction cmp) {
    Node* tmp = head->Root();

    while (tmp) {
        const int comp = cmp(elm, tmp);
        if (comp < 0) {
            tmp = RB_LEFT(tmp);
        } else if (comp > 0) {
            tmp = RB_RIGHT(tmp);
        } else {
            return tmp;
        }
    }

    return nullptr;
}

// Finds the first node greater than or equal to the search key
template <typename Node, typename CompareFunction>
Node* RB_NFIND(RBHead<Node>* head, Node* elm, CompareFunction cmp) {
    Node* tmp = head->Root();
    Node* res = nullptr;

    while (tmp) {
        const int comp = cmp(elm, tmp);
        if (comp < 0) {
            res = tmp;
            tmp = RB_LEFT(tmp);
        } else if (comp > 0) {
            tmp = RB_RIGHT(tmp);
        } else {
            return tmp;
        }
    }

    return res;
}

// Finds the node with the same key as lelm
template <typename Node, typename CompareFunction>
Node* RB_FIND_LIGHT(RBHead<Node>* head, const void* lelm, CompareFunction lcmp) {
    Node* tmp = head->Root();

    while (tmp) {
        const int comp = lcmp(lelm, tmp);
        if (comp < 0) {
            tmp = RB_LEFT(tmp);
        } else if (comp > 0) {
            tmp = RB_RIGHT(tmp);
        } else {
            return tmp;
        }
    }

    return nullptr;
}

// Finds the first node greater than or equal to the search key
template <typename Node, typename CompareFunction>
Node* RB_NFIND_LIGHT(RBHead<Node>* head, const void* lelm, CompareFunction lcmp) {
    Node* tmp = head->Root();
    Node* res = nullptr;

    while (tmp) {
        const int comp = lcmp(lelm, tmp);
        if (comp < 0) {
            res = tmp;
            tmp = RB_LEFT(tmp);
        } else if (comp > 0) {
            tmp = RB_RIGHT(tmp);
        } else {
            return tmp;
        }
    }

    return res;
}

template <typename Node>
Node* RB_NEXT(Node* elm) {
    if (RB_RIGHT(elm)) {
        elm = RB_RIGHT(elm);
        while (RB_LEFT(elm)) {
            elm = RB_LEFT(elm);
        }
    } else {
        if (RB_PARENT(elm) && (elm == RB_LEFT(RB_PARENT(elm)))) {
            elm = RB_PARENT(elm);
        } else {
            while (RB_PARENT(elm) && (elm == RB_RIGHT(RB_PARENT(elm)))) {
                elm = RB_PARENT(elm);
            }
            elm = RB_PARENT(elm);
        }
    }
    return elm;
}

template <typename Node>
Node* RB_PREV(Node* elm) {
    if (RB_LEFT(elm)) {
        elm = RB_LEFT(elm);
        while (RB_RIGHT(elm)) {
            elm = RB_RIGHT(elm);
        }
    } else {
        if (RB_PARENT(elm) && (elm == RB_RIGHT(RB_PARENT(elm)))) {
            elm = RB_PARENT(elm);
        } else {
            while (RB_PARENT(elm) && (elm == RB_LEFT(RB_PARENT(elm)))) {
                elm = RB_PARENT(elm);
            }
            elm = RB_PARENT(elm);
        }
    }
    return elm;
}

template <typename Node>
Node* RB_MINMAX(RBHead<Node>* head, bool is_min) {
    Node* tmp = head->Root();
    Node* parent = nullptr;

    while (tmp) {
        parent = tmp;
        if (is_min) {
            tmp = RB_LEFT(tmp);
        } else {
            tmp = RB_RIGHT(tmp);
        }
    }

    return parent;
}

template <typename Node>
Node* RB_MIN(RBHead<Node>* head) {
    return RB_MINMAX(head, true);
}

template <typename Node>
Node* RB_MAX(RBHead<Node>* head) {
    return RB_MINMAX(head, false);
}
} // namespace Common
