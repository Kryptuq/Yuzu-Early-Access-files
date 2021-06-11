// Copyright 2016, VIXL authors
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

#include <iostream>
#include <set>
#include <sstream>
#include <vector>

#include "test-runner.h"

#include "cpu-features.h"
#include "utils-vixl.h"

#if __cplusplus >= 201103L
#include <type_traits>
#endif

#define TEST(name) TEST_(API_##name)

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

namespace vixl {

// Describe the result of a test. Should IsUintN() and IsIntN() return true or
// false for N and X?
template <typename T>
struct UintIntTest {
  bool is_uintn;
  bool is_intn;
  unsigned n;
  T x;
};

// Test IsUintN() and IsIntN() against various values and integral types.
TEST(IsUint_IsInt) {
  UintIntTest<uint32_t> test_little_values_unsigned[] = {
      {true, true, 1, UINT32_C(0x0)},   {true, false, 1, UINT32_C(0x1)},
      {false, false, 1, UINT32_C(0x2)}, {false, false, 1, UINT32_C(0x3)},
      {false, false, 1, UINT32_C(0x4)}, {false, false, 1, UINT32_C(0x5)},
      {false, false, 1, UINT32_C(0x6)}, {false, false, 1, UINT32_C(0x7)},
      {false, false, 1, UINT32_C(0x8)}, {false, false, 1, UINT32_C(0x9)},
      {false, false, 1, UINT32_C(0xa)}, {false, false, 1, UINT32_C(0xb)},
      {false, false, 1, UINT32_C(0xc)}, {false, false, 1, UINT32_C(0xd)},
      {false, false, 1, UINT32_C(0xe)}, {false, false, 1, UINT32_C(0xf)},

      {true, true, 2, UINT32_C(0x0)},   {true, true, 2, UINT32_C(0x1)},
      {true, false, 2, UINT32_C(0x2)},  {true, false, 2, UINT32_C(0x3)},
      {false, false, 2, UINT32_C(0x4)}, {false, false, 2, UINT32_C(0x5)},
      {false, false, 2, UINT32_C(0x6)}, {false, false, 2, UINT32_C(0x7)},
      {false, false, 2, UINT32_C(0x8)}, {false, false, 2, UINT32_C(0x9)},
      {false, false, 2, UINT32_C(0xa)}, {false, false, 2, UINT32_C(0xb)},
      {false, false, 2, UINT32_C(0xc)}, {false, false, 2, UINT32_C(0xd)},
      {false, false, 2, UINT32_C(0xe)}, {false, false, 2, UINT32_C(0xf)},
  };

  UintIntTest<int32_t> test_little_values_signed[] = {
      {true, true, 1, INT32_C(0)},    {true, false, 1, INT32_C(1)},
      {false, false, 1, INT32_C(2)},  {false, false, 1, INT32_C(3)},
      {false, false, 1, INT32_C(4)},  {false, false, 1, INT32_C(5)},
      {false, false, 1, INT32_C(6)},  {false, false, 1, INT32_C(7)},
      {false, true, 1, INT32_C(-1)},  {false, false, 1, INT32_C(-2)},
      {false, false, 1, INT32_C(-3)}, {false, false, 1, INT32_C(-4)},
      {false, false, 1, INT32_C(-5)}, {false, false, 1, INT32_C(-6)},
      {false, false, 1, INT32_C(-7)}, {false, false, 1, INT32_C(-8)},

      {true, true, 2, INT32_C(0)},    {true, true, 2, INT32_C(1)},
      {true, false, 2, INT32_C(2)},   {true, false, 2, INT32_C(3)},
      {false, false, 2, INT32_C(4)},  {false, false, 2, INT32_C(5)},
      {false, false, 2, INT32_C(6)},  {false, false, 2, INT32_C(7)},
      {false, true, 2, INT32_C(-1)},  {false, true, 2, INT32_C(-2)},
      {false, false, 2, INT32_C(-3)}, {false, false, 2, INT32_C(-4)},
      {false, false, 2, INT32_C(-5)}, {false, false, 2, INT32_C(-6)},
      {false, false, 2, INT32_C(-7)}, {false, false, 2, INT32_C(-8)},
  };

  UintIntTest<uint32_t> test_u16[] = {
      {true, true, 16, UINT32_C(0x0)},
      {true, false, 16, UINT32_C(0xabcd)},
      {true, false, 16, UINT32_C(0x8000)},
      {true, false, 16, UINT32_C(0xffff)},
      {false, false, 16, UINT32_C(0x10000)},
      {false, false, 16, UINT32_C(0xffff0000)},
      {false, false, 16, UINT32_C(0xffff8000)},
      {false, false, 16, UINT32_C(0xffffffff)},
  };

  UintIntTest<int32_t> test_i16[] = {
      {true, true, 16, INT32_C(0x0)},
      {true, false, 16, INT32_C(0xabcd)},
      {true, false, 16, INT32_C(0x8000)},
      {true, false, 16, INT32_C(0xffff)},
      {false, false, 16, INT32_C(0x10000)},
      {true, true, 16, INT32_C(42)},
      {false, true, 16, INT32_C(-42)},
      {false, true, 16, INT32_C(-1)},
  };

  UintIntTest<uint64_t> test_u32[] = {
      {true, true, 32, UINT64_C(0x0)},
      {true, false, 32, UINT64_C(0xabcdabcd)},
      {true, false, 32, UINT64_C(0x80000000)},
      {true, false, 32, UINT64_C(0xffffffff)},
  };

  UintIntTest<int64_t> test_i32[] = {
      {true, true, 32, INT64_C(0)},
      {true, true, 32, INT64_C(42)},
      {false, true, 32, INT64_C(-42)},
      {false, true, 32, INT64_C(-1)},
      {true, true, 32, INT64_C(2147483647)},    // (1 << (32 - 1)) - 1
      {false, true, 32, INT64_C(-2147483648)},  // -(1 << (32 - 1))
  };

  UintIntTest<uint64_t> test_unsigned_higher_than_32[] = {
      {false, false, 54, UINT64_C(0xabcdef9012345678)},
      {true, false, 33, UINT64_C(0x100000000)},
      {true, false, 62, UINT64_C(0x3fffffffffffffff)},
      {true, false, 63, UINT64_C(0x7fffffffffffffff)},
  };

  UintIntTest<int64_t> test_signed_higher_than_32[] = {
      {true, true, 54, INT64_C(9007199254740991)},   // (1 << (54 - 1)) - 1
      {true, false, 54, INT64_C(9007199254740992)},  // 1 << (54 - 1)
      {true, true, 33, INT64_C(4294967295)},         // (1 << (33 - 1) - 1)
      {false, true, 33, INT64_C(-4294967296)},       // -(1 << (33 - 1))
  };

#define TEST_LIST(M)              \
  M(test_little_values_unsigned)  \
  M(test_little_values_signed)    \
  M(test_u16)                     \
  M(test_i16)                     \
  M(test_u32)                     \
  M(test_i32)                     \
  M(test_unsigned_higher_than_32) \
  M(test_signed_higher_than_32)


#define TEST_UINT(test_vector)                                  \
  for (unsigned i = 0; i < ARRAY_SIZE(test_vector); i++) {      \
    if (test_vector[i].is_uintn) {                              \
      VIXL_CHECK(IsUintN(test_vector[i].n, test_vector[i].x));  \
    } else {                                                    \
      VIXL_CHECK(!IsUintN(test_vector[i].n, test_vector[i].x)); \
    }                                                           \
  }

#define TEST_INT(test_vector)                                  \
  for (unsigned i = 0; i < ARRAY_SIZE(test_vector); i++) {     \
    if (test_vector[i].is_intn) {                              \
      VIXL_CHECK(IsIntN(test_vector[i].n, test_vector[i].x));  \
    } else {                                                   \
      VIXL_CHECK(!IsIntN(test_vector[i].n, test_vector[i].x)); \
    }                                                          \
  }

  TEST_LIST(TEST_UINT)
  TEST_LIST(TEST_INT)

#undef TEST_UINT
#undef TEST_INT

#undef TEST_LIST
}


TEST(CPUFeatures_iterator_api) {
  // CPUFeaturesIterator does not fully satisfy the requirements of C++'s
  // iterator concepts, but it should implement enough for some basic usage.

  // Arbitrary feature lists.
  CPUFeatures f1(CPUFeatures::kFP, CPUFeatures::kNEON);
  CPUFeatures f2(CPUFeatures::kFP, CPUFeatures::kNEON, CPUFeatures::kCRC32);
  CPUFeatures f3;

  typedef CPUFeatures::const_iterator It;

  It it0;
  It it1_neon(&f1, CPUFeatures::kNEON);
  It it2_neon(&f2, CPUFeatures::kNEON);
  It it2_crc32(&f2, CPUFeatures::kCRC32);
  It it3(&f3);

  // Equality
  VIXL_CHECK(it0 == it0);
  VIXL_CHECK(it1_neon == it1_neon);
  VIXL_CHECK(it2_neon == it2_neon);
  VIXL_CHECK(it2_crc32 == it2_crc32);
  VIXL_CHECK(it3 == it3);
  VIXL_CHECK(!(it0 == it1_neon));
  VIXL_CHECK(!(it0 == it3));
  VIXL_CHECK(!(it1_neon == it2_neon));
  VIXL_CHECK(!(it1_neon == it2_crc32));
  VIXL_CHECK(!(it1_neon == it3));
  VIXL_CHECK(!(it2_neon == it2_crc32));
  VIXL_CHECK(!(it3 == it0));
  VIXL_CHECK(!(it3 == it1_neon));

  // Inequality
  //   (a == b)  <->  !(a != b)
  VIXL_CHECK(!(it0 != it0));
  VIXL_CHECK(!(it1_neon != it1_neon));
  VIXL_CHECK(!(it2_neon != it2_neon));
  VIXL_CHECK(!(it2_crc32 != it2_crc32));
  VIXL_CHECK(!(it3 != it3));
  //   !(a == b)  <->  (a != b)
  VIXL_CHECK(it0 != it1_neon);
  VIXL_CHECK(it0 != it3);
  VIXL_CHECK(it1_neon != it2_neon);
  VIXL_CHECK(it1_neon != it2_crc32);
  VIXL_CHECK(it1_neon != it3);
  VIXL_CHECK(it2_neon != it2_crc32);
  VIXL_CHECK(it3 != it0);
  VIXL_CHECK(it3 != it1_neon);

  // Dereferenceable
  VIXL_CHECK(*it0 == CPUFeatures::kNone);
  VIXL_CHECK(*it1_neon == CPUFeatures::kNEON);
  VIXL_CHECK(*it2_neon == CPUFeatures::kNEON);
  VIXL_CHECK(*it2_crc32 == CPUFeatures::kCRC32);
  VIXL_CHECK(*it3 == CPUFeatures::kNone);

#if __cplusplus >= 201103L
  VIXL_STATIC_ASSERT(std::is_copy_constructible<It>::value);
  VIXL_STATIC_ASSERT(std::is_copy_assignable<It>::value);
  VIXL_STATIC_ASSERT(std::is_destructible<It>::value);
#endif
  // Copy constructable
  It test0 = it0;
  It test1 = it1_neon;
  It test2(it2_neon);
  VIXL_CHECK(test0 == It(NULL, CPUFeatures::kNone));
  VIXL_CHECK(test1 == It(&f1, CPUFeatures::kNEON));
  VIXL_CHECK(test2 == It(&f2, CPUFeatures::kNEON));

  // Copy assignable
  test2 = it2_crc32;
  VIXL_CHECK(test2 == It(&f2, CPUFeatures::kCRC32));

  // Incrementable
  // - Incrementing has no effect on an empty CPUFeatures.
  VIXL_CHECK(*it3++ == CPUFeatures::kNone);
  VIXL_CHECK(*(++it3) == CPUFeatures::kNone);
  VIXL_CHECK(it3 == It(&f3, CPUFeatures::kNone));
  // - Incrementing moves to the next feature, wrapping around (through kNone).
  //   This test will need to be updated if the Feature enum is reordered.
  VIXL_CHECK(*it2_neon++ == CPUFeatures::kNEON);
  VIXL_CHECK(*it2_neon++ == CPUFeatures::kCRC32);
  VIXL_CHECK(*it2_neon++ == CPUFeatures::kNone);
  VIXL_CHECK(*it2_neon++ == CPUFeatures::kFP);
  VIXL_CHECK(it2_neon == It(&f2, CPUFeatures::kNEON));
  VIXL_CHECK(*(++it2_crc32) == CPUFeatures::kNone);
  VIXL_CHECK(*(++it2_crc32) == CPUFeatures::kFP);
  VIXL_CHECK(*(++it2_crc32) == CPUFeatures::kNEON);
  VIXL_CHECK(*(++it2_crc32) == CPUFeatures::kCRC32);
  VIXL_CHECK(it2_crc32 == It(&f2, CPUFeatures::kCRC32));
}


TEST(CPUFeatures_iterator_loops) {
  // Check that CPUFeaturesIterator can be used for some simple loops.

  // Arbitrary feature lists.
  CPUFeatures f1(CPUFeatures::kFP, CPUFeatures::kNEON);
  CPUFeatures f2(CPUFeatures::kFP, CPUFeatures::kNEON, CPUFeatures::kCRC32);
  CPUFeatures f3;

  // This test will need to be updated if the Feature enum is reordered.

  std::vector<CPUFeatures::Feature> f1_list;
  for (CPUFeatures::const_iterator it = f1.begin(); it != f1.end(); ++it) {
    f1_list.push_back(*it);
  }
  VIXL_CHECK(f1_list.size() == 2);
  VIXL_CHECK(f1_list[0] == CPUFeatures::kFP);
  VIXL_CHECK(f1_list[1] == CPUFeatures::kNEON);

  std::vector<CPUFeatures::Feature> f2_list;
  for (CPUFeatures::const_iterator it = f2.begin(); it != f2.end(); ++it) {
    f2_list.push_back(*it);
  }
  VIXL_CHECK(f2_list.size() == 3);
  VIXL_CHECK(f2_list[0] == CPUFeatures::kFP);
  VIXL_CHECK(f2_list[1] == CPUFeatures::kNEON);
  VIXL_CHECK(f2_list[2] == CPUFeatures::kCRC32);

  std::vector<CPUFeatures::Feature> f3_list;
  for (CPUFeatures::const_iterator it = f3.begin(); it != f3.end(); ++it) {
    f3_list.push_back(*it);
  }
  VIXL_CHECK(f3_list.size() == 0);

  std::vector<CPUFeatures::Feature> f2_list_cxx11;
  for (auto&& feature : f2) {
    f2_list_cxx11.push_back(feature);
  }
  VIXL_CHECK(f2_list_cxx11.size() == 3);
  VIXL_CHECK(f2_list_cxx11[0] == CPUFeatures::kFP);
  VIXL_CHECK(f2_list_cxx11[1] == CPUFeatures::kNEON);
  VIXL_CHECK(f2_list_cxx11[2] == CPUFeatures::kCRC32);

  std::vector<CPUFeatures::Feature> f3_list_cxx11;
  for (auto&& feature : f3) {
    f3_list_cxx11.push_back(feature);
  }
  VIXL_CHECK(f3_list_cxx11.size() == 0);
}


TEST(CPUFeatures_empty) {
  // A default-constructed CPUFeatures has no features enabled.
  CPUFeatures features;
  for (auto f : features) {
    USE(f);
    VIXL_ABORT();
  }
  VIXL_CHECK(features.HasNoFeatures());
  VIXL_CHECK(features.Count() == 0);
}


static void CPUFeaturesFormatHelper(const char* expected,
                                    const CPUFeatures& features) {
  std::stringstream os;
  os << features;
  std::string os_str = os.str();
  if (os_str != expected) {
    std::cout << "Found: " << os_str << "\n";
    std::cout << "Expected: " << expected << "\n";
    VIXL_ABORT();
  }
}


TEST(CPUFeatures_format) {
  // Check that the debug output is complete and accurate.

  // Individual features.
  CPUFeaturesFormatHelper("", CPUFeatures(CPUFeatures::kNone));
  CPUFeaturesFormatHelper("FP", CPUFeatures(CPUFeatures::kFP));
  CPUFeaturesFormatHelper("NEON", CPUFeatures(CPUFeatures::kNEON));
  CPUFeaturesFormatHelper("AES", CPUFeatures(CPUFeatures::kAES));
  CPUFeaturesFormatHelper("Pmull1Q", CPUFeatures(CPUFeatures::kPmull1Q));
  CPUFeaturesFormatHelper("SHA1", CPUFeatures(CPUFeatures::kSHA1));
  CPUFeaturesFormatHelper("SHA2", CPUFeatures(CPUFeatures::kSHA2));
  CPUFeaturesFormatHelper("CRC32", CPUFeatures(CPUFeatures::kCRC32));

  // Combinations of (arbitrary) features.
  // This test will need to be updated if the Feature enum is reordered.
  CPUFeatures f(CPUFeatures::kFP, CPUFeatures::kNEON);
  CPUFeaturesFormatHelper("FP, NEON", f);
  f.Combine(CPUFeatures::kCRC32);
  CPUFeaturesFormatHelper("FP, NEON, CRC32", f);
  f.Combine(CPUFeatures::kFcma);
  CPUFeaturesFormatHelper("FP, NEON, CRC32, Fcma", f);
  f.Combine(CPUFeatures::kSHA1);
  CPUFeaturesFormatHelper("FP, NEON, CRC32, SHA1, Fcma", f);
}


static void CPUFeaturesPredefinedResultCheckHelper(
    const std::set<CPUFeatures::Feature>& unexpected,
    const std::set<CPUFeatures::Feature>& expected) {
  // Print a helpful diagnostic before checking the result.
  if (!unexpected.empty()) {
    std::cout << "Unexpected features:\n";
    for (auto f : unexpected) {
      std::cout << "  " << f << "\n";
    }
  }
  if (!expected.empty()) {
    std::cout << "Missing features:\n";
    for (auto f : expected) {
      std::cout << "  " << f << "\n";
    }
  }
  VIXL_CHECK(unexpected.empty() && expected.empty());
}


TEST(CPUFeatures_predefined_legacy) {
  CPUFeatures features = CPUFeatures::AArch64LegacyBaseline();
  std::set<CPUFeatures::Feature> unexpected;
  std::set<CPUFeatures::Feature> expected;
  expected.insert(CPUFeatures::kFP);
  expected.insert(CPUFeatures::kNEON);
  expected.insert(CPUFeatures::kCRC32);

  for (auto f : features) {
    if (expected.erase(f) == 0) unexpected.insert(f);
  }
  CPUFeaturesPredefinedResultCheckHelper(unexpected, expected);
}


TEST(CPUFeatures_predefined_all) {
  CPUFeatures features = CPUFeatures::All();
  std::set<CPUFeatures::Feature> found;

  for (auto f : features) {
    found.insert(f);
  }
  VIXL_CHECK(found.size() == CPUFeatures::kNumberOfFeatures);
  VIXL_CHECK(found.size() == features.Count());
}

// The CPUFeaturesScope constructor is templated, and needs an object which
// implements `CPUFeatures* GetCPUFeatures()`. This is normally something like
// the Assembler, but for the tests we use an architecture-independent wrapper.
class GetCPUFeaturesWrapper {
 public:
  explicit GetCPUFeaturesWrapper(CPUFeatures* cpu_features)
      : cpu_features_(cpu_features) {}

  CPUFeatures* GetCPUFeatures() const { return cpu_features_; }

 private:
  CPUFeatures* cpu_features_;
};

TEST(CPUFeaturesScope) {
  // Test that CPUFeaturesScope properly preserves state.

  CPUFeatures cpu(CPUFeatures::kCRC32, CPUFeatures::kSHA1, CPUFeatures::kAES);
  GetCPUFeaturesWrapper top_level(&cpu);

  const CPUFeatures original_outer = cpu;

  {  // Test setting both new and existing features.
    CPUFeaturesScope outer(&top_level, CPUFeatures::kSHA2, CPUFeatures::kAES);
    VIXL_CHECK(outer.GetCPUFeatures() == &cpu);
    VIXL_CHECK(cpu.Has(CPUFeatures::kCRC32,
                       CPUFeatures::kSHA1,
                       CPUFeatures::kSHA2,
                       CPUFeatures::kAES));

    // Features can be added or removed directly, in the usual fashion.
    // (The scope will restore their original status when it ends.)
    cpu.Combine(CPUFeatures::kSHA1, CPUFeatures::kAtomics);
    VIXL_CHECK(cpu.Has(CPUFeatures::kCRC32,
                       CPUFeatures::kSHA1,
                       CPUFeatures::kSHA2,
                       CPUFeatures::kAES));
    VIXL_CHECK(cpu.Has(CPUFeatures::kAtomics));

    cpu.Remove(CPUFeatures::kSHA2, CPUFeatures::kAES);
    VIXL_CHECK(!cpu.Has(CPUFeatures::kSHA2, CPUFeatures::kAES));
    VIXL_CHECK(cpu.Has(CPUFeatures::kCRC32,
                       CPUFeatures::kSHA1,
                       CPUFeatures::kAtomics));

    const CPUFeatures original_inner = cpu;

    // Scopes can be nested.
    {
      // A CPUFeaturesScope can be constructed from a CPUFeatures*, or any
      // (non-local) object that implements `CPUFeatures* GetCPUFeatures()`.
      // Typically, this would be an Assembler or MacroAssembler, but
      // CPUFeatureScope itself provides this method, and allows the test to
      // remain architecture-agnostic.

      CPUFeatures auth(CPUFeatures::kPAuth,
                       CPUFeatures::kPAuthQARMA,
                       CPUFeatures::kPAuthGeneric,
                       CPUFeatures::kPAuthGenericQARMA,
                       CPUFeatures::kPAuthEnhancedPAC2,
                       CPUFeatures::kPAuthFPAC,
                       CPUFeatures::kPAuthFPACCombined);

      CPUFeaturesScope inner(&outer, auth);
      VIXL_CHECK(inner.GetCPUFeatures() == &cpu);
      VIXL_CHECK(cpu.Has(auth.With(CPUFeatures::kCRC32,
                                   CPUFeatures::kSHA1,
                                   CPUFeatures::kAtomics)));
    }
    // Check for equivalence.
    VIXL_CHECK(cpu.Has(original_inner));
    VIXL_CHECK(original_inner.Has(cpu));
  }

  {
    // Scopes can be initialised with no features.
    CPUFeaturesScope scope(&top_level);
  }

  // Check for equivalence.
  VIXL_CHECK(cpu.Has(original_outer));
  VIXL_CHECK(original_outer.Has(cpu));
}

TEST(CPUFeatures_infer_from_os) {
  // Test that CPUFeatures::InferFromOS functions on supported platforms.
  CPUFeatures os;
  VIXL_ASSERT(os.HasNoFeatures());
  os = CPUFeatures::InferFromOS();

  // Every real platform has FP and NEON. However, InferFromOS does not support
  // every platform, so we also have to tolerate empty results.
  if (os.HasNoFeatures()) {
    std::cout << "Warning: CPUFeatures::InferFromOS() returned no results.\n";
  } else {
    std::cout << "CPUFeatures::InferFromOS():\n  {" << os << "}\n";
    VIXL_CHECK(os.Has(CPUFeatures::kFP));
    VIXL_CHECK(os.Has(CPUFeatures::kNEON));
  }
}

TEST(CPUFeatures_infer_from_id_registers) {
  CPUFeatures os_only =
      CPUFeatures::InferFromOS(CPUFeatures::kDontQueryIDRegisters);
  std::cout << "CPUFeatures::InferFromOS(kDontQueryIDRegisters):\n  {"
            << os_only << "}\n";
  if (os_only.Has(CPUFeatures::kIDRegisterEmulation)) {
    CPUFeatures id_regs = CPUFeatures::InferFromIDRegisters();
    std::cout << "CPUFeatures::InferFromIDRegisters():\n  {" << id_regs
              << "}\n";
    // The ID registers should return at least as many features as the OS
    // information. This is intended to verify VIXL's InferFromIDRegisters
    // logic, but it also relies on the OS presenting consistent information.
    VIXL_CHECK(id_regs.Has(os_only));

    // The default InferFromOS should combine its results with
    // InferFromIDRegisters.
    CPUFeatures os_auto = CPUFeatures::InferFromOS();
    CPUFeatures os_with_id_regs = os_only.With(id_regs);
    // Check equivalence.
    VIXL_CHECK(os_auto.Has(os_with_id_regs));
    VIXL_CHECK(os_with_id_regs.Has(os_auto));
  } else {
    // Note: This message needs to match REGEXP_MISSING_FEATURES from
    // tools/threaded_test.py.
    std::cout << "SKIPPED: Missing features: { "
              << CPUFeatures::kIDRegisterEmulation << " }\n";
    std::cout << "This test requires the following features to run its "
                 "generated code on this CPU: "
              << CPUFeatures::kIDRegisterEmulation << "\n";
  }
}

}  // namespace vixl
