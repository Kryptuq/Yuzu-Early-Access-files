// SPDX-FileCopyrightText: Copyright (c) 2022 merryhime <https://mary.rs>
// SPDX-License-Identifier: MIT

#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

#include "oaknut/impl/enum.hpp"
#include "oaknut/impl/imm.hpp"
#include "oaknut/impl/list.hpp"
#include "oaknut/impl/multi_typed_name.hpp"
#include "oaknut/impl/offset.hpp"
#include "oaknut/impl/reg.hpp"
#include "oaknut/impl/string_literal.hpp"

namespace oaknut {

namespace detail {

template<StringLiteral bs, StringLiteral barg>
constexpr std::uint32_t get_bits()
{
    std::uint32_t result = 0;
    for (std::size_t i = 0; i < 32; i++) {
        for (std::size_t a = 0; a < barg.strlen; a++) {
            if (bs.value[i] == barg.value[a]) {
                result |= 1 << (31 - i);
            }
        }
    }
    return result;
}

template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

}  // namespace detail

struct Label {
public:
    Label() = default;

private:
    template<typename Policy>
    friend class BasicCodeGenerator;

    explicit Label(std::uintptr_t addr)
        : m_addr(addr)
    {}

    using EmitFunctionType = std::uint32_t (*)(std::uintptr_t wb_addr, std::uintptr_t resolved_addr);

    struct Writeback {
        std::uintptr_t m_wb_addr;
        std::uint32_t m_mask;
        EmitFunctionType m_fn;
    };

    std::optional<std::uintptr_t> m_addr;
    std::vector<Writeback> m_wbs;
};

template<typename Policy>
class BasicCodeGenerator : public Policy {
public:
    BasicCodeGenerator(typename Policy::constructor_argument_type arg)
        : Policy(arg)
    {}

    Label l()
    {
        return Label{Policy::current_address()};
    }

    void l(Label& label)
    {
        if (label.m_addr)
            throw "label already resolved";

        const auto target_addr = Policy::current_address();
        label.m_addr = target_addr;
        for (auto& wb : label.m_wbs) {
            const std::uint32_t value = wb.m_fn(wb.m_wb_addr, target_addr);
            Policy::set_at_address(wb.m_wb_addr, value, wb.m_mask);
        }
        label.m_wbs.clear();
    }

#include "oaknut/impl/arm64_mnemonics.inc.hpp"
#include "oaknut/impl/fpsimd_mnemonics.inc.hpp"

    void RET()
    {
        return RET(XReg{30});
    }

    void MOV(WReg wd, uint32_t imm)
    {
        if (wd.index() == 31)
            return;
        if (MovImm16::is_valid(imm))
            return MOVZ(wd, imm);
        if (MovImm16::is_valid(~static_cast<std::uint64_t>(imm)))
            return MOVN(wd, imm);
        if (detail::encode_bit_imm(imm))
            return ORR(wd, WzrReg{}, imm);

        MOVZ(wd, {static_cast<std::uint16_t>(imm >> 0), MovImm16Shift::SHL_0});
        MOVK(wd, {static_cast<std::uint16_t>(imm >> 16), MovImm16Shift::SHL_16});
    }

    void MOV(XReg xd, uint64_t imm)
    {
        if (xd.index() == 31)
            return;
        if (imm >> 32 == 0)
            return MOV(xd.toW(), static_cast<std::uint32_t>(imm));
        if (MovImm16::is_valid(imm))
            return MOVZ(xd, imm);
        if (MovImm16::is_valid(~imm))
            return MOVN(xd, imm);
        if (detail::encode_bit_imm(imm))
            return ORR(xd, ZrReg{}, imm);

        bool movz_done = false;
        int shift_count = 0;

        if (detail::encode_bit_imm(static_cast<std::uint32_t>(imm))) {
            ORR(xd.toW(), WzrReg{}, static_cast<std::uint32_t>(imm));
            imm >>= 32;
            movz_done = true;
            shift_count = 2;
        }

        while (imm != 0) {
            const uint16_t hw = static_cast<uint16_t>(imm);
            if (hw != 0) {
                if (movz_done) {
                    MOVK(xd, {hw, static_cast<MovImm16Shift>(shift_count)});
                } else {
                    MOVZ(xd, {hw, static_cast<MovImm16Shift>(shift_count)});
                    movz_done = true;
                }
            }
            imm >>= 16;
            shift_count++;
        }
    }

private:
#include "oaknut/impl/arm64_encode_helpers.inc.hpp"

    template<StringLiteral bs, StringLiteral... bargs, typename... Ts>
    void emit(Ts... args)
    {
        std::uint32_t encoding = detail::get_bits<bs, "1">();
        encoding |= (0 | ... | encode<detail::get_bits<bs, bargs>()>(std::forward<Ts>(args)));
        Policy::append(encoding);
    }

    template<std::uint32_t splat, std::size_t size, std::size_t align>
    std::uint32_t encode(AddrOffset<size, align> v)
    {
        static_assert(std::popcount(splat) == size - align);

        const auto encode_fn = [](std::uintptr_t current_addr, std::uintptr_t target) {
            const std::ptrdiff_t diff = target - current_addr;
            return pdep<splat>(AddrOffset<size, align>::encode(diff));
        };

        return std::visit(detail::overloaded{
                              [&](std::uint32_t encoding) {
                                  return pdep<splat>(encoding);
                              },
                              [&](Label* label) {
                                  if (label->m_addr) {
                                      return encode_fn(Policy::current_address(), *label->m_addr);
                                  }

                                  label->m_wbs.emplace_back(Label::Writeback{Policy::current_address(), ~splat, static_cast<Label::EmitFunctionType>(encode_fn)});
                                  return 0u;
                              },
                              [&](void* p) {
                                  return encode_fn(Policy::current_address(), reinterpret_cast<std::uintptr_t>(p));
                              },
                          },
                          v.m_payload);
    }

    template<std::uint32_t splat, std::size_t size>
    std::uint32_t encode(PageOffset<size> v)
    {
        static_assert(std::popcount(splat) == size);

        const auto encode_fn = [](std::uintptr_t current_addr, std::uintptr_t target) {
            return pdep<splat>(PageOffset<size>::encode(current_addr, target));
        };

        return std::visit(detail::overloaded{
                              [&](Label* label) {
                                  if (label->m_addr) {
                                      return encode_fn(Policy::current_address(), *label->m_addr);
                                  }

                                  label->m_wbs.emplace_back(Label::Writeback{Policy::current_address(), ~splat, static_cast<Label::EmitFunctionType>(encode_fn)});
                                  return 0u;
                              },
                              [&](void* p) {
                                  return encode_fn(Policy::current_address(), reinterpret_cast<std::uintptr_t>(p));
                              },
                          },
                          v.m_payload);
    }
};

struct PointerCodeGeneratorPolicy {
public:
    template<typename T>
    T ptr()
    {
        static_assert(std::is_pointer_v<T>);
        return reinterpret_cast<T>(m_ptr);
    }

    void set_ptr(std::uint32_t* ptr_)
    {
        m_ptr = ptr_;
    }

protected:
    using constructor_argument_type = std::uint32_t*;

    PointerCodeGeneratorPolicy(std::uint32_t* ptr_)
        : m_ptr(ptr_)
    {}

    void append(std::uint32_t instruction)
    {
        *m_ptr++ = instruction;
    }

    std::uintptr_t current_address()
    {
        return reinterpret_cast<std::uintptr_t>(m_ptr);
    }

    void set_at_address(std::uintptr_t addr, std::uint32_t value, std::uint32_t mask)
    {
        std::uint32_t* p = reinterpret_cast<std::uint32_t*>(addr);
        *p = (*p & mask) | value;
    }

private:
    std::uint32_t* m_ptr;
};

using CodeGenerator = BasicCodeGenerator<PointerCodeGeneratorPolicy>;

namespace util {

inline constexpr WReg W0{0}, W1{1}, W2{2}, W3{3}, W4{4}, W5{5}, W6{6}, W7{7}, W8{8}, W9{9}, W10{10}, W11{11}, W12{12}, W13{13}, W14{14}, W15{15}, W16{16}, W17{17}, W18{18}, W19{19}, W20{20}, W21{21}, W22{22}, W23{23}, W24{24}, W25{25}, W26{26}, W27{27}, W28{28}, W29{29}, W30{30};
inline constexpr XReg X0{0}, X1{1}, X2{2}, X3{3}, X4{4}, X5{5}, X6{6}, X7{7}, X8{8}, X9{9}, X10{10}, X11{11}, X12{12}, X13{13}, X14{14}, X15{15}, X16{16}, X17{17}, X18{18}, X19{19}, X20{20}, X21{21}, X22{22}, X23{23}, X24{24}, X25{25}, X26{26}, X27{27}, X28{28}, X29{29}, X30{30};
inline constexpr ZrReg ZR{}, XZR{};
inline constexpr WzrReg WZR{};
inline constexpr SpReg SP{}, XSP{};
inline constexpr WspReg WSP{};

inline constexpr VRegSelector V0{0}, V1{1}, V2{2}, V3{3}, V4{4}, V5{5}, V6{6}, V7{7}, V8{8}, V9{9}, V10{10}, V11{11}, V12{12}, V13{13}, V14{14}, V15{15}, V16{16}, V17{17}, V18{18}, V19{19}, V20{20}, V21{21}, V22{22}, V23{23}, V24{24}, V25{25}, V26{26}, V27{27}, V28{28}, V29{29}, V30{30}, V31{31};
inline constexpr QReg Q0{0}, Q1{1}, Q2{2}, Q3{3}, Q4{4}, Q5{5}, Q6{6}, Q7{7}, Q8{8}, Q9{9}, Q10{10}, Q11{11}, Q12{12}, Q13{13}, Q14{14}, Q15{15}, Q16{16}, Q17{17}, Q18{18}, Q19{19}, Q20{20}, Q21{21}, Q22{22}, Q23{23}, Q24{24}, Q25{25}, Q26{26}, Q27{27}, Q28{28}, Q29{29}, Q30{30}, Q31{31};
inline constexpr DReg D0{0}, D1{1}, D2{2}, D3{3}, D4{4}, D5{5}, D6{6}, D7{7}, D8{8}, D9{9}, D10{10}, D11{11}, D12{12}, D13{13}, D14{14}, D15{15}, D16{16}, D17{17}, D18{18}, D19{19}, D20{20}, D21{21}, D22{22}, D23{23}, D24{24}, D25{25}, D26{26}, D27{27}, D28{28}, D29{29}, D30{30}, D31{31};
inline constexpr SReg S0{0}, S1{1}, S2{2}, S3{3}, S4{4}, S5{5}, S6{6}, S7{7}, S8{8}, S9{9}, S10{10}, S11{11}, S12{12}, S13{13}, S14{14}, S15{15}, S16{16}, S17{17}, S18{18}, S19{19}, S20{20}, S21{21}, S22{22}, S23{23}, S24{24}, S25{25}, S26{26}, S27{27}, S28{28}, S29{29}, S30{30}, S31{31};
inline constexpr HReg H0{0}, H1{1}, H2{2}, H3{3}, H4{4}, H5{5}, H6{6}, H7{7}, H8{8}, H9{9}, H10{10}, H11{11}, H12{12}, H13{13}, H14{14}, H15{15}, H16{16}, H17{17}, H18{18}, H19{19}, H20{20}, H21{21}, H22{22}, H23{23}, H24{24}, H25{25}, H26{26}, H27{27}, H28{28}, H29{29}, H30{30}, H31{31};
inline constexpr BReg B0{0}, B1{1}, B2{2}, B3{3}, B4{4}, B5{5}, B6{6}, B7{7}, B8{8}, B9{9}, B10{10}, B11{11}, B12{12}, B13{13}, B14{14}, B15{15}, B16{16}, B17{17}, B18{18}, B19{19}, B20{20}, B21{21}, B22{22}, B23{23}, B24{24}, B25{25}, B26{26}, B27{27}, B28{28}, B29{29}, B30{30}, B31{31};

inline constexpr Cond EQ{Cond::EQ}, NE{Cond::NE}, CS{Cond::CS}, CC{Cond::CC}, MI{Cond::MI}, PL{Cond::PL}, VS{Cond::VS}, VC{Cond::VC}, HI{Cond::HI}, LS{Cond::LS}, GE{Cond::GE}, LT{Cond::LT}, GT{Cond::GT}, LE{Cond::LE}, AL{Cond::AL}, NV{Cond::NV}, HS{Cond::HS}, LO{Cond::LO};

inline constexpr auto UXTB{MultiTypedName<AddSubExt::UXTB>{}};
inline constexpr auto UXTH{MultiTypedName<AddSubExt::UXTH>{}};
inline constexpr auto UXTW{MultiTypedName<AddSubExt::UXTW, IndexExt::UXTW>{}};
inline constexpr auto UXTX{MultiTypedName<AddSubExt::UXTX>{}};
inline constexpr auto SXTB{MultiTypedName<AddSubExt::SXTB>{}};
inline constexpr auto SXTH{MultiTypedName<AddSubExt::SXTH>{}};
inline constexpr auto SXTW{MultiTypedName<AddSubExt::SXTW, IndexExt::SXTW>{}};
inline constexpr auto SXTX{MultiTypedName<AddSubExt::SXTX, IndexExt::SXTX>{}};
inline constexpr auto LSL{MultiTypedName<AddSubExt::LSL, IndexExt::LSL, AddSubShift::LSL, LogShift::LSL, LslSymbol::LSL>{}};
inline constexpr auto LSR{MultiTypedName<AddSubShift::LSR, LogShift::LSR>{}};
inline constexpr auto ASR{MultiTypedName<AddSubShift::ASR, LogShift::ASR>{}};
inline constexpr auto ROR{MultiTypedName<LogShift::ROR>{}};

inline constexpr PostIndexed POST_INDEXED{};
inline constexpr PreIndexed PRE_INDEXED{};
inline constexpr MslSymbol MSL{MslSymbol::MSL};

}  // namespace util

}  // namespace oaknut
