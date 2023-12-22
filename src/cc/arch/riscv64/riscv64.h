#pragma once

#include <stdbool.h>

#include "emit_util.h"

#ifndef IM
#define IM(x)  im(x)
#endif
#ifndef IMMEDIATE_OFFSET
#define IMMEDIATE_OFFSET(ofs, reg)  immediate_offset(ofs, reg)
#endif
#ifndef IMMEDIATE_OFFSET0
#define IMMEDIATE_OFFSET0(reg)  immediate_offset(0, reg)
#endif
#ifndef LABEL_OFFSET_HI
#define LABEL_OFFSET_HI(label)  label_offset_hi(label)
#endif
#ifndef LABEL_OFFSET_LO
#define LABEL_OFFSET_LO(label)  label_offset_lo(label)
#endif

#define _UXTW(shift)  fmt("uxtw #%d", shift)
#define _LSL(shift)   fmt("lsl #%d", shift)

#define ZERO  "zero"  // x0: Zero register
#define RA    "ra"    // x1: Return Address
#define SP    "sp"    // x2: Stack Pointer
#define FP    "fp"    // x8: Frame Pointer
#define A0    "a0"
#define A1    "a1"
#define A2    "a2"
#define A3    "a3"
#define A4    "a4"
#define A5    "a5"
#define A6    "a6"
#define A7    "a7"
#define S0    "s0"
#define S1    "s1"
#define S2    "s2"
#define S3    "s3"
#define S4    "s4"
#define S5    "s5"
#define S6    "s6"
#define S7    "s7"
#define S8    "s8"
#define S9    "s9"
#define S10   "s10"
#define S11   "s11"
#define T0    "t0"
#define T1    "t1"
#define T2    "t2"
#define T3    "t3"
#define T4    "t4"
#define T5    "t5"
#define T6    "t6"

#define FA0   "fa0"
#define FA1   "fa1"
#define FA2   "fa2"
#define FA3   "fa3"
#define FA4   "fa4"
#define FA5   "fa5"
#define FA6   "fa6"
#define FA7   "fa7"
#define FS0   "fs0"
#define FS1   "fs1"
#define FS2   "fs2"
#define FS3   "fs3"
#define FS4   "fs4"
#define FS5   "fs5"
#define FS6   "fs6"
#define FS7   "fs7"
#define FS8   "fs8"
#define FS9   "fs9"
#define FS10  "fs10"
#define FS11  "fs11"
#define FT0   "ft0"
#define FT1   "ft1"
#define FT2   "ft2"
#define FT3   "ft3"
#define FT4   "ft4"
#define FT5   "ft5"
#define FT6   "ft6"
#define FT7   "ft7"
#define FT8   "ft8"
#define FT9   "ft9"
#define FT10  "ft10"
#define FT11  "ft11"

// Condition
#define CEQ   "eq"
#define CNE   "ne"
#define CLT   "lt"
#define CGE   "ge"
#define CLTU  "ltu"
#define CGEU  "geu"

#define LI(o1, o2)            EMIT_ASM("li", o1, o2)
#define LUI(o1, o2)           EMIT_ASM("lui", o1, o2)
#define ADD(o1, o2, o3)       EMIT_ASM("add", o1, o2, o3)
#define ADDI(o1, o2, o3)      EMIT_ASM("addi", o1, o2, o3)
#define ADDW(o1, o2, o3)      EMIT_ASM("addw", o1, o2, o3)
#define ADDIW(o1, o2, o3)     EMIT_ASM("addiw", o1, o2, o3)
#define SUB(o1, o2, o3)       EMIT_ASM("sub", o1, o2, o3)
#define SUBW(o1, o2, o3)      EMIT_ASM("subw", o1, o2, o3)
#define MUL(o1, o2, o3)       EMIT_ASM("mul", o1, o2, o3)
#define MULW(o1, o2, o3)      EMIT_ASM("mulw", o1, o2, o3)
#define DIV(o1, o2, o3)       EMIT_ASM("div", o1, o2, o3)
#define DIVU(o1, o2, o3)      EMIT_ASM("divu", o1, o2, o3)
#define DIVW(o1, o2, o3)      EMIT_ASM("divw", o1, o2, o3)
#define DIVUW(o1, o2, o3)     EMIT_ASM("divuw", o1, o2, o3)
#define REM(o1, o2, o3)       EMIT_ASM("rem", o1, o2, o3)
#define REMU(o1, o2, o3)      EMIT_ASM("remu", o1, o2, o3)
#define REMW(o1, o2, o3)      EMIT_ASM("remw", o1, o2, o3)
#define REMUW(o1, o2, o3)     EMIT_ASM("remuw", o1, o2, o3)
#define AND(o1, o2, o3)       EMIT_ASM("and", o1, o2, o3)
#define ANDI(o1, o2, o3)      EMIT_ASM("andi", o1, o2, o3)
#define OR(o1, o2, o3)        EMIT_ASM("or", o1, o2, o3)
#define ORI(o1, o2, o3)       EMIT_ASM("ori", o1, o2, o3)
#define XOR(o1, o2, o3)       EMIT_ASM("xor", o1, o2, o3)
#define XORI(o1, o2, o3)      EMIT_ASM("xori", o1, o2, o3)
#define SLL(o1, o2, o3)       EMIT_ASM("sll", o1, o2, o3)    // Logical left shift
#define SLLI(o1, o2, o3)      EMIT_ASM("slli", o1, o2, o3)   // Logical left shift
#define SLLIW(o1, o2, o3)     EMIT_ASM("slliw", o1, o2, o3)  // Logical left shift, 32bit
#define SRL(o1, o2, o3)       EMIT_ASM("srl", o1, o2, o3)    // Logical right shift
#define SRLI(o1, o2, o3)      EMIT_ASM("srli", o1, o2, o3)   // Logical right shift
#define SRA(o1, o2, o3)       EMIT_ASM("sra", o1, o2, o3)    // Arithmetic right shift
#define SRAI(o1, o2, o3)      EMIT_ASM("srai", o1, o2, o3)   // Arithmetic right shift
#define J(o1)                 EMIT_ASM("j", o1)              // => jal zero, o1
#define JR(o1)                EMIT_ASM("jr", o1)             // => jalr zero, 0(o1)
#define JALR(o1)              EMIT_ASM("jalr", o1)           // => jalr ra, 0(o1)
#define Bcc(c, o1, o2, o3)    EMIT_ASM("b" c, o1, o2, o3)
#define CALL(o1)              EMIT_ASM("call", o1)
#define RET()                 EMIT_ASM("ret")

#define LB(o1, o2)            EMIT_ASM("lb", o1, o2)
#define LH(o1, o2)            EMIT_ASM("lh", o1, o2)
#define LW(o1, o2)            EMIT_ASM("lw", o1, o2)
#define LD(o1, o2)            EMIT_ASM("ld", o1, o2)
#define LBU(o1, o2)           EMIT_ASM("lbu", o1, o2)
#define LHU(o1, o2)           EMIT_ASM("lhu", o1, o2)
#define LWU(o1, o2)           EMIT_ASM("lwu", o1, o2)
#define SB(o1, o2)            EMIT_ASM("sb", o1, o2)
#define SH(o1, o2)            EMIT_ASM("sh", o1, o2)
#define SW(o1, o2)            EMIT_ASM("sw", o1, o2)
#define SD(o1, o2)            EMIT_ASM("sd", o1, o2)

#define MV(o1, o2)            EMIT_ASM("mv", o1, o2)         // => addi o1, o2, 0
#define NEG(o1, o2)           EMIT_ASM("neg", o1, o2)        // => sub o1, zero, o2
#define NOT(o1, o2)           EMIT_ASM("not", o1, o2)        // => xori o1, o2, -1
#define SEXTW(o1, o2)         EMIT_ASM("sext.w", o1, o2)     // => addiw o1, o2, 0

#define SEQZ(o1, o2)          EMIT_ASM("seqz", o1, o2)
#define SNEZ(o1, o2)          EMIT_ASM("snez", o1, o2)
#define SLTZ(o1, o2)          EMIT_ASM("sltz", o1, o2)
#define SGTZ(o1, o2)          EMIT_ASM("sgtz", o1, o2)
#define SLT(o1, o2, o3)       EMIT_ASM("slt", o1, o2, o3)
#define SLTI(o1, o2, o3)      EMIT_ASM("slti", o1, o2, o3)
#define SLTU(o1, o2, o3)      EMIT_ASM("sltu", o1, o2, o3)
#define SLTIU(o1, o2, o3)     EMIT_ASM("sltiu", o1, o2, o3)


#define FMV_D(o1, o2)         EMIT_ASM("fmv.d", o1, o2)  // dst <- src
#define FADD_D(o1, o2, o3)    EMIT_ASM("fadd.d", o1, o2, o3)
#define FADD_S(o1, o2, o3)    EMIT_ASM("fadd.s", o1, o2, o3)
#define FSUB_D(o1, o2, o3)    EMIT_ASM("fsub.d", o1, o2, o3)
#define FSUB_S(o1, o2, o3)    EMIT_ASM("fsub.s", o1, o2, o3)
#define FMUL_D(o1, o2, o3)    EMIT_ASM("fmul.d", o1, o2, o3)
#define FMUL_S(o1, o2, o3)    EMIT_ASM("fmul.s", o1, o2, o3)
#define FDIV_D(o1, o2, o3)    EMIT_ASM("fdiv.d", o1, o2, o3)
#define FDIV_S(o1, o2, o3)    EMIT_ASM("fdiv.s", o1, o2, o3)
#define FLD(o1, o2)           EMIT_ASM("fld", o1, o2)
#define FLW(o1, o2)           EMIT_ASM("flw", o1, o2)
#define FSD(o1, o2)           EMIT_ASM("fsd", o1, o2)
#define FSW(o1, o2)           EMIT_ASM("fsw", o1, o2)

#define FCVT_W_D(o1, o2)      EMIT_ASM("fcvt.w.d", o1, o2, "rtz")  // int <- double
#define FCVT_W_S(o1, o2)      EMIT_ASM("fcvt.w.s", o1, o2, "rtz")  // int <- float
#define FCVT_D_W(o1, o2)      EMIT_ASM("fcvt.d.w", o1, o2)   // double <- int
#define FCVT_D_WU(o1, o2)     EMIT_ASM("fcvt.d.wu", o1, o2)  // double <- unsigned int
#define FCVT_S_W(o1, o2)      EMIT_ASM("fcvt.s.w", o1, o2)   // float <- int
#define FCVT_S_WU(o1, o2)     EMIT_ASM("fcvt.s.wu", o1, o2)  // float <- unsigned int
#define FCVT_D_S(o1, o2)      EMIT_ASM("fcvt.d.s", o1, o2)   // double <- float
#define FCVT_S_D(o1, o2)      EMIT_ASM("fcvt.s.d", o1, o2)   // float <- double

#define FMV_X_W(o1, o2)       EMIT_ASM("fmv.x.w", o1, o2)    // int <- float(hex)
#define FMV_X_D(o1, o2)       EMIT_ASM("fmv.x.d", o1, o2)    // int <- double(hex)

#define FEQ_D(o1, o2, o3)     EMIT_ASM("feq.d", o1, o2, o3)
#define FEQ_S(o1, o2, o3)     EMIT_ASM("feq.s", o1, o2, o3)
#define FLT_D(o1, o2, o3)     EMIT_ASM("flt.d", o1, o2, o3)
#define FLT_S(o1, o2, o3)     EMIT_ASM("flt.s", o1, o2, o3)
#define FLE_D(o1, o2, o3)     EMIT_ASM("fle.d", o1, o2, o3)
#define FLE_S(o1, o2, o3)     EMIT_ASM("fle.s", o1, o2, o3)
