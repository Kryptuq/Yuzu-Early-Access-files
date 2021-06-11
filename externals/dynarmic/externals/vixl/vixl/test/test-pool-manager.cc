// Copyright 2017, VIXL authors
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

#include "test-pool-manager.h"

#include <stdio.h>

#include "pool-manager-impl.h"
#include "pool-manager.h"
#include "test-runner.h"

#define TEST(Name) TEST_(POOL_MANAGER_##Name)

#define IF_VERBOSE(exp) \
  if (Test::verbose()) exp

#define BUFFER_ALIGNMENT 16

using namespace vixl;

static int Random() { return static_cast<int>(std::abs(mrand48())); }

static int RandomObjectID(size_t num_objects) { return Random() % num_objects; }

static int RandomObjectSize() { return 1 + Random() % 256; }

static int RandomObjectAlignment(int size) {
  const int limit = static_cast<int>(floor(log2(BUFFER_ALIGNMENT)));
  int log2Size = static_cast<int>(floor(log2(size)));
  // Restrict alignment due to buffer alignment.
  log2Size = std::min(log2Size, limit);
  return (1 << (Random() % (1 + log2Size)));
}

// The size of the instruction.
static int RandomReferenceSize() { return (Random() % 2) ? 2 : 4; }

// The alignment of an instruction is either 2 or 4.
static int RandomInstructionAlignment() { return (Random() % 2) ? 2 : 4; }

static int32_t RandomMinOffset() {
  const int N = 3;
  static const int offsets[N] = {0, 2, 4};
  return offsets[Random() % N];
}

static int32_t RandomMaxOffset() {
  const int N = 5;
  static const int offsets[N] = {255, 1020, 1024, 4096, 16384};
  return offsets[Random() % N];
}

static int32_t RandomBranchMaxOffset() {
  const int N = 10;
  // The maximum offsets used for testing are taken from A32 and T32.
  static const int offsets[N] =
      {126, 254, 255, 1020, 1024, 2046, 4095, 1048574, 16777214, 33554428};
  return offsets[Random() % N];
}

static int RandomPCIncrement() {
  // A multiple of two.
  return 2 * (Random() % 4 + 1);
}

class TestObject : public LocationBase<int32_t> {
 public:
  TestObject(int size, int alignment, int id = 0)
      : LocationBase(0 /*type*/, size, alignment), id_(id) {}

  void EmitPoolObject(MacroAssemblerInterface *masm) VIXL_OVERRIDE {
    USE(masm);
  }

  bool ShouldDeletePoolObjectOnPlacement() const VIXL_OVERRIDE { return true; }

  // Update the references to this object.
  void ResolveReferences(internal::AssemblerBase *assembler) VIXL_OVERRIDE {
    int32_t location = GetLocation();
    USE(assembler);
    for (std::vector<ForwardReference<int32_t> *>::iterator iter =
             references_.begin();
         iter != references_.end();) {
      ForwardReference<int32_t> *ref = *iter;
      VIXL_ASSERT(ref->LocationIsEncodable(location));
      delete ref;
      iter = references_.erase(iter);
    }
    IF_VERBOSE(printf("Placed object %d at location: 0x%x (%u)\n",
                      id_,
                      location,
                      location));
  }

  void AddReference(ForwardReference<int32_t> *ref) {
    references_.push_back(ref);
  }

  int GetID() { return id_; }

  static TestObject *CreateRandom(int id) {
    int size = RandomObjectSize();
    int alignment = RandomObjectAlignment(size);
    IF_VERBOSE(printf("Object %d -> size = %d, alignment = %d\n",
                      id,
                      size,
                      alignment));
    return new TestObject(size, alignment, id);
  }

 private:
  // Store pointers to ForwardReference objects - TestObject is responsible
  // for deleting them.
  std::vector<ForwardReference<int32_t> *> references_;
  // Object id used for debugging.
  int id_;
};

class TestBranchObject : public LocationBase<int32_t> {
 public:
  TestBranchObject(int size, int alignment, int id = 0)
      : LocationBase(1 /* type */, size, alignment), id_(id) {}

  bool UsePoolObjectEmissionMargin() const VIXL_OVERRIDE { return true; }
  int32_t GetPoolObjectEmissionMargin() const VIXL_OVERRIDE {
    return 1 * KBytes;
  }

  // Do nothing for now.
  void EmitPoolObject(MacroAssemblerInterface *masm) VIXL_OVERRIDE {
    USE(masm);
  }

  bool ShouldDeletePoolObjectOnPlacement() const VIXL_OVERRIDE { return false; }

  virtual void UpdatePoolObject(PoolObject<int32_t> *object) VIXL_OVERRIDE {
    // Reference from the last emitted veneer:
    int32_t min = location_ + min_offset_;
    int32_t max = location_ + max_offset_;
    // The alignment that the new "veneer" requires of the label.
    int reference_alignment = RandomInstructionAlignment();
    reference_alignment =
        std::max(reference_alignment, GetPoolObjectAlignment());
    ForwardReference<int32_t> *ref =
        new ForwardReference<int32_t>(location_,
                                      4 /*size*/,
                                      min,
                                      max,
                                      reference_alignment);
    AddReference(ref);
    object->Update(min, max, reference_alignment);
  }

  // Update the references to this object.
  void ResolveReferences(internal::AssemblerBase *assembler) VIXL_OVERRIDE {
    int32_t location = GetLocation();
    USE(assembler);
    for (std::vector<ForwardReference<int32_t> *>::iterator iter =
             references_.begin();
         iter != references_.end();) {
      ForwardReference<int32_t> *ref = *iter;
      VIXL_ASSERT(ref->LocationIsEncodable(location));
      delete ref;
      iter = references_.erase(iter);
    }
    IF_VERBOSE(printf("Veneer %d placed at location: 0x%x (%u)\n",
                      id_,
                      location,
                      location));
  }

  void AddReference(ForwardReference<int32_t> *ref) {
    references_.push_back(ref);
  }

  virtual int GetMaxAlignment() const VIXL_OVERRIDE {
    int max_alignment = GetPoolObjectAlignment();
    for (std::vector<ForwardReference<int32_t> *>::const_iterator iter =
             references_.begin();
         iter != references_.end();
         ++iter) {
      const ForwardReference<int32_t> *ref = *iter;
      if (ref->GetAlignment() > max_alignment)
        max_alignment = ref->GetAlignment();
    }
    return max_alignment;
  }
  virtual int32_t GetMinLocation() const VIXL_OVERRIDE {
    int32_t min_location = 0;
    for (std::vector<ForwardReference<int32_t> *>::const_iterator iter =
             references_.begin();
         iter != references_.end();
         ++iter) {
      const ForwardReference<int32_t> *ref = *iter;
      if (ref->GetMinLocation() > min_location)
        min_location = ref->GetMinLocation();
    }
    return min_location;
  }

  int GetID() { return id_; }

  static TestBranchObject *CreateRandom(int id) {
    int size = RandomReferenceSize();
    int alignment = size;
    IF_VERBOSE(printf("Object %d -> size = %d, alignment = %d\n",
                      id,
                      size,
                      alignment));
    return new TestBranchObject(size, alignment, id);
  }

 private:
  // Store pointers to ForwardReference objects - TestBranchObject is
  // responsible for deleting them.
  std::vector<ForwardReference<int32_t> *> references_;
  // Object id used for debugging.
  int id_;

  // These are the min and max offsets of the type of branch used for the
  // veneer.
  static const int32_t min_offset_ = 0;
  static const int32_t max_offset_ = 16 * 1024 * 1024;
};

// MacroAssembler implementation that does nothing but print in verbose mode.
class TestMacroAssembler : public MacroAssemblerInterface {
 public:
  TestMacroAssembler() : assembler_(128) {}

  void EmitPoolHeader() VIXL_OVERRIDE {
    IF_VERBOSE(printf("[MASM] Emitting pool header.\n"));
  }
  void EmitPoolFooter() VIXL_OVERRIDE {
    IF_VERBOSE(printf("[MASM] Emitting pool footer.\n"));
  }
  void EmitPaddingBytes(int n) VIXL_OVERRIDE {
    IF_VERBOSE(printf("[MASM] Added %d bytes of padding.\n", n));
  }
  void EmitNopBytes(int n) VIXL_OVERRIDE {
    IF_VERBOSE(printf("[MASM] Added %d bytes of NOPs.\n", n));
  }
  bool ArePoolsBlocked() const VIXL_OVERRIDE { return false; }
  bool AllowMacroInstructions() const VIXL_OVERRIDE { return false; }
  void SetAllowMacroInstructions(bool allow) VIXL_OVERRIDE { USE(allow); }

  void BlockPools() VIXL_OVERRIDE {}
  void ReleasePools() VIXL_OVERRIDE {}
  void EnsureEmitPoolsFor(size_t) VIXL_OVERRIDE {}
  internal::AssemblerBase *AsAssemblerBase() VIXL_OVERRIDE {
    return &assembler_;
  }

 private:
  internal::AssemblerBase assembler_;
};

// Used for debugging.
namespace vixl {
template <>
void PoolManager<int32_t>::DumpCurrentState(int32_t pc) const {
  IF_VERBOSE(
      printf("Number of objects: %d\n", static_cast<int>(objects_.size())));
  IF_VERBOSE(printf("Current pc = 0x%x (%d)\n", pc, pc));

  for (int i = 0; i < static_cast<int>(objects_.size()); ++i) {
    const PoolObject<int32_t> &object = objects_[i];
    IF_VERBOSE(
        printf("Object %d -> size = %d, alignment = %d, range = (%d,%d)\n",
               i,
               object.label_base_->GetPoolObjectSizeInBytes(),
               object.alignment_,
               object.min_location_,
               object.max_location_));
  }
}
}

// Basic test - checks that emitting a very simple pool works.
TEST(Basic) {
  TestMacroAssembler masm;

  PoolManager<int32_t> pool_manager(4 /*header_size*/,
                                    2 /*header_alignment*/,
                                    BUFFER_ALIGNMENT);
  TestObject object1(4 /*size*/, 4 /*alignment*/);
  TestObject object2(128 /*size*/, 4 /*alignment*/);
  ForwardReference<int32_t> *ref1_obj1 =
      new ForwardReference<int32_t>(0 /*location*/, 2 /*size*/, 0, 200);
  ForwardReference<int32_t> *ref2_obj1 =
      new ForwardReference<int32_t>(2 /*location*/, 2 /*size*/, 2, 202);
  ForwardReference<int32_t> *ref3_obj1 =
      new ForwardReference<int32_t>(4 /*location*/, 2 /*size*/, 4, 204);
  object1.AddReference(ref1_obj1);
  object1.AddReference(ref2_obj1);
  object1.AddReference(ref3_obj1);
  ForwardReference<int32_t> *ref1_obj2 =
      new ForwardReference<int32_t>(8 /*location*/, 2 /*size*/, 8, 500);
  ForwardReference<int32_t> *ref2_obj2 =
      new ForwardReference<int32_t>(12 /*location*/, 4 /*size*/, 12, 300);
  ForwardReference<int32_t> *ref3_obj2 =
      new ForwardReference<int32_t>(16 /*location*/, 4 /*size*/, 16, 400);
  object2.AddReference(ref1_obj2);
  object2.AddReference(ref2_obj2);
  object2.AddReference(ref3_obj2);

  pool_manager.AddObjectReference(ref1_obj1, &object1);
  pool_manager.AddObjectReference(ref2_obj1, &object1);
  pool_manager.AddObjectReference(ref3_obj1, &object1);
  pool_manager.AddObjectReference(ref1_obj2, &object2);
  pool_manager.AddObjectReference(ref2_obj2, &object2);
  pool_manager.AddObjectReference(ref3_obj2, &object2);

  pool_manager.Emit(&masm, 20);
}

static ForwardReference<int32_t> *CreateReference(int id,
                                                  int32_t pc,
                                                  int size,
                                                  int32_t min_offset,
                                                  int32_t max_offset,
                                                  int alignment) {
  IF_VERBOSE(printf(
      "About to add a new reference to object %d with min location = %d, max "
      "location = %d, alignment = %d, size = %d\n",
      id,
      min_offset + pc,
      max_offset + pc,
      alignment,
      size));
  return new ForwardReference<int32_t>(pc,
                                       size,
                                       min_offset + pc,
                                       max_offset + pc,
                                       alignment);
}

// Fuzz test that uses literal-like objects, that get deleted when they are
// placed.
TEST(FuzzObjectDeletedWhenPlaced) {
  TestMacroAssembler masm;
  PoolManager<int32_t> pool_manager(4 /*header_size*/,
                                    2 /*header_alignment*/,
                                    BUFFER_ALIGNMENT);

  const int kObjectNum = 100;
  std::vector<TestObject *> objects;

  // Create objects.
  for (int i = 0; i < kObjectNum; ++i) {
    objects.push_back(TestObject::CreateRandom(i));
  }

  int32_t pc = 0;
  for (int i = 0; !objects.empty(); ++i) {
    IF_VERBOSE(printf("PC = 0x%x (%d)\n", pc, pc));
    int32_t pc_increment = RandomPCIncrement();
    IF_VERBOSE(printf("Attempting to increment PC by %d\n", pc_increment));
    if (pool_manager.MustEmit(pc, pc_increment)) {
      pc = pool_manager.Emit(&masm, pc, pc_increment);
    }
    pc += pc_increment;
    // Pick an object, randomly.
    TestObject *object = objects[RandomObjectID(objects.size())];
    int32_t min_offset = RandomMinOffset();
    int32_t max_offset = RandomMaxOffset();
    int32_t size = RandomReferenceSize();
    int32_t alignment =
        RandomObjectAlignment(object->GetPoolObjectSizeInBytes());
    ForwardReference<int32_t> *ref = CreateReference(object->GetID(),
                                                     pc,
                                                     size,
                                                     min_offset,
                                                     max_offset,
                                                     alignment);
    if (pool_manager.MustEmit(pc, size, ref, object)) {
      pc = pool_manager.Emit(&masm, pc, size, ref, object);
      delete ref;
      // We must recreate the reference, the PC has changed, but only if
      // it still is a forward reference.
      if (!object->IsBound()) {
        ref = CreateReference(object->GetID(),
                              pc,
                              size,
                              min_offset,
                              max_offset,
                              alignment);
      }
    }
    IF_VERBOSE(printf("Incrementing PC by size of reference (%d).\n", size));
    pc += size;
    // We only need to track the reference if it's a forward reference.
    if (!object->IsBound()) {
      object->AddReference(ref);
      pool_manager.AddObjectReference(ref, object);
    }
    VIXL_ASSERT(!pool_manager.MustEmit(pc - 1));
    // Remove bound objects.
    for (std::vector<TestObject *>::iterator iter = objects.begin();
         iter != objects.end();) {
      TestObject *object = *iter;
      if (object->IsBound()) {
        delete object;
        iter = objects.erase(iter);
      } else {
        ++iter;
      }
    }
  }

  pool_manager.Emit(&masm, pc);
}

// Fuzz test that uses veneer-like objects, that get updated when they are
// placed and get deleted when they are bound by the user.
TEST(FuzzObjectUpdatedWhenPlaced) {
  TestMacroAssembler masm;
  PoolManager<int32_t> pool_manager(4 /*header_size*/,
                                    2 /*header_alignment*/,
                                    BUFFER_ALIGNMENT);
  const int kObjectNum = 1000;
  std::vector<TestBranchObject *> objects;

  // Create objects.
  for (int i = 0; i < kObjectNum; ++i) {
    objects.push_back(TestBranchObject::CreateRandom(i));
  }

  int32_t pc = 0;
  for (int i = 0; !objects.empty(); ++i) {
    IF_VERBOSE(printf("PC = 0x%x (%d)\n", pc, pc));

    int32_t pc_increment = RandomPCIncrement();
    IF_VERBOSE(printf("Attempting to increment PC by %d\n", pc_increment));

    if (pool_manager.MustEmit(pc, pc_increment)) {
      pc = pool_manager.Emit(&masm, pc, pc_increment);
    }
    pc += pc_increment;

    // Pick a random object.
    TestBranchObject *object = objects[RandomObjectID(objects.size())];
    int32_t min_offset = RandomMinOffset();
    int32_t max_offset = RandomBranchMaxOffset();
    int32_t size = RandomReferenceSize();
    int32_t alignment =
        RandomObjectAlignment(object->GetPoolObjectSizeInBytes());
    ForwardReference<int32_t> *ref = CreateReference(object->GetID(),
                                                     pc,
                                                     size,
                                                     min_offset,
                                                     max_offset,
                                                     alignment);
    if (pool_manager.MustEmit(pc, size, ref, object)) {
      pc = pool_manager.Emit(&masm, pc, size);
      delete ref;
      // We must recreate the reference, the PC has changed.
      ref = CreateReference(object->GetID(),
                            pc,
                            size,
                            min_offset,
                            max_offset,
                            alignment);
    }
    IF_VERBOSE(printf("Incrementing PC by size of reference (%d).\n", size));
    pc += size;
    object->AddReference(ref);
    pool_manager.AddObjectReference(ref, object);
    VIXL_ASSERT(!pool_manager.MustEmit(pc - 1));

    // Pick another random label to bind.
    const int kProbabilityToBind = 20;
    if ((Random() % 100) < kProbabilityToBind) {
      TestBranchObject *object = objects[RandomObjectID(objects.size())];
      // Binding can cause the pool emission, so check if we need to emit
      // the pools. The actual backends will know the max alignment we
      // might need here, so can simplify the check (won't need to check
      // the object references).
      int max_padding = object->GetMaxAlignment() - 1;
      if (pool_manager.MustEmit(pc, max_padding)) {
        pc = pool_manager.Emit(&masm, pc, max_padding);
      }
      pc = pool_manager.Bind(&masm, object, pc);
    }

    // Remove bound objects.
    for (std::vector<TestBranchObject *>::iterator iter = objects.begin();
         iter != objects.end();) {
      TestBranchObject *object = *iter;
      if (object->IsBound()) {
        delete object;
        iter = objects.erase(iter);
      } else {
        ++iter;
      }
    }
  }

  pool_manager.Emit(&masm, pc);
}

// Test that binding an unused label works.
TEST(BindUnusedLabel) {
  TestMacroAssembler masm;

  PoolManager<int32_t> pool_manager(4 /*header_size*/,
                                    2 /*header_alignment*/,
                                    BUFFER_ALIGNMENT);
  TestBranchObject *object = new TestBranchObject(4 /*size*/, 4 /*alignment*/);
  int32_t pc = 0;
  pool_manager.Bind(&masm, object, pc);
  delete object;
}

// Test that binding a label adds necessary padding.
TEST(BindLabelNeedsPadding) {
  TestMacroAssembler masm;

  PoolManager<int32_t> pool_manager(4 /*header_size*/,
                                    2 /*header_alignment*/,
                                    BUFFER_ALIGNMENT);

  // Label that needs padding because of the minimum location of the reference.
  TestBranchObject *object = new TestBranchObject(4 /*size*/, 2 /*alignment*/);
  ForwardReference<int32_t> *ref =
      new ForwardReference<int32_t>(0 /*location*/,
                                    2 /*size*/,
                                    4 /*min_location*/,
                                    500 /*max_location*/);
  object->AddReference(ref);
  pool_manager.AddObjectReference(ref, object);
  int32_t pc = 2;
  pc = pool_manager.Bind(&masm, object, pc);
  VIXL_ASSERT(pc == 4);
  delete object;

  // Label that needs padding because of the alignment of the object.
  object = new TestBranchObject(4 /*size*/, 4 /*alignment*/);
  ref = new ForwardReference<int32_t>(0 /*location*/,
                                      2 /*size*/,
                                      0 /*min_location*/,
                                      500 /*max_location*/);
  object->AddReference(ref);
  pool_manager.AddObjectReference(ref, object);

  pc = 2;
  pc = pool_manager.Bind(&masm, object, pc);
  VIXL_ASSERT(pc == 4);
  delete object;

  // Label that needs padding because of the alignment of the reference.
  object = new TestBranchObject(4 /*size*/, 1 /*alignment*/);
  ref = new ForwardReference<int32_t>(0 /*location*/,
                                      2 /*size*/,
                                      0 /*min_location*/,
                                      500 /*max_location*/,
                                      4 /*alignment*/);
  object->AddReference(ref);
  pool_manager.AddObjectReference(ref, object);

  pc = 2;
  pc = pool_manager.Bind(&masm, object, pc);
  VIXL_ASSERT(pc == 4);
  delete object;
}

// This test checks that when we omit the pool header, we insert any padding
// needed in order to meet the minimum location of the first object.
TEST(PoolWithoutHeaderMinLocation) {
  TestMacroAssembler masm;

  PoolManager<int32_t> pool_manager(4 /*header_size*/,
                                    2 /*header_alignment*/,
                                    BUFFER_ALIGNMENT);
  int object_size = 4;
  int object_alignment = 1;  // Do not restrict alignment for this test.
  int min_location = 4;      // We emit the pool at location 2, so need padding.
  int max_location = 500;
  TestObject object(object_size, object_alignment);
  ForwardReference<int32_t> *ref = new ForwardReference<int32_t>(0 /*location*/,
                                                                 2 /*size*/,
                                                                 min_location,
                                                                 max_location);
  object.AddReference(ref);
  pool_manager.AddObjectReference(ref, &object);

  int32_t new_pc = pool_manager.Emit(&masm,
                                     2,
                                     0, /* no new code added */
                                     NULL,
                                     NULL,
                                     PoolManager<int32_t>::kNoBranchRequired);
  USE(new_pc);
  VIXL_ASSERT(new_pc == min_location + object_size);
}

// This test checks that when we omit the pool header, we insert any padding
// needed in order to meet the alignment of the first object.
TEST(PoolWithoutHeaderAlignment) {
  TestMacroAssembler masm;

  PoolManager<int32_t> pool_manager(4 /*header_size*/,
                                    2 /*header_alignment*/,
                                    BUFFER_ALIGNMENT);
  int object_size = 4;
  int object_alignment = 4;  // We emit the pool at location 2, so need padding.
  int min_location = 0;      // Do not restrict this for this test.
  int max_location = 500;
  TestObject object(object_size, object_alignment);
  ForwardReference<int32_t> *ref = new ForwardReference<int32_t>(0 /*location*/,
                                                                 2 /*size*/,
                                                                 min_location,
                                                                 max_location);
  object.AddReference(ref);
  pool_manager.AddObjectReference(ref, &object);

  int32_t pc = 2;
  int32_t new_pc = pool_manager.Emit(&masm,
                                     pc,
                                     0, /* no new code added */
                                     NULL,
                                     NULL,
                                     PoolManager<int32_t>::kNoBranchRequired);
  USE(pc);
  USE(new_pc);
  VIXL_ASSERT(new_pc == AlignUp(pc, object_alignment) + object_size);
}

static int32_t AddNBranches(PoolManager<int32_t> *pool_manager,
                            int32_t pc,
                            TestBranchObject *labels[],
                            int num_branches,
                            int branch_size,
                            int veneer_size,
                            int veneer_alignment,
                            int branch_range) {
  for (int i = 0; i < num_branches; ++i) {
    labels[i] = new TestBranchObject(veneer_size, veneer_alignment);
    int32_t min_location = pc;
    int32_t max_location = pc + branch_range;
    ForwardReference<int32_t> *ref =
        new ForwardReference<int32_t>(pc,
                                      branch_size,
                                      min_location,
                                      max_location);
    labels[i]->AddReference(ref);
    // We have picked the object sizes so that we do not need to emit now.
    VIXL_ASSERT(!pool_manager->MustEmit(pc, branch_size, ref, labels[i]));
    pool_manager->AddObjectReference(ref, labels[i]);
    pc += branch_size;
  }
  return pc;
}

TEST(MustEmitNewReferenceDueToRange) {
  const int kHeaderSize = 4;
  const int kHeaderAlignment = 2;
  const int kNumBranches = 550;
  const int kBranchSize = 4;
  const int kVeneerSize = 4;
  const int kVeneerAlignment = 2;
  const int kBranchRange = 1 * MBytes;
  int32_t pc = 0;

  TestMacroAssembler masm;
  TestBranchObject *labels[kNumBranches];
  PoolManager<int32_t> pool_manager(kHeaderSize,
                                    kHeaderAlignment,
                                    BUFFER_ALIGNMENT);
  AddNBranches(&pool_manager,
               pc,
               labels,
               kNumBranches,
               kBranchSize,
               kVeneerSize,
               kVeneerAlignment,
               kBranchRange);

  // Increment PC to close to the checkpoint of the pools.
  TestPoolManager test(&pool_manager);
  pc = test.GetPoolCheckpoint() - 4;
  VIXL_ASSERT(!pool_manager.MustEmit(pc));

  // Now, attempt to add a reference that would make the problem impossible.
  // We need to emit the pool immediately after this new instruction, and
  // the current size of the pool is kVeneerSize * kNumBranches, so adding a
  // short-range (smaller than the pool size) reference should trigger pool
  // emission.
  const int kPoolSize = kVeneerSize * kNumBranches + kHeaderSize;

  const int kNewObjectSize = 2;
  TestObject new_object(kNewObjectSize, 1);

  ForwardReference<int32_t> temp_ref(pc,
                                     kBranchSize,
                                     pc,
                                     pc + kPoolSize + kBranchSize - 1);
  VIXL_ASSERT(pool_manager.MustEmit(pc, kBranchSize, &temp_ref, &new_object));

  // Before actually emitting the pool, try a few different references to make
  // sure that this works as expected.
  {
    // This reference has a large enough range, so should not force pool
    // emission.
    ForwardReference<int32_t> far_ref(pc,
                                      kBranchSize,
                                      pc,
                                      pc + kPoolSize + kBranchSize);
    VIXL_ASSERT(!pool_manager.MustEmit(pc, kBranchSize, &far_ref, &new_object));

    // This reference had a large enough range but will be restricted by
    // alignment so should force pool emission.
    int alignment = 16;
    VIXL_ASSERT((pc & (alignment - 1)) != 0);
    ForwardReference<int32_t> aligned_ref(pc,
                                          kBranchSize,
                                          pc,
                                          pc + kPoolSize + kBranchSize,
                                          alignment);
    VIXL_ASSERT(
        pool_manager.MustEmit(pc, kBranchSize, &aligned_ref, &new_object));
  }

  // Emit the pool and check its size.
  int32_t new_pc =
      pool_manager.Emit(&masm, pc, kBranchSize, &temp_ref, &new_object);
  VIXL_ASSERT(pc % kHeaderAlignment == 0);  // No need for padding.
  VIXL_ASSERT(new_pc == pc + kPoolSize);
  pc = new_pc;

  // Add the new reference, safely.
  ForwardReference<int32_t> *ref =
      new ForwardReference<int32_t>(pc, 4 /*size*/, pc, pc + kBranchRange);
  new_object.AddReference(ref);
  pool_manager.AddObjectReference(ref, &new_object);
  pc += 4;

  // Emit the pool again.
  new_pc = pool_manager.Emit(&masm, pc);
  VIXL_ASSERT(pc % kHeaderAlignment == 0);  // No need for padding.
  VIXL_ASSERT(new_pc == pc + kNewObjectSize + kHeaderSize);
  pc = new_pc;

  // Finally, bind the labels.
  for (int i = 0; i < kNumBranches; ++i) {
    pc = pool_manager.Bind(&masm, labels[i], pc);
    delete labels[i];
  }
}

TEST(MustEmitNewReferenceDueToSizeOfObject) {
  const int kHeaderSize = 4;
  const int kHeaderAlignment = 2;
  const int kNumBranches = 550;
  const int kBranchSize = 4;
  const int kVeneerSize = 4;
  const int kVeneerAlignment = 2;
  const int kBranchRange = 1 * MBytes;
  int32_t pc = 0;

  TestMacroAssembler masm;
  PoolManager<int32_t> pool_manager(kHeaderSize,
                                    kHeaderAlignment,
                                    BUFFER_ALIGNMENT);
  TestBranchObject *labels[kNumBranches];
  AddNBranches(&pool_manager,
               pc,
               labels,
               kNumBranches,
               kBranchSize,
               kVeneerSize,
               kVeneerAlignment,
               kBranchRange);


  // Increment PC to close to the checkpoint of the pools minus a known
  // thershold.
  const int kBigObjectSize = 1024;
  TestPoolManager test(&pool_manager);
  pc = test.GetPoolCheckpoint() - kBigObjectSize;
  VIXL_ASSERT(!pool_manager.MustEmit(pc));

  // Now, attempt to add a reference that would make the problem impossible.
  // If we add a short-range (smaller than the pool size) reference with a
  // large size (larger than the margin we have until pool emission), pool
  // emission should be triggered.
  const int kPoolSize = kVeneerSize * kNumBranches + kHeaderSize;

  TestObject new_object(kBigObjectSize, 1);
  ForwardReference<int32_t> temp_ref(pc, kBranchSize, pc, pc + kPoolSize);
  VIXL_ASSERT(pool_manager.MustEmit(pc, kBranchSize, &temp_ref, &new_object));

  // Before actually emitting the pool, try a few different references to make
  // sure that this works as expected.
  {
    // If the object is smaller, we can emit the reference.
    TestObject smaller_object(kBigObjectSize - 4, 1);
    ForwardReference<int32_t> temp_ref(pc, kBranchSize, pc, pc + kPoolSize);
    VIXL_ASSERT(
        !pool_manager.MustEmit(pc, kBranchSize, &temp_ref, &smaller_object));

    // If the reference is going to be added after the current objects in the
    // pool, we can still emit it.
    ForwardReference<int32_t> far_ref(pc, kBranchSize, pc, pc + kBranchRange);
    VIXL_ASSERT(!pool_manager.MustEmit(pc, kBranchSize, &far_ref, &new_object));
  }

  // Emit the pool and check its size.
  int32_t new_pc =
      pool_manager.Emit(&masm, pc, kBranchSize, &temp_ref, &new_object);
  VIXL_ASSERT(pc % kHeaderAlignment == 0);  // No need for padding.
  VIXL_ASSERT(new_pc == pc + kPoolSize);
  pc = new_pc;

  // Add the new reference, safely.
  ForwardReference<int32_t> *ref =
      new ForwardReference<int32_t>(pc, 4 /*size*/, pc, pc + kBranchRange);
  new_object.AddReference(ref);
  pool_manager.AddObjectReference(ref, &new_object);
  pc += 4;

  // Emit the pool again.
  new_pc = pool_manager.Emit(&masm, pc);
  VIXL_ASSERT(pc % kHeaderAlignment == 0);  // No need for padding.
  VIXL_ASSERT(new_pc == pc + kBigObjectSize + kHeaderSize);
  pc = new_pc;

  // Finally, bind the labels.
  for (int i = 0; i < kNumBranches; ++i) {
    pc = pool_manager.Bind(&masm, labels[i], pc);
    delete labels[i];
  }
}

template <typename ObjectType>
void ManagedLocationBaseTestHelper() {
  TestMacroAssembler masm;

  PoolManager<int32_t> pool_manager(4 /*header_size*/,
                                    2 /*header_alignment*/,
                                    BUFFER_ALIGNMENT);
  ObjectType *object1 = new ObjectType();
  ObjectType *object2 = new ObjectType();
  ForwardReference<int32_t> *ref_obj1 =
      new ForwardReference<int32_t>(0 /*location*/, 2 /*size*/, 0, 200);
  object1->AddReference(ref_obj1);
  ForwardReference<int32_t> *ref_obj2 =
      new ForwardReference<int32_t>(8 /*location*/, 2 /*size*/, 8, 500);
  object2->AddReference(ref_obj2);

  pool_manager.AddObjectReference(ref_obj1, object1);
  pool_manager.AddObjectReference(ref_obj2, object2);

  pool_manager.Emit(&masm, 20);
}

class TestObjectDeletedOnPlacement : public TestObject {
 public:
  TestObjectDeletedOnPlacement() : TestObject(4 /*size*/, 4 /*alignment*/) {}
  // After passing ownership of this type of object to the pool manager, it is
  // not safe to use it anymore.
  virtual bool ShouldBeDeletedOnPlacementByPoolManager() const VIXL_OVERRIDE {
    return true;
  }
};

TEST(DeleteLocationBaseOnPlacement) {
  ManagedLocationBaseTestHelper<TestObjectDeletedOnPlacement>();
}

class TestObjectDeletedOnPoolManagerDestruction : public TestObject {
 public:
  TestObjectDeletedOnPoolManagerDestruction()
      : TestObject(4 /*size*/, 4 /*alignment*/) {}
  // We can continue using this type of object after passing its ownership to
  // the pool manager, as it will be deleted only when the pool manager is
  // destroyed.
  virtual bool ShouldBeDeletedOnPoolManagerDestruction() const VIXL_OVERRIDE {
    return true;
  }
};


TEST(DeleteLocationBaseOnPoolManagerDestruction) {
  ManagedLocationBaseTestHelper<TestObjectDeletedOnPoolManagerDestruction>();
}
