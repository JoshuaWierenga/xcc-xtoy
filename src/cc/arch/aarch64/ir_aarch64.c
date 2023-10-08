#include "../../../config.h"
#include "./arch_config.h"
#include "ir.h"

#include <assert.h>
#include <stdlib.h>  // malloc
#include <string.h>

#include "aarch64.h"
#include "emit_code.h"
#include "regalloc.h"
#include "table.h"
#include "util.h"

static Vector *push_caller_save_regs(unsigned long living);
static void pop_caller_save_regs(Vector *saves);

// Register allocator

// AArch64: Calling Convention
//   X8(XR):              Indirect return value address.
//   X16(IP0), X17(IP1):  Intra-Procedure-call scratch registers.
//   X18(PR):             Platform register. Used for some operating-system-specific special purpose or an additional caller-saved register.
//   X29(FP):             Frame pointer (Callee save)

static const char *kReg32s[PHYSICAL_REG_MAX] = {
  W0, W1, W2, W3, W4, W5, W6, W7, W8, W9, W16,            // Temporary
  W19, W20, W21, W22, W23, W24, W25, W26, W27, W28, W29,  // Callee save
  W10, W11, W12, W13, W14, W15, W18};                     // Caller save
static const char *kReg64s[PHYSICAL_REG_MAX] = {
  X0, X1, X2, X3, X4, X5, X6, X7, X8, X9, X16,            // Temporary
  X19, X20, X21, X22, X23, X24, X25, X26, X27, X28, X29,  // Callee save
  X10, X11, X12, X13, X14, X15, X18};                     // Caller save

#define GET_X0_INDEX()   0
#define GET_X16_INDEX()  10

#define CALLEE_SAVE_REG_COUNT  ((int)(sizeof(kCalleeSaveRegs) / sizeof(*kCalleeSaveRegs)))
static const int kCalleeSaveRegs[] = {11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21};

#define CALLER_SAVE_REG_COUNT  ((int)(sizeof(kCallerSaveRegs) / sizeof(*kCallerSaveRegs)))
static const int kCallerSaveRegs[] = {22, 23, 24, 25, 26, 27, 28};

const int ArchRegParamMapping[] = {0, 1, 2, 3, 4, 5, 6, 7};

const char **kRegSizeTable[] = {kReg32s, kReg32s, kReg32s, kReg64s};
static const char *kZeroRegTable[] = {WZR, WZR, WZR, XZR};

// Break x17 in store, mod and tjmp
static const char *kTmpRegTable[] = {W17, W17, W17, X17};

#define SZ_FLOAT   VRegSize4
#define SZ_DOUBLE  VRegSize8
const char *kFReg32s[PHYSICAL_FREG_MAX] = {
   S0,  S1,  S2,  S3,  S4,  S5,  S6,  S7,
   S8,  S9, S10, S11, S12, S13, S14, S15,
  S16, S17, S18, S19, S20, S21, S22, S23,
  S24, S25, S26, S27, S28, S29, S30, S31,
};
const char *kFReg64s[PHYSICAL_FREG_MAX] = {
   D0,  D1,  D2,  D3,  D4,  D5,  D6,  D7,
   D8,  D9, D10, D11, D12, D13, D14, D15,
  D16, D17, D18, D19, D20, D21, D22, D23,
  D24, D25, D26, D27, D28, D29, D30, D31,
};

#define GET_D0_INDEX()   0

#define CALLEE_SAVE_FREG_COUNT  ((int)(sizeof(kCalleeSaveFRegs) / sizeof(*kCalleeSaveFRegs)))
static const int kCalleeSaveFRegs[] = {8, 9, 10, 11, 12, 13, 14, 15};

#define CALLER_SAVE_FREG_COUNT  ((int)(sizeof(kCallerSaveFRegs) / sizeof(*kCallerSaveFRegs)))
static const int kCallerSaveFRegs[] = {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};

static unsigned long detect_extra_occupied(RegAlloc *ra, IR *ir) {
  unsigned long ioccupy = 0;
  switch (ir->kind) {
  case IR_JMP: case IR_TJMP: case IR_CALL:
    ioccupy = 1UL << GET_X16_INDEX();
    break;
  default: break;
  }
  if (ra->flag & RAF_STACK_FRAME)
    ioccupy |= 1UL << GET_FPREG_INDEX();
  return ioccupy;
}

const RegAllocSettings kArchRegAllocSettings = {
  .detect_extra_occupied = detect_extra_occupied,
  .reg_param_mapping = ArchRegParamMapping,
  .phys_max = PHYSICAL_REG_MAX,
  .phys_temporary_count = PHYSICAL_REG_TEMPORARY,
#ifndef __NO_FLONUM
  .fphys_max = PHYSICAL_FREG_MAX,
  .fphys_temporary_count = PHYSICAL_FREG_TEMPORARY,
#endif
};

//

static bool is_im48(intptr_t x) {
  return x <= ((1L << 47) - 1) && x >= -(1L << 47);
}

void mov_immediate(const char *dst, int64_t value, bool b64, bool is_unsigned) {
  // TODO: Investigate available immediate value range.
  // It looks the architecture can take more than 16-bit immediate.

  if (value == 0) {
    MOV(dst, b64 ? XZR : WZR);
  } else if (is_im16(value)) {
    MOV(dst, IM(value));
  } else {
    int32_t l = !is_unsigned && value < 0
        ? (int16_t)value == 0 ? (int32_t)0xffff0000  // This bit patten can be represented in MOV.
                              : (int32_t)-((uint16_t)(-(int)value))
        : (int32_t)(uint16_t)value;
    MOV(dst, IM(l));
    if (!b64 || is_im32(value)) {
      uint32_t u = value;
      MOVK(dst, IM(u >> 16), _LSL(16));
    } else {
      MOVK(dst, IM((value >> 16) & 0xffff), _LSL(16));
      MOVK(dst, IM((value >> 32) & 0xffff), _LSL(32));
      if (!is_im48(value))
        MOVK(dst, IM((value >> 48) & 0xffff), _LSL(48));
    }
  }
}

static bool is_got(const Name *name) {
#ifdef __APPLE__
  // TODO: How to detect the label is GOT?
  return name->bytes >= 5 && strncmp(name->chars, "__std", 5) == 0;  // __stdinp, etc.
#else
  // TODO: How to detect the label is GOT?
  return name->bytes >= 3 && strncmp(name->chars, "std", 3) == 0;  // stdin, etc.
#endif
}

static void ei_bofs(IR *ir) {
  const char *dst = kReg64s[ir->dst->phys];
  int ofs = ir->bofs.frameinfo->offset;
  if (ofs < 4096 && ofs > -4096) {
    ADD(dst, FP, IM(ofs));
  } else {
    mov_immediate(dst, ofs, true, false);
    ADD(dst, dst, FP);
  }
}

static void ei_iofs(IR *ir) {
  char *label = fmt_name(ir->iofs.label);
  if (ir->iofs.global)
    label = MANGLE(label);
  label = quote_label(label);
  const char *dst = kReg64s[ir->dst->phys];
  if (!is_got(ir->iofs.label)) {
    ADRP(dst, LABEL_AT_PAGE(label));
    ADD(dst, dst, LABEL_AT_PAGEOFF(label));
  } else {
    ADRP(dst, LABEL_AT_GOTPAGE(label));
    LDR(dst, fmt("[%s,#%s]", dst, LABEL_AT_GOTPAGEOFF(label)));
  }
}

static void ei_sofs(IR *ir) {
  assert(ir->opr1->flag & VRF_CONST);
  const char *dst = kReg64s[ir->dst->phys];
  int ofs = ir->opr1->frame.offset;
  if (ofs < 4096 && ofs > -4096) {
    ADD(dst, SP, IM(ir->opr1->fixnum));
  } else {
    mov_immediate(dst, ofs, true, false);
    ADD(dst, dst, SP);
  }
}

#define ei_load_s  ei_load
static void ei_load(IR *ir) {
  assert(!(ir->opr1->flag & VRF_CONST));
  const char *src;
  if (ir->kind == IR_LOAD) {
    assert(!(ir->opr1->flag & VRF_SPILLED));
    src = IMMEDIATE_OFFSET0(kReg64s[ir->opr1->phys]);
  } else {
    assert(ir->opr1->flag & VRF_SPILLED);
    if (ir->opr1->frame.offset >= -256 && ir->opr1->frame.offset <= 256) {
      src = IMMEDIATE_OFFSET(FP, ir->opr1->frame.offset);
    } else {
      const char *tmp = kTmpRegTable[3];
      mov_immediate(tmp, ir->opr1->frame.offset, true, false);
      src = REG_OFFSET(FP, tmp, NULL);
    }
  }

  const char *dst;
  if (ir->dst->flag & VRF_FLONUM) {
    switch (ir->dst->vsize) {
    case SZ_FLOAT:   dst = kFReg32s[ir->dst->phys]; break;
    case SZ_DOUBLE:  dst = kFReg64s[ir->dst->phys]; break;
    default: assert(false); break;
    }
    LDR(dst, src);
  } else {
    int pow = ir->dst->vsize;
    assert(0 <= pow && pow < 4);
    const char **regs = kRegSizeTable[pow];
    dst = regs[ir->dst->phys];
    switch (pow) {
    case 0:
      if (ir->dst->flag & VRF_UNSIGNED) LDRB(dst, src);
      else                              LDRSB(dst, src);
      break;
    case 1:
      if (ir->dst->flag & VRF_UNSIGNED) LDRH(dst, src);
      else                              LDRSH(dst, src);
      break;
    case 2: case 3:
      LDR(dst, src);
      break;
    default: assert(false); break;
    }
  }
}

#define ei_store_s  ei_store
static void ei_store(IR *ir) {
  assert(!(ir->opr2->flag & VRF_CONST));
  int pow = ir->opr1->vsize;
  const char *target;
  if (ir->kind == IR_STORE) {
    assert(!(ir->opr2->flag & VRF_SPILLED));
    target = IMMEDIATE_OFFSET0(kReg64s[ir->opr2->phys]);
  } else {
    assert(ir->opr2->flag & VRF_SPILLED);
    if (ir->opr2->frame.offset >= -256 && ir->opr2->frame.offset <= 256) {
      target = IMMEDIATE_OFFSET(FP, ir->opr2->frame.offset);
    } else {
      const char *tmp = kTmpRegTable[3];
      mov_immediate(tmp, ir->opr2->frame.offset, true, false);
      target = REG_OFFSET(FP, tmp, NULL);
    }
  }
  const char *src;
  if (ir->opr1->flag & VRF_FLONUM) {
    switch (ir->opr1->vsize) {
    default: assert(false); // Fallthrough
    case SZ_FLOAT:   src = kFReg32s[ir->opr1->phys]; break;
    case SZ_DOUBLE:  src = kFReg64s[ir->opr1->phys]; break;
    }
  } else if (ir->opr1->flag & VRF_CONST) {
    if (ir->opr1->fixnum == 0)
      src = kZeroRegTable[pow];
    else
      mov_immediate(src = kTmpRegTable[pow], ir->opr1->fixnum, pow >= 3,
                    ir->opr1->flag & VRF_UNSIGNED);
  } else {
    src = kRegSizeTable[pow][ir->opr1->phys];
  }
  switch (pow) {
  case 0:          STRB(src, target); break;
  case 1:          STRH(src, target); break;
  case 2: case 3:  STR(src, target); break;
  default: assert(false); break;
  }
}

static void ei_add(IR *ir) {
  if (ir->dst->flag & VRF_FLONUM) {
    const char **regs;
    switch (ir->dst->vsize) {
    default: assert(false);  // Fallthrough
    case SZ_FLOAT:   regs = kFReg32s; break;
    case SZ_DOUBLE:  regs = kFReg64s; break;
    }
    FADD(regs[ir->dst->phys], regs[ir->opr1->phys], regs[ir->opr2->phys]);
  } else {
    assert(!(ir->opr1->flag & VRF_CONST));
    int pow = ir->dst->vsize;
    assert(0 <= pow && pow < 4);
    const char **regs = kRegSizeTable[pow];
    if (ir->opr2->flag & VRF_CONST) {
      assert(ir->opr2->fixnum >= 0);
      ADD(regs[ir->dst->phys], regs[ir->opr1->phys], IM(ir->opr2->fixnum));
    } else {
      ADD(regs[ir->dst->phys], regs[ir->opr1->phys], regs[ir->opr2->phys]);
    }
  }
}

static void ei_sub(IR *ir) {
  if (ir->dst->flag & VRF_FLONUM) {
    const char **regs;
    switch (ir->dst->vsize) {
    default: assert(false);  // Fallthrough
    case SZ_FLOAT:   regs = kFReg32s; break;
    case SZ_DOUBLE:  regs = kFReg64s; break;
    }
    FSUB(regs[ir->dst->phys], regs[ir->opr1->phys], regs[ir->opr2->phys]);
  } else {
    assert(!(ir->opr1->flag & VRF_CONST));
    int pow = ir->dst->vsize;
    assert(0 <= pow && pow < 4);
    const char **regs = kRegSizeTable[pow];
    if (ir->opr2->flag & VRF_CONST) {
      assert(ir->opr2->fixnum >= 0);
      SUB(regs[ir->dst->phys], regs[ir->opr1->phys], IM(ir->opr2->fixnum));
    } else {
      SUB(regs[ir->dst->phys], regs[ir->opr1->phys], regs[ir->opr2->phys]);
    }
  }
}

static void ei_mul(IR *ir) {
  if (ir->dst->flag & VRF_FLONUM) {
    const char **regs;
    switch (ir->dst->vsize) {
    default: assert(false);  // Fallthrough
    case SZ_FLOAT:   regs = kFReg32s; break;
    case SZ_DOUBLE:  regs = kFReg64s; break;
    }
    FMUL(regs[ir->dst->phys], regs[ir->opr1->phys], regs[ir->opr2->phys]);
  } else {
    assert(!(ir->opr1->flag & VRF_CONST) && !(ir->opr2->flag & VRF_CONST));
    int pow = ir->dst->vsize;
    assert(0 <= pow && pow < 4);
    const char **regs = kRegSizeTable[pow];
    MUL(regs[ir->dst->phys], regs[ir->opr1->phys], regs[ir->opr2->phys]);
  }
}

static void ei_div(IR *ir) {
  if (ir->dst->flag & VRF_FLONUM) {
    const char **regs;
    switch (ir->dst->vsize) {
    default: assert(false);  // Fallthrough
    case SZ_FLOAT:   regs = kFReg32s; break;
    case SZ_DOUBLE:  regs = kFReg64s; break;
    }
    FDIV(regs[ir->dst->phys], regs[ir->opr1->phys], regs[ir->opr2->phys]);
  } else {
    assert(!(ir->opr1->flag & VRF_CONST) && !(ir->opr2->flag & VRF_CONST));
    int pow = ir->dst->vsize;
    assert(0 <= pow && pow < 4);
    const char **regs = kRegSizeTable[pow];
    if (!(ir->dst->flag & VRF_UNSIGNED))
      SDIV(regs[ir->dst->phys], regs[ir->opr1->phys], regs[ir->opr2->phys]);
    else
      UDIV(regs[ir->dst->phys], regs[ir->opr1->phys], regs[ir->opr2->phys]);
  }
}

static void ei_mod(IR *ir) {
  assert(!(ir->opr1->flag & VRF_CONST) && !(ir->opr2->flag & VRF_CONST));
  int pow = ir->dst->vsize;
  assert(0 <= pow && pow < 4);
  const char **regs = kRegSizeTable[pow];
  const char *num = regs[ir->opr1->phys];
  const char *div = regs[ir->opr2->phys];
  const char *tmp = kTmpRegTable[pow];
  if (!(ir->dst->flag & VRF_UNSIGNED))
    SDIV(tmp, num, div);
  else
    UDIV(tmp, num, div);
  const char *dst = regs[ir->dst->phys];
  MSUB(dst, tmp, div, num);
}

static void ei_bitand(IR *ir) {
  assert(!(ir->opr1->flag & VRF_CONST) && !(ir->opr2->flag & VRF_CONST));
  int pow = ir->dst->vsize;
  assert(0 <= pow && pow < 4);
  const char **regs = kRegSizeTable[pow];
  AND(regs[ir->dst->phys], regs[ir->opr1->phys], regs[ir->opr2->phys]);
}

static void ei_bitor(IR *ir) {
  assert(!(ir->opr1->flag & VRF_CONST) && !(ir->opr2->flag & VRF_CONST));
  int pow = ir->dst->vsize;
  assert(0 <= pow && pow < 4);
  const char **regs = kRegSizeTable[pow];
  ORR(regs[ir->dst->phys], regs[ir->opr1->phys], regs[ir->opr2->phys]);
}

static void ei_bitxor(IR *ir) {
  assert(!(ir->opr1->flag & VRF_CONST) && !(ir->opr2->flag & VRF_CONST));
  int pow = ir->dst->vsize;
  assert(0 <= pow && pow < 4);
  const char **regs = kRegSizeTable[pow];
  EOR(regs[ir->dst->phys], regs[ir->opr1->phys], regs[ir->opr2->phys]);
}

static void ei_lshift(IR *ir) {
  assert(!(ir->opr1->flag & VRF_CONST));
  int pow = ir->dst->vsize;
  assert(0 <= pow && pow < 4);
  const char **regs = kRegSizeTable[pow];
  if (ir->opr2->flag & VRF_CONST)
    LSL(regs[ir->dst->phys], regs[ir->opr1->phys], IM(ir->opr2->fixnum));
  else
    LSL(regs[ir->dst->phys], regs[ir->opr1->phys], regs[ir->opr2->phys]);
}

static void ei_rshift(IR *ir) {
  assert(!(ir->opr1->flag & VRF_CONST));
  int pow = ir->dst->vsize;
  assert(0 <= pow && pow < 4);
  const char **regs = kRegSizeTable[pow];
  if (ir->opr2->flag & VRF_CONST)
    ASR(regs[ir->dst->phys], regs[ir->opr1->phys], IM(ir->opr2->fixnum));
  else
    ASR(regs[ir->dst->phys], regs[ir->opr1->phys], regs[ir->opr2->phys]);
}

static void ei_result(IR *ir) {
  if (ir->opr1->flag & VRF_FLONUM) {
    int dstphys = ir->dst != NULL ? ir->dst->phys : GET_D0_INDEX();
    if (ir->opr1->phys != dstphys) {  // Source is not return register.
      const char **regs;
      switch (ir->opr1->vsize) {
      default: assert(false);  // Fallthroguh
      case SZ_FLOAT:  regs = kFReg32s; break;
      case SZ_DOUBLE: regs = kFReg64s; break;
      }
      FMOV(regs[dstphys], regs[ir->opr1->phys]);
    }
  } else {
    int pow = ir->opr1->vsize;
    assert(0 <= pow && pow < 4);
    int dstphys = ir->dst != NULL ? ir->dst->phys : GET_X0_INDEX();
    const char *dst = kRegSizeTable[pow][dstphys];
    if (ir->opr1->flag & VRF_CONST) {
      mov_immediate(dst, ir->opr1->fixnum, pow >= 3, ir->opr1->flag & VRF_UNSIGNED);
    } else if (ir->opr1->phys != dstphys) {  // Source is not return register.
      MOV(dst, kRegSizeTable[pow][ir->opr1->phys]);
    }
  }
}

static void ei_subsp(IR *ir) {
  if (ir->opr1->flag & VRF_CONST) {
    assert(ir->opr1->fixnum % 16 == 0);
    if (ir->opr1->fixnum > 0)
      SUB(SP, SP, IM(ir->opr1->fixnum));
    else if (ir->opr1->fixnum < 0)
      ADD(SP, SP, IM(-ir->opr1->fixnum));
  } else {
    SUB(SP, SP, kReg64s[ir->opr1->phys]);
  }
  if (ir->dst != NULL)
    MOV(kReg64s[ir->dst->phys], SP);
}

static void ei_mov(IR *ir) {
  if (ir->dst->flag & VRF_FLONUM) {
    if (ir->opr1->phys != ir->dst->phys) {
      const char *src, *dst;
      switch (ir->dst->vsize) {
      default: assert(false); // Fallthrough
      case SZ_FLOAT:   dst = kFReg32s[ir->dst->phys]; src = kFReg32s[ir->opr1->phys]; break;
      case SZ_DOUBLE:  dst = kFReg64s[ir->dst->phys]; src = kFReg64s[ir->opr1->phys]; break;
      }
      FMOV(dst, src);
    }
  } else {
    assert(!(ir->dst->flag & VRF_CONST));
    int pow = ir->dst->vsize;
    assert(0 <= pow && pow < 4);
    const char **regs = kRegSizeTable[pow];
    if (ir->opr1->flag & VRF_CONST) {
      mov_immediate(regs[ir->dst->phys], ir->opr1->fixnum, pow >= 3, ir->opr1->flag & VRF_UNSIGNED);
    } else {
      if (ir->opr1->phys != ir->dst->phys)
        MOV(regs[ir->dst->phys], regs[ir->opr1->phys]);
    }
  }
}

static void ei_cmp(IR *ir) {
  if (ir->opr1->flag & VRF_FLONUM) {
    assert(ir->opr2->flag & VRF_FLONUM);
    const char *opr1, *opr2;
    switch (ir->opr1->vsize) {
    default: assert(false); // Fallthrough
    case SZ_FLOAT:   opr1 = kFReg32s[ir->opr1->phys]; opr2 = kFReg32s[ir->opr2->phys]; break;
    case SZ_DOUBLE:  opr1 = kFReg64s[ir->opr1->phys]; opr2 = kFReg64s[ir->opr2->phys]; break;
    }
    FCMP(opr1, opr2);
  } else {
    assert(!(ir->opr1->flag & VRF_CONST));
    int pow = ir->opr1->vsize;
    assert(0 <= pow && pow < 4);
    const char **regs = kRegSizeTable[pow];
    if (ir->opr2->flag & VRF_CONST) {
      if (ir->opr2->fixnum == 0)
        CMP(regs[ir->opr1->phys], kZeroRegTable[pow]);
      else if (ir->opr2->fixnum > 0)
        CMP(regs[ir->opr1->phys], IM(ir->opr2->fixnum));
      else
        CMN(regs[ir->opr1->phys], IM(-ir->opr2->fixnum));
    } else {
      CMP(regs[ir->opr1->phys], regs[ir->opr2->phys]);
    }
  }
}

static void ei_neg(IR *ir) {
  assert(!(ir->dst->flag & VRF_CONST));
  int pow = ir->dst->vsize;
  assert(0 <= pow && pow < 4);
  const char **regs = kRegSizeTable[pow];
  SUB(regs[ir->dst->phys], kZeroRegTable[pow], regs[ir->opr1->phys]);
}

static void ei_bitnot(IR *ir) {
  assert(!(ir->dst->flag & VRF_CONST));
  int pow = ir->dst->vsize;
  assert(0 <= pow && pow < 4);
  const char **regs = kRegSizeTable[pow];
  EON(regs[ir->dst->phys], regs[ir->opr1->phys], kZeroRegTable[pow]);
}

static void ei_cond(IR *ir) {
  assert(!(ir->dst->flag & VRF_CONST));
  const char *dst = kReg32s[ir->dst->phys];  // Assume bool is 4 byte.
  // On aarch64, flag for comparing flonum is signed.
  switch (ir->cond.kind & (COND_MASK | COND_UNSIGNED)) {
  case COND_EQ | COND_UNSIGNED:  // Fallthrough
  case COND_EQ:  CSET(dst, CEQ); break;

  case COND_NE | COND_UNSIGNED:  // Fallthrough
  case COND_NE:  CSET(dst, CNE); break;

  case COND_LT:  CSET(dst, CLT); break;
  case COND_GT:  CSET(dst, CGT); break;
  case COND_LE:  CSET(dst, CLE); break;
  case COND_GE:  CSET(dst, CGE); break;

  case COND_LT | COND_UNSIGNED:  CSET(dst, CLO); break;
  case COND_GT | COND_UNSIGNED:  CSET(dst, CHI); break;
  case COND_LE | COND_UNSIGNED:  CSET(dst, CLS); break;
  case COND_GE | COND_UNSIGNED:  CSET(dst, CHS); break;
  default: assert(false); break;
  }
}

static void ei_jmp(IR *ir) {
  const char *label = fmt_name(ir->jmp.bb->label);
  // On aarch64, flag for comparing flonum is signed.
  switch (ir->jmp.cond & (COND_MASK | COND_UNSIGNED)) {
  case COND_ANY: BRANCH(label); break;

  case COND_EQ | COND_UNSIGNED:  // Fallthrough
  case COND_EQ:  Bcc(CEQ, label); break;

  case COND_NE | COND_UNSIGNED:  // Fallthrough
  case COND_NE:  Bcc(CNE, label); break;

  case COND_LT:  Bcc(CLT, label); break;
  case COND_GT:  Bcc(CGT, label); break;
  case COND_LE:  Bcc(CLE, label); break;
  case COND_GE:  Bcc(CGE, label); break;

  case COND_LT | COND_UNSIGNED:  Bcc(CLO, label); break;
  case COND_GT | COND_UNSIGNED:  Bcc(CHI, label); break;
  case COND_LE | COND_UNSIGNED:  Bcc(CLS, label); break;
  case COND_GE | COND_UNSIGNED:  Bcc(CHS, label); break;
  default: assert(false); break;
  }
}

static void ei_tjmp(IR *ir) {
  int phys = ir->opr1->phys;
  const int powd = 3;
  int pows = ir->opr1->vsize;
  assert(0 <= pows && pows < 4);

  const char *dst = kTmpRegTable[3];
  const Name *table_label = alloc_label();
  char *label = fmt_name(table_label);
  ADRP(dst, LABEL_AT_PAGE(label));
  ADD(dst, dst, LABEL_AT_PAGEOFF(label));
  // dst = label + (opr1 << 3)
  LDR(dst, REG_OFFSET(dst, kRegSizeTable[pows][phys], pows < powd ? _UXTW(3) : _LSL(3)));
  BR(dst);

  _RODATA();
  EMIT_ALIGN(8);
  EMIT_LABEL(fmt_name(table_label));
  for (size_t i = 0, len = ir->tjmp.len; i < len; ++i) {
    BB *bb = ir->tjmp.bbs[i];
    _QUAD(fmt("%.*s", NAMES(bb->label)));
  }
  _TEXT();
}

static void ei_precall(IR *ir) {
  // Living registers are not modified between preparing function arguments,
  // so safely saved before calculating argument values.
  ir->precall.caller_saves = push_caller_save_regs(ir->precall.living_pregs);

  int align_stack = (16 - (ir->precall.stack_args_size)) & 15;
  ir->precall.stack_aligned = align_stack;

  if (align_stack > 0) {
    SUB(SP, SP, IM(align_stack));
  }
}

static void ei_pusharg(IR *ir) {
  assert(!(ir->opr1->flag & VRF_CONST));
  int pow = ir->opr1->vsize;
  if (ir->opr1->flag & VRF_FLONUM) {
    // Assume parameter registers are arranged from index 0.
    if (ir->pusharg.index != ir->opr1->phys) {
      switch (ir->opr1->vsize) {
      case SZ_FLOAT:  FMOV(kFReg32s[ir->pusharg.index], kFReg32s[ir->opr1->phys]); break;
      case SZ_DOUBLE:  FMOV(kFReg64s[ir->pusharg.index], kFReg64s[ir->opr1->phys]); break;
      default: assert(false); break;
      }
    }
  } else {
    // Assume parameter registers are arranged from index 0.
    if (ir->pusharg.index != ir->opr1->phys)
      MOV(kRegSizeTable[pow][ir->pusharg.index], kRegSizeTable[pow][ir->opr1->phys]);
  }
}

static void ei_call(IR *ir) {
  if (ir->call.label != NULL) {
    char *label = fmt_name(ir->call.label);
    if (ir->call.global)
      label = MANGLE(label);
    BL(quote_label(label));
  } else {
    assert(!(ir->opr1->flag & VRF_CONST));
    BLR(kReg64s[ir->opr1->phys]);
  }

  IR *precall = ir->call.precall;
  int align_stack = precall->precall.stack_aligned + precall->precall.stack_args_size;
  if (align_stack != 0) {
    ADD(SP, SP, IM(align_stack));
  }

  // Resore caller save registers.
  pop_caller_save_regs(precall->precall.caller_saves);

  if (ir->dst != NULL) {
    if (ir->dst->flag & VRF_FLONUM) {
      if (ir->dst->phys != GET_D0_INDEX()) {
        const char *src, *dst;
        switch (ir->dst->vsize) {
        default: assert(false);  // Fallthrough
        case SZ_FLOAT:   src = S0; dst = kFReg32s[ir->dst->phys]; break;
        case SZ_DOUBLE:  src = D0; dst = kFReg64s[ir->dst->phys]; break;
        }
        FMOV(dst, src);
      }
    } else {
      if (ir->dst->phys != GET_X0_INDEX()) {
        int pow = ir->dst->vsize;
        assert(0 <= pow && pow < 4);
        const char **regs = kRegSizeTable[pow];
        MOV(regs[ir->dst->phys], kRegSizeTable[pow][GET_X0_INDEX()]);
      }
    }
  }
}

static void ei_cast(IR *ir) {
  assert((ir->opr1->flag & VRF_CONST) == 0);
  if (ir->dst->flag & VRF_FLONUM) {
    if (ir->opr1->flag & VRF_FLONUM) {
      // flonum->flonum
      // Assume flonum are just two types.
      const char *src, *dst;
      switch (ir->dst->vsize) {
      default: assert(false); // Fallthrough
      case SZ_FLOAT:   dst = kFReg32s[ir->dst->phys]; src = kFReg64s[ir->opr1->phys]; break;
      case SZ_DOUBLE:  dst = kFReg64s[ir->dst->phys]; src = kFReg32s[ir->opr1->phys]; break;
      }
      FCVT(dst, src);
    } else {
      // fix->flonum
      int pows = ir->opr1->vsize;
      assert(0 <= pows && pows < 4);

      const char *dst;
      switch (ir->dst->vsize) {
      case SZ_FLOAT:   dst = kFReg32s[ir->dst->phys]; break;
      case SZ_DOUBLE:  dst = kFReg64s[ir->dst->phys]; break;
      default: assert(false); break;
      }
      const char *src = kRegSizeTable[pows][ir->opr1->phys];
      if (ir->opr1->flag & VRF_UNSIGNED)  UCVTF(dst, src);
      else                                SCVTF(dst, src);
    }
  } else if (ir->opr1->flag & VRF_FLONUM) {
    // flonum->fix
    int powd = ir->dst->vsize;
    switch (ir->opr1->vsize) {
    case SZ_FLOAT:   FCVTZS(kRegSizeTable[powd][ir->dst->phys], kFReg32s[ir->opr1->phys]); break;
    case SZ_DOUBLE:  FCVTZS(kRegSizeTable[powd][ir->dst->phys], kFReg64s[ir->opr1->phys]); break;
    default: assert(false); break;
    }
  } else {
    // fix->fix
    if (ir->dst->vsize <= ir->opr1->vsize) {
      if (ir->dst->phys != ir->opr1->phys) {
        int pow = ir->dst->vsize;
        assert(0 <= pow && pow < 4);
        const char **regs = kRegSizeTable[pow];
        MOV(regs[ir->dst->phys], regs[ir->opr1->phys]);
      }
    } else {
      int pows = ir->opr1->vsize;
      int powd = ir->dst->vsize;
      assert(0 <= pows && pows < 4);
      assert(0 <= powd && powd < 4);
      if (ir->opr1->flag & VRF_UNSIGNED) {
        switch (pows) {
        case 0:  UXTB(kRegSizeTable[powd][ir->dst->phys], kRegSizeTable[pows][ir->opr1->phys]); break;
        case 1:  UXTH(kRegSizeTable[powd][ir->dst->phys], kRegSizeTable[pows][ir->opr1->phys]); break;
        case 2:  UXTW(kRegSizeTable[powd][ir->dst->phys], kRegSizeTable[pows][ir->opr1->phys]); break;
        default: assert(false); break;
        }
      } else {
        switch (pows) {
        case 0:  SXTB(kRegSizeTable[powd][ir->dst->phys], kRegSizeTable[pows][ir->opr1->phys]); break;
        case 1:  SXTH(kRegSizeTable[powd][ir->dst->phys], kRegSizeTable[pows][ir->opr1->phys]); break;
        case 2:  SXTW(kRegSizeTable[powd][ir->dst->phys], kRegSizeTable[pows][ir->opr1->phys]); break;
        default: assert(false); break;
        }
      }
    }
  }
}

static void ei_asm(IR *ir) {
  EMIT_ASM(ir->asm_.str);
  if (ir->dst != NULL) {
    assert(!(ir->dst->flag & VRF_CONST));
    int pow = ir->dst->vsize;
    assert(0 <= pow && pow < 4);
    const char **regs = kRegSizeTable[pow];
    MOV(regs[ir->dst->phys], regs[GET_X0_INDEX()]);
  }
}

//

static int enum_callee_save_regs(unsigned long bit, int n, const int *indices, const char **regs,
                                 const char *saves[CALLEE_SAVE_REG_COUNT]) {
  int count = 0;
  for (int i = 0; i < n; ++i) {
    int ireg = indices[i];
    if (bit & (1 << ireg))
      saves[count++] = regs[ireg];
  }
  return count;
}

#define N  (CALLEE_SAVE_REG_COUNT > CALLEE_SAVE_FREG_COUNT ? CALLEE_SAVE_REG_COUNT \
                                                           : CALLEE_SAVE_FREG_COUNT)

int push_callee_save_regs(unsigned long used, unsigned long fused) {
  const char *saves[(N + 1) & ~1];
  int count = enum_callee_save_regs(used, CALLEE_SAVE_REG_COUNT, kCalleeSaveRegs, kReg64s, saves);
  for (int i = 0; i < count; i += 2) {
    if (i + 1 < count)
      STP(saves[i], saves[i + 1], PRE_INDEX(SP, -16));
    else
      STR(saves[i], PRE_INDEX(SP, -16));
  }
  int fcount = enum_callee_save_regs(fused, CALLEE_SAVE_FREG_COUNT, kCalleeSaveFRegs, kFReg64s,
                                     saves);
  for (int i = 0; i < fcount; i += 2) {
    if (i + 1 < fcount)
      STP(saves[i], saves[i + 1], PRE_INDEX(SP, -16));
    else
      STR(saves[i], PRE_INDEX(SP, -16));
  }
  return count + fcount;
}

void pop_callee_save_regs(unsigned long used, unsigned long fused) {
  const char *saves[(N + 1) & ~1];
  int fcount = enum_callee_save_regs(fused, CALLEE_SAVE_FREG_COUNT, kCalleeSaveFRegs, kFReg64s,
                                     saves);
  if ((fcount & 1) != 0)
    LDR(saves[--fcount], POST_INDEX(SP, 16));
  for (int i = fcount; i > 0; ) {
    i -= 2;
    LDP(saves[i], saves[i + 1], POST_INDEX(SP, 16));
  }
  int count = enum_callee_save_regs(used, CALLEE_SAVE_REG_COUNT, kCalleeSaveRegs, kReg64s, saves);
  if ((count & 1) != 0)
    LDR(saves[--count], POST_INDEX(SP, 16));
  for (int i = count; i > 0; ) {
    i -= 2;
    LDP(saves[i], saves[i + 1], POST_INDEX(SP, 16));
  }
}
#undef N

static Vector *push_caller_save_regs(unsigned long living) {
  Vector *saves = new_vector();

  for (int i = 0; i < CALLER_SAVE_REG_COUNT; ++i) {
    int ireg = kCallerSaveRegs[i];
    if (living & (1UL << ireg)) {
      vec_push(saves, kReg64s[ireg]);
    }
  }

  {
    for (int i = 0; i < CALLER_SAVE_FREG_COUNT; ++i) {
      int ireg = kCallerSaveFRegs[i];
      if (living & (1UL << (ireg + PHYSICAL_REG_MAX))) {
        // TODO: Detect register size.
        vec_push(saves, kFReg64s[ireg]);
      }
    }
  }

  int n = saves->len;
  for (int i = 1; i < n; i += 2) {
    STP(saves->data[i - 1], saves->data[i], PRE_INDEX(SP, -16));
  }
  if ((n & 1) != 0) {
    STR(saves->data[n - 1], PRE_INDEX(SP, -16));
  }

  return saves;
}

static void pop_caller_save_regs(Vector *saves) {
  int i = saves->len;
  if ((i & 1) != 0) {
    LDR(saves->data[i - 1], POST_INDEX(SP, 16));
    --i;
  }
  for (; i > 0; i -= 2) {
    LDP(saves->data[i - 2], saves->data[i - 1], POST_INDEX(SP, 16));
  }
}

void emit_bb_irs(BBContainer *bbcon) {
  typedef void (*EmitIrFunc)(IR *);
  static const EmitIrFunc table[] = {
    [IR_BOFS] = ei_bofs, [IR_IOFS] = ei_iofs, [IR_SOFS] = ei_sofs,
    [IR_LOAD] = ei_load, [IR_LOAD_S] = ei_load_s, [IR_STORE] = ei_store, [IR_STORE_S] = ei_store_s,
    [IR_ADD] = ei_add, [IR_SUB] = ei_sub, [IR_MUL] = ei_mul, [IR_DIV] = ei_div,
    [IR_MOD] = ei_mod, [IR_BITAND] = ei_bitand, [IR_BITOR] = ei_bitor,
    [IR_BITXOR] = ei_bitxor, [IR_LSHIFT] = ei_lshift, [IR_RSHIFT] = ei_rshift,
    [IR_CMP] = ei_cmp, [IR_NEG] = ei_neg, [IR_BITNOT] = ei_bitnot,
    [IR_COND] = ei_cond, [IR_JMP] = ei_jmp, [IR_TJMP] = ei_tjmp,
    [IR_PRECALL] = ei_precall, [IR_PUSHARG] = ei_pusharg, [IR_CALL] = ei_call,
    [IR_RESULT] = ei_result, [IR_SUBSP] = ei_subsp, [IR_CAST] = ei_cast,
    [IR_MOV] = ei_mov, [IR_ASM] = ei_asm,
  };

  for (int i = 0; i < bbcon->bbs->len; ++i) {
    BB *bb = bbcon->bbs->data[i];
#ifndef NDEBUG
    // Check BB connection.
    if (i < bbcon->bbs->len - 1) {
      BB *nbb = bbcon->bbs->data[i + 1];
      UNUSED(nbb);
      assert(bb->next == nbb);
    } else {
      assert(bb->next == NULL);
    }
#endif

    EMIT_LABEL(fmt_name(bb->label));
    for (int j = 0; j < bb->irs->len; ++j) {
      IR *ir = bb->irs->data[j];
      assert(ir->kind < (int)(sizeof(table) / sizeof(*table)));
      assert(table[ir->kind] != NULL);
      (*table[ir->kind])(ir);
    }
  }
}

//

static void swap_opr12(IR *ir) {
  VReg *tmp = ir->opr1;
  ir->opr1 = ir->opr2;
  ir->opr2 = tmp;
}

static void insert_const_mov(VReg **pvreg, RegAlloc *ra, Vector *irs, int i) {
  VReg *c = *pvreg;
  VReg *tmp = reg_alloc_spawn(ra, c->vsize, 0);
  IR *mov = new_ir_mov(tmp, c);
  vec_insert(irs, i, mov);
  *pvreg = tmp;
}

void tweak_irs(FuncBackend *fnbe) {
  BBContainer *bbcon = fnbe->bbcon;
  RegAlloc *ra = fnbe->ra;
  for (int i = 0; i < bbcon->bbs->len; ++i) {
    BB *bb = bbcon->bbs->data[i];
    Vector *irs = bb->irs;
    for (int j = 0; j < irs->len; ++j) {
      IR *ir = irs->data[j];
      switch (ir->kind) {
      case IR_ADD:
        assert(!(ir->opr1->flag & VRF_CONST) || !(ir->opr2->flag & VRF_CONST));
        if (ir->opr1->flag & VRF_CONST)
          swap_opr12(ir);
        if (ir->opr2->flag & VRF_CONST) {
          if (ir->opr2->fixnum < 0) {
            ir->kind = IR_SUB;
            ir->opr2->fixnum = -ir->opr2->fixnum;
          }
          if (ir->opr2->fixnum > 0x0fff)
            insert_const_mov(&ir->opr2, ra, irs, j++);
        }
        break;
      case IR_SUB:
        assert(!(ir->opr1->flag & VRF_CONST) || !(ir->opr2->flag & VRF_CONST));
        if (ir->opr1->flag & VRF_CONST) {
          if (ir->opr1->fixnum == 0) {
            ir->kind = IR_NEG;
            ir->opr1 = ir->opr2;
            ir->opr2 = NULL;
            break;
          }
          insert_const_mov(&ir->opr1, ra, irs, j++);
        }
        if (ir->opr2->flag & VRF_CONST) {
          if (ir->opr2->fixnum < 0) {
            ir->kind = IR_ADD;
            ir->opr2->fixnum = -ir->opr2->fixnum;
          }
          if (ir->opr2->fixnum > 0x0fff)
            insert_const_mov(&ir->opr2, ra, irs, j++);
        }
        break;
      case IR_MUL:
      case IR_DIV:
      case IR_MOD:
      case IR_BITAND:
      case IR_BITOR:
      case IR_BITXOR:
        assert(!(ir->opr1->flag & VRF_CONST) || !(ir->opr2->flag & VRF_CONST));
        if (ir->opr1->flag & VRF_CONST)
          insert_const_mov(&ir->opr1, ra, irs, j++);
        if (ir->opr2->flag & VRF_CONST)
          insert_const_mov(&ir->opr2, ra, irs, j++);
        break;
      case IR_LSHIFT:
      case IR_RSHIFT:
        assert(!(ir->opr1->flag & VRF_CONST) || !(ir->opr2->flag & VRF_CONST));
        if (ir->opr1->flag & VRF_CONST)
          insert_const_mov(&ir->opr1, ra, irs, j++);
        break;
      case IR_CMP:
        if ((ir->opr2->flag & VRF_CONST) &&
            (ir->opr2->fixnum > 0x0fff || ir->opr2->fixnum < -0x0fff))
          insert_const_mov(&ir->opr2, ra, irs, j++);
        break;
      case IR_PUSHARG:
        if (ir->opr1->flag & VRF_CONST)
          insert_const_mov(&ir->opr1, ra, irs, j++);
        break;

      default: break;
      }
    }
  }
}
