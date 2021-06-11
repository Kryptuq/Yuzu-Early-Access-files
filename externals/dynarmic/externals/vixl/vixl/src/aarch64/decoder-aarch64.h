// Copyright 2019, VIXL authors
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

#ifndef VIXL_AARCH64_DECODER_AARCH64_H_
#define VIXL_AARCH64_DECODER_AARCH64_H_

#include <list>
#include <map>
#include <string>

#include "../globals-vixl.h"

#include "instructions-aarch64.h"


// List macro containing all visitors needed by the decoder class.

#define VISITOR_LIST_THAT_RETURN(V)                              \
  V(AddSubExtended)                                              \
  V(AddSubImmediate)                                             \
  V(AddSubShifted)                                               \
  V(AddSubWithCarry)                                             \
  V(AtomicMemory)                                                \
  V(Bitfield)                                                    \
  V(CompareBranch)                                               \
  V(ConditionalBranch)                                           \
  V(ConditionalCompareImmediate)                                 \
  V(ConditionalCompareRegister)                                  \
  V(ConditionalSelect)                                           \
  V(Crypto2RegSHA)                                               \
  V(Crypto3RegSHA)                                               \
  V(CryptoAES)                                                   \
  V(DataProcessing1Source)                                       \
  V(DataProcessing2Source)                                       \
  V(DataProcessing3Source)                                       \
  V(EvaluateIntoFlags)                                           \
  V(Exception)                                                   \
  V(Extract)                                                     \
  V(FPCompare)                                                   \
  V(FPConditionalCompare)                                        \
  V(FPConditionalSelect)                                         \
  V(FPDataProcessing1Source)                                     \
  V(FPDataProcessing2Source)                                     \
  V(FPDataProcessing3Source)                                     \
  V(FPFixedPointConvert)                                         \
  V(FPImmediate)                                                 \
  V(FPIntegerConvert)                                            \
  V(LoadLiteral)                                                 \
  V(LoadStoreExclusive)                                          \
  V(LoadStorePAC)                                                \
  V(LoadStorePairNonTemporal)                                    \
  V(LoadStorePairOffset)                                         \
  V(LoadStorePairPostIndex)                                      \
  V(LoadStorePairPreIndex)                                       \
  V(LoadStorePostIndex)                                          \
  V(LoadStorePreIndex)                                           \
  V(LoadStoreRCpcUnscaledOffset)                                 \
  V(LoadStoreRegisterOffset)                                     \
  V(LoadStoreUnscaledOffset)                                     \
  V(LoadStoreUnsignedOffset)                                     \
  V(LogicalImmediate)                                            \
  V(LogicalShifted)                                              \
  V(MoveWideImmediate)                                           \
  V(NEON2RegMisc)                                                \
  V(NEON2RegMiscFP16)                                            \
  V(NEON3Different)                                              \
  V(NEON3Same)                                                   \
  V(NEON3SameExtra)                                              \
  V(NEON3SameFP16)                                               \
  V(NEONAcrossLanes)                                             \
  V(NEONByIndexedElement)                                        \
  V(NEONCopy)                                                    \
  V(NEONExtract)                                                 \
  V(NEONLoadStoreMultiStruct)                                    \
  V(NEONLoadStoreMultiStructPostIndex)                           \
  V(NEONLoadStoreSingleStruct)                                   \
  V(NEONLoadStoreSingleStructPostIndex)                          \
  V(NEONModifiedImmediate)                                       \
  V(NEONPerm)                                                    \
  V(NEONScalar2RegMisc)                                          \
  V(NEONScalar2RegMiscFP16)                                      \
  V(NEONScalar3Diff)                                             \
  V(NEONScalar3Same)                                             \
  V(NEONScalar3SameExtra)                                        \
  V(NEONScalar3SameFP16)                                         \
  V(NEONScalarByIndexedElement)                                  \
  V(NEONScalarCopy)                                              \
  V(NEONScalarPairwise)                                          \
  V(NEONScalarShiftImmediate)                                    \
  V(NEONShiftImmediate)                                          \
  V(NEONTable)                                                   \
  V(PCRelAddressing)                                             \
  V(RotateRightIntoFlags)                                        \
  V(SVE32BitGatherLoad_ScalarPlus32BitUnscaledOffsets)           \
  V(SVE32BitGatherLoad_VectorPlusImm)                            \
  V(SVE32BitGatherLoadHalfwords_ScalarPlus32BitScaledOffsets)    \
  V(SVE32BitGatherLoadWords_ScalarPlus32BitScaledOffsets)        \
  V(SVE32BitGatherPrefetch_ScalarPlus32BitScaledOffsets)         \
  V(SVE32BitGatherPrefetch_VectorPlusImm)                        \
  V(SVE32BitScatterStore_ScalarPlus32BitScaledOffsets)           \
  V(SVE32BitScatterStore_ScalarPlus32BitUnscaledOffsets)         \
  V(SVE32BitScatterStore_VectorPlusImm)                          \
  V(SVE64BitGatherLoad_ScalarPlus32BitUnpackedScaledOffsets)     \
  V(SVE64BitGatherLoad_ScalarPlus64BitScaledOffsets)             \
  V(SVE64BitGatherLoad_ScalarPlus64BitUnscaledOffsets)           \
  V(SVE64BitGatherLoad_ScalarPlusUnpacked32BitUnscaledOffsets)   \
  V(SVE64BitGatherLoad_VectorPlusImm)                            \
  V(SVE64BitGatherPrefetch_ScalarPlus64BitScaledOffsets)         \
  V(SVE64BitGatherPrefetch_ScalarPlusUnpacked32BitScaledOffsets) \
  V(SVE64BitGatherPrefetch_VectorPlusImm)                        \
  V(SVE64BitScatterStore_ScalarPlus64BitScaledOffsets)           \
  V(SVE64BitScatterStore_ScalarPlus64BitUnscaledOffsets)         \
  V(SVE64BitScatterStore_ScalarPlusUnpacked32BitScaledOffsets)   \
  V(SVE64BitScatterStore_ScalarPlusUnpacked32BitUnscaledOffsets) \
  V(SVE64BitScatterStore_VectorPlusImm)                          \
  V(SVEAddressGeneration)                                        \
  V(SVEBitwiseLogicalUnpredicated)                               \
  V(SVEBitwiseShiftUnpredicated)                                 \
  V(SVEFFRInitialise)                                            \
  V(SVEFFRWriteFromPredicate)                                    \
  V(SVEFPAccumulatingReduction)                                  \
  V(SVEFPArithmeticUnpredicated)                                 \
  V(SVEFPCompareVectors)                                         \
  V(SVEFPCompareWithZero)                                        \
  V(SVEFPComplexAddition)                                        \
  V(SVEFPComplexMulAdd)                                          \
  V(SVEFPComplexMulAddIndex)                                     \
  V(SVEFPFastReduction)                                          \
  V(SVEFPMulIndex)                                               \
  V(SVEFPMulAdd)                                                 \
  V(SVEFPMulAddIndex)                                            \
  V(SVEFPUnaryOpUnpredicated)                                    \
  V(SVEIncDecByPredicateCount)                                   \
  V(SVEIndexGeneration)                                          \
  V(SVEIntArithmeticUnpredicated)                                \
  V(SVEIntCompareSignedImm)                                      \
  V(SVEIntCompareUnsignedImm)                                    \
  V(SVEIntCompareVectors)                                        \
  V(SVEIntMulAddPredicated)                                      \
  V(SVEIntMulAddUnpredicated)                                    \
  V(SVEIntReduction)                                             \
  V(SVEIntUnaryArithmeticPredicated)                             \
  V(SVEMovprfx)                                                  \
  V(SVEMulIndex)                                                 \
  V(SVEPermuteVectorExtract)                                     \
  V(SVEPermuteVectorInterleaving)                                \
  V(SVEPredicateCount)                                           \
  V(SVEPredicateLogical)                                         \
  V(SVEPropagateBreak)                                           \
  V(SVEStackFrameAdjustment)                                     \
  V(SVEStackFrameSize)                                           \
  V(SVEVectorSelect)                                             \
  V(SVEBitwiseLogical_Predicated)                                \
  V(SVEBitwiseLogicalWithImm_Unpredicated)                       \
  V(SVEBitwiseShiftByImm_Predicated)                             \
  V(SVEBitwiseShiftByVector_Predicated)                          \
  V(SVEBitwiseShiftByWideElements_Predicated)                    \
  V(SVEBroadcastBitmaskImm)                                      \
  V(SVEBroadcastFPImm_Unpredicated)                              \
  V(SVEBroadcastGeneralRegister)                                 \
  V(SVEBroadcastIndexElement)                                    \
  V(SVEBroadcastIntImm_Unpredicated)                             \
  V(SVECompressActiveElements)                                   \
  V(SVEConditionallyBroadcastElementToVector)                    \
  V(SVEConditionallyExtractElementToSIMDFPScalar)                \
  V(SVEConditionallyExtractElementToGeneralRegister)             \
  V(SVEConditionallyTerminateScalars)                            \
  V(SVEConstructivePrefix_Unpredicated)                          \
  V(SVEContiguousFirstFaultLoad_ScalarPlusScalar)                \
  V(SVEContiguousLoad_ScalarPlusImm)                             \
  V(SVEContiguousLoad_ScalarPlusScalar)                          \
  V(SVEContiguousNonFaultLoad_ScalarPlusImm)                     \
  V(SVEContiguousNonTemporalLoad_ScalarPlusImm)                  \
  V(SVEContiguousNonTemporalLoad_ScalarPlusScalar)               \
  V(SVEContiguousNonTemporalStore_ScalarPlusImm)                 \
  V(SVEContiguousNonTemporalStore_ScalarPlusScalar)              \
  V(SVEContiguousPrefetch_ScalarPlusImm)                         \
  V(SVEContiguousPrefetch_ScalarPlusScalar)                      \
  V(SVEContiguousStore_ScalarPlusImm)                            \
  V(SVEContiguousStore_ScalarPlusScalar)                         \
  V(SVECopySIMDFPScalarRegisterToVector_Predicated)              \
  V(SVECopyFPImm_Predicated)                                     \
  V(SVECopyGeneralRegisterToVector_Predicated)                   \
  V(SVECopyIntImm_Predicated)                                    \
  V(SVEElementCount)                                             \
  V(SVEExtractElementToSIMDFPScalarRegister)                     \
  V(SVEExtractElementToGeneralRegister)                          \
  V(SVEFPArithmetic_Predicated)                                  \
  V(SVEFPArithmeticWithImm_Predicated)                           \
  V(SVEFPConvertPrecision)                                       \
  V(SVEFPConvertToInt)                                           \
  V(SVEFPExponentialAccelerator)                                 \
  V(SVEFPRoundToIntegralValue)                                   \
  V(SVEFPTrigMulAddCoefficient)                                  \
  V(SVEFPTrigSelectCoefficient)                                  \
  V(SVEFPUnaryOp)                                                \
  V(SVEIncDecRegisterByElementCount)                             \
  V(SVEIncDecVectorByElementCount)                               \
  V(SVEInsertSIMDFPScalarRegister)                               \
  V(SVEInsertGeneralRegister)                                    \
  V(SVEIntAddSubtractImm_Unpredicated)                           \
  V(SVEIntAddSubtractVectors_Predicated)                         \
  V(SVEIntCompareScalarCountAndLimit)                            \
  V(SVEIntConvertToFP)                                           \
  V(SVEIntDivideVectors_Predicated)                              \
  V(SVEIntMinMaxImm_Unpredicated)                                \
  V(SVEIntMinMaxDifference_Predicated)                           \
  V(SVEIntMulImm_Unpredicated)                                   \
  V(SVEIntMulVectors_Predicated)                                 \
  V(SVELoadAndBroadcastElement)                                  \
  V(SVELoadAndBroadcastQuadword_ScalarPlusImm)                   \
  V(SVELoadAndBroadcastQuadword_ScalarPlusScalar)                \
  V(SVELoadMultipleStructures_ScalarPlusImm)                     \
  V(SVELoadMultipleStructures_ScalarPlusScalar)                  \
  V(SVELoadPredicateRegister)                                    \
  V(SVELoadVectorRegister)                                       \
  V(SVEPartitionBreakCondition)                                  \
  V(SVEPermutePredicateElements)                                 \
  V(SVEPredicateFirstActive)                                     \
  V(SVEPredicateInitialize)                                      \
  V(SVEPredicateNextActive)                                      \
  V(SVEPredicateReadFromFFR_Predicated)                          \
  V(SVEPredicateReadFromFFR_Unpredicated)                        \
  V(SVEPredicateTest)                                            \
  V(SVEPredicateZero)                                            \
  V(SVEPropagateBreakToNextPartition)                            \
  V(SVEReversePredicateElements)                                 \
  V(SVEReverseVectorElements)                                    \
  V(SVEReverseWithinElements)                                    \
  V(SVESaturatingIncDecRegisterByElementCount)                   \
  V(SVESaturatingIncDecVectorByElementCount)                     \
  V(SVEStoreMultipleStructures_ScalarPlusImm)                    \
  V(SVEStoreMultipleStructures_ScalarPlusScalar)                 \
  V(SVEStorePredicateRegister)                                   \
  V(SVEStoreVectorRegister)                                      \
  V(SVETableLookup)                                              \
  V(SVEUnpackPredicateElements)                                  \
  V(SVEUnpackVectorElements)                                     \
  V(SVEVectorSplice_Destructive)                                 \
  V(System)                                                      \
  V(TestBranch)                                                  \
  V(Unallocated)                                                 \
  V(UnconditionalBranch)                                         \
  V(UnconditionalBranchToRegister)                               \
  V(Unimplemented)

#define VISITOR_LIST_THAT_DONT_RETURN(V) V(Reserved)

#define VISITOR_LIST(V)       \
  VISITOR_LIST_THAT_RETURN(V) \
  VISITOR_LIST_THAT_DONT_RETURN(V)

namespace vixl {
namespace aarch64 {

// The Visitor interface. Disassembler and simulator (and other tools)
// must provide implementations for all of these functions.
//
// Note that this class must change in breaking ways with even minor additions
// to VIXL, and so its API should be considered unstable. User classes that
// inherit from this one should be expected to break even on minor version
// updates. If this is a problem, consider using DecoderVisitorWithDefaults
// instead.
class DecoderVisitor {
 public:
  enum VisitorConstness { kConstVisitor, kNonConstVisitor };
  explicit DecoderVisitor(VisitorConstness constness = kConstVisitor)
      : constness_(constness) {}

  virtual ~DecoderVisitor() {}

#define DECLARE(A) virtual void Visit##A(const Instruction* instr) = 0;
  VISITOR_LIST(DECLARE)
#undef DECLARE

  bool IsConstVisitor() const { return constness_ == kConstVisitor; }
  Instruction* MutableInstruction(const Instruction* instr) {
    VIXL_ASSERT(!IsConstVisitor());
    return const_cast<Instruction*>(instr);
  }

 private:
  const VisitorConstness constness_;
};

// As above, but a default (no-op) implementation for each visitor is provided.
// This is useful for derived class that only care about specific visitors.
//
// A minor version update may add a visitor, but will never remove one, so it is
// safe (and recommended) to use `override` in derived classes.
class DecoderVisitorWithDefaults : public DecoderVisitor {
 public:
  explicit DecoderVisitorWithDefaults(
      VisitorConstness constness = kConstVisitor)
      : DecoderVisitor(constness) {}

  virtual ~DecoderVisitorWithDefaults() {}

#define DECLARE(A) \
  virtual void Visit##A(const Instruction* instr) VIXL_OVERRIDE { USE(instr); }
  VISITOR_LIST(DECLARE)
#undef DECLARE
};

class DecodeNode;
class CompiledDecodeNode;

// The instruction decoder is constructed from a graph of decode nodes. At each
// node, a number of bits are sampled from the instruction being decoded. The
// resulting value is used to look up the next node in the graph, which then
// samples other bits, and moves to other decode nodes. Eventually, a visitor
// node is reached, and the corresponding visitor function is called, which
// handles the instruction.
class Decoder {
 public:
  Decoder() { ConstructDecodeGraph(); }

  // Top-level wrappers around the actual decoding function.
  void Decode(const Instruction* instr);
  void Decode(Instruction* instr);

  // Decode all instructions from start (inclusive) to end (exclusive).
  template <typename T>
  void Decode(T start, T end) {
    for (T instr = start; instr < end; instr = instr->GetNextInstruction()) {
      Decode(instr);
    }
  }

  // Register a new visitor class with the decoder.
  // Decode() will call the corresponding visitor method from all registered
  // visitor classes when decoding reaches the leaf node of the instruction
  // decode tree.
  // Visitors are called in order.
  // A visitor can be registered multiple times.
  //
  //   d.AppendVisitor(V1);
  //   d.AppendVisitor(V2);
  //   d.PrependVisitor(V2);
  //   d.AppendVisitor(V3);
  //
  //   d.Decode(i);
  //
  // will call in order visitor methods in V2, V1, V2, V3.
  void AppendVisitor(DecoderVisitor* visitor);
  void PrependVisitor(DecoderVisitor* visitor);
  // These helpers register `new_visitor` before or after the first instance of
  // `registered_visiter` in the list.
  // So if
  //   V1, V2, V1, V2
  // are registered in this order in the decoder, calls to
  //   d.InsertVisitorAfter(V3, V1);
  //   d.InsertVisitorBefore(V4, V2);
  // will yield the order
  //   V1, V3, V4, V2, V1, V2
  //
  // For more complex modifications of the order of registered visitors, one can
  // directly access and modify the list of visitors via the `visitors()'
  // accessor.
  void InsertVisitorBefore(DecoderVisitor* new_visitor,
                           DecoderVisitor* registered_visitor);
  void InsertVisitorAfter(DecoderVisitor* new_visitor,
                          DecoderVisitor* registered_visitor);

  // Remove all instances of a previously registered visitor class from the list
  // of visitors stored by the decoder.
  void RemoveVisitor(DecoderVisitor* visitor);

#define DECLARE(A) void Visit##A(const Instruction* instr);
  VISITOR_LIST(DECLARE)
#undef DECLARE

  std::list<DecoderVisitor*>* visitors() { return &visitors_; }

  // Get a DecodeNode by name from the Decoder's map.
  DecodeNode* GetDecodeNode(std::string name);

 private:
  // Decodes an instruction and calls the visitor functions registered with the
  // Decoder class.
  void DecodeInstruction(const Instruction* instr);

  // Add an initialised DecodeNode to the decode_node_ map.
  void AddDecodeNode(const DecodeNode& node);

  // Visitors are registered in a list.
  std::list<DecoderVisitor*> visitors_;

  // Compile the dynamically generated decode graph based on the static
  // information in kDecodeMapping and kVisitorNodes.
  void ConstructDecodeGraph();

  // Root node for the compiled decoder graph, stored here to avoid a map lookup
  // for every instruction decoded.
  CompiledDecodeNode* compiled_decoder_root_;

  // Map of node names to DecodeNodes.
  std::map<std::string, DecodeNode> decode_nodes_;
};

const int kMaxDecodeSampledBits = 16;
const int kMaxDecodeMappings = 100;
typedef void (Decoder::*DecodeFnPtr)(const Instruction*);
typedef uint32_t (Instruction::*BitExtractFn)(void) const;

// A Visitor node maps the name of a visitor to the function that handles it.
struct VisitorNode {
  const char* name;
  const DecodeFnPtr visitor_fn;
};

// DecodePattern and DecodeMapping represent the input data to the decoder
// compilation stage. After compilation, the decoder is embodied in the graph
// of CompiledDecodeNodes pointer to by compiled_decoder_root_.

// A DecodePattern maps a pattern of set/unset/don't care (1, 0, x) bits as a
// string to the name of its handler.
struct DecodePattern {
  const char* pattern;
  const char* handler;
};

// A DecodeMapping consists of the name of a handler, the bits sampled in the
// instruction by that handler, and a mapping from the pattern that those
// sampled bits match to the corresponding name of a node.
struct DecodeMapping {
  const char* name;
  const uint8_t sampled_bits[kMaxDecodeSampledBits];
  const DecodePattern mapping[kMaxDecodeMappings];
};

// For speed, before nodes can be used for decoding instructions, they must
// be compiled. This converts the mapping "bit pattern strings to decoder name
// string" stored in DecodeNodes to an array look up for the pointer to the next
// node, stored in CompiledDecodeNodes. Compilation may also apply other
// optimisations for simple decode patterns.
class CompiledDecodeNode {
 public:
  // Constructor for decode node, containing a decode table and pointer to a
  // function that extracts the bits to be sampled.
  CompiledDecodeNode(BitExtractFn bit_extract_fn, size_t decode_table_size)
      : bit_extract_fn_(bit_extract_fn),
        visitor_fn_(NULL),
        decode_table_size_(decode_table_size),
        decoder_(NULL) {
    decode_table_ = new CompiledDecodeNode*[decode_table_size_];
    memset(decode_table_, 0, decode_table_size_ * sizeof(decode_table_[0]));
  }

  // Constructor for wrappers around visitor functions. These require no
  // decoding, so no bit extraction function or decode table is assigned.
  explicit CompiledDecodeNode(DecodeFnPtr visitor_fn, Decoder* decoder)
      : bit_extract_fn_(NULL),
        visitor_fn_(visitor_fn),
        decode_table_(NULL),
        decode_table_size_(0),
        decoder_(decoder) {}

  ~CompiledDecodeNode() VIXL_NEGATIVE_TESTING_ALLOW_EXCEPTION {
    // Free the decode table, if this is a compiled, non-leaf node.
    if (decode_table_ != NULL) {
      VIXL_ASSERT(!IsLeafNode());
      delete[] decode_table_;
    }
  }

  // Decode the instruction by either sampling the bits using the bit extract
  // function to find the next node, or, if we're at a leaf, calling the visitor
  // function.
  void Decode(const Instruction* instr) const;

  // A leaf node is a wrapper for a visitor function.
  bool IsLeafNode() const {
    VIXL_ASSERT(((visitor_fn_ == NULL) && (bit_extract_fn_ != NULL)) ||
                ((visitor_fn_ != NULL) && (bit_extract_fn_ == NULL)));
    return visitor_fn_ != NULL;
  }

  // Get a pointer to the next node required in the decode process, based on the
  // bits sampled by the current node.
  CompiledDecodeNode* GetNodeForBits(uint32_t bits) const {
    VIXL_ASSERT(bits < decode_table_size_);
    return decode_table_[bits];
  }

  // Set the next node in the decode process for the pattern of sampled bits in
  // the current node.
  void SetNodeForBits(uint32_t bits, CompiledDecodeNode* n) {
    VIXL_ASSERT(bits < decode_table_size_);
    VIXL_ASSERT(n != NULL);
    decode_table_[bits] = n;
  }

 private:
  // Pointer to an instantiated template function for extracting the bits
  // sampled by this node. Set to NULL for leaf nodes.
  const BitExtractFn bit_extract_fn_;

  // Visitor function that handles the instruction identified. Set only for
  // leaf nodes, where no extra decoding is required, otherwise NULL.
  const DecodeFnPtr visitor_fn_;

  // Mapping table from instruction bits to next decode stage.
  CompiledDecodeNode** decode_table_;
  const size_t decode_table_size_;

  // Pointer to the decoder containing this node, used to call its visitor
  // function for leaf nodes. Set to NULL for non-leaf nodes.
  Decoder* decoder_;
};

class DecodeNode {
 public:
  // Default constructor needed for map initialisation.
  DecodeNode() : compiled_node_(NULL) {}

  // Constructor for DecodeNode wrappers around visitor functions. These are
  // marked as "compiled", as there is no decoding left to do.
  explicit DecodeNode(const VisitorNode& visitor, Decoder* decoder)
      : name_(visitor.name),
        visitor_fn_(visitor.visitor_fn),
        decoder_(decoder),
        compiled_node_(NULL) {}

  // Constructor for DecodeNodes that map bit patterns to other DecodeNodes.
  explicit DecodeNode(const DecodeMapping& map, Decoder* decoder = NULL)
      : name_(map.name),
        visitor_fn_(NULL),
        decoder_(decoder),
        compiled_node_(NULL) {
    // The length of the bit string in the first mapping determines the number
    // of sampled bits. When adding patterns later, we assert that all mappings
    // sample the same number of bits.
    VIXL_CHECK(strcmp(map.mapping[0].pattern, "otherwise") != 0);
    int bit_count = static_cast<int>(strlen(map.mapping[0].pattern));
    VIXL_CHECK((bit_count > 0) && (bit_count <= 32));
    SetSampledBits(map.sampled_bits, bit_count);
    AddPatterns(map.mapping);
  }

  ~DecodeNode() {
    // Delete the compiled version of this node, if one was created.
    if (compiled_node_ != NULL) {
      delete compiled_node_;
    }
  }

  // Set the bits sampled from the instruction by this node.
  void SetSampledBits(const uint8_t* bits, int bit_count);

  // Get the bits sampled from the instruction by this node.
  std::vector<uint8_t> GetSampledBits() const;

  // Get the number of bits sampled from the instruction by this node.
  size_t GetSampledBitsCount() const;

  // Add patterns to this node's internal pattern table.
  void AddPatterns(const DecodePattern* patterns);

  // A leaf node is a DecodeNode that wraps the visitor function for the
  // identified instruction class.
  bool IsLeafNode() const { return visitor_fn_ != NULL; }

  std::string GetName() const { return name_; }

  // Create a CompiledDecodeNode of specified table size that uses
  // bit_extract_fn to sample bits from the instruction.
  void CreateCompiledNode(BitExtractFn bit_extract_fn, size_t table_size) {
    VIXL_ASSERT(bit_extract_fn != NULL);
    VIXL_ASSERT(table_size > 0);
    compiled_node_ = new CompiledDecodeNode(bit_extract_fn, table_size);
  }

  // Create a CompiledDecodeNode wrapping a visitor function. No decoding is
  // required for this node; the visitor function is called instead.
  void CreateVisitorNode() {
    compiled_node_ = new CompiledDecodeNode(visitor_fn_, decoder_);
  }

  // Find and compile the DecodeNode named "name", and set it as the node for
  // the pattern "bits".
  void CompileNodeForBits(Decoder* decoder, std::string name, uint32_t bits);

  // Get a pointer to an instruction method that extracts the instruction bits
  // specified by the mask argument, and returns those sampled bits as a
  // contiguous sequence, suitable for indexing an array.
  // For example, a mask of 0b1010 returns a function that, given an instruction
  // 0bXYZW, will return 0bXZ.
  BitExtractFn GetBitExtractFunction(uint32_t mask);

  // Get a pointer to an Instruction method that applies a mask to the
  // instruction bits, and tests if the result is equal to value. The returned
  // function gives a 1 result if (inst & mask == value), 0 otherwise.
  BitExtractFn GetBitExtractFunction(uint32_t mask, uint32_t value);

  // Compile this DecodeNode into a new CompiledDecodeNode and returns a pointer
  // to it. This pointer is also stored inside the DecodeNode itself. Destroying
  // a DecodeNode frees its associated CompiledDecodeNode.
  CompiledDecodeNode* Compile(Decoder* decoder);

  // Get a pointer to the CompiledDecodeNode associated with this DecodeNode.
  // Returns NULL if the node has not been compiled yet.
  CompiledDecodeNode* GetCompiledNode() const { return compiled_node_; }
  bool IsCompiled() const { return GetCompiledNode() != NULL; }

 private:
  // Generate a mask and value pair from a string constructed from 0, 1 and x
  // (don't care) characters.
  // For example "10x1" should return mask = 0b1101, value = 0b1001.
  typedef std::pair<Instr, Instr> MaskValuePair;
  MaskValuePair GenerateMaskValuePair(std::string pattern) const;

  // Generate a pattern string ordered by the bit positions sampled by this
  // node. The first character in the string corresponds to the lowest sampled
  // bit.
  // For example, a pattern of "1x0" expected when sampling bits 31, 1 and 30
  // returns the pattern "x01"; bit 1 should be 'x', bit 30 '0' and bit 31 '1'.
  // This output makes comparisons easier between the pattern and bits sampled
  // from an instruction using the fast "compress" algorithm. See
  // Instruction::Compress().
  std::string GenerateOrderedPattern(std::string pattern) const;

  // Generate a mask with a bit set at each sample position.
  uint32_t GenerateSampledBitsMask() const;

  // Try to compile a more optimised decode operation for this node, returning
  // true if successful.
  bool TryCompileOptimisedDecodeTable(Decoder* decoder);

  // Name of this decoder node, used to construct edges in the decode graph.
  std::string name_;

  // Vector of bits sampled from an instruction to determine which node to look
  // up next in the decode process.
  std::vector<uint8_t> sampled_bits_;

  // Visitor function that handles the instruction identified. Set only for leaf
  // nodes, where no extra decoding is required. For non-leaf decoding nodes,
  // this pointer is NULL.
  DecodeFnPtr visitor_fn_;

  // Source mapping from bit pattern to name of next decode stage.
  std::vector<DecodePattern> pattern_table_;

  // Pointer to the decoder containing this node, used to call its visitor
  // function for leaf nodes.
  Decoder* decoder_;

  // Pointer to the compiled version of this node. Is this node hasn't been
  // compiled yet, this pointer is NULL.
  CompiledDecodeNode* compiled_node_;
};

}  // namespace aarch64
}  // namespace vixl

#endif  // VIXL_AARCH64_DECODER_AARCH64_H_
