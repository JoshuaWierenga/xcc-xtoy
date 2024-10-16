#pragma once

#include "../../../config.h"

#include "emit_util.h"

#define R0 "r0" // Zero Register
#define R1 "r1"
#define R2 "r2"
#define R3 "r3"
#define R4 "r4"
#define R5 "r5"
#define R6 "r6"
#define R7 "r7"
#define R8 "r8" // Return Address
#define R9 "r9"
#define RA "rA"
#define RB "rB"
#define RC "rC"
#define RD "rD"
#define RE "rE" // Frame Pointer
#define RF "rF" // Stack Pointer

#define TMP_REG         R7
#define RET_ADDRESS_REG R8
#define FRAME_PTR_REG   RE
#define STACK_PTR_REG   RF


#define ADD(r1, r2, r3) EMIT_ASM("add", r1, r2, r3) // r1 = r2 + r3
#define SUB(r1, r2, r3) EMIT_ASM("sub", r1, r2, r3) // r1 = r2 - r3
#define ASL(r1, r2, r3) EMIT_ASM("asl", r1, r2, r3) // r1 = r2 << r3
#define LDA(r, a)       EMIT_ASM("lda", r, a)       // r = a
#define LOD(r, a)       EMIT_ASM("lod", r, a)       // r = mem[a]
#define STR(r, a)       EMIT_ASM("str", r, a)       // mem[a] = r
#define LDI(r1, r2)     EMIT_ASM("ldi", r1, r2)     // r1 = mem[r2]
#define STI(r1, r2)     EMIT_ASM("sti", r1, r2)     // mem[r2] = r1
#define HLT()           EMIT_ASM("hlt")             // stop CPU
#define BRZ(r, a)       EMIT_ASM("brz", r, a)       // if (r == 0): PC = a
#define JMP(r)          EMIT_ASM("jmp", r)          // PC = r
#define JSR(r, a)       EMIT_ASM("jsr", r, a)       // r = PC, PC = a

// r2 = PTR_SIZE, STACK_PTR_REG = STACK_PTR_REG - r2, mem[STACK_PTR_REG] = r1
#define START_PUSH(r1, r2)          \
  LDA(r2, im(TARGET_POINTER_SIZE)); \
  CONTINUE_PUSH(r1, r2)

// STACK_PTR_REG = STACK_PTR_REG - r2, mem[STACK_PTR_REG] = r1
#define CONTINUE_PUSH(r1, r2)            \
  SUB(STACK_PTR_REG, STACK_PTR_REG, r2); \
  STI(r1, STACK_PTR_REG)

// r2 = PTR_SIZE, r1 = mem[STACK_PTR_REG], STACK_PTR_REG = STACK_PTR_REG + r2
#define START_POP(r1, r2)           \
  LDA(r2, im(TARGET_POINTER_SIZE)); \
  CONTINUE_POP(r1, r2)

// r1 = mem[STACK_PTR_REG], STACK_PTR_REG = STACK_PTR_REG + r2
#define CONTINUE_POP(r1, r2) \
  LDI(r1, STACK_PTR_REG);    \
  ADD(STACK_PTR_REG, STACK_PTR_REG, r2)

#define MOV(r1, r2) ADD(r1, r2, R0) // r1 = r2 + R0(0) i.e. r1 = r2

#define RET() JMP(RET_ADDRESS_REG) // PC = RET_ADDRESS_REG
