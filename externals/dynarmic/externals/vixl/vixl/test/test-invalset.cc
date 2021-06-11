// Copyright 2014, VIXL authors
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of ARM Limited nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "invalset-vixl.h"
#include "test-runner.h"

namespace vixl {

// This file contains tests for the `InvalSet` and `InvalSetIterator` classes.

#define TEST(name) TEST_(INVALSET_##name)

typedef ptrdiff_t KeyType;
typedef ptrdiff_t ValType;

// We test with an object for which the key and the value are distinct.
class Obj {
 public:
  Obj() {}
  Obj(KeyType key, ValType val) : key_(key), val_(val) {}
  KeyType key_;
  ValType val_;

  bool operator==(const Obj& other) const {
    return (key_ == other.key_) && (val_ == other.val_);
  }
  bool operator<(const Obj& other) const {
    return (key_ < other.key_) || ((key_ == other.key_) && (val_ < other.val_));
  }
  bool operator<=(const Obj& other) const {
    return (key_ <= other.key_) ||
           ((key_ == other.key_) && (val_ <= other.val_));
  }
  bool operator>(const Obj& other) const {
    return (key_ > other.key_) || ((key_ == other.key_) && (val_ > other.val_));
  }
};

static const unsigned kNPreallocatedElements = 8;
static const KeyType kInvalidKey = PTRDIFF_MAX;
static const size_t kReclaimFrom = 1000;
static const unsigned kReclaimFactor = 10;

typedef InvalSet<Obj,
                 kNPreallocatedElements,
                 KeyType,
                 kInvalidKey,
                 kReclaimFrom,
                 kReclaimFactor>
    TestSet;

template <>
inline KeyType InvalSet<Obj,
                        kNPreallocatedElements,
                        KeyType,
                        kInvalidKey,
                        kReclaimFrom,
                        kReclaimFactor>::GetKey(const Obj& obj) {
  return obj.key_;
}
template <>
inline void InvalSet<Obj,
                     kNPreallocatedElements,
                     KeyType,
                     kInvalidKey,
                     kReclaimFrom,
                     kReclaimFactor>::SetKey(Obj* obj, KeyType key) {
  obj->key_ = key;
}


TEST(basic_test) {
  TestSet set;
  VIXL_CHECK(set.empty() && (set.size() == 0));

  for (unsigned i = 0; i < kNPreallocatedElements; i++) {
    set.insert(Obj(i, i));
  }
  VIXL_CHECK(set.size() == kNPreallocatedElements);

  set.insert(Obj(-123, 456));
  set.insert(Obj(2718, 2871828));
  VIXL_CHECK(set.size() == kNPreallocatedElements + 2);
  VIXL_CHECK(set.GetMinElement() == Obj(-123, 456));

  set.erase(Obj(-123, 456));
  VIXL_CHECK(set.GetMinElementKey() == 0);

  set.clear();
  VIXL_CHECK(set.empty() && (set.size() == 0));
}


TEST(valid_element) {
  VIXL_CHECK(TestSet::IsValid(Obj(0, 0)));
  VIXL_CHECK(TestSet::IsValid(Obj(-1, 0)));
  VIXL_CHECK(TestSet::IsValid(Obj(kInvalidKey - 1, 0)));
  VIXL_CHECK(!TestSet::IsValid(Obj(kInvalidKey, 0)));
}


TEST(insert) {
  TestSet set;
  VIXL_CHECK(set.empty() && (set.size() == 0));

  for (unsigned i = 0; i < kNPreallocatedElements; i++) {
    set.insert(Obj(i, i));
  }
  VIXL_CHECK(set.size() == kNPreallocatedElements);
  set.insert(Obj(-123, 1));
  VIXL_CHECK(set.size() == kNPreallocatedElements + 1);
  set.insert(Obj(-123, 2));
  set.insert(Obj(-123, 3));
  VIXL_CHECK(set.size() == kNPreallocatedElements + 3);

  set.clear();
  VIXL_CHECK(set.empty() && (set.size() == 0));
}


TEST(erase) {
  TestSet set;
  VIXL_CHECK(set.empty() && (set.size() == 0));

  // Test with only preallocated elements in the set.
  VIXL_STATIC_ASSERT(kNPreallocatedElements >= 2);
  set.insert(Obj(2718, 0));
  set.erase(Obj(2718, 0));
  VIXL_CHECK(set.empty() && (set.size() == 0));
  set.insert(Obj(2718, 0));
  VIXL_CHECK(set.size() == 1);
  set.insert(Obj(2718, 1));
  VIXL_CHECK(set.size() == 2);
  set.erase(Obj(2718, 0));
  VIXL_CHECK(set.size() == 1);

  // Test with more elements.
  for (unsigned i = 0; i < 100 * kNPreallocatedElements; i++) {
    set.insert(Obj(i * i, i % 30));
    set.insert(Obj(i, -1));
  }
  size_t max_size = set.size();
  set.erase(Obj(100, -1));
  VIXL_CHECK(set.size() == max_size - 1);
  for (size_t i = 2; i <= max_size; i++) {
    set.erase(set.GetMinElement());
    VIXL_CHECK(set.size() == max_size - i);
  }

  VIXL_CHECK(set.empty() && (set.size() == 0));
}


TEST(min) {
  TestSet set;
  VIXL_CHECK(set.empty() && (set.size() == 0));

  // Test with only preallocated elements in the set.
  VIXL_STATIC_ASSERT(kNPreallocatedElements >= 4);
  set.insert(Obj(-1, -1));
  set.insert(Obj(-1, 0));
  set.insert(Obj(0, 0));
  set.insert(Obj(1, 0));
  VIXL_CHECK(set.GetMinElement() == Obj(-1, -1));
  VIXL_CHECK(set.GetMinElementKey() == -1);
  VIXL_CHECK(set.GetMinElement().key_ == set.GetMinElementKey());

  // Test with more elements.
  set.clear();
  int max_index = 100 * kNPreallocatedElements;
  for (int i = 0; i <= max_index; i++) {
    // Insert elements out of order.
    int sign = ((i % 2) == 0) ? -1 : 1;
    set.insert(Obj(sign * i, i));
  }
  VIXL_CHECK(set.GetMinElement() == Obj(-max_index, max_index));
  VIXL_CHECK(set.GetMinElement().key_ == set.GetMinElementKey());

  set.erase(Obj(0, 0));
  VIXL_CHECK(set.GetMinElement() == Obj(-max_index, max_index));
  set.erase(set.GetMinElement());
  VIXL_CHECK(set.GetMinElement() == Obj(-(max_index - 2), max_index - 2));

  set.clear();
  VIXL_CHECK(set.empty() && (set.size() == 0));
}


TEST(iterator) {
  TestSet set;
  VIXL_CHECK(set.empty() && (set.size() == 0));

  // Ensure that set.begin() == set.end() for the empty set.
  VIXL_CHECK(set.begin() == set.end());

  // Test with only preallocated elements in the set.
  size_t expected_total = 0;
  for (unsigned i = 0; i < kNPreallocatedElements; i++) {
    set.insert(Obj(i, i));
    expected_total += i;
  }

  size_t size = 0;
  size_t total = 0;
  for (InvalSetIterator<TestSet> it = set.begin(); it != set.end(); ++it) {
    total += it->val_;
    size++;
  }
  VIXL_CHECK(size == set.size());
  VIXL_CHECK(total == expected_total);

  // Test with more elements.
  for (unsigned i = kNPreallocatedElements; i < 4 * kNPreallocatedElements;
       i++) {
    set.insert(Obj(i, i));
    expected_total += i;
  }

  size = 0;
  total = 0;
  for (InvalSetIterator<TestSet> it = set.begin(); it != set.end(); ++it) {
    total += it->val_;
    size++;
  }
  VIXL_CHECK(size == set.size());
  VIXL_CHECK(total == expected_total);

  // Test after elements have been deleted.
  // - Select an interesting list of elements to erase.
  std::vector<Obj> to_erase;
  unsigned step = 0;
  for (unsigned i = 0; i < set.size(); i += step, step++) {
    to_erase.push_back(Obj(i, i));
  }
  to_erase.push_back(Obj(set.size() - 1, set.size() - 1));  // The last element.
  to_erase.push_back(Obj(set.size(), set.size()));          // Not in the set.

  // - Erase one at a time, retesting after each one.
  while (!to_erase.empty()) {
    size_t erased = set.erase(to_erase.back());
    if (erased > 0) {
      VIXL_CHECK(erased == 1);
      expected_total -= to_erase.back().val_;
    } else {
    }
    to_erase.pop_back();

    size = 0;
    total = 0;
    for (InvalSetIterator<TestSet> it = set.begin(); it != set.end(); ++it) {
      total += it->val_;
      size++;
    }
    VIXL_CHECK(size == set.size());
    VIXL_CHECK(total == expected_total);
  }
}


#if __cplusplus >= 201103L
TEST(iterator_cxx11) {
  TestSet set;
  VIXL_CHECK(set.empty() && (set.size() == 0));

  // Test with only preallocated elements in the set.
  size_t expected_total = 0;
  for (unsigned i = 0; i < kNPreallocatedElements; i++) {
    set.insert(Obj(i, i));
    expected_total += i;
  }

  size_t size = 0;
  size_t total = 0;
  for (auto object : set) {
    total += object.val_;
    size++;
  }
  VIXL_CHECK(size == set.size());
  VIXL_CHECK(total == expected_total);

  // Test with more elements.
  for (unsigned i = kNPreallocatedElements; i < 4 * kNPreallocatedElements;
       i++) {
    set.insert(Obj(i, i));
    expected_total += i;
  }

  size = 0;
  total = 0;
  for (auto object : set) {
    total += object.val_;
    size++;
  }
  VIXL_CHECK(size == set.size());
  VIXL_CHECK(total == expected_total);

  // Test after elements have been deleted.
  // - Select an interesting list of elements to erase.
  std::vector<Obj> to_erase;
  unsigned step = 0;
  for (unsigned i = 0; i < set.size(); i += step, step++) {
    to_erase.push_back(Obj(i, i));
  }
  to_erase.push_back(Obj(set.size() - 1, set.size() - 1));  // The last element.
  to_erase.push_back(Obj(set.size(), set.size()));          // Not in the set.

  // - Erase one at a time, retesting after each one.
  while (!to_erase.empty()) {
    size_t erased = set.erase(to_erase.back());
    if (erased > 0) {
      VIXL_CHECK(erased == 1);
      expected_total -= to_erase.back().val_;
    } else {
    }
    to_erase.pop_back();

    size = 0;
    total = 0;
    for (auto object : set) {
      total += object.val_;
      size++;
    }
    VIXL_CHECK(size == set.size());
    VIXL_CHECK(total == expected_total);
  }
}
#endif


TEST(stl_forward_iterator) {
  {
    TestSet::iterator default_it;           // Default-constructible.
    TestSet::iterator copy_it(default_it);  // Copy-constructible.
    copy_it = default_it;                   // Copy-assignable.
  }                                         // Destructible.

  TestSet set1;
  VIXL_CHECK(set1.empty() && (set1.size() == 0));

  TestSet set2;
  VIXL_CHECK(set2.empty() && (set2.size() == 0));

  // Test with only preallocated elements in the set.
  for (unsigned i = 0; i < kNPreallocatedElements; i++) {
    set1.insert(Obj(i, 1));
    set2.insert(Obj(i, 2));
  }

  TestSet::iterator it1_a = set1.begin();
  TestSet::iterator it1_b = set1.begin();

  // Incrementable (whilst valid).
  it1_a++;
  ++it1_b;

  // Testable for equivalence.
  VIXL_CHECK(it1_a == it1_b);
  VIXL_CHECK(set1.begin() != set1.end());
  VIXL_CHECK(set2.begin() != set2.end());
  VIXL_CHECK(set1.begin() != set2.begin());
  VIXL_CHECK(set1.end() != set2.end());

  // Dereferencable.
  VIXL_CHECK((*it1_a++).key_ == 1);
  VIXL_CHECK(((*it1_a).key_ == 2) && ((*it1_a).val_ == 1));
  *it1_b = Obj(42, 1);
  VIXL_CHECK((it1_b->key_ == 42) && (it1_b->val_ == 1));

#if __cplusplus >= 201103L
  // Swappable.
  std::swap(it1_a, it1_b);
  VIXL_CHECK(it1_a->key_ == 42);
  VIXL_CHECK(it1_b->key_ == 2);
#endif
}


}  // namespace vixl
