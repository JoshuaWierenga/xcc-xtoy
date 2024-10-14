#pragma once

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


#define ADD(r1, r2, r3) EMIT_ASM("add", r1, r2, r3) // r1 = r2 + r3
#define SUB(r1, r2, r3) EMIT_ASM("sub", r1, r2, r3) // r1 = r2 - r3
#define ASL(r1, r2, r3) EMIT_ASM("asl", r1, r2, r3) // r1 = r2 << r3
#define LDA(r, a)       EMIT_ASM("lda", r, a)       // r = a
#define STR(r, a)       EMIT_ASM("str", r, a)       // mem[a] = r
#define LDI(r1, r2)     EMIT_ASM("ldi", r1, r2)     // r1 = mem[r2]
#define STI(r1, r2)     EMIT_ASM("sti", r1, r2)     // mem[r2] = r1
#define HLT()           EMIT_ASM("hlt")             // stop CPU
#define JMP(r)          EMIT_ASM("jmp", r)          // PC = r

// r2 = PTR_SIZE, RF = RF - r2, mem[RF] = r1
#define START_PUSH(r1, r2) \
  LDA(r2, im(PTR_SIZE));   \
  CONTINUE_PUSH(r1, r2)

// RF = RF - r2, mem[RF] = r1
#define CONTINUE_PUSH(r1, r2) \
  SUB(RF, RF, r2);            \
  STI(r1, RF)

// r2 = PTR_SIZE, r1 = mem[RF], RF = RF + r2
#define START_POP(r1, r2) \
  LDA(r2, im(PTR_SIZE));  \
  CONTINUE_POP(r1, r2)

// r1 = mem[RF], RF = RF + r2
#define CONTINUE_POP(r1, r2) \
  LDI(r1, RF);               \
  ADD(RF, RF, r2)

#define MOV(r1, r2) ADD(r1, r2, R0) // r1 = r2 + R0(0) i.e. r1 = r2

#define RET() JMP(R8) // PC = R8
