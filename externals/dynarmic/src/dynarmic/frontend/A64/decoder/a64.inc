// Data processing - Immediate - PC relative addressing
INST(ADR,                    "ADR",                                       "0ii10000iiiiiiiiiiiiiiiiiiiddddd")
INST(ADRP,                   "ADRP",                                      "1ii10000iiiiiiiiiiiiiiiiiiiddddd")

// Data processing - Immediate - Add/Sub (with tags)
//INST(ADDG,                   "ADDG",                                      "1001000110iiiiii00IIIInnnnnddddd") // ARMv8.5
//INST(SUBG,                   "SUBG",                                      "1101000110iiiiii00IIIInnnnnddddd") // ARMv8.5

// Data processing - Immediate - Add/Sub
INST(ADD_imm,                "ADD (immediate)",                           "z0010001ssiiiiiiiiiiiinnnnnddddd")
INST(ADDS_imm,               "ADDS (immediate)",                          "z0110001ssiiiiiiiiiiiinnnnnddddd")
INST(SUB_imm,                "SUB (immediate)",                           "z1010001ssiiiiiiiiiiiinnnnnddddd")
INST(SUBS_imm,               "SUBS (immediate)",                          "z1110001ssiiiiiiiiiiiinnnnnddddd")

// Data processing - Immediate - Logical
INST(AND_imm,                "AND (immediate)",                           "z00100100Nrrrrrrssssssnnnnnddddd")
INST(ORR_imm,                "ORR (immediate)",                           "z01100100Nrrrrrrssssssnnnnnddddd")
INST(EOR_imm,                "EOR (immediate)",                           "z10100100Nrrrrrrssssssnnnnnddddd")
INST(ANDS_imm,               "ANDS (immediate)",                          "z11100100Nrrrrrrssssssnnnnnddddd")

// Data processing - Immediate - Move Wide
INST(MOVN,                   "MOVN",                                      "z00100101ssiiiiiiiiiiiiiiiiddddd")
INST(MOVZ,                   "MOVZ",                                      "z10100101ssiiiiiiiiiiiiiiiiddddd")
INST(MOVK,                   "MOVK",                                      "z11100101ssiiiiiiiiiiiiiiiiddddd")

// Data processing - Immediate - Bitfield
INST(SBFM,                   "SBFM",                                      "z00100110Nrrrrrrssssssnnnnnddddd")
INST(BFM,                    "BFM",                                       "z01100110Nrrrrrrssssssnnnnnddddd")
INST(UBFM,                   "UBFM",                                      "z10100110Nrrrrrrssssssnnnnnddddd")
INST(ASR_1,                  "ASR (immediate, 32-bit)",                   "00010011000rrrrr011111nnnnnddddd")
INST(ASR_2,                  "ASR (immediate, 64-bit)",                   "1001001101rrrrrr111111nnnnnddddd")
INST(SXTB_1,                 "SXTB (32-bit)",                             "0001001100000000000111nnnnnddddd")
INST(SXTB_2,                 "SXTB (64-bit)",                             "1001001101000000000111nnnnnddddd")
INST(SXTH_1,                 "SXTH (32-bit)",                             "0001001100000000001111nnnnnddddd")
INST(SXTH_2,                 "SXTH (64-bit)",                             "1001001101000000001111nnnnnddddd")
INST(SXTW,                   "SXTW",                                      "1001001101000000011111nnnnnddddd")

// Data processing - Immediate - Extract
INST(EXTR,                   "EXTR",                                      "z00100111N0mmmmmssssssnnnnnddddd")

// Conditional branch
INST(B_cond,                 "B.cond",                                    "01010100iiiiiiiiiiiiiiiiiii0cccc")

// Exception generation
INST(SVC,                    "SVC",                                       "11010100000iiiiiiiiiiiiiiii00001")
//INST(HVC,                    "HVC",                                       "11010100000iiiiiiiiiiiiiiii00010")
//INST(SMC,                    "SMC",                                       "11010100000iiiiiiiiiiiiiiii00011")
INST(BRK,                    "BRK",                                       "11010100001iiiiiiiiiiiiiiii00000")
//INST(HLT,                    "HLT",                                       "11010100010iiiiiiiiiiiiiiii00000")
//INST(DCPS1,                  "DCPS1",                                     "11010100101iiiiiiiiiiiiiiii00001")
//INST(DCPS2,                  "DCPS2",                                     "11010100101iiiiiiiiiiiiiiii00010")
//INST(DCPS3,                  "DCPS3",                                     "11010100101iiiiiiiiiiiiiiii00011")

// System
//INST(MSR_imm,                "MSR (immediate)",                           "1101010100000ooo0100MMMMooo11111")
INST(HINT,                   "HINT",                                      "11010101000000110010MMMMooo11111")
INST(NOP,                    "NOP",                                       "11010101000000110010000000011111")
INST(YIELD,                  "YIELD",                                     "11010101000000110010000000111111")
INST(WFE,                    "WFE",                                       "11010101000000110010000001011111")
INST(WFI,                    "WFI",                                       "11010101000000110010000001111111")
INST(SEV,                    "SEV",                                       "11010101000000110010000010011111")
INST(SEVL,                   "SEVL",                                      "11010101000000110010000010111111")
//INST(DGH,                    "DGH",                                       "11010101000000110010000011011111") // v8.6
//INST(WFET,                   "WFET",                                      "110101010000001100010000000ddddd") // v8.7
//INST(WFIT,                   "WFIT",                                      "110101010000001100010000001ddddd") // v8.7
//INST(XPAC_1,                 "XPACD, XPACI, XPACLRI",                     "110110101100000101000D11111ddddd")
//INST(XPAC_2,                 "XPACD, XPACI, XPACLRI",                     "11010101000000110010000011111111")
//INST(PACIA_1,                "PACIA, PACIA1716, PACIASP, PACIAZ, PACIZA", "110110101100000100Z000nnnnnddddd")
//INST(PACIA_2,                "PACIA, PACIA1716, PACIASP, PACIAZ, PACIZA", "1101010100000011001000-100-11111")
//INST(PACIB_1,                "PACIB, PACIB1716, PACIBSP, PACIBZ, PACIZB", "110110101100000100Z001nnnnnddddd")
//INST(PACIB_2,                "PACIB, PACIB1716, PACIBSP, PACIBZ, PACIZB", "1101010100000011001000-101-11111")
//INST(AUTIA_1,                "AUTIA, AUTIA1716, AUTIASP, AUTIAZ, AUTIZA", "110110101100000100Z100nnnnnddddd")
//INST(AUTIA_2,                "AUTIA, AUTIA1716, AUTIASP, AUTIAZ, AUTIZA", "1101010100000011001000-110-11111")
//INST(AUTIB_1,                "AUTIB, AUTIB1716, AUTIBSP, AUTIBZ, AUTIZB", "110110101100000100Z101nnnnnddddd")
//INST(AUTIB_2,                "AUTIB, AUTIB1716, AUTIBSP, AUTIBZ, AUTIZB", "1101010100000011001000-111-11111")
//INST(BTI,                    "BTI",                                       "110101010000001100100100ii011111") // ARMv8.5
//INST(ESB,                    "ESB",                                       "11010101000000110010001000011111")
//INST(PSB,                    "PSB CSYNC",                                 "11010101000000110010001000111111")
//INST(TSB,                    "TSB CSYNC",                                 "11010101000000110010001001011111") // ARMv8.5
//INST(CSDB,                   "CSDB",                                      "11010101000000110010001010011111")
INST(CLREX,                  "CLREX",                                     "11010101000000110011MMMM01011111")
INST(DSB,                    "DSB",                                       "11010101000000110011MMMM10011111")
//INST(SSBB,                   "SSBB",                                      "11010101000000110011000010011111")
//INST(PSSBB,                  "PSSBB",                                     "11010101000000110011010010011111")
INST(DMB,                    "DMB",                                       "11010101000000110011MMMM10111111")
INST(ISB,                    "ISB",                                       "11010101000000110011MMMM11011111")
//INST(SB,                     "SB",                                        "11010101000000110011000011111111")
//INST(SYS,                    "SYS",                                       "1101010100001oooNNNNMMMMooottttt")
INST(MSR_reg,                "MSR (register)",                            "110101010001poooNNNNMMMMooottttt")
//INST(SYSL,                   "SYSL",                                      "1101010100101oooNNNNMMMMooottttt")
INST(MRS,                    "MRS",                                       "110101010011poooNNNNMMMMooottttt")

// System - Flag manipulation instructions
INST(CFINV,                  "CFINV",                                     "11010101000000000100000000011111") // ARMv8.4
INST(RMIF,                   "RMIF",                                      "10111010000iiiiii00001nnnnn0IIII") // ARMv8.4
//INST(SETF8,                  "SETF8",                                     "0011101000000000000010nnnnn01101") // ARMv8.4
//INST(SETF16,                 "SETF16",                                    "0011101000000000010010nnnnn01101") // ARMv8.4

// System - Flag format instructions
INST(XAFlag,                 "XAFlag",                                    "11010101000000000100000000111111") // ARMv8.5
INST(AXFlag,                 "AXFlag",                                    "11010101000000000100000001011111") // ARMv8.5

// SYS: Data Cache
INST(DC_IVAC,                "DC IVAC",                                   "110101010000100001110110001ttttt")
INST(DC_ISW,                 "DC ISW",                                    "110101010000100001110110010ttttt")
INST(DC_CSW,                 "DC CSW",                                    "110101010000100001111010010ttttt")
INST(DC_CISW,                "DC CISW",                                   "110101010000100001111110010ttttt")
INST(DC_ZVA,                 "DC ZVA",                                    "110101010000101101110100001ttttt")
INST(DC_CVAC,                "DC CVAC",                                   "110101010000101101111010001ttttt")
INST(DC_CVAU,                "DC CVAU",                                   "110101010000101101111011001ttttt")
INST(DC_CVAP,                "DC CVAP",                                   "110101010000101101111100001ttttt")
INST(DC_CIVAC,               "DC CIVAC",                                  "110101010000101101111110001ttttt")

// SYS: Instruction Cache
INST(IC_IALLU,               "IC IALLU",                                  "11010101000010000111010100011111")
INST(IC_IALLUIS,             "IC IALLUIS",                                "11010101000010000111000100011111")
INST(IC_IVAU,                "IC IVAU",                                   "110101010000101101110101001ttttt")

// Unconditional branch (Register)
INST(BLR,                    "BLR",                                       "1101011000111111000000nnnnn00000")
INST(BR,                     "BR",                                        "1101011000011111000000nnnnn00000")
//INST(DRPS,                   "DRPS",                                      "11010110101111110000001111100000")
//INST(ERET,                   "ERET",                                      "11010110100111110000001111100000")
INST(RET,                    "RET",                                       "1101011001011111000000nnnnn00000")
//INST(BLRA,                   "BLRAA, BLRAAZ, BLRAB, BLRABZ",              "1101011Z0011111100001Mnnnnnmmmmm") // ARMv8.3
//INST(BRA,                    "BRAA, BRAAZ, BRAB, BRABZ",                  "1101011Z0001111100001Mnnnnnmmmmm") // ARMv8.3
//INST(ERETA,                  "ERETAA, ERETAB",                            "110101101001111100001M1111111111") // ARMv8.3
//INST(RETA,                   "RETAA, RETAB",                              "110101100101111100001M1111111111") // ARMv8.3

// Unconditional branch (immediate)
INST(B_uncond,               "B",                                         "000101iiiiiiiiiiiiiiiiiiiiiiiiii")
INST(BL,                     "BL",                                        "100101iiiiiiiiiiiiiiiiiiiiiiiiii")

// Compare and branch (immediate)
INST(CBZ,                    "CBZ",                                       "z0110100iiiiiiiiiiiiiiiiiiittttt")
INST(CBNZ,                   "CBNZ",                                      "z0110101iiiiiiiiiiiiiiiiiiittttt")
INST(TBZ,                    "TBZ",                                       "b0110110bbbbbiiiiiiiiiiiiiittttt")
INST(TBNZ,                   "TBNZ",                                      "b0110111bbbbbiiiiiiiiiiiiiittttt")

// Loads and stores - Advanced SIMD Load/Store multiple structures
INST(STx_mult_1,             "STx (multiple structures)",                 "0Q00110000000000oooozznnnnnttttt")
INST(STx_mult_2,             "STx (multiple structures)",                 "0Q001100100mmmmmoooozznnnnnttttt")
INST(LDx_mult_1,             "LDx (multiple structures)",                 "0Q00110001000000oooozznnnnnttttt")
INST(LDx_mult_2,             "LDx (multiple structures)",                 "0Q001100110mmmmmoooozznnnnnttttt")

// Loads and stores - Advanced SIMD Load/Store single structures
INST(ST1_sngl_1,             "ST1 (single structure)",                    "0Q00110100000000oo0Szznnnnnttttt")
INST(ST1_sngl_2,             "ST1 (single structure)",                    "0Q001101100mmmmmoo0Szznnnnnttttt")
INST(ST3_sngl_1,             "ST3 (single structure)",                    "0Q00110100000000oo1Szznnnnnttttt")
INST(ST3_sngl_2,             "ST3 (single structure)",                    "0Q001101100mmmmmoo1Szznnnnnttttt")
INST(ST2_sngl_1,             "ST2 (single structure)",                    "0Q00110100100000oo0Szznnnnnttttt")
INST(ST2_sngl_2,             "ST2 (single structure)",                    "0Q001101101mmmmmoo0Szznnnnnttttt")
INST(ST4_sngl_1,             "ST4 (single structure)",                    "0Q00110100100000oo1Szznnnnnttttt")
INST(ST4_sngl_2,             "ST4 (single structure)",                    "0Q001101101mmmmmoo1Szznnnnnttttt")
INST(LD1_sngl_1,             "LD1 (single structure)",                    "0Q00110101000000oo0Szznnnnnttttt")
INST(LD1_sngl_2,             "LD1 (single structure)",                    "0Q001101110mmmmmoo0Szznnnnnttttt")
INST(LD3_sngl_1,             "LD3 (single structure)",                    "0Q00110101000000oo1Szznnnnnttttt")
INST(LD3_sngl_2,             "LD3 (single structure)",                    "0Q001101110mmmmmoo1Szznnnnnttttt")
INST(LD1R_1,                 "LD1R",                                      "0Q001101010000001100zznnnnnttttt")
INST(LD1R_2,                 "LD1R",                                      "0Q001101110mmmmm1100zznnnnnttttt")
INST(LD3R_1,                 "LD3R",                                      "0Q001101010000001110zznnnnnttttt")
INST(LD3R_2,                 "LD3R",                                      "0Q001101110mmmmm1110zznnnnnttttt")
INST(LD2_sngl_1,             "LD2 (single structure)",                    "0Q00110101100000oo0Szznnnnnttttt")
INST(LD2_sngl_2,             "LD2 (single structure)",                    "0Q001101111mmmmmoo0Szznnnnnttttt")
INST(LD4_sngl_1,             "LD4 (single structure)",                    "0Q00110101100000oo1Szznnnnnttttt")
INST(LD4_sngl_2,             "LD4 (single structure)",                    "0Q001101111mmmmmoo1Szznnnnnttttt")
INST(LD2R_1,                 "LD2R",                                      "0Q001101011000001100zznnnnnttttt")
INST(LD2R_2,                 "LD2R",                                      "0Q001101111mmmmm1100zznnnnnttttt")
INST(LD4R_1,                 "LD4R",                                      "0Q001101011000001110zznnnnnttttt")
INST(LD4R_2,                 "LD4R",                                      "0Q001101111mmmmm1110zznnnnnttttt")

// Loads and stores - Load/Store Exclusive
INST(STXR,                   "STXRB, STXRH, STXR",                        "zz001000000sssss011111nnnnnttttt")
INST(STLXR,                  "STLXRB, STLXRH, STLXR",                     "zz001000000sssss111111nnnnnttttt")
INST(STXP,                   "STXP",                                      "1z001000001sssss0uuuuunnnnnttttt")
INST(STLXP,                  "STLXP",                                     "1z001000001sssss1uuuuunnnnnttttt")
INST(LDXR,                   "LDXRB, LDXRH, LDXR",                        "zz00100001011111011111nnnnnttttt")
INST(LDAXR,                  "LDAXRB, LDAXRH, LDAXR",                       "zz00100001011111111111nnnnnttttt")
INST(LDXP,                   "LDXP",                                      "1z001000011111110uuuuunnnnnttttt")
INST(LDAXP,                  "LDAXP",                                     "1z001000011111111uuuuunnnnnttttt")
INST(STLLR,                  "STLLRB, STLLRH, STLLR",                     "zz00100010011111011111nnnnnttttt")
INST(STLR,                   "STLRB, STLRH, STLR",                        "zz00100010011111111111nnnnnttttt")
INST(LDLAR,                  "LDLARB, LDLARH, LDLAR",                     "zz00100011011111011111nnnnnttttt")
INST(LDAR,                   "LDARB, LDARH, LDAR",                        "zz00100011011111111111nnnnnttttt")
//INST(CASP,                   "CASP, CASPA, CASPAL, CASPL",                "0z0010000L1sssssp11111nnnnnttttt") // ARMv8.1
//INST(CASB,                   "CASB, CASAB, CASALB, CASLB",                "000010001L1sssssp11111nnnnnttttt") // ARMv8.1
//INST(CASH,                   "CASH, CASAH, CASALH, CASLH",                "010010001L1sssssp11111nnnnnttttt") // ARMv8.1
//INST(CAS,                    "CAS, CASA, CASAL, CASL",                    "1z0010001L1sssssp11111nnnnnttttt") // ARMv8.1

// Loads and stores - Load register (literal)
INST(LDR_lit_gen,            "LDR (literal)",                             "0z011000iiiiiiiiiiiiiiiiiiittttt")
INST(LDRSW_lit,              "LDRSW (literal)",                           "10011000iiiiiiiiiiiiiiiiiiittttt")
INST(PRFM_lit,               "PRFM (literal)",                            "11011000iiiiiiiiiiiiiiiiiiittttt")
INST(LDR_lit_fpsimd,         "LDR (literal, SIMD&FP)",                    "oo011100iiiiiiiiiiiiiiiiiiittttt")

// Loads and stores - Load/Store no-allocate pair
INST(STNP_LDNP_gen,          "STNP/LDNP",                                 "o01010000Liiiiiiiuuuuunnnnnttttt")
INST(STNP_LDNP_fpsimd,       "STNP/LDNP (SIMD&FP)",                       "oo1011000Liiiiiiiuuuuunnnnnttttt")

// Loads and stores - Load/Store register pair
INST(STP_LDP_gen,            "STP/LDP",                                   "oo10100pwLiiiiiiiuuuuunnnnnttttt")
INST(UnallocatedEncoding,    "",                                          "--1010000-----------------------")
INST(STP_LDP_fpsimd,         "STP/LDP (SIMD&FP)",                         "oo10110pwLiiiiiiiuuuuunnnnnttttt")
INST(UnallocatedEncoding,    "",                                          "--1011000-----------------------")

// Loads and stores - Load/Store register (unscaled immediate)
INST(STURx_LDURx,            "STURx/LDURx",                               "zz111000oo0iiiiiiiii00nnnnnttttt")
INST(UnallocatedEncoding,    "",                                          "111110001-0---------00----------")
INST(UnallocatedEncoding,    "",                                          "10111000110---------00----------")
INST(PRFM_imm,               "PRFM (immediate)",                          "1111100110iiiiiiiiiiiinnnnnttttt")
INST(PRFM_unscaled_imm,      "PRFM (unscaled offset)",                    "11111000100iiiiiiiii00nnnnnttttt")
INST(STUR_fpsimd,            "STUR (SIMD&FP)",                            "zz111100o00iiiiiiiii00nnnnnttttt")
INST(LDUR_fpsimd,            "LDUR (SIMD&FP)",                            "zz111100o10iiiiiiiii00nnnnnttttt")

// Loads and stores - Load/Store register (immediate pre/post-indexed)
INST(STRx_LDRx_imm_1,        "STRx/LDRx (immediate)",                     "zz111000oo0iiiiiiiiip1nnnnnttttt")
INST(STRx_LDRx_imm_2,        "STRx/LDRx (immediate)",                     "zz111001ooiiiiiiiiiiiinnnnnttttt")
INST(UnallocatedEncoding,    "",                                          "111110001-0----------1----------")
INST(UnallocatedEncoding,    "",                                          "10111000110----------1----------")
INST(UnallocatedEncoding,    "",                                          "1111100111----------------------")
INST(UnallocatedEncoding,    "",                                          "1011100111----------------------")
INST(STR_imm_fpsimd_1,       "STR (immediate, SIMD&FP)",                  "zz111100o00iiiiiiiiip1nnnnnttttt")
INST(STR_imm_fpsimd_2,       "STR (immediate, SIMD&FP)",                  "zz111101o0iiiiiiiiiiiinnnnnttttt")
INST(LDR_imm_fpsimd_1,       "LDR (immediate, SIMD&FP)",                  "zz111100o10iiiiiiiiip1nnnnnttttt")
INST(LDR_imm_fpsimd_2,       "LDR (immediate, SIMD&FP)",                  "zz111101o1iiiiiiiiiiiinnnnnttttt")
//INST(STGP_1,               "STGP (post-index)",                         "0110100010iiiiiiimmmmmnnnnnttttt") // ARMv8.5
//INST(STGP_2,               "STGP (pre-index)",                          "0110100110iiiiiiimmmmmnnnnnttttt") // ARMv8.5
//INST(STGP_3,               "STGP (signed-offset)",                      "0110100100iiiiiiimmmmmnnnnnttttt") // ARMv8.5

// Loads and stores - Load/Store register (unprivileged)
INST(STTRB,                  "STTRB",                                     "00111000000iiiiiiiii10nnnnnttttt")
INST(LDTRB,                  "LDTRB",                                     "00111000010iiiiiiiii10nnnnnttttt")
INST(LDTRSB,                 "LDTRSB",                                    "00111000oo0iiiiiiiii10nnnnnttttt")
INST(STTRH,                  "STTRH",                                     "01111000000iiiiiiiii10nnnnnttttt")
INST(LDTRH,                  "LDTRH",                                     "01111000010iiiiiiiii10nnnnnttttt")
INST(LDTRSH,                 "LDTRSH",                                    "01111000oo0iiiiiiiii10nnnnnttttt")
INST(STTR,                   "STTR",                                      "zz111000000iiiiiiiii10nnnnnttttt")
INST(LDTR,                   "LDTR",                                      "zz111000010iiiiiiiii10nnnnnttttt")
INST(LDTRSW,                 "LDTRSW",                                    "10111000100iiiiiiiii10nnnnnttttt")

// Loads and stores - Atomic memory options
//INST(LDADDB,                 "LDADDB, LDADDAB, LDADDALB, LDADDLB",        "00111000AR1sssss000000nnnnnttttt")
//INST(LDCLRB,                 "LDCLRB, LDCLRAB, LDCLRALB, LDCLRLB",        "00111000AR1sssss000100nnnnnttttt")
//INST(LDEORB,                 "LDEORB, LDEORAB, LDEORALB, LDEORLB",        "00111000AR1sssss001000nnnnnttttt")
//INST(LDSETB,                 "LDSETB, LDSETAB, LDSETALB, LDSETLB",        "00111000AR1sssss001100nnnnnttttt")
//INST(LDSMAXB,                "LDSMAXB, LDSMAXAB, LDSMAXALB, LDSMAXLB",    "00111000AR1sssss010000nnnnnttttt")
//INST(LDSMINB,                "LDSMINB, LDSMINAB, LDSMINALB, LDSMINLB",    "00111000AR1sssss010100nnnnnttttt")
//INST(LDUMAXB,                "LDUMAXB, LDUMAXAB, LDUMAXALB, LDUMAXLB",    "00111000AR1sssss011000nnnnnttttt")
//INST(LDUMINB,                "LDUMINB, LDUMINAB, LDUMINALB, LDUMINLB",    "00111000AR1sssss011100nnnnnttttt")
//INST(SWPB,                   "SWPB, SWPAB, SWPALB, SWPLB",                "00111000AR1sssss100000nnnnnttttt")
//INST(LDAPRB,                 "LDAPRB",                                    "0011100010111111110000nnnnnttttt")
//INST(LDADDH,                 "LDADDH, LDADDAH, LDADDALH, LDADDLH",        "01111000AR1sssss000000nnnnnttttt")
//INST(LDCLRH,                 "LDCLRH, LDCLRAH, LDCLRALH, LDCLRLH",        "01111000AR1sssss000100nnnnnttttt")
//INST(LDEORH,                 "LDEORH, LDEORAH, LDEORALH, LDEORLH",        "01111000AR1sssss001000nnnnnttttt")
//INST(LDSETH,                 "LDSETH, LDSETAH, LDSETALH, LDSETLH",        "01111000AR1sssss001100nnnnnttttt")
//INST(LDSMAXH,                "LDSMAXH, LDSMAXAH, LDSMAXALH, LDSMAXLH",    "01111000AR1sssss010000nnnnnttttt")
//INST(LDSMINH,                "LDSMINH, LDSMINAH, LDSMINALH, LDSMINLH",    "01111000AR1sssss010100nnnnnttttt")
//INST(LDUMAXH,                "LDUMAXH, LDUMAXAH, LDUMAXALH, LDUMAXLH",    "01111000AR1sssss011000nnnnnttttt")
//INST(LDUMINH,                "LDUMINH, LDUMINAH, LDUMINALH, LDUMINLH",    "01111000AR1sssss011100nnnnnttttt")
//INST(SWPH,                   "SWPH, SWPAH, SWPALH, SWPLH",                "01111000AR1sssss100000nnnnnttttt")
//INST(LDAPRH,                 "LDAPRH",                                    "0111100010111111110000nnnnnttttt")
//INST(LDADD,                  "LDADD, LDADDA, LDADDAL, LDADDL",            "1-111000AR1sssss000000nnnnnttttt")
//INST(LDCLR,                  "LDCLR, LDCLRA, LDCLRAL, LDCLRL",            "1-111000AR1sssss000100nnnnnttttt")
//INST(LDEOR,                  "LDEOR, LDEORA, LDEORAL, LDEORL",            "1-111000AR1sssss001000nnnnnttttt")
//INST(LDSET,                  "LDSET, LDSETA, LDSETAL, LDSETL",            "1-111000AR1sssss001100nnnnnttttt")
//INST(LDSMAX,                 "LDSMAX, LDSMAXA, LDSMAXAL, LDSMAXL",        "1-111000AR1sssss010000nnnnnttttt")
//INST(LDSMIN,                 "LDSMIN, LDSMINA, LDSMINAL, LDSMINL",        "1-111000AR1sssss010100nnnnnttttt")
//INST(LDUMAX,                 "LDUMAX, LDUMAXA, LDUMAXAL, LDUMAXL",        "1-111000AR1sssss011000nnnnnttttt")
//INST(LDUMIN,                 "LDUMIN, LDUMINA, LDUMINAL, LDUMINL",        "1-111000AR1sssss011100nnnnnttttt")
//INST(SWP,                    "SWP, SWPA, SWPAL, SWPL",                    "1-111000AR1sssss100000nnnnnttttt")
//INST(LDAPR,                  "LDAPR",                                     "1-11100010111111110000nnnnnttttt")
//INST(LD64B,                  "LD64B",                                     "1111100000111111110100nnnnnttttt") // v8.7
//INST(ST64B,                  "ST64B",                                     "1111100000111111100100nnnnnttttt") // v8.7
//INST(ST64BV,                 "ST64BV",                                    "11111000001sssss101100nnnnnttttt") // v8.7
//INST(ST64BV0,                "ST64BV0",                                   "11111000001sssss101000nnnnnttttt") // v8.7

// Loads and stores - Load/Store register (register offset)
INST(STRx_reg,               "STRx (register)",                           "zz111000o01mmmmmxxxS10nnnnnttttt")
INST(LDRx_reg,               "LDRx (register)",                           "zz111000o11mmmmmxxxS10nnnnnttttt")
INST(STR_reg_fpsimd,         "STR (register, SIMD&FP)",                   "zz111100o01mmmmmxxxS10nnnnnttttt")
INST(LDR_reg_fpsimd,         "LDR (register, SIMD&FP)",                   "zz111100o11mmmmmxxxS10nnnnnttttt")

// Loads and stores - Load/Store memory tags
//INST(STG_1,                  "STG (post-index)",                          "11011001001iiiiiiiii01nnnnn11111") // ARMv8.5
//INST(STG_2,                  "STG (pre-index)",                           "11011001001iiiiiiiii11nnnnn11111") // ARMv8.5
//INST(STG_3,                  "STG (signed-offset)",                       "11011001001iiiiiiiii10nnnnn11111") // ARMv8.5
//INST(LDG,                    "LDG",                                       "11011001011iiiiiiiii00nnnnnttttt") // ARMv8.5
//INST(STZG_1,                 "STZG (post-index)",                         "11011001011iiiiiiiii01nnnnn11111") // ARMv8.5
//INST(STZG_2,                 "STZG (pre-index)",                          "11011001011iiiiiiiii11nnnnn11111") // ARMv8.5
//INST(STZG_3,                 "STZG (signed-offset)",                      "11011001011iiiiiiiii10nnnnn11111") // ARMv8.5
//INST(ST2G_1,                 "ST2G (post-index)",                         "11011001101iiiiiiiii01nnnnn11111") // ARMv8.5
//INST(ST2G_2,                 "ST2G (pre-index)",                          "11011001101iiiiiiiii11nnnnn11111") // ARMv8.5
//INST(ST2G_3,                 "ST2G (signed-offset)",                      "11011001101iiiiiiiii10nnnnn11111") // ARMv8.5
//INST(STGV,                   "STGV",                                      "1101100110100000000000nnnnnttttt") // ARMv8.5
//INST(STZ2G_1,                "STZ2G (post-index)",                        "11011001111iiiiiiiii01nnnnn11111") // ARMv8.5
//INST(STZ2G_2,                "STZ2G (pre-index)",                         "11011001111iiiiiiiii11nnnnn11111") // ARMv8.5
//INST(STZ2G_3,                "STZ2G (signed-offset)",                     "11011001111iiiiiiiii10nnnnn11111") // ARMv8.5
//INST(LDGV,                   "LDGV",                                      "1101100111100000000000nnnnnttttt") // ARMv8.5

// Loads and stores - Load/Store register (pointer authentication)
//INST(LDRA,                   "LDRAA, LDRAB",                              "11111000MS1iiiiiiiiiW1nnnnnttttt")

// Data Processing - Register - 2 source
INST(UDIV,                   "UDIV",                                      "z0011010110mmmmm000010nnnnnddddd")
INST(SDIV,                   "SDIV",                                      "z0011010110mmmmm000011nnnnnddddd")
INST(LSLV,                   "LSLV",                                      "z0011010110mmmmm001000nnnnnddddd")
INST(LSRV,                   "LSRV",                                      "z0011010110mmmmm001001nnnnnddddd")
INST(ASRV,                   "ASRV",                                      "z0011010110mmmmm001010nnnnnddddd")
INST(RORV,                   "RORV",                                      "z0011010110mmmmm001011nnnnnddddd")
INST(CRC32,                  "CRC32B, CRC32H, CRC32W, CRC32X",            "z0011010110mmmmm0100zznnnnnddddd")
INST(CRC32C,                 "CRC32CB, CRC32CH, CRC32CW, CRC32CX",        "z0011010110mmmmm0101zznnnnnddddd")
//INST(PACGA,                  "PACGA",                                     "10011010110mmmmm001100nnnnnddddd")
//INST(SUBP,                   "SUBP",                                      "10011010110mmmmm000000nnnnnddddd") // ARMv8.5
//INST(IRG,                    "IRG",                                       "10011010110mmmmm000100nnnnnddddd") // ARMv8.5
//INST(GMI,                    "GMI",                                       "10011010110mmmmm000101nnnnnddddd") // ARMv8.5
//INST(SUBPS,                  "SUBPS",                                     "10111010110mmmmm000000nnnnnddddd") // ARMv8.5

// Data Processing - Register - 1 source
INST(RBIT_int,               "RBIT",                                      "z101101011000000000000nnnnnddddd")
INST(REV16_int,              "REV16",                                     "z101101011000000000001nnnnnddddd")
INST(REV,                    "REV",                                       "z10110101100000000001onnnnnddddd")
INST(CLZ_int,                "CLZ",                                       "z101101011000000000100nnnnnddddd")
INST(CLS_int,                "CLS",                                       "z101101011000000000101nnnnnddddd")
INST(REV32_int,              "REV32",                                     "1101101011000000000010nnnnnddddd")
//INST(PACDA,                  "PACDA, PACDZA",                             "110110101100000100Z010nnnnnddddd")
//INST(PACDB,                  "PACDB, PACDZB",                             "110110101100000100Z011nnnnnddddd")
//INST(AUTDA,                  "AUTDA, AUTDZA",                             "110110101100000100Z110nnnnnddddd")
//INST(AUTDB,                  "AUTDB, AUTDZB",                             "110110101100000100Z111nnnnnddddd")

// Data Processing - Register - Logical (shifted register)
INST(AND_shift,              "AND (shifted register)",                    "z0001010ss0mmmmmiiiiiinnnnnddddd")
INST(BIC_shift,              "BIC (shifted register)",                    "z0001010ss1mmmmmiiiiiinnnnnddddd")
INST(ORR_shift,              "ORR (shifted register)",                    "z0101010ss0mmmmmiiiiiinnnnnddddd")
INST(ORN_shift,              "ORN (shifted register)",                    "z0101010ss1mmmmmiiiiiinnnnnddddd")
INST(EOR_shift,              "EOR (shifted register)",                    "z1001010ss0mmmmmiiiiiinnnnnddddd")
INST(EON,                    "EON (shifted register)",                    "z1001010ss1mmmmmiiiiiinnnnnddddd")
INST(ANDS_shift,             "ANDS (shifted register)",                   "z1101010ss0mmmmmiiiiiinnnnnddddd")
INST(BICS,                   "BICS (shifted register)",                   "z1101010ss1mmmmmiiiiiinnnnnddddd")

// Data Processing - Register - Add/Sub (shifted register)
INST(ADD_shift,              "ADD (shifted register)",                    "z0001011ss0mmmmmiiiiiinnnnnddddd")
INST(ADDS_shift,             "ADDS (shifted register)",                   "z0101011ss0mmmmmiiiiiinnnnnddddd")
INST(SUB_shift,              "SUB (shifted register)",                    "z1001011ss0mmmmmiiiiiinnnnnddddd")
INST(SUBS_shift,             "SUBS (shifted register)",                   "z1101011ss0mmmmmiiiiiinnnnnddddd")

// Data Processing - Register - Add/Sub (shifted register)
INST(ADD_ext,                "ADD (extended register)",                   "z0001011001mmmmmxxxiiinnnnnddddd")
INST(ADDS_ext,               "ADDS (extended register)",                  "z0101011001mmmmmxxxiiinnnnnddddd")
INST(SUB_ext,                "SUB (extended register)",                   "z1001011001mmmmmxxxiiinnnnnddddd")
INST(SUBS_ext,               "SUBS (extended register)",                  "z1101011001mmmmmxxxiiinnnnnddddd")

// Data Processing - Register - Add/Sub (with carry)
INST(ADC,                    "ADC",                                       "z0011010000mmmmm000000nnnnnddddd")
INST(ADCS,                   "ADCS",                                      "z0111010000mmmmm000000nnnnnddddd")
INST(SBC,                    "SBC",                                       "z1011010000mmmmm000000nnnnnddddd")
INST(SBCS,                   "SBCS",                                      "z1111010000mmmmm000000nnnnnddddd")

// Data Processing - Register - Conditional compare
INST(CCMN_reg,               "CCMN (register)",                           "z0111010010mmmmmcccc00nnnnn0ffff")
INST(CCMP_reg,               "CCMP (register)",                           "z1111010010mmmmmcccc00nnnnn0ffff")
INST(CCMN_imm,               "CCMN (immediate)",                          "z0111010010iiiiicccc10nnnnn0ffff")
INST(CCMP_imm,               "CCMP (immediate)",                          "z1111010010iiiiicccc10nnnnn0ffff")

// Data Processing - Register - Conditional select
INST(CSEL,                   "CSEL",                                      "z0011010100mmmmmcccc00nnnnnddddd")
INST(CSINC,                  "CSINC",                                     "z0011010100mmmmmcccc01nnnnnddddd")
INST(CSINV,                  "CSINV",                                     "z1011010100mmmmmcccc00nnnnnddddd")
INST(CSNEG,                  "CSNEG",                                     "z1011010100mmmmmcccc01nnnnnddddd")

// Data Processing - Register - 3 source
INST(MADD,                   "MADD",                                      "z0011011000mmmmm0aaaaannnnnddddd")
INST(MSUB,                   "MSUB",                                      "z0011011000mmmmm1aaaaannnnnddddd")
INST(SMADDL,                 "SMADDL",                                    "10011011001mmmmm0aaaaannnnnddddd")
INST(SMSUBL,                 "SMSUBL",                                    "10011011001mmmmm1aaaaannnnnddddd")
INST(SMULH,                  "SMULH",                                     "10011011010mmmmm011111nnnnnddddd")
INST(UMADDL,                 "UMADDL",                                    "10011011101mmmmm0aaaaannnnnddddd")
INST(UMSUBL,                 "UMSUBL",                                    "10011011101mmmmm1aaaaannnnnddddd")
INST(UMULH,                  "UMULH",                                     "10011011110mmmmm011111nnnnnddddd")

// Data Processing - FP and SIMD - AES
INST(AESE,                   "AESE",                                      "0100111000101000010010nnnnnddddd")
INST(AESD,                   "AESD",                                      "0100111000101000010110nnnnnddddd")
INST(AESMC,                  "AESMC",                                     "0100111000101000011010nnnnnddddd")
INST(AESIMC,                 "AESIMC",                                    "0100111000101000011110nnnnnddddd")

// Data Processing - FP and SIMD - SHA
INST(SHA1C,                  "SHA1C",                                     "01011110000mmmmm000000nnnnnddddd")
INST(SHA1P,                  "SHA1P",                                     "01011110000mmmmm000100nnnnnddddd")
INST(SHA1M,                  "SHA1M",                                     "01011110000mmmmm001000nnnnnddddd")
INST(SHA1SU0,                "SHA1SU0",                                   "01011110000mmmmm001100nnnnnddddd")
INST(SHA256H,                "SHA256H",                                   "01011110000mmmmm010000nnnnnddddd")
INST(SHA256H2,               "SHA256H2",                                  "01011110000mmmmm010100nnnnnddddd")
INST(SHA256SU1,              "SHA256SU1",                                 "01011110000mmmmm011000nnnnnddddd")
INST(SHA1H,                  "SHA1H",                                     "0101111000101000000010nnnnnddddd")
INST(SHA1SU1,                "SHA1SU1",                                   "0101111000101000000110nnnnnddddd")
INST(SHA256SU0,              "SHA256SU0",                                 "0101111000101000001010nnnnnddddd")

// Data Processing - FP and SIMD - Scalar copy
INST(DUP_elt_1,              "DUP (element)",                             "01011110000iiiii000001nnnnnddddd")

// Data Processing - FP and SIMD - Scalar three
//INST(FMULX_vec_1,            "FMULX",                                     "01011110010mmmmm000111nnnnnddddd")
INST(FMULX_vec_2,            "FMULX",                                     "010111100z1mmmmm110111nnnnnddddd")
INST(FCMEQ_reg_1,            "FCMEQ (register)",                          "01011110010mmmmm001001nnnnnddddd")
INST(FCMEQ_reg_2,            "FCMEQ (register)",                          "010111100z1mmmmm111001nnnnnddddd")
INST(FRECPS_1,               "FRECPS",                                    "01011110010mmmmm001111nnnnnddddd")
INST(FRECPS_2,               "FRECPS",                                    "010111100z1mmmmm111111nnnnnddddd")
INST(FRSQRTS_1,              "FRSQRTS",                                   "01011110110mmmmm001111nnnnnddddd")
INST(FRSQRTS_2,              "FRSQRTS",                                   "010111101z1mmmmm111111nnnnnddddd")
//INST(FCMGE_reg_1,            "FCMGE (register)",                          "01111110010mmmmm001001nnnnnddddd")
INST(FCMGE_reg_2,            "FCMGE (register)",                          "011111100z1mmmmm111001nnnnnddddd")
//INST(FACGE_1,                "FACGE",                                     "01111110010mmmmm001011nnnnnddddd")
INST(FACGE_2,                "FACGE",                                     "011111100z1mmmmm111011nnnnnddddd")
//INST(FABD_1,                 "FABD",                                      "01111110110mmmmm000101nnnnnddddd")
INST(FABD_2,                 "FABD",                                      "011111101z1mmmmm110101nnnnnddddd")
//INST(FCMGT_reg_1,            "FCMGT (register)",                          "01111110110mmmmm001001nnnnnddddd")
INST(FCMGT_reg_2,            "FCMGT (register)",                          "011111101z1mmmmm111001nnnnnddddd")
//INST(FACGT_1,                "FACGT",                                     "01111110110mmmmm001011nnnnnddddd")
INST(FACGT_2,                "FACGT",                                     "011111101z1mmmmm111011nnnnnddddd")

// Data Processing - FP and SIMD - Scalar two register misc
//INST(FCVTNS_1,               "FCVTNS (vector)",                           "0101111001111001101010nnnnnddddd")
INST(FCVTNS_2,               "FCVTNS (vector)",                           "010111100z100001101010nnnnnddddd")
//INST(FCVTMS_1,               "FCVTMS (vector)",                           "0101111001111001101110nnnnnddddd")
INST(FCVTMS_2,               "FCVTMS (vector)",                           "010111100z100001101110nnnnnddddd")
//INST(FCVTAS_1,               "FCVTAS (vector)",                           "0101111001111001110010nnnnnddddd")
INST(FCVTAS_2,               "FCVTAS (vector)",                           "010111100z100001110010nnnnnddddd")
//INST(SCVTF_int_1,            "SCVTF (vector, integer)",                   "0101111001111001110110nnnnnddddd")
INST(SCVTF_int_2,            "SCVTF (vector, integer)",                   "010111100z100001110110nnnnnddddd")
//INST(FCMGT_zero_1,           "FCMGT (zero)",                              "0101111011111000110010nnnnnddddd")
INST(FCMGT_zero_2,           "FCMGT (zero)",                              "010111101z100000110010nnnnnddddd")
INST(FCMEQ_zero_1,           "FCMEQ (zero)",                              "0101111011111000110110nnnnnddddd")
INST(FCMEQ_zero_2,           "FCMEQ (zero)",                              "010111101z100000110110nnnnnddddd")
//INST(FCMLT_1,                "FCMLT (zero)",                              "0101111011111000111010nnnnnddddd")
INST(FCMLT_2,                "FCMLT (zero)",                              "010111101z100000111010nnnnnddddd")
//INST(FCVTPS_1,               "FCVTPS (vector)",                           "0101111011111001101010nnnnnddddd")
INST(FCVTPS_2,               "FCVTPS (vector)",                           "010111101z100001101010nnnnnddddd")
//INST(FCVTZS_int_1,           "FCVTZS (vector, integer)",                  "0101111011111001101110nnnnnddddd")
INST(FCVTZS_int_2,           "FCVTZS (vector, integer)",                  "010111101z100001101110nnnnnddddd")
INST(FRECPE_1,               "FRECPE",                                    "0101111011111001110110nnnnnddddd")
INST(FRECPE_2,               "FRECPE",                                    "010111101z100001110110nnnnnddddd")
INST(FRECPX_1,               "FRECPX",                                    "0101111011111001111110nnnnnddddd")
INST(FRECPX_2,               "FRECPX",                                    "010111101z100001111110nnnnnddddd")
//INST(FCVTNU_1,               "FCVTNU (vector)",                           "0111111001111001101010nnnnnddddd")
INST(FCVTNU_2,               "FCVTNU (vector)",                           "011111100z100001101010nnnnnddddd")
//INST(FCVTMU_1,               "FCVTMU (vector)",                           "0111111001111001101110nnnnnddddd")
INST(FCVTMU_2,               "FCVTMU (vector)",                           "011111100z100001101110nnnnnddddd")
//INST(FCVTAU_1,               "FCVTAU (vector)",                           "0111111001111001110010nnnnnddddd")
INST(FCVTAU_2,               "FCVTAU (vector)",                           "011111100z100001110010nnnnnddddd")
//INST(UCVTF_int_1,            "UCVTF (vector, integer)",                   "0111111001111001110110nnnnnddddd")
INST(UCVTF_int_2,            "UCVTF (vector, integer)",                   "011111100z100001110110nnnnnddddd")
//INST(FCMGE_zero_1,           "FCMGE (zero)",                              "0111111011111000110010nnnnnddddd")
INST(FCMGE_zero_2,           "FCMGE (zero)",                              "011111101z100000110010nnnnnddddd")
//INST(FCMLE_1,                "FCMLE (zero)",                              "0111111011111000110110nnnnnddddd")
INST(FCMLE_2,                "FCMLE (zero)",                              "011111101z100000110110nnnnnddddd")
//INST(FCVTPU_1,               "FCVTPU (vector)",                           "0111111011111001101010nnnnnddddd")
INST(FCVTPU_2,               "FCVTPU (vector)",                           "011111101z100001101010nnnnnddddd")
//INST(FCVTZU_int_1,           "FCVTZU (vector, integer)",                  "0111111011111001101110nnnnnddddd")
INST(FCVTZU_int_2,           "FCVTZU (vector, integer)",                  "011111101z100001101110nnnnnddddd")
INST(FRSQRTE_1,              "FRSQRTE",                                   "0111111011111001110110nnnnnddddd")
INST(FRSQRTE_2,              "FRSQRTE",                                   "011111101z100001110110nnnnnddddd")

// Data Processing - FP and SIMD - Scalar three same extra
//INST(SQRDMLAH_vec_1,         "SQRDMLAH (vector)",                         "01111110zz0mmmmm100001nnnnnddddd")
//INST(SQRDMLAH_vec_2,         "SQRDMLAH (vector)",                         "0Q101110zz0mmmmm100001nnnnnddddd")
//INST(SQRDMLSH_vec_1,         "SQRDMLSH (vector)",                         "01111110zz0mmmmm100011nnnnnddddd")
//INST(SQRDMLSH_vec_2,         "SQRDMLSH (vector)",                         "0Q101110zz0mmmmm100011nnnnnddddd")

// Data Processing - FP and SIMD - Scalar two-register misc
INST(SUQADD_1,               "SUQADD",                                    "01011110zz100000001110nnnnnddddd")
INST(SQABS_1,                "SQABS",                                     "01011110zz100000011110nnnnnddddd")
INST(CMGT_zero_1,            "CMGT (zero)",                               "01011110zz100000100010nnnnnddddd")
INST(CMEQ_zero_1,            "CMEQ (zero)",                               "01011110zz100000100110nnnnnddddd")
INST(CMLT_1,                 "CMLT (zero)",                               "01011110zz100000101010nnnnnddddd")
INST(ABS_1,                  "ABS",                                       "01011110zz100000101110nnnnnddddd")
INST(SQXTN_1,                "SQXTN, SQXTN2",                             "01011110zz100001010010nnnnnddddd")
INST(USQADD_1,               "USQADD",                                    "01111110zz100000001110nnnnnddddd")
INST(SQNEG_1,                "SQNEG",                                     "01111110zz100000011110nnnnnddddd")
INST(CMGE_zero_1,            "CMGE (zero)",                               "01111110zz100000100010nnnnnddddd")
INST(CMLE_1,                 "CMLE (zero)",                               "01111110zz100000100110nnnnnddddd")
INST(NEG_1,                  "NEG (vector)",                              "01111110zz100000101110nnnnnddddd")
INST(SQXTUN_1,               "SQXTUN, SQXTUN2",                           "01111110zz100001001010nnnnnddddd")
INST(UQXTN_1,                "UQXTN, UQXTN2",                             "01111110zz100001010010nnnnnddddd")
INST(FCVTXN_1,               "FCVTXN, FCVTXN2",                           "011111100z100001011010nnnnnddddd")

// Data Processing - FP and SIMD - SIMD Scalar pairwise
INST(ADDP_pair,              "ADDP (scalar)",                             "01011110zz110001101110nnnnnddddd")
//INST(FMAXNMP_pair_1,         "FMAXNMP (scalar)",                          "0101111000110000110010nnnnnddddd")
INST(FMAXNMP_pair_2,         "FMAXNMP (scalar)",                          "011111100z110000110010nnnnnddddd")
//INST(FADDP_pair_1,           "FADDP (scalar)",                            "0101111000110000110110nnnnnddddd")
INST(FADDP_pair_2,           "FADDP (scalar)",                            "011111100z110000110110nnnnnddddd")
//INST(FMAXP_pair_1,           "FMAXP (scalar)",                            "0101111000110000111110nnnnnddddd")
INST(FMAXP_pair_2,           "FMAXP (scalar)",                            "011111100z110000111110nnnnnddddd")
//INST(FMINNMP_pair_1,         "FMINNMP (scalar)",                          "0101111010110000110010nnnnnddddd")
INST(FMINNMP_pair_2,         "FMINNMP (scalar)",                          "011111101z110000110010nnnnnddddd")
//INST(FMINP_pair_1,           "FMINP (scalar)",                            "0101111010110000111110nnnnnddddd")
INST(FMINP_pair_2,           "FMINP (scalar)",                            "011111101z110000111110nnnnnddddd")

// Data Processing - FP and SIMD - SIMD Scalar three different
//INST(SQDMLAL_vec_1,          "SQDMLAL, SQDMLAL2 (vector)",                "01011110zz1mmmmm100100nnnnnddddd")
//INST(SQDMLSL_vec_1,          "SQDMLSL, SQDMLSL2 (vector)",                "01011110zz1mmmmm101100nnnnnddddd")
//INST(SQDMULL_vec_1,          "SQDMULL, SQDMULL2 (vector)",                "01011110zz1mmmmm110100nnnnnddddd")

// Data Processing - FP and SIMD - SIMD Scalar three same
INST(SQADD_1,                "SQADD",                                     "01011110zz1mmmmm000011nnnnnddddd")
INST(SQSUB_1,                "SQSUB",                                     "01011110zz1mmmmm001011nnnnnddddd")
INST(CMGT_reg_1,             "CMGT (register)",                           "01011110zz1mmmmm001101nnnnnddddd")
INST(CMGE_reg_1,             "CMGE (register)",                           "01011110zz1mmmmm001111nnnnnddddd")
INST(SSHL_1,                 "SSHL",                                      "01011110zz1mmmmm010001nnnnnddddd")
INST(SQSHL_reg_1,            "SQSHL (register)",                          "01011110zz1mmmmm010011nnnnnddddd")
INST(SRSHL_1,                "SRSHL",                                     "01011110zz1mmmmm010101nnnnnddddd")
//INST(SQRSHL_1,               "SQRSHL",                                    "01011110zz1mmmmm010111nnnnnddddd")
INST(ADD_1,                  "ADD (vector)",                              "01011110zz1mmmmm100001nnnnnddddd")
INST(CMTST_1,                "CMTST",                                     "01011110zz1mmmmm100011nnnnnddddd")
INST(SQDMULH_vec_1,          "SQDMULH (vector)",                          "01011110zz1mmmmm101101nnnnnddddd")
INST(UQADD_1,                "UQADD",                                     "01111110zz1mmmmm000011nnnnnddddd")
INST(UQSUB_1,                "UQSUB",                                     "01111110zz1mmmmm001011nnnnnddddd")
INST(CMHI_1,                 "CMHI (register)",                           "01111110zz1mmmmm001101nnnnnddddd")
INST(CMHS_1,                 "CMHS (register)",                           "01111110zz1mmmmm001111nnnnnddddd")
INST(USHL_1,                 "USHL",                                      "01111110zz1mmmmm010001nnnnnddddd")
INST(UQSHL_reg_1,            "UQSHL (register)",                          "01111110zz1mmmmm010011nnnnnddddd")
INST(URSHL_1,                "URSHL",                                     "01111110zz1mmmmm010101nnnnnddddd")
//INST(UQRSHL_1,               "UQRSHL",                                    "01111110zz1mmmmm010111nnnnnddddd")
INST(SUB_1,                  "SUB (vector)",                              "01111110zz1mmmmm100001nnnnnddddd")
INST(CMEQ_reg_1,             "CMEQ (register)",                           "01111110zz1mmmmm100011nnnnnddddd")
INST(SQRDMULH_vec_1,         "SQRDMULH (vector)",                         "01111110zz1mmmmm101101nnnnnddddd")

// Data Processing - FP and SIMD - SIMD Scalar shift by immediate
INST(SSHR_1,                 "SSHR",                                      "010111110IIIIiii000001nnnnnddddd")
INST(SSRA_1,                 "SSRA",                                      "010111110IIIIiii000101nnnnnddddd")
INST(SRSHR_1,                "SRSHR",                                     "010111110IIIIiii001001nnnnnddddd")
INST(SRSRA_1,                "SRSRA",                                     "010111110IIIIiii001101nnnnnddddd")
INST(SHL_1,                  "SHL",                                       "010111110IIIIiii010101nnnnnddddd")
INST(SQSHL_imm_1,            "SQSHL (immediate)",                         "010111110IIIIiii011101nnnnnddddd")
INST(SQSHRN_1,               "SQSHRN, SQSHRN2",                           "010111110IIIIiii100101nnnnnddddd")
//INST(SQRSHRN_1,              "SQRSHRN, SQRSHRN2",                         "010111110IIIIiii100111nnnnnddddd")
INST(SCVTF_fix_1,            "SCVTF (vector, fixed-point)",               "010111110IIIIiii111001nnnnnddddd")
INST(FCVTZS_fix_1,           "FCVTZS (vector, fixed-point)",              "010111110IIIIiii111111nnnnnddddd")
INST(USHR_1,                 "USHR",                                      "011111110IIIIiii000001nnnnnddddd")
INST(USRA_1,                 "USRA",                                      "011111110IIIIiii000101nnnnnddddd")
INST(URSHR_1,                "URSHR",                                     "011111110IIIIiii001001nnnnnddddd")
INST(URSRA_1,                "URSRA",                                     "011111110IIIIiii001101nnnnnddddd")
INST(SRI_1,                  "SRI",                                       "011111110IIIIiii010001nnnnnddddd")
INST(SLI_1,                  "SLI",                                       "011111110IIIIiii010101nnnnnddddd")
INST(SQSHLU_1,               "SQSHLU",                                    "011111110IIIIiii011001nnnnnddddd")
INST(UQSHL_imm_1,            "UQSHL (immediate)",                         "011111110IIIIiii011101nnnnnddddd")
INST(SQSHRUN_1,              "SQSHRUN, SQSHRUN2",                         "011111110IIIIiii100001nnnnnddddd")
//INST(SQRSHRUN_1,             "SQRSHRUN, SQRSHRUN2",                       "011111110IIIIiii100011nnnnnddddd")
INST(UQSHRN_1,               "UQSHRN, UQSHRN2",                           "011111110IIIIiii100101nnnnnddddd")
//INST(UQRSHRN_1,              "UQRSHRN, UQRSHRN2",                         "011111110IIIIiii100111nnnnnddddd")
INST(UCVTF_fix_1,            "UCVTF (vector, fixed-point)",               "011111110IIIIiii111001nnnnnddddd")
INST(FCVTZU_fix_1,           "FCVTZU (vector, fixed-point)",              "011111110IIIIiii111111nnnnnddddd")

// Data Processing - FP and SIMD - SIMD Scalar x indexed element
//INST(SQDMLAL_elt_1,          "SQDMLAL, SQDMLAL2 (by element)",            "01011111zzLMmmmm0011H0nnnnnddddd")
//INST(SQDMLSL_elt_1,          "SQDMLSL, SQDMLSL2 (by element)",            "01011111zzLMmmmm0111H0nnnnnddddd")
INST(SQDMULL_elt_1,          "SQDMULL, SQDMULL2 (by element)",            "01011111zzLMmmmm1011H0nnnnnddddd")
INST(SQDMULH_elt_1,          "SQDMULH (by element)",                      "01011111zzLMmmmm1100H0nnnnnddddd")
INST(SQRDMULH_elt_1,         "SQRDMULH (by element)",                     "01011111zzLMmmmm1101H0nnnnnddddd")
INST(FMLA_elt_1,             "FMLA (by element)",                         "0101111100LMmmmm0001H0nnnnnddddd")
INST(FMLA_elt_2,             "FMLA (by element)",                         "010111111zLMmmmm0001H0nnnnnddddd")
INST(FMLS_elt_1,             "FMLS (by element)",                         "0101111100LMmmmm0101H0nnnnnddddd")
INST(FMLS_elt_2,             "FMLS (by element)",                         "010111111zLMmmmm0101H0nnnnnddddd")
//INST(FMUL_elt_1,             "FMUL (by element)",                         "0101111100LMmmmm1001H0nnnnnddddd")
INST(FMUL_elt_2,             "FMUL (by element)",                         "010111111zLMmmmm1001H0nnnnnddddd")
//INST(SQRDMLAH_elt_1,         "SQRDMLAH (by element)",                     "01111111zzLMmmmm1101H0nnnnnddddd")
//INST(SQRDMLSH_elt_1,         "SQRDMLSH (by element)",                     "01111111zzLMmmmm1111H0nnnnnddddd")
//INST(FMULX_elt_1,            "FMULX (by element)",                        "0111111100LMmmmm1001H0nnnnnddddd")
INST(FMULX_elt_2,            "FMULX (by element)",                        "011111111zLMmmmm1001H0nnnnnddddd")

// Data Processing - FP and SIMD - SIMD Table Lookup
INST(TBL,                    "TBL",                                       "0Q001110000mmmmm0LL000nnnnnddddd")
INST(TBX,                    "TBX",                                       "0Q001110000mmmmm0LL100nnnnnddddd")

// Data Processing - FP and SIMD - SIMD Permute
INST(UZP1,                   "UZP1",                                      "0Q001110zz0mmmmm000110nnnnnddddd")
INST(TRN1,                   "TRN1",                                      "0Q001110zz0mmmmm001010nnnnnddddd")
INST(ZIP1,                   "ZIP1",                                      "0Q001110zz0mmmmm001110nnnnnddddd")
INST(UZP2,                   "UZP2",                                      "0Q001110zz0mmmmm010110nnnnnddddd")
INST(TRN2,                   "TRN2",                                      "0Q001110zz0mmmmm011010nnnnnddddd")
INST(ZIP2,                   "ZIP2",                                      "0Q001110zz0mmmmm011110nnnnnddddd")

// Data Processing - FP and SIMD - SIMD Extract
INST(EXT,                    "EXT",                                       "0Q101110000mmmmm0iiii0nnnnnddddd")

// Data Processing - FP and SIMD - SIMD Copy
INST(DUP_elt_2,              "DUP (element)",                             "0Q001110000iiiii000001nnnnnddddd")
INST(DUP_gen,                "DUP (general)",                             "0Q001110000iiiii000011nnnnnddddd")
INST(SMOV,                   "SMOV",                                      "0Q001110000iiiii001011nnnnnddddd")
INST(UMOV,                   "UMOV",                                      "0Q001110000iiiii001111nnnnnddddd")
INST(INS_gen,                "INS (general)",                             "01001110000iiiii000111nnnnnddddd")
INST(INS_elt,                "INS (element)",                             "01101110000iiiii0iiii1nnnnnddddd")

// Data Processing - FP and SIMD - SIMD Three same
//INST(FMULX_vec_3,            "FMULX",                                     "0Q001110010mmmmm000111nnnnnddddd")
INST(FCMEQ_reg_3,            "FCMEQ (register)",                          "0Q001110010mmmmm001001nnnnnddddd")
INST(FRECPS_3,               "FRECPS",                                    "0Q001110010mmmmm001111nnnnnddddd")
INST(FRSQRTS_3,              "FRSQRTS",                                   "0Q001110110mmmmm001111nnnnnddddd")
//INST(FCMGE_reg_3,            "FCMGE (register)",                          "0Q101110010mmmmm001001nnnnnddddd")
//INST(FACGE_3,                "FACGE",                                     "0Q101110010mmmmm001011nnnnnddddd")
//INST(FABD_3,                 "FABD",                                      "0Q101110110mmmmm000101nnnnnddddd")
//INST(FCMGT_reg_3,            "FCMGT (register)",                          "0Q101110110mmmmm001001nnnnnddddd")
//INST(FACGT_3,                "FACGT",                                     "0Q101110110mmmmm001011nnnnnddddd")
//INST(FMAXNM_1,               "FMAXNM (vector)",                           "0Q001110010mmmmm000001nnnnnddddd")
INST(FMLA_vec_1,             "FMLA (vector)",                             "0Q001110010mmmmm000011nnnnnddddd")
//INST(FADD_1,                 "FADD (vector)",                             "0Q001110010mmmmm000101nnnnnddddd")
//INST(FMAX_1,                 "FMAX (vector)",                             "0Q001110010mmmmm001101nnnnnddddd")
//INST(FMINNM_1,               "FMINNM (vector)",                           "0Q001110110mmmmm000001nnnnnddddd")
INST(FMLS_vec_1,             "FMLS (vector)",                             "0Q001110110mmmmm000011nnnnnddddd")
//INST(FSUB_1,                 "FSUB (vector)",                             "0Q001110110mmmmm000101nnnnnddddd")
//INST(FMIN_1,                 "FMIN (vector)",                             "0Q001110110mmmmm001101nnnnnddddd")
//INST(FMAXNMP_vec_1,          "FMAXNMP (vector)",                          "0Q101110010mmmmm000001nnnnnddddd")
//INST(FADDP_vec_1,            "FADDP (vector)",                            "0Q101110010mmmmm000101nnnnnddddd")
//INST(FMUL_vec_1,             "FMUL (vector)",                             "0Q101110010mmmmm000111nnnnnddddd")
//INST(FMAXP_vec_1,            "FMAXP (vector)",                            "0Q101110010mmmmm001101nnnnnddddd")
//INST(FDIV_1,                 "FDIV (vector)",                             "0Q101110010mmmmm001111nnnnnddddd")
//INST(FMINNMP_vec_1,          "FMINNMP (vector)",                          "0Q101110110mmmmm000001nnnnnddddd")
//INST(FMINP_vec_1,            "FMINP (vector)",                            "0Q101110110mmmmm001101nnnnnddddd")

// Data Processing - FP and SIMD - SIMD Three same extra
//INST(SMMLA_vec,              "SMMLA",                                     "01001110100mmmmm101001nnnnnddddd") // v8.6
//INST(UMMLA_vec,              "UMMLA",                                     "01101110100mmmmm101001nnnnnddddd") // v8.6
//INST(USMMLA_vec,             "USMMLA",                                    "01001110100mmmmm101011nnnnnddddd") // v8.6
//INST(SUDOT_element,          "SUDOT (by element)",                        "0Q00111100LMmmmm1111H0nnnnnddddd") // v8.6
//INST(USDOT_element,          "USDOT (by_element)",                        "0Q00111110LMmmmm1111H0nnnnnddddd") // v8.6
//INST(USDOT_vec,              "USDOT (vector)",                            "0Q001110100mmmmm100111nnnnnddddd") // v8.6
INST(SDOT_vec,               "SDOT (vector)",                             "0Q001110zz0mmmmm100101nnnnnddddd")
INST(UDOT_vec,               "UDOT (vector)",                             "0Q101110zz0mmmmm100101nnnnnddddd")
INST(FCMLA_vec,              "FCMLA",                                     "0Q101110zz0mmmmm110rr1nnnnnddddd")
INST(FCADD_vec,              "FCADD",                                     "0Q101110zz0mmmmm111r01nnnnnddddd")

// Data Processing - FP and SIMD - SIMD Two-register misc
INST(REV64_asimd,            "REV64",                                     "0Q001110zz100000000010nnnnnddddd")
INST(REV16_asimd,            "REV16 (vector)",                            "0Q001110zz100000000110nnnnnddddd")
INST(SADDLP,                 "SADDLP",                                    "0Q001110zz100000001010nnnnnddddd")
INST(SUQADD_2,               "SUQADD",                                    "0Q001110zz100000001110nnnnnddddd")
INST(CLS_asimd,              "CLS (vector)",                              "0Q001110zz100000010010nnnnnddddd")
INST(CNT,                    "CNT",                                       "0Q001110zz100000010110nnnnnddddd")
INST(SADALP,                 "SADALP",                                    "0Q001110zz100000011010nnnnnddddd")
INST(SQABS_2,                "SQABS",                                     "0Q001110zz100000011110nnnnnddddd")
INST(CMGT_zero_2,            "CMGT (zero)",                               "0Q001110zz100000100010nnnnnddddd")
INST(CMEQ_zero_2,            "CMEQ (zero)",                               "0Q001110zz100000100110nnnnnddddd")
INST(CMLT_2,                 "CMLT (zero)",                               "0Q001110zz100000101010nnnnnddddd")
INST(ABS_2,                  "ABS",                                       "0Q001110zz100000101110nnnnnddddd")
INST(XTN,                    "XTN, XTN2",                                 "0Q001110zz100001001010nnnnnddddd")
INST(SQXTN_2,                "SQXTN, SQXTN2",                             "0Q001110zz100001010010nnnnnddddd")
INST(FCVTN,                  "FCVTN, FCVTN2",                             "0Q0011100z100001011010nnnnnddddd")
INST(FCVTL,                  "FCVTL, FCVTL2",                             "0Q0011100z100001011110nnnnnddddd")
INST(FRINTN_1,               "FRINTN (vector)",                           "0Q00111001111001100010nnnnnddddd")
INST(FRINTN_2,               "FRINTN (vector)",                           "0Q0011100z100001100010nnnnnddddd")
INST(FRINTM_1,               "FRINTM (vector)",                           "0Q00111001111001100110nnnnnddddd")
INST(FRINTM_2,               "FRINTM (vector)",                           "0Q0011100z100001100110nnnnnddddd")
//INST(FCVTNS_3,               "FCVTNS (vector)",                           "0Q00111001111001101010nnnnnddddd")
INST(FCVTNS_4,               "FCVTNS (vector)",                           "0Q0011100z100001101010nnnnnddddd")
//INST(FCVTMS_3,               "FCVTMS (vector)",                           "0Q00111001111001101110nnnnnddddd")
INST(FCVTMS_4,               "FCVTMS (vector)",                           "0Q0011100z100001101110nnnnnddddd")
//INST(FCVTAS_3,               "FCVTAS (vector)",                           "0Q00111001111001110010nnnnnddddd")
INST(FCVTAS_4,               "FCVTAS (vector)",                           "0Q0011100z100001110010nnnnnddddd")
//INST(SCVTF_int_3,            "SCVTF (vector, integer)",                   "0Q00111001111001110110nnnnnddddd")
INST(SCVTF_int_4,            "SCVTF (vector, integer)",                   "0Q0011100z100001110110nnnnnddddd")
//INST(FCMGT_zero_3,           "FCMGT (zero)",                              "0Q00111011111000110010nnnnnddddd")
INST(FCMGT_zero_4,           "FCMGT (zero)",                              "0Q0011101z100000110010nnnnnddddd")
INST(FCMEQ_zero_3,           "FCMEQ (zero)",                              "0Q00111011111000110110nnnnnddddd")
INST(FCMEQ_zero_4,           "FCMEQ (zero)",                              "0Q0011101z100000110110nnnnnddddd")
//INST(FCMLT_3,                "FCMLT (zero)",                              "0Q00111011111000111010nnnnnddddd")
INST(FCMLT_4,                "FCMLT (zero)",                              "0Q0011101z100000111010nnnnnddddd")
INST(FABS_1,                 "FABS (vector)",                             "0Q00111011111000111110nnnnnddddd")
INST(FABS_2,                 "FABS (vector)",                             "0Q0011101z100000111110nnnnnddddd")
INST(FRINTP_1,               "FRINTP (vector)",                           "0Q00111011111001100010nnnnnddddd")
INST(FRINTP_2,               "FRINTP (vector)",                           "0Q0011101z100001100010nnnnnddddd")
INST(FRINTZ_1,               "FRINTZ (vector)",                           "0Q00111011111001100110nnnnnddddd")
INST(FRINTZ_2,               "FRINTZ (vector)",                           "0Q0011101z100001100110nnnnnddddd")
//INST(FCVTPS_3,               "FCVTPS (vector)",                           "0Q00111011111001101010nnnnnddddd")
INST(FCVTPS_4,               "FCVTPS (vector)",                           "0Q0011101z100001101010nnnnnddddd")
//INST(FCVTZS_int_3,           "FCVTZS (vector, integer)",                  "0Q00111011111001101110nnnnnddddd")
INST(FCVTZS_int_4,           "FCVTZS (vector, integer)",                  "0Q0011101z100001101110nnnnnddddd")
INST(URECPE,                 "URECPE",                                    "0Q0011101z100001110010nnnnnddddd")
INST(FRECPE_3,               "FRECPE",                                    "0Q00111011111001110110nnnnnddddd")
INST(FRECPE_4,               "FRECPE",                                    "0Q0011101z100001110110nnnnnddddd")
INST(REV32_asimd,            "REV32 (vector)",                            "0Q101110zz100000000010nnnnnddddd")
INST(UADDLP,                 "UADDLP",                                    "0Q101110zz100000001010nnnnnddddd")
INST(USQADD_2,               "USQADD",                                    "0Q101110zz100000001110nnnnnddddd")
INST(CLZ_asimd,              "CLZ (vector)",                              "0Q101110zz100000010010nnnnnddddd")
INST(UADALP,                 "UADALP",                                    "0Q101110zz100000011010nnnnnddddd")
INST(SQNEG_2,                "SQNEG",                                     "0Q101110zz100000011110nnnnnddddd")
INST(CMGE_zero_2,            "CMGE (zero)",                               "0Q101110zz100000100010nnnnnddddd")
INST(CMLE_2,                 "CMLE (zero)",                               "0Q101110zz100000100110nnnnnddddd")
INST(NEG_2,                  "NEG (vector)",                              "0Q101110zz100000101110nnnnnddddd")
INST(SQXTUN_2,               "SQXTUN, SQXTUN2",                           "0Q101110zz100001001010nnnnnddddd")
INST(SHLL,                   "SHLL, SHLL2",                               "0Q101110zz100001001110nnnnnddddd")
INST(UQXTN_2,                "UQXTN, UQXTN2",                             "0Q101110zz100001010010nnnnnddddd")
INST(FCVTXN_2,               "FCVTXN, FCVTXN2",                           "0Q1011100z100001011010nnnnnddddd")
INST(FRINTA_1,               "FRINTA (vector)",                           "0Q10111001111001100010nnnnnddddd")
INST(FRINTA_2,               "FRINTA (vector)",                           "0Q1011100z100001100010nnnnnddddd")
INST(FRINTX_1,               "FRINTX (vector)",                           "0Q10111001111001100110nnnnnddddd")
INST(FRINTX_2,               "FRINTX (vector)",                           "0Q1011100z100001100110nnnnnddddd")
//INST(FCVTNU_3,               "FCVTNU (vector)",                           "0Q10111001111001101010nnnnnddddd")
INST(FCVTNU_4,               "FCVTNU (vector)",                           "0Q1011100z100001101010nnnnnddddd")
//INST(FCVTMU_3,               "FCVTMU (vector)",                           "0Q10111001111001101110nnnnnddddd")
INST(FCVTMU_4,               "FCVTMU (vector)",                           "0Q1011100z100001101110nnnnnddddd")
//INST(FCVTAU_3,               "FCVTAU (vector)",                           "0Q10111001111001110010nnnnnddddd")
INST(FCVTAU_4,               "FCVTAU (vector)",                           "0Q1011100z100001110010nnnnnddddd")
//INST(UCVTF_int_3,            "UCVTF (vector, integer)",                   "0Q10111001111001110110nnnnnddddd")
INST(UCVTF_int_4,            "UCVTF (vector, integer)",                   "0Q1011100z100001110110nnnnnddddd")
INST(NOT,                    "NOT",                                       "0Q10111000100000010110nnnnnddddd")
INST(RBIT_asimd,             "RBIT (vector)",                             "0Q10111001100000010110nnnnnddddd")
INST(FNEG_1,                 "FNEG (vector)",                             "0Q10111011111000111110nnnnnddddd")
INST(FNEG_2,                 "FNEG (vector)",                             "0Q1011101z100000111110nnnnnddddd")
INST(FRINTI_1,               "FRINTI (vector)",                           "0Q10111011111001100110nnnnnddddd")
INST(FRINTI_2,               "FRINTI (vector)",                           "0Q1011101z100001100110nnnnnddddd")
//INST(FCMGE_zero_3,           "FCMGE (zero)",                              "0Q10111011111000110010nnnnnddddd")
INST(FCMGE_zero_4,           "FCMGE (zero)",                              "0Q1011101z100000110010nnnnnddddd")
//INST(FCMLE_3,                "FCMLE (zero)",                              "0Q10111011111000110110nnnnnddddd")
INST(FCMLE_4,                "FCMLE (zero)",                              "0Q1011101z100000110110nnnnnddddd")
//INST(FCVTPU_3,               "FCVTPU (vector)",                           "0Q10111011111001101010nnnnnddddd")
INST(FCVTPU_4,               "FCVTPU (vector)",                           "0Q1011101z100001101010nnnnnddddd")
//INST(FCVTZU_int_3,           "FCVTZU (vector, integer)",                  "0Q10111011111001101110nnnnnddddd")
INST(FCVTZU_int_4,           "FCVTZU (vector, integer)",                  "0Q1011101z100001101110nnnnnddddd")
INST(URSQRTE,                "URSQRTE",                                   "0Q1011101z100001110010nnnnnddddd")
INST(FRSQRTE_3,              "FRSQRTE",                                   "0Q10111011111001110110nnnnnddddd")
INST(FRSQRTE_4,              "FRSQRTE",                                   "0Q1011101z100001110110nnnnnddddd")
//INST(FSQRT_1,                "FSQRT (vector)",                            "0Q10111011111001111110nnnnnddddd")
INST(FSQRT_2,                "FSQRT (vector)",                            "0Q1011101z100001111110nnnnnddddd")
//INST(FRINT32X_1,             "FRINT32X (vector)",                         "0Q1011100z100001111110nnnnnddddd") // ARMv8.5
//INST(FRINT64X_1,             "FRINT64X (vector)",                         "0Q1011100z100001111010nnnnnddddd") // ARMv8.5
//INST(FRINT32Z_1,             "FRINT32Z (vector)",                         "0Q0011100z100001111010nnnnnddddd") // ARMv8.5
//INST(FRINT64Z_1,             "FRINT64Z (vector)",                         "0Q0011100z100001111110nnnnnddddd") // ARMv8.5

// Data Processing - FP and SIMD - SIMD across lanes
INST(SADDLV,                 "SADDLV",                                    "0Q001110zz110000001110nnnnnddddd")
INST(SMAXV,                  "SMAXV",                                     "0Q001110zz110000101010nnnnnddddd")
INST(SMINV,                  "SMINV",                                     "0Q001110zz110001101010nnnnnddddd")
INST(ADDV,                   "ADDV",                                      "0Q001110zz110001101110nnnnnddddd")
//INST(FMAXNMV_1,              "FMAXNMV",                                   "0Q00111000110000110010nnnnnddddd")
INST(FMAXNMV_2,              "FMAXNMV",                                   "0Q1011100z110000110010nnnnnddddd")
//INST(FMAXV_1,                "FMAXV",                                     "0Q00111000110000111110nnnnnddddd")
INST(FMAXV_2,                "FMAXV",                                     "0Q1011100z110000111110nnnnnddddd")
//INST(FMINNMV_1,              "FMINNMV",                                   "0Q00111010110000110010nnnnnddddd")
INST(FMINNMV_2,              "FMINNMV",                                   "0Q1011101z110000110010nnnnnddddd")
//INST(FMINV_1,                "FMINV",                                     "0Q00111010110000111110nnnnnddddd")
INST(FMINV_2,                "FMINV",                                     "0Q1011101z110000111110nnnnnddddd")
INST(UADDLV,                 "UADDLV",                                    "0Q101110zz110000001110nnnnnddddd")
INST(UMAXV,                  "UMAXV",                                     "0Q101110zz110000101010nnnnnddddd")
INST(UMINV,                  "UMINV",                                     "0Q101110zz110001101010nnnnnddddd")

// Data Processing - FP and SIMD - SIMD three different
INST(SADDL,                  "SADDL, SADDL2",                             "0Q001110zz1mmmmm000000nnnnnddddd")
INST(SADDW,                  "SADDW, SADDW2",                             "0Q001110zz1mmmmm000100nnnnnddddd")
INST(SSUBL,                  "SSUBL, SSUBL2",                             "0Q001110zz1mmmmm001000nnnnnddddd")
INST(SSUBW,                  "SSUBW, SSUBW2",                             "0Q001110zz1mmmmm001100nnnnnddddd")
INST(ADDHN,                  "ADDHN, ADDHN2",                             "0Q001110zz1mmmmm010000nnnnnddddd")
INST(SABAL,                  "SABAL, SABAL2",                             "0Q001110zz1mmmmm010100nnnnnddddd")
INST(SUBHN,                  "SUBHN, SUBHN2",                             "0Q001110zz1mmmmm011000nnnnnddddd")
INST(SABDL,                  "SABDL, SABDL2",                             "0Q001110zz1mmmmm011100nnnnnddddd")
INST(SMLAL_vec,              "SMLAL, SMLAL2 (vector)",                    "0Q001110zz1mmmmm100000nnnnnddddd")
INST(SMLSL_vec,              "SMLSL, SMLSL2 (vector)",                    "0Q001110zz1mmmmm101000nnnnnddddd")
INST(SMULL_vec,              "SMULL, SMULL2 (vector)",                    "0Q001110zz1mmmmm110000nnnnnddddd")
INST(PMULL,                  "PMULL, PMULL2",                             "0Q001110zz1mmmmm111000nnnnnddddd")
INST(UADDL,                  "UADDL, UADDL2",                             "0Q101110zz1mmmmm000000nnnnnddddd")
INST(UADDW,                  "UADDW, UADDW2",                             "0Q101110zz1mmmmm000100nnnnnddddd")
INST(USUBL,                  "USUBL, USUBL2",                             "0Q101110zz1mmmmm001000nnnnnddddd")
INST(USUBW,                  "USUBW, USUBW2",                             "0Q101110zz1mmmmm001100nnnnnddddd")
INST(RADDHN,                 "RADDHN, RADDHN2",                           "0Q101110zz1mmmmm010000nnnnnddddd")
INST(UABAL,                  "UABAL, UABAL2",                             "0Q101110zz1mmmmm010100nnnnnddddd")
INST(RSUBHN,                 "RSUBHN, RSUBHN2",                           "0Q101110zz1mmmmm011000nnnnnddddd")
INST(UABDL,                  "UABDL, UABDL2",                             "0Q101110zz1mmmmm011100nnnnnddddd")
INST(UMLAL_vec,              "UMLAL, UMLAL2 (vector)",                    "0Q101110zz1mmmmm100000nnnnnddddd")
INST(UMLSL_vec,              "UMLSL, UMLSL2 (vector)",                    "0Q101110zz1mmmmm101000nnnnnddddd")
INST(UMULL_vec,              "UMULL, UMULL2 (vector)",                    "0Q101110zz1mmmmm110000nnnnnddddd")
//INST(SQDMLAL_vec_2,          "SQDMLAL, SQDMLAL2 (vector)",                "0Q001110zz1mmmmm100100nnnnnddddd")
//INST(SQDMLSL_vec_2,          "SQDMLSL, SQDMLSL2 (vector)",                "0Q001110zz1mmmmm101100nnnnnddddd")
INST(SQDMULL_vec_2,          "SQDMULL, SQDMULL2 (vector)",                "0Q001110zz1mmmmm110100nnnnnddddd")

// Data Processing - FP and SIMD - SIMD three same
INST(SHADD,                  "SHADD",                                     "0Q001110zz1mmmmm000001nnnnnddddd")
INST(SQADD_2,                "SQADD",                                     "0Q001110zz1mmmmm000011nnnnnddddd")
INST(SRHADD,                 "SRHADD",                                    "0Q001110zz1mmmmm000101nnnnnddddd")
INST(SHSUB,                  "SHSUB",                                     "0Q001110zz1mmmmm001001nnnnnddddd")
INST(SQSUB_2,                "SQSUB",                                     "0Q001110zz1mmmmm001011nnnnnddddd")
INST(CMGT_reg_2,             "CMGT (register)",                           "0Q001110zz1mmmmm001101nnnnnddddd")
INST(CMGE_reg_2,             "CMGE (register)",                           "0Q001110zz1mmmmm001111nnnnnddddd")
INST(SSHL_2,                 "SSHL",                                      "0Q001110zz1mmmmm010001nnnnnddddd")
INST(SQSHL_reg_2,            "SQSHL (register)",                          "0Q001110zz1mmmmm010011nnnnnddddd")
INST(SRSHL_2,                "SRSHL",                                     "0Q001110zz1mmmmm010101nnnnnddddd")
//INST(SQRSHL_2,               "SQRSHL",                                    "0Q001110zz1mmmmm010111nnnnnddddd")
INST(SMAX,                   "SMAX",                                      "0Q001110zz1mmmmm011001nnnnnddddd")
INST(SMIN,                   "SMIN",                                      "0Q001110zz1mmmmm011011nnnnnddddd")
INST(SABD,                   "SABD",                                      "0Q001110zz1mmmmm011101nnnnnddddd")
INST(SABA,                   "SABA",                                      "0Q001110zz1mmmmm011111nnnnnddddd")
INST(ADD_vector,             "ADD (vector)",                              "0Q001110zz1mmmmm100001nnnnnddddd")
INST(CMTST_2,                "CMTST",                                     "0Q001110zz1mmmmm100011nnnnnddddd")
INST(MLA_vec,                "MLA (vector)",                              "0Q001110zz1mmmmm100101nnnnnddddd")
INST(MUL_vec,                "MUL (vector)",                              "0Q001110zz1mmmmm100111nnnnnddddd")
INST(SMAXP,                  "SMAXP",                                     "0Q001110zz1mmmmm101001nnnnnddddd")
INST(SMINP,                  "SMINP",                                     "0Q001110zz1mmmmm101011nnnnnddddd")
INST(SQDMULH_vec_2,          "SQDMULH (vector)",                          "0Q001110zz1mmmmm101101nnnnnddddd")
INST(ADDP_vec,               "ADDP (vector)",                             "0Q001110zz1mmmmm101111nnnnnddddd")
INST(FMAXNM_2,               "FMAXNM (vector)",                           "0Q0011100z1mmmmm110001nnnnnddddd")
INST(FMLA_vec_2,             "FMLA (vector)",                             "0Q0011100z1mmmmm110011nnnnnddddd")
INST(FADD_2,                 "FADD (vector)",                             "0Q0011100z1mmmmm110101nnnnnddddd")
INST(FMAX_2,                 "FMAX (vector)",                             "0Q0011100z1mmmmm111101nnnnnddddd")
INST(FMULX_vec_4,            "FMULX",                                     "0Q0011100z1mmmmm110111nnnnnddddd")
INST(FCMEQ_reg_4,            "FCMEQ (register)",                          "0Q0011100z1mmmmm111001nnnnnddddd")
//INST(FMLAL_vec_1,            "FMLAL, FMLAL2 (vector)",                    "0Q0011100z1mmmmm111011nnnnnddddd")
INST(FRECPS_4,               "FRECPS",                                    "0Q0011100z1mmmmm111111nnnnnddddd")
INST(AND_asimd,              "AND (vector)",                              "0Q001110001mmmmm000111nnnnnddddd")
INST(BIC_asimd_reg,          "BIC (vector, register)",                    "0Q001110011mmmmm000111nnnnnddddd")
INST(FMINNM_2,               "FMINNM (vector)",                           "0Q0011101z1mmmmm110001nnnnnddddd")
INST(FMLS_vec_2,             "FMLS (vector)",                             "0Q0011101z1mmmmm110011nnnnnddddd")
INST(FSUB_2,                 "FSUB (vector)",                             "0Q0011101z1mmmmm110101nnnnnddddd")
//INST(FMLSL_vec_1,            "FMLSL, FMLSL2 (vector)",                    "0Q0011101z1mmmmm111011nnnnnddddd")
INST(FMIN_2,                 "FMIN (vector)",                             "0Q0011101z1mmmmm111101nnnnnddddd")
INST(FRSQRTS_4,              "FRSQRTS",                                   "0Q0011101z1mmmmm111111nnnnnddddd")
INST(ORR_asimd_reg,          "ORR (vector, register)",                    "0Q001110101mmmmm000111nnnnnddddd")
INST(ORN_asimd,              "ORN (vector)",                              "0Q001110111mmmmm000111nnnnnddddd")
INST(UHADD,                  "UHADD",                                     "0Q101110zz1mmmmm000001nnnnnddddd")
INST(UQADD_2,                "UQADD",                                     "0Q101110zz1mmmmm000011nnnnnddddd")
INST(URHADD,                 "URHADD",                                    "0Q101110zz1mmmmm000101nnnnnddddd")
INST(UHSUB,                  "UHSUB",                                     "0Q101110zz1mmmmm001001nnnnnddddd")
INST(UQSUB_2,                "UQSUB",                                     "0Q101110zz1mmmmm001011nnnnnddddd")
INST(CMHI_2,                 "CMHI (register)",                           "0Q101110zz1mmmmm001101nnnnnddddd")
INST(CMHS_2,                 "CMHS (register)",                           "0Q101110zz1mmmmm001111nnnnnddddd")
INST(USHL_2,                 "USHL",                                      "0Q101110zz1mmmmm010001nnnnnddddd")
INST(UQSHL_reg_2,            "UQSHL (register)",                          "0Q101110zz1mmmmm010011nnnnnddddd")
INST(URSHL_2,                "URSHL",                                     "0Q101110zz1mmmmm010101nnnnnddddd")
//INST(UQRSHL_2,               "UQRSHL",                                    "0Q101110zz1mmmmm010111nnnnnddddd")
INST(UMAX,                   "UMAX",                                      "0Q101110zz1mmmmm011001nnnnnddddd")
INST(UMIN,                   "UMIN",                                      "0Q101110zz1mmmmm011011nnnnnddddd")
INST(UABD,                   "UABD",                                      "0Q101110zz1mmmmm011101nnnnnddddd")
INST(UABA,                   "UABA",                                      "0Q101110zz1mmmmm011111nnnnnddddd")
INST(SUB_2,                  "SUB (vector)",                              "0Q101110zz1mmmmm100001nnnnnddddd")
INST(CMEQ_reg_2,             "CMEQ (register)",                           "0Q101110zz1mmmmm100011nnnnnddddd")
INST(MLS_vec,                "MLS (vector)",                              "0Q101110zz1mmmmm100101nnnnnddddd")
INST(PMUL,                   "PMUL",                                      "0Q101110zz1mmmmm100111nnnnnddddd")
INST(UMAXP,                  "UMAXP",                                     "0Q101110zz1mmmmm101001nnnnnddddd")
INST(UMINP,                  "UMINP",                                     "0Q101110zz1mmmmm101011nnnnnddddd")
INST(SQRDMULH_vec_2,         "SQRDMULH (vector)",                         "0Q101110zz1mmmmm101101nnnnnddddd")
INST(FMAXNMP_vec_2,          "FMAXNMP (vector)",                          "0Q1011100z1mmmmm110001nnnnnddddd")
//INST(FMLAL_vec_2,            "FMLAL, FMLAL2 (vector)",                    "0Q1011100z1mmmmm110011nnnnnddddd")
INST(FADDP_vec_2,            "FADDP (vector)",                            "0Q1011100z1mmmmm110101nnnnnddddd")
INST(FMUL_vec_2,             "FMUL (vector)",                             "0Q1011100z1mmmmm110111nnnnnddddd")
INST(FCMGE_reg_4,            "FCMGE (register)",                          "0Q1011100z1mmmmm111001nnnnnddddd")
INST(FACGE_4,                "FACGE",                                     "0Q1011100z1mmmmm111011nnnnnddddd")
INST(FMAXP_vec_2,            "FMAXP (vector)",                            "0Q1011100z1mmmmm111101nnnnnddddd")
INST(FDIV_2,                 "FDIV (vector)",                             "0Q1011100z1mmmmm111111nnnnnddddd")
INST(EOR_asimd,              "EOR (vector)",                              "0Q101110001mmmmm000111nnnnnddddd")
INST(BSL,                    "BSL",                                       "0Q101110011mmmmm000111nnnnnddddd")
INST(FMINNMP_vec_2,          "FMINNMP (vector)",                          "0Q1011101z1mmmmm110001nnnnnddddd")
//INST(FMLSL_vec_2,            "FMLSL, FMLSL2 (vector)",                    "0Q1011101z1mmmmm110011nnnnnddddd")
INST(FABD_4,                 "FABD",                                      "0Q1011101z1mmmmm110101nnnnnddddd")
INST(FCMGT_reg_4,            "FCMGT (register)",                          "0Q1011101z1mmmmm111001nnnnnddddd")
INST(FACGT_4,                "FACGT",                                     "0Q1011101z1mmmmm111011nnnnnddddd")
INST(FMINP_vec_2,            "FMINP (vector)",                            "0Q1011101z1mmmmm111101nnnnnddddd")
INST(BIT,                    "BIT",                                       "0Q101110101mmmmm000111nnnnnddddd")
INST(BIF,                    "BIF",                                       "0Q101110111mmmmm000111nnnnnddddd")

// Data Processing - FP and SIMD - SIMD modified immediate
INST(MOVI,                   "MOVI, MVNI, ORR, BIC (vector, immediate)",  "0Qo0111100000abcmmmm01defghddddd")
INST(FMOV_2,                 "FMOV (vector, immediate)",                  "0Qo0111100000abc111101defghddddd")
INST(FMOV_3,                 "FMOV (vector, immediate)",                  "0Q00111100000abc111111defghddddd")
INST(UnallocatedEncoding,    "Unallocated SIMD modified immediate",       "0--0111100000-------11----------")

// Data Processing - FP and SIMD - SIMD Shift by immediate
INST(SSHR_2,                 "SSHR",                                      "0Q0011110IIIIiii000001nnnnnddddd")
INST(SSRA_2,                 "SSRA",                                      "0Q0011110IIIIiii000101nnnnnddddd")
INST(SRSHR_2,                "SRSHR",                                     "0Q0011110IIIIiii001001nnnnnddddd")
INST(SRSRA_2,                "SRSRA",                                     "0Q0011110IIIIiii001101nnnnnddddd")
INST(SHL_2,                  "SHL",                                       "0Q0011110IIIIiii010101nnnnnddddd")
INST(SQSHL_imm_2,            "SQSHL (immediate)",                         "0Q0011110IIIIiii011101nnnnnddddd")
INST(SHRN,                   "SHRN, SHRN2",                               "0Q0011110IIIIiii100001nnnnnddddd")
INST(RSHRN,                  "RSHRN, RSHRN2",                             "0Q0011110IIIIiii100011nnnnnddddd")
INST(SQSHRN_2,               "SQSHRN, SQSHRN2",                           "0Q0011110IIIIiii100101nnnnnddddd")
INST(SQRSHRN_2,              "SQRSHRN, SQRSHRN2",                         "0Q0011110IIIIiii100111nnnnnddddd")
INST(SSHLL,                  "SSHLL, SSHLL2",                             "0Q0011110IIIIiii101001nnnnnddddd")
INST(SCVTF_fix_2,            "SCVTF (vector, fixed-point)",               "0Q0011110IIIIiii111001nnnnnddddd")
INST(FCVTZS_fix_2,           "FCVTZS (vector, fixed-point)",              "0Q0011110IIIIiii111111nnnnnddddd")
INST(USHR_2,                 "USHR",                                      "0Q1011110IIIIiii000001nnnnnddddd")
INST(USRA_2,                 "USRA",                                      "0Q1011110IIIIiii000101nnnnnddddd")
INST(URSHR_2,                "URSHR",                                     "0Q1011110IIIIiii001001nnnnnddddd")
INST(URSRA_2,                "URSRA",                                     "0Q1011110IIIIiii001101nnnnnddddd")
INST(SRI_2,                  "SRI",                                       "0Q1011110IIIIiii010001nnnnnddddd")
INST(SLI_2,                  "SLI",                                       "0Q1011110IIIIiii010101nnnnnddddd")
INST(SQSHLU_2,               "SQSHLU",                                    "0Q1011110IIIIiii011001nnnnnddddd")
INST(UQSHL_imm_2,            "UQSHL (immediate)",                         "0Q1011110IIIIiii011101nnnnnddddd")
INST(SQSHRUN_2,              "SQSHRUN, SQSHRUN2",                         "0Q1011110IIIIiii100001nnnnnddddd")
INST(SQRSHRUN_2,             "SQRSHRUN, SQRSHRUN2",                       "0Q1011110IIIIiii100011nnnnnddddd")
INST(UQSHRN_2,               "UQSHRN, UQSHRN2",                           "0Q1011110IIIIiii100101nnnnnddddd")
INST(UQRSHRN_2,              "UQRSHRN, UQRSHRN2",                         "0Q1011110IIIIiii100111nnnnnddddd")
INST(USHLL,                  "USHLL, USHLL2",                             "0Q1011110IIIIiii101001nnnnnddddd")
INST(UCVTF_fix_2,            "UCVTF (vector, fixed-point)",               "0Q1011110IIIIiii111001nnnnnddddd")
INST(FCVTZU_fix_2,           "FCVTZU (vector, fixed-point)",              "0Q1011110IIIIiii111111nnnnnddddd")

// Data Processing - FP and SIMD - SIMD vector x indexed element
INST(SMLAL_elt,              "SMLAL, SMLAL2 (by element)",                "0Q001111zzLMmmmm0010H0nnnnnddddd")
//INST(SQDMLAL_elt_2,          "SQDMLAL, SQDMLAL2 (by element)",            "0Q001111zzLMmmmm0011H0nnnnnddddd")
INST(SMLSL_elt,              "SMLSL, SMLSL2 (by element)",                "0Q001111zzLMmmmm0110H0nnnnnddddd")
//INST(SQDMLSL_elt_2,          "SQDMLSL, SQDMLSL2 (by element)",            "0Q001111zzLMmmmm0111H0nnnnnddddd")
INST(MUL_elt,                "MUL (by element)",                          "0Q001111zzLMmmmm1000H0nnnnnddddd")
INST(SMULL_elt,              "SMULL, SMULL2 (by element)",                "0Q001111zzLMmmmm1010H0nnnnnddddd")
INST(SQDMULL_elt_2,          "SQDMULL, SQDMULL2 (by element)",            "0Q001111zzLMmmmm1011H0nnnnnddddd")
INST(SQDMULH_elt_2,          "SQDMULH (by element)",                      "0Q001111zzLMmmmm1100H0nnnnnddddd")
INST(SQRDMULH_elt_2,         "SQRDMULH (by element)",                     "0Q001111zzLMmmmm1101H0nnnnnddddd")
INST(SDOT_elt,               "SDOT (by element)",                         "0Q001111zzLMmmmm1110H0nnnnnddddd")
INST(FMLA_elt_3,             "FMLA (by element)",                         "0Q00111100LMmmmm0001H0nnnnnddddd")
INST(FMLA_elt_4,             "FMLA (by element)",                         "0Q0011111zLMmmmm0001H0nnnnnddddd")
INST(FMLS_elt_3,             "FMLS (by element)",                         "0Q00111100LMmmmm0101H0nnnnnddddd")
INST(FMLS_elt_4,             "FMLS (by element)",                         "0Q0011111zLMmmmm0101H0nnnnnddddd")
//INST(FMUL_elt_3,             "FMUL (by element)",                         "0Q00111100LMmmmm1001H0nnnnnddddd")
INST(FMUL_elt_4,             "FMUL (by element)",                         "0Q0011111zLMmmmm1001H0nnnnnddddd")
//INST(FMLAL_elt_1,            "FMLAL, FMLAL2 (by element)",                "0Q0011111zLMmmmm0000H0nnnnnddddd")
//INST(FMLAL_elt_2,            "FMLAL, FMLAL2 (by element)",                "0Q1011111zLMmmmm1000H0nnnnnddddd")
//INST(FMLSL_elt_1,            "FMLSL, FMLSL2 (by element)",                "0Q0011111zLMmmmm0100H0nnnnnddddd")
//INST(FMLSL_elt_2,            "FMLSL, FMLSL2 (by element)",                "0Q1011111zLMmmmm1100H0nnnnnddddd")
INST(MLA_elt,                "MLA (by element)",                          "0Q101111zzLMmmmm0000H0nnnnnddddd")
INST(UMLAL_elt,              "UMLAL, UMLAL2 (by element)",                "0Q101111zzLMmmmm0010H0nnnnnddddd")
INST(MLS_elt,                "MLS (by element)",                          "0Q101111zzLMmmmm0100H0nnnnnddddd")
INST(UMLSL_elt,              "UMLSL, UMLSL2 (by element)",                "0Q101111zzLMmmmm0110H0nnnnnddddd")
INST(UMULL_elt,              "UMULL, UMULL2 (by element)",                "0Q101111zzLMmmmm1010H0nnnnnddddd")
//INST(SQRDMLAH_elt_2,         "SQRDMLAH (by element)",                     "0Q101111zzLMmmmm1101H0nnnnnddddd")
INST(UDOT_elt,               "UDOT (by element)",                         "0Q101111zzLMmmmm1110H0nnnnnddddd")
//INST(SQRDMLSH_elt_2,         "SQRDMLSH (by element)",                     "0Q101111zzLMmmmm1111H0nnnnnddddd")
//INST(FMULX_elt_3,            "FMULX (by element)",                        "0Q10111100LMmmmm1001H0nnnnnddddd")
INST(FMULX_elt_4,            "FMULX (by element)",                        "0Q1011111zLMmmmm1001H0nnnnnddddd")
INST(FCMLA_elt,              "FCMLA (by element)",                        "0Q101111zzLMmmmm0rr1H0nnnnnddddd")

// Data Processing - FP and SIMD - Cryptographic three register
INST(SM3TT1A,                "SM3TT1A",                                   "11001110010mmmmm10ii00nnnnnddddd")
INST(SM3TT1B,                "SM3TT1B",                                   "11001110010mmmmm10ii01nnnnnddddd")
INST(SM3TT2A,                "SM3TT2A",                                   "11001110010mmmmm10ii10nnnnnddddd")
INST(SM3TT2B,                "SM3TT2B",                                   "11001110010mmmmm10ii11nnnnnddddd")

// Data Processing - FP and SIMD - SHA512 three register
INST(SHA512H,                "SHA512H",                                   "11001110011mmmmm100000nnnnnddddd")
INST(SHA512H2,               "SHA512H2",                                  "11001110011mmmmm100001nnnnnddddd")
INST(SHA512SU1,              "SHA512SU1",                                 "11001110011mmmmm100010nnnnnddddd")
INST(RAX1,                   "RAX1",                                      "11001110011mmmmm100011nnnnnddddd")
INST(SM3PARTW1,              "SM3PARTW1",                                 "11001110011mmmmm110000nnnnnddddd")
INST(SM3PARTW2,              "SM3PARTW2",                                 "11001110011mmmmm110001nnnnnddddd")
INST(SM4EKEY,                "SM4EKEY",                                   "11001110011mmmmm110010nnnnnddddd")
INST(XAR,                    "XAR",                                       "11001110100mmmmmiiiiiinnnnnddddd")

// Data Processing - FP and SIMD - Cryptographic four register
INST(EOR3,                   "EOR3",                                      "11001110000mmmmm0aaaaannnnnddddd")
INST(BCAX,                   "BCAX",                                      "11001110001mmmmm0aaaaannnnnddddd")
INST(SM3SS1,                 "SM3SS1",                                    "11001110010mmmmm0aaaaannnnnddddd")

// Data Processing - FP and SIMD - SHA512 two register
INST(SHA512SU0,              "SHA512SU0",                                 "1100111011000000100000nnnnnddddd")
INST(SM4E,                   "SM4E",                                      "1100111011000000100001nnnnnddddd")

// Data Processing - FP and SIMD - Conversion between floating point and fixed point
INST(SCVTF_float_fix,        "SCVTF (scalar, fixed-point)",               "z0011110yy000010ppppppnnnnnddddd")
INST(UCVTF_float_fix,        "UCVTF (scalar, fixed-point)",               "z0011110yy000011ppppppnnnnnddddd")
INST(FCVTZS_float_fix,       "FCVTZS (scalar, fixed-point)",              "z0011110yy011000ppppppnnnnnddddd")
INST(FCVTZU_float_fix,       "FCVTZU (scalar, fixed-point)",              "z0011110yy011001ppppppnnnnnddddd")

// Data Processing - FP and SIMD - Conversion between floating point and integer
INST(FCVTNS_float,           "FCVTNS (scalar)",                           "z0011110yy100000000000nnnnnddddd")
INST(FCVTNU_float,           "FCVTNU (scalar)",                           "z0011110yy100001000000nnnnnddddd")
INST(SCVTF_float_int,        "SCVTF (scalar, integer)",                   "z0011110yy100010000000nnnnnddddd")
INST(UCVTF_float_int,        "UCVTF (scalar, integer)",                   "z0011110yy100011000000nnnnnddddd")
INST(FCVTAS_float,           "FCVTAS (scalar)",                           "z0011110yy100100000000nnnnnddddd")
INST(FCVTAU_float,           "FCVTAU (scalar)",                           "z0011110yy100101000000nnnnnddddd")
INST(FMOV_float_gen,         "FMOV (general)",                            "z0011110yy10r11o000000nnnnnddddd")
INST(FCVTPS_float,           "FCVTPS (scalar)",                           "z0011110yy101000000000nnnnnddddd")
INST(FCVTPU_float,           "FCVTPU (scalar)",                           "z0011110yy101001000000nnnnnddddd")
INST(FCVTMS_float,           "FCVTMS (scalar)",                           "z0011110yy110000000000nnnnnddddd")
INST(FCVTMU_float,           "FCVTMU (scalar)",                           "z0011110yy110001000000nnnnnddddd")
INST(FCVTZS_float_int,       "FCVTZS (scalar, integer)",                  "z0011110yy111000000000nnnnnddddd")
INST(FCVTZU_float_int,       "FCVTZU (scalar, integer)",                  "z0011110yy111001000000nnnnnddddd")
//INST(FJCVTZS,                "FJCVTZS",                                   "0001111001111110000000nnnnnddddd")

// Data Processing - FP and SIMD - Floating point data processing
INST(FMOV_float,             "FMOV (register)",                           "00011110yy100000010000nnnnnddddd")
INST(FABS_float,             "FABS (scalar)",                             "00011110yy100000110000nnnnnddddd")
INST(FNEG_float,             "FNEG (scalar)",                             "00011110yy100001010000nnnnnddddd")
INST(FSQRT_float,            "FSQRT (scalar)",                            "00011110yy100001110000nnnnnddddd")
INST(FCVT_float,             "FCVT",                                      "00011110yy10001oo10000nnnnnddddd")
INST(FRINTN_float,           "FRINTN (scalar)",                           "00011110yy100100010000nnnnnddddd")
INST(FRINTP_float,           "FRINTP (scalar)",                           "00011110yy100100110000nnnnnddddd")
INST(FRINTM_float,           "FRINTM (scalar)",                           "00011110yy100101010000nnnnnddddd")
INST(FRINTZ_float,           "FRINTZ (scalar)",                           "00011110yy100101110000nnnnnddddd")
INST(FRINTA_float,           "FRINTA (scalar)",                           "00011110yy100110010000nnnnnddddd")
INST(FRINTX_float,           "FRINTX (scalar)",                           "00011110yy100111010000nnnnnddddd")
INST(FRINTI_float,           "FRINTI (scalar)",                           "00011110yy100111110000nnnnnddddd")
//INST(FRINT32X_float,         "FRINT32X (scalar)",                       "00011110yy101000110000nnnnnddddd") // ARMv8.5
//INST(FRINT64X_float,         "FRINT64X (scalar)",                       "00011110yy101001110000nnnnnddddd") // ARMv8.5
//INST(FRINT32Z_float,         "FRINT32Z (scalar)",                       "00011110yy101000010000nnnnnddddd") // ARMv8.5
//INST(FRINT64Z_float,         "FRINT64Z (scalar)",                       "00011110yy101001010000nnnnnddddd") // ARMv8.5

// Data Processing - FP and SIMD - Floating point compare
INST(FCMP_float,             "FCMP",                                      "00011110yy1mmmmm001000nnnnn0o000")
INST(FCMPE_float,            "FCMPE",                                     "00011110yy1mmmmm001000nnnnn1o000")

// Data Processing - FP and SIMD - Floating point immediate
INST(FMOV_float_imm,         "FMOV (scalar, immediate)",                  "00011110yy1iiiiiiii10000000ddddd")

// Data Processing - FP and SIMD - Floating point conditional compare
INST(FCCMP_float,            "FCCMP",                                     "00011110yy1mmmmmcccc01nnnnn0ffff")
INST(FCCMPE_float,           "FCCMPE",                                    "00011110yy1mmmmmcccc01nnnnn1ffff")

// Data Processing - FP and SIMD - Floating point data processing two register
INST(FMUL_float,             "FMUL (scalar)",                             "00011110yy1mmmmm000010nnnnnddddd")
INST(FDIV_float,             "FDIV (scalar)",                             "00011110yy1mmmmm000110nnnnnddddd")
INST(FADD_float,             "FADD (scalar)",                             "00011110yy1mmmmm001010nnnnnddddd")
INST(FSUB_float,             "FSUB (scalar)",                             "00011110yy1mmmmm001110nnnnnddddd")
INST(FMAX_float,             "FMAX (scalar)",                             "00011110yy1mmmmm010010nnnnnddddd")
INST(FMIN_float,             "FMIN (scalar)",                             "00011110yy1mmmmm010110nnnnnddddd")
INST(FMAXNM_float,           "FMAXNM (scalar)",                           "00011110yy1mmmmm011010nnnnnddddd")
INST(FMINNM_float,           "FMINNM (scalar)",                           "00011110yy1mmmmm011110nnnnnddddd")
INST(FNMUL_float,            "FNMUL (scalar)",                            "00011110yy1mmmmm100010nnnnnddddd")

// Data Processing - FP and SIMD - Floating point conditional select
INST(FCSEL_float,            "FCSEL",                                     "00011110yy1mmmmmcccc11nnnnnddddd")

// Data Processing - FP and SIMD - Floating point data processing three register
INST(FMADD_float,            "FMADD",                                     "00011111yy0mmmmm0aaaaannnnnddddd")
INST(FMSUB_float,            "FMSUB",                                     "00011111yy0mmmmm1aaaaannnnnddddd")
INST(FNMADD_float,           "FNMADD",                                    "00011111yy1mmmmm0aaaaannnnnddddd")
INST(FNMSUB_float,           "FNMSUB",                                    "00011111yy1mmmmm1aaaaannnnnddddd")

// BFloat16
//INST(BFCVT,                  "BFCVT",                                     "0001111001100011010000nnnnnddddd") // v8.6
//INST(BFCVTN,                 "BFCVTN{2}",                                 "0Q00111010100001011010nnnnnddddd") // v8.6
//INST(BFDOT_element,          "BFDOT (by element)",                        "0Q00111101LMmmmm1111H0nnnnnddddd") // v8.6
//INST(BFDOT_vec,              "BFDOT (vector)",                            "0Q101110010mmmmm111111nnnnnddddd") // v8.6
//INST(BFMLALX_element,        "BFMLALX (by element)",                      "0Q00111111LMmmmm1111H0nnnnnddddd") // v8.6
//INST(BFMLALX_vector,         "BFMLALX (vector)",                          "0Q101110110mmmmm111111nnnnnddddd") // v8.6
//INST(BFMMLA,                 "BFMMLA",                                    "01101110010mmmmm111011nnnnnddddd") // v8.6
