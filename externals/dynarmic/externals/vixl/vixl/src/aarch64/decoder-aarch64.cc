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

#include <string>

#include "../globals-vixl.h"
#include "../utils-vixl.h"

#include "decoder-aarch64.h"
#include "decoder-constants-aarch64.h"

namespace vixl {
namespace aarch64 {

void Decoder::Decode(const Instruction* instr) {
  std::list<DecoderVisitor*>::iterator it;
  for (it = visitors_.begin(); it != visitors_.end(); it++) {
    VIXL_ASSERT((*it)->IsConstVisitor());
  }
  VIXL_ASSERT(compiled_decoder_root_ != NULL);
  compiled_decoder_root_->Decode(instr);
}

void Decoder::Decode(Instruction* instr) {
  compiled_decoder_root_->Decode(const_cast<const Instruction*>(instr));
}

void Decoder::AddDecodeNode(const DecodeNode& node) {
  decode_nodes_.insert(std::make_pair(node.GetName(), node));
}

DecodeNode* Decoder::GetDecodeNode(std::string name) {
  if (decode_nodes_.count(name) != 1) {
    std::string msg = "Can't find decode node " + name + ".\n";
    VIXL_ABORT_WITH_MSG(msg.c_str());
  }
  return &decode_nodes_[name];
}

void Decoder::ConstructDecodeGraph() {
  // Add all of the decoding nodes to the Decoder.
  for (unsigned i = 0; i < ArrayLength(kDecodeMapping); i++) {
    AddDecodeNode(DecodeNode(kDecodeMapping[i], this));
  }

  // Add the visitor function wrapping nodes to the Decoder.
  for (unsigned i = 0; i < ArrayLength(kVisitorNodes); i++) {
    AddDecodeNode(DecodeNode(kVisitorNodes[i], this));
  }

  // Compile the graph from the root.
  compiled_decoder_root_ = GetDecodeNode("Root")->Compile(this);
}

void Decoder::AppendVisitor(DecoderVisitor* new_visitor) {
  visitors_.push_back(new_visitor);
}


void Decoder::PrependVisitor(DecoderVisitor* new_visitor) {
  visitors_.push_front(new_visitor);
}


void Decoder::InsertVisitorBefore(DecoderVisitor* new_visitor,
                                  DecoderVisitor* registered_visitor) {
  std::list<DecoderVisitor*>::iterator it;
  for (it = visitors_.begin(); it != visitors_.end(); it++) {
    if (*it == registered_visitor) {
      visitors_.insert(it, new_visitor);
      return;
    }
  }
  // We reached the end of the list. The last element must be
  // registered_visitor.
  VIXL_ASSERT(*it == registered_visitor);
  visitors_.insert(it, new_visitor);
}


void Decoder::InsertVisitorAfter(DecoderVisitor* new_visitor,
                                 DecoderVisitor* registered_visitor) {
  std::list<DecoderVisitor*>::iterator it;
  for (it = visitors_.begin(); it != visitors_.end(); it++) {
    if (*it == registered_visitor) {
      it++;
      visitors_.insert(it, new_visitor);
      return;
    }
  }
  // We reached the end of the list. The last element must be
  // registered_visitor.
  VIXL_ASSERT(*it == registered_visitor);
  visitors_.push_back(new_visitor);
}


void Decoder::RemoveVisitor(DecoderVisitor* visitor) {
  visitors_.remove(visitor);
}

#define DEFINE_VISITOR_CALLERS(A)                               \
  void Decoder::Visit##A(const Instruction* instr) {            \
    VIXL_ASSERT(((A##FMask == 0) && (A##Fixed == 0)) ||         \
                (instr->Mask(A##FMask) == A##Fixed));           \
    std::list<DecoderVisitor*>::iterator it;                    \
    for (it = visitors_.begin(); it != visitors_.end(); it++) { \
      (*it)->Visit##A(instr);                                   \
    }                                                           \
  }
VISITOR_LIST(DEFINE_VISITOR_CALLERS)
#undef DEFINE_VISITOR_CALLERS

void DecodeNode::SetSampledBits(const uint8_t* bits, int bit_count) {
  VIXL_ASSERT(!IsCompiled());

  sampled_bits_.resize(bit_count);
  for (int i = 0; i < bit_count; i++) {
    sampled_bits_[i] = bits[i];
  }
}

std::vector<uint8_t> DecodeNode::GetSampledBits() const {
  return sampled_bits_;
}

size_t DecodeNode::GetSampledBitsCount() const { return sampled_bits_.size(); }

void DecodeNode::AddPatterns(const DecodePattern* patterns) {
  VIXL_ASSERT(!IsCompiled());
  for (unsigned i = 0; i < kMaxDecodeMappings; i++) {
    // Empty string indicates end of patterns.
    if (patterns[i].pattern == NULL) break;
    VIXL_ASSERT((strlen(patterns[i].pattern) == GetSampledBitsCount()) ||
                (strcmp(patterns[i].pattern, "otherwise") == 0));
    pattern_table_.push_back(patterns[i]);
  }
}

void DecodeNode::CompileNodeForBits(Decoder* decoder,
                                    std::string name,
                                    uint32_t bits) {
  DecodeNode* n = decoder->GetDecodeNode(name);
  VIXL_ASSERT(n != NULL);
  if (!n->IsCompiled()) {
    n->Compile(decoder);
  }
  VIXL_ASSERT(n->IsCompiled());
  compiled_node_->SetNodeForBits(bits, n->GetCompiledNode());
}

BitExtractFn DecodeNode::GetBitExtractFunction(uint32_t mask) {
  // Instantiate a templated bit extraction function for every pattern we
  // might encounter. If the assertion in the default clause is reached, add a
  // new instantiation below using the information in the failure message.
  BitExtractFn bit_extract_fn = NULL;
  switch (mask) {
#define INSTANTIATE_TEMPLATE(M)                    \
  case M:                                          \
    bit_extract_fn = &Instruction::ExtractBits<M>; \
    break;
    INSTANTIATE_TEMPLATE(0x000001e0);
    INSTANTIATE_TEMPLATE(0x00000400);
    INSTANTIATE_TEMPLATE(0x00000800);
    INSTANTIATE_TEMPLATE(0x00000c00);
    INSTANTIATE_TEMPLATE(0x00001000);
    INSTANTIATE_TEMPLATE(0x00001800);
    INSTANTIATE_TEMPLATE(0x00001c00);
    INSTANTIATE_TEMPLATE(0x00004000);
    INSTANTIATE_TEMPLATE(0x00008000);
    INSTANTIATE_TEMPLATE(0x0000f000);
    INSTANTIATE_TEMPLATE(0x0000fc00);
    INSTANTIATE_TEMPLATE(0x00060010);
    INSTANTIATE_TEMPLATE(0x00093e00);
    INSTANTIATE_TEMPLATE(0x000c1000);
    INSTANTIATE_TEMPLATE(0x00100000);
    INSTANTIATE_TEMPLATE(0x00101800);
    INSTANTIATE_TEMPLATE(0x00140000);
    INSTANTIATE_TEMPLATE(0x00180000);
    INSTANTIATE_TEMPLATE(0x00181000);
    INSTANTIATE_TEMPLATE(0x00190000);
    INSTANTIATE_TEMPLATE(0x00191400);
    INSTANTIATE_TEMPLATE(0x001c0000);
    INSTANTIATE_TEMPLATE(0x001c1800);
    INSTANTIATE_TEMPLATE(0x001f0000);
    INSTANTIATE_TEMPLATE(0x0020fc00);
    INSTANTIATE_TEMPLATE(0x0038f000);
    INSTANTIATE_TEMPLATE(0x00400000);
    INSTANTIATE_TEMPLATE(0x00400010);
    INSTANTIATE_TEMPLATE(0x0040f000);
    INSTANTIATE_TEMPLATE(0x00500000);
    INSTANTIATE_TEMPLATE(0x00800000);
    INSTANTIATE_TEMPLATE(0x00800010);
    INSTANTIATE_TEMPLATE(0x00801800);
    INSTANTIATE_TEMPLATE(0x009f0000);
    INSTANTIATE_TEMPLATE(0x00c00000);
    INSTANTIATE_TEMPLATE(0x00c00010);
    INSTANTIATE_TEMPLATE(0x00cf8000);
    INSTANTIATE_TEMPLATE(0x00db0000);
    INSTANTIATE_TEMPLATE(0x00dc0000);
    INSTANTIATE_TEMPLATE(0x00e00003);
    INSTANTIATE_TEMPLATE(0x00f80400);
    INSTANTIATE_TEMPLATE(0x01e00000);
    INSTANTIATE_TEMPLATE(0x03800000);
    INSTANTIATE_TEMPLATE(0x04c0f000);
    INSTANTIATE_TEMPLATE(0x10800400);
    INSTANTIATE_TEMPLATE(0x1e000000);
    INSTANTIATE_TEMPLATE(0x20000000);
    INSTANTIATE_TEMPLATE(0x20000410);
    INSTANTIATE_TEMPLATE(0x20007000);
    INSTANTIATE_TEMPLATE(0x20007800);
    INSTANTIATE_TEMPLATE(0x2000f000);
    INSTANTIATE_TEMPLATE(0x2000f800);
    INSTANTIATE_TEMPLATE(0x201e0c00);
    INSTANTIATE_TEMPLATE(0x20803800);
    INSTANTIATE_TEMPLATE(0x20c0cc00);
    INSTANTIATE_TEMPLATE(0x20c0f000);
    INSTANTIATE_TEMPLATE(0x20c0f800);
    INSTANTIATE_TEMPLATE(0x20c1f000);
    INSTANTIATE_TEMPLATE(0x51e00000);
    INSTANTIATE_TEMPLATE(0x60007800);
    INSTANTIATE_TEMPLATE(0x6000f800);
    INSTANTIATE_TEMPLATE(0x601e0000);
    INSTANTIATE_TEMPLATE(0x80007c00);
    INSTANTIATE_TEMPLATE(0x80017c00);
    INSTANTIATE_TEMPLATE(0x80408000);
    INSTANTIATE_TEMPLATE(0x80a07c00);
    INSTANTIATE_TEMPLATE(0x80df0000);
    INSTANTIATE_TEMPLATE(0x80e08000);
    INSTANTIATE_TEMPLATE(0xa0c00000);
    INSTANTIATE_TEMPLATE(0xb5a00000);
    INSTANTIATE_TEMPLATE(0xc0c00c00);
    INSTANTIATE_TEMPLATE(0xc4400000);
    INSTANTIATE_TEMPLATE(0xc4c00000);
    INSTANTIATE_TEMPLATE(0xe0400000);
    INSTANTIATE_TEMPLATE(0xe120e000);
    INSTANTIATE_TEMPLATE(0xe3c00000);
    INSTANTIATE_TEMPLATE(0xf1200000);
#undef INSTANTIATE_TEMPLATE
    default:
      printf("Node %s: No template instantiated for extracting 0x%08x.\n",
             GetName().c_str(),
             GenerateSampledBitsMask());
      printf("Add one in %s above line %d:\n", __FILE__, __LINE__);
      printf("  INSTANTIATE_TEMPLATE(0x%08x);\n", GenerateSampledBitsMask());
      VIXL_UNREACHABLE();
  }
  return bit_extract_fn;
}

BitExtractFn DecodeNode::GetBitExtractFunction(uint32_t mask, uint32_t value) {
  // Instantiate a templated bit extraction function for every pattern we
  // might encounter. If the assertion in the following check fails, add a
  // new instantiation below using the information in the failure message.
  bool instantiated = false;
  BitExtractFn bit_extract_fn = NULL;
#define INSTANTIATE_TEMPLATE(M, V)                      \
  if ((mask == M) && (value == V)) {                    \
    bit_extract_fn = &Instruction::IsMaskedValue<M, V>; \
    instantiated = true;                                \
  }
  INSTANTIATE_TEMPLATE(0x0000001c, 0x00000000);
  INSTANTIATE_TEMPLATE(0x00000210, 0x00000000);
  INSTANTIATE_TEMPLATE(0x000003c0, 0x00000000);
  INSTANTIATE_TEMPLATE(0x00001c00, 0x00000000);
  INSTANTIATE_TEMPLATE(0x00001c0f, 0x00000000);
  INSTANTIATE_TEMPLATE(0x00003000, 0x00000000);
  INSTANTIATE_TEMPLATE(0x00007800, 0x00000000);
  INSTANTIATE_TEMPLATE(0x0000e000, 0x0000a000);
  INSTANTIATE_TEMPLATE(0x0000f000, 0x00000000);
  INSTANTIATE_TEMPLATE(0x00030400, 0x00000000);
  INSTANTIATE_TEMPLATE(0x0003801f, 0x0000000d);
  INSTANTIATE_TEMPLATE(0x00060210, 0x00000000);
  INSTANTIATE_TEMPLATE(0x00060810, 0x00000000);
  INSTANTIATE_TEMPLATE(0x00060a10, 0x00000000);
  INSTANTIATE_TEMPLATE(0x00060bf0, 0x00000000);
  INSTANTIATE_TEMPLATE(0x00061e10, 0x00000000);
  INSTANTIATE_TEMPLATE(0x00061e10, 0x00000400);
  INSTANTIATE_TEMPLATE(0x00070200, 0x00000000);
  INSTANTIATE_TEMPLATE(0x000b1e10, 0x00000000);
  INSTANTIATE_TEMPLATE(0x000f0000, 0x00000000);
  INSTANTIATE_TEMPLATE(0x00130e1f, 0x00000000);
  INSTANTIATE_TEMPLATE(0x00130fff, 0x00000000);
  INSTANTIATE_TEMPLATE(0x00180000, 0x00000000);
  INSTANTIATE_TEMPLATE(0x00180000, 0x00100000);
  INSTANTIATE_TEMPLATE(0x001e0000, 0x00000000);
  INSTANTIATE_TEMPLATE(0x001f0000, 0x00000000);
  INSTANTIATE_TEMPLATE(0x001f0000, 0x001f0000);
  INSTANTIATE_TEMPLATE(0x0038e000, 0x00000000);
  INSTANTIATE_TEMPLATE(0x0039e000, 0x00002000);
  INSTANTIATE_TEMPLATE(0x003ae000, 0x00002000);
  INSTANTIATE_TEMPLATE(0x003ce000, 0x00042000);
  INSTANTIATE_TEMPLATE(0x005f0000, 0x001f0000);
  INSTANTIATE_TEMPLATE(0x00780000, 0x00000000);
  INSTANTIATE_TEMPLATE(0x00870210, 0x00000000);
  INSTANTIATE_TEMPLATE(0x00c00000, 0x00000000);
  INSTANTIATE_TEMPLATE(0x00c00000, 0x00800000);
  INSTANTIATE_TEMPLATE(0x00c00000, 0x00c00000);
  INSTANTIATE_TEMPLATE(0x00c00010, 0x00800000);
  INSTANTIATE_TEMPLATE(0x00ca1e10, 0x00000000);
  INSTANTIATE_TEMPLATE(0x01000010, 0x00000000);
  INSTANTIATE_TEMPLATE(0x20000800, 0x00000000);
  INSTANTIATE_TEMPLATE(0x20008000, 0x00000000);
  INSTANTIATE_TEMPLATE(0x20040000, 0x00000000);
  INSTANTIATE_TEMPLATE(0x201e8000, 0x00000000);
  INSTANTIATE_TEMPLATE(0x60000000, 0x00000000);
  INSTANTIATE_TEMPLATE(0x60000000, 0x20000000);
  INSTANTIATE_TEMPLATE(0x60000000, 0x60000000);
  INSTANTIATE_TEMPLATE(0x60200000, 0x00000000);
  INSTANTIATE_TEMPLATE(0x80008000, 0x00000000);
  INSTANTIATE_TEMPLATE(0x80008000, 0x00008000);
  INSTANTIATE_TEMPLATE(0x80400000, 0x00400000);
  INSTANTIATE_TEMPLATE(0xa00003e0, 0x00000000);
  INSTANTIATE_TEMPLATE(0xa000c007, 0x00000000);
  INSTANTIATE_TEMPLATE(0xa0100000, 0x00000000);
  INSTANTIATE_TEMPLATE(0xc4000000, 0xc0000000);
  INSTANTIATE_TEMPLATE(0xc4000000, 0xc4000000);
  INSTANTIATE_TEMPLATE(0xe0000010, 0xa0000000);
  INSTANTIATE_TEMPLATE(0xe01c0000, 0x20000000);
  INSTANTIATE_TEMPLATE(0xe1ff0000, 0x00000000);
#undef INSTANTIATE_TEMPLATE

  if (!instantiated) {
    printf(
        "Node %s: no template instantiated for mask 0x%08x, value = "
        "0x%08x.\n",
        GetName().c_str(),
        mask,
        value);
    printf("Add one in %s above line %d:\n", __FILE__, __LINE__);
    printf("  INSTANTIATE_TEMPLATE(0x%08x, 0x%08x);\n", mask, value);
    VIXL_UNREACHABLE();
  }
  return bit_extract_fn;
}

bool DecodeNode::TryCompileOptimisedDecodeTable(Decoder* decoder) {
  // EitherOr optimisation: if there are only one or two patterns in the table,
  // try to optimise the node to exploit that.
  size_t table_size = pattern_table_.size();
  if ((table_size <= 2) && (GetSampledBitsCount() > 1)) {
    // TODO: support 'x' in this optimisation by dropping the sampled bit
    // positions before making the mask/value.
    if ((strchr(pattern_table_[0].pattern, 'x') == NULL) &&
        ((table_size == 1) ||
         (strcmp(pattern_table_[1].pattern, "otherwise") == 0))) {
      // A pattern table consisting of a fixed pattern with no x's, and an
      // "otherwise" or absent case. Optimise this into an instruction mask and
      // value test.
      uint32_t single_decode_mask = 0;
      uint32_t single_decode_value = 0;
      std::vector<uint8_t> bits = GetSampledBits();

      // Construct the instruction mask and value from the pattern.
      VIXL_ASSERT(bits.size() == strlen(pattern_table_[0].pattern));
      for (size_t i = 0; i < bits.size(); i++) {
        single_decode_mask |= 1U << bits[i];
        if (pattern_table_[0].pattern[i] == '1') {
          single_decode_value |= 1U << bits[i];
        }
      }
      BitExtractFn bit_extract_fn =
          GetBitExtractFunction(single_decode_mask, single_decode_value);

      // Create a compiled node that contains a two entry table for the
      // either/or cases.
      CreateCompiledNode(bit_extract_fn, 2);

      // Set DecodeNode for when the instruction after masking doesn't match the
      // value.
      const char* doesnt_match_handler =
          (table_size == 1) ? "VisitUnallocated" : pattern_table_[1].handler;
      CompileNodeForBits(decoder, doesnt_match_handler, 0);

      // Set DecodeNode for when it does match.
      CompileNodeForBits(decoder, pattern_table_[0].handler, 1);

      return true;
    }
  }
  return false;
}

CompiledDecodeNode* DecodeNode::Compile(Decoder* decoder) {
  if (IsLeafNode()) {
    // A leaf node is a simple wrapper around a visitor function, with no
    // instruction decoding to do.
    CreateVisitorNode();
  } else if (!TryCompileOptimisedDecodeTable(decoder)) {
    // The "otherwise" node is the default next node if no pattern matches.
    std::string otherwise = "VisitUnallocated";

    // For each pattern in pattern_table_, create an entry in matches that
    // has a corresponding mask and value for the pattern.
    std::vector<MaskValuePair> matches;
    for (size_t i = 0; i < pattern_table_.size(); i++) {
      if (strcmp(pattern_table_[i].pattern, "otherwise") == 0) {
        // "otherwise" must be the last pattern in the list, otherwise the
        // indices won't match for pattern_table_ and matches.
        VIXL_ASSERT(i == pattern_table_.size() - 1);
        otherwise = pattern_table_[i].handler;
      } else {
        matches.push_back(GenerateMaskValuePair(
            GenerateOrderedPattern(pattern_table_[i].pattern)));
      }
    }

    BitExtractFn bit_extract_fn =
        GetBitExtractFunction(GenerateSampledBitsMask());

    // Create a compiled node that contains a table with an entry for every bit
    // pattern.
    CreateCompiledNode(bit_extract_fn, 1U << GetSampledBitsCount());
    VIXL_ASSERT(compiled_node_ != NULL);

    // When we find a pattern matches the representation, set the node's decode
    // function for that representation to the corresponding function.
    for (uint32_t bits = 0; bits < (1U << GetSampledBitsCount()); bits++) {
      for (size_t i = 0; i < matches.size(); i++) {
        if ((bits & matches[i].first) == matches[i].second) {
          // Only one instruction class should match for each value of bits, so
          // if we get here, the node pointed to should still be unallocated.
          VIXL_ASSERT(compiled_node_->GetNodeForBits(bits) == NULL);
          CompileNodeForBits(decoder, pattern_table_[i].handler, bits);
          break;
        }
      }

      // If the decode_table_ entry for these bits is still NULL, the
      // instruction must be handled by the "otherwise" case, which by default
      // is the Unallocated visitor.
      if (compiled_node_->GetNodeForBits(bits) == NULL) {
        CompileNodeForBits(decoder, otherwise, bits);
      }
    }
  }

  VIXL_ASSERT(compiled_node_ != NULL);
  return compiled_node_;
}

void CompiledDecodeNode::Decode(const Instruction* instr) const {
  if (IsLeafNode()) {
    // If this node is a leaf, call the registered visitor function.
    VIXL_ASSERT(decoder_ != NULL);
    (decoder_->*visitor_fn_)(instr);
  } else {
    // Otherwise, using the sampled bit extractor for this node, look up the
    // next node in the decode tree, and call its Decode method.
    VIXL_ASSERT(bit_extract_fn_ != NULL);
    VIXL_ASSERT((instr->*bit_extract_fn_)() < decode_table_size_);
    VIXL_ASSERT(decode_table_[(instr->*bit_extract_fn_)()] != NULL);
    decode_table_[(instr->*bit_extract_fn_)()]->Decode(instr);
  }
}

DecodeNode::MaskValuePair DecodeNode::GenerateMaskValuePair(
    std::string pattern) const {
  uint32_t mask = 0, value = 0;
  for (size_t i = 0; i < pattern.size(); i++) {
    mask |= ((pattern[i] == 'x') ? 0 : 1) << i;
    value |= ((pattern[i] == '1') ? 1 : 0) << i;
  }
  return std::make_pair(mask, value);
}

std::string DecodeNode::GenerateOrderedPattern(std::string pattern) const {
  std::vector<uint8_t> sampled_bits = GetSampledBits();
  // Construct a temporary 32-character string containing '_', then at each
  // sampled bit position, set the corresponding pattern character.
  std::string temp(32, '_');
  for (size_t i = 0; i < sampled_bits.size(); i++) {
    temp[sampled_bits[i]] = pattern[i];
  }

  // Iterate through the temporary string, filtering out the non-'_' characters
  // into a new ordered pattern result string.
  std::string result;
  for (size_t i = 0; i < temp.size(); i++) {
    if (temp[i] != '_') {
      result.push_back(temp[i]);
    }
  }
  VIXL_ASSERT(result.size() == sampled_bits.size());
  return result;
}

uint32_t DecodeNode::GenerateSampledBitsMask() const {
  std::vector<uint8_t> sampled_bits = GetSampledBits();
  uint32_t mask = 0;
  for (size_t i = 0; i < sampled_bits.size(); i++) {
    mask |= 1 << sampled_bits[i];
  }
  return mask;
}

}  // namespace aarch64
}  // namespace vixl
