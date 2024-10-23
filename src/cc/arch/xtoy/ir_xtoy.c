#include "../../../config.h"
#include "./arch_config.h"

#include <assert.h>   // assert
#include <inttypes.h> // PRIu16, PRIX16, INT16_MAX, UINT16_MAX
#include <stdbool.h>
#include <stdlib.h>   // free

#include "ast.h"
#include "emit_code.h"
#include "regalloc.h"
#include "util.h"
#include "xasprintf.h"
#include "xtoy.h"

// Register allocator

// X-Toy: Registers
//   R0:    Zero register
//   R1-R5: Integer argument registers (Caller saved), R1 is also the integer return register
//   R7-R8: Temporary registers, R8 is also the return address
//   R9-RD: Callee saved
//   RE:    Frame pointer (Callee saved)
//   RF:    Stack pointer (Globally saved)

static Vector *push_caller_save_regs(unsigned long living);
static void pop_caller_save_regs(Vector *saves);

const char *kReg16s[PHYSICAL_REG_MAX] = {
  R1, R2, R3, R4, R5,     // Caller saved
  R6, R7, R8,             // Temporary
  R9, RA, RB, RC, RD, RE  // Callee saved
};

#define INT_RET_REG_INDEX 0

#define CALLEE_SAVE_REG_COUNT  ((int)ARRAY_SIZE(kCalleeSaveRegs))
static const int kCalleeSaveRegs[] = {8, 9, 10, 11, 12, 13};

#define CALLER_SAVE_REG_COUNT  ((int)ARRAY_SIZE(kCallerSaveRegs))
static const int kCallerSaveRegs[] = {0, 1, 2, 3, 4};

const int ArchRegParamMapping[] = {0, 1, 2, 3, 4};

static unsigned long detect_extra_occupied(RegAlloc *ra, IR *ir) {
  UNUSED(ir);
  unsigned long ioccupy = 0;
  if (ra->flag & RAF_STACK_FRAME) {
    ioccupy |= 1UL << GET_FPREG_INDEX();
  }
  return ioccupy;
}

const RegAllocSettings kArchRegAllocSettings = {
  .detect_extra_occupied = detect_extra_occupied,
  .reg_param_mapping = ArchRegParamMapping,
  .phys_max = PHYSICAL_REG_MAX,
  .phys_temporary_count = PHYSICAL_REG_TEMPORARY
};

//

static uint16_t labelCount = 0;

static void load_val(const char *dst, int32_t val) {
  // Visual X-Toy only supports signed integers
  assert(val <= INT16_MAX);
  if ((uint16_t)val <= UINT8_MAX) {
      LDA(dst, im((uint8_t)val));
  } else {
    /* TODO: 2 words with 1 label may be possible:
      .SECTION data
        ...
      label1:
        EMIT((uint16_t)val);
        ...
      .SECTION text
        ...
        LOD(dst, label1);
    */
    const char *label1 = fmt("%s%" PRIu16, "label_", labelCount);
    ++labelCount;
    const char *label2 = fmt("%s%" PRIu16, "label_", labelCount);
    ++labelCount;
    // For some reason PRIX16 is "X" on glibc, casting to uint16_t
    // appears to give the correct output, PRId16 would have caused
    // more issues(-001 vs 65535 as output for hd and d respectively).
    const char *im_val = fmt("%04" PRIX16, (uint16_t)val);

    BRZ(R0, label2);
    EMIT_LABEL(label1);
    EMIT_ASM(".WORD", im_val);
    EMIT_LABEL(label2);
    LOD(dst, label1);
  }
}

//

// TODO: Skip all moves where dst and src as the same

static void ei_bofs(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not supported", __func__));
}

static void ei_iofs(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not supported", __func__));
}

static void ei_sofs(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not supported", __func__));
}

static void ei_load(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not supported", __func__));
}

// TODO: Merge with ei_load?
static void ei_load_s(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not supported", __func__));
}

static void ei_store(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not supported", __func__));
}

// TODO: Merge with ei_store?
static void ei_store_s(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not supported", __func__));
}

static void ei_add(IR *ir) {
  assert(!(ir->opr1->flag & VRF_CONST));
  int pow = ir->dst->vsize;
  assert(0 <= pow && pow < 1);
  const char *dst = kReg16s[ir->dst->phys];
  const char *src1 = kReg16s[ir->opr1->phys];
  if (ir->opr2->flag & VRF_CONST) {
    int val = ir->opr2->fixnum;
    if (0 == val) {
      MOV(dst, src1);
    } else {
      load_val(TMP_1_REG, val);
      ADD(dst, src1, TMP_1_REG);
    }
  } else {
    ADD(dst, src1, kReg16s[ir->opr2->phys]);
  }
}

static void ei_sub(IR *ir) {
  assert(!(ir->opr1->flag & VRF_CONST));
  int pow = ir->dst->vsize;
  assert(0 <= pow && pow < 1);
  const char *dst = kReg16s[ir->dst->phys];
  const char *src1 = kReg16s[ir->opr1->phys];
  if (ir->opr2->flag & VRF_CONST) {
    int val = ir->opr2->fixnum;
    if (0 == val) {
      MOV(dst, src1);
    } else {
      load_val(TMP_1_REG, val);
      SUB(dst, src1, TMP_1_REG);
    }
  } else {
    SUB(dst, src1, kReg16s[ir->opr2->phys]);
  }
}

static void ei_mul(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not supported", __func__));
}

static void ei_div(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not supported", __func__));
}

static void ei_mod(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not supported", __func__));
}

static void ei_bitand(IR *ir) {
  assert(!(ir->opr1->flag & VRF_CONST));
  int pow = ir->dst->vsize;
  assert(0 <= pow && pow < 1);
  const char *dst = kReg16s[ir->dst->phys];
  const char *src1 = kReg16s[ir->opr1->phys];
  if (ir->opr2->flag & VRF_CONST) {
    int val = ir->opr2->fixnum;
    if (0 == val) {
      MOV(dst, R0);
    } else {
      load_val(TMP_1_REG, val);
      AND(dst, src1, TMP_1_REG);
    }
  } else {
    AND(dst, src1, kReg16s[ir->opr2->phys]);
  }
}

static void ei_bitor(IR *ir) {
  assert(!(ir->opr1->flag & VRF_CONST));
  int pow = ir->dst->vsize;
  assert(0 <= pow && pow < 1);
  const char *dst = kReg16s[ir->dst->phys];
  const char *src1 = kReg16s[ir->opr1->phys];
  // Visual X-Toy does not support bitwise or
  // However A or B = (A xor B) xor (A and B)
  if (ir->opr2->flag & VRF_CONST) {
    int val = ir->opr2->fixnum;
    if (0 == val) {
      MOV(dst, src1);
    } else {
      load_val(TMP_2_REG, val);
      XOR(TMP_1_REG, src1, TMP_2_REG);
      AND(dst, src1, TMP_2_REG);
      XOR(dst, TMP_1_REG, dst);
    }
  } else {
    const char *src2 = kReg16s[ir->opr2->phys];
    XOR(TMP_1_REG, src1, src2);
    AND(dst, src1, src2);
    XOR(dst, TMP_1_REG, dst);
  }
}

static void ei_bitxor(IR *ir) {
  assert(!(ir->opr1->flag & VRF_CONST));
  int pow = ir->dst->vsize;
  assert(0 <= pow && pow < 1);
  const char *dst = kReg16s[ir->dst->phys];
  const char *src1 = kReg16s[ir->opr1->phys];
  if (ir->opr2->flag & VRF_CONST) {
    int val = ir->opr2->fixnum;
    if (0 == val) {
      MOV(dst, src1);
    } else {
      load_val(TMP_1_REG, val);
      XOR(dst, src1, TMP_1_REG);
    }
  } else {
    XOR(dst, src1, kReg16s[ir->opr2->phys]);
  }
}

static void ei_lshift(IR *ir) {
  assert(!(ir->opr1->flag & VRF_CONST));
  int pow = ir->dst->vsize;
  assert(0 <= pow && pow < 1);
  const char *dst = kReg16s[ir->dst->phys];
  const char *src1 = kReg16s[ir->opr1->phys];
  if (ir->opr2->flag & VRF_CONST) {
    int val = ir->opr2->fixnum;
    assert(0 <= val && 0xF >= val);
    if (0 == val) {
      MOV(dst, src1);
    } else {
      load_val(TMP_1_REG, val);
      ASL(dst, src1, TMP_1_REG);
    }
  } else {
    ASL(dst, src1, kReg16s[ir->opr2->phys]);
  }
}

static void ei_rshift(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not supported", __func__));
}

static void ei_neg(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not supported", __func__));
}

static void ei_bitnot(IR *ir) {
  int pow = ir->dst->vsize;
  assert(0 <= pow && pow < 1);
  const char *dst = kReg16s[ir->dst->phys];
  if (ir->opr1->flag & VRF_CONST) {
    int val = ir->opr1->fixnum;
    load_val(dst, (int16_t)UINT16_MAX);
    if (0 == val) {
      // Do nothing
    } else {
      load_val(TMP_1_REG, val);
      XOR(dst, TMP_1_REG, dst);
    }
  } else {
    load_val(TMP_1_REG, (int16_t)UINT16_MAX);
    XOR(dst, kReg16s[ir->opr1->phys], TMP_1_REG);
  }
}

static void ei_cond(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not supported", __func__));
}

static void ei_jmp(IR *ir) {
  const char *label_true = fmt_name(ir->jmp.bb->label);
  int cond = ir->jmp.cond;
  if (cond == COND_ANY) {
    BRZ(R0, label_true);
    return;
  }

  assert(!(ir->opr1->flag & VRF_CONST));
  const char *opr1 = kReg16s[ir->opr1->phys];

  const char *opr2;
  if (ir->opr2->flag & VRF_CONST) {
    if (ir->opr2->fixnum == 0) {
      opr2 = R0;
    } else {
      // TODO: Optimise by flipping LHS(X >= Y => Y <= X)
      // With opr1 as a constant we can remove most of the cases
      load_val(TMP_2_REG, ir->opr2->fixnum);
      opr2 = TMP_2_REG;
    }
  } else {
    opr2 = kReg16s[ir->opr2->phys];
  }

  const char *label_false = xasprintf("%s%" PRIu16, "label_", labelCount);
  ++labelCount;

  switch (cond) {
    case COND_EQ | COND_UNSIGNED:
    case COND_EQ:
      error("JMP EQ is not supported");
      break;

    case COND_NE | COND_UNSIGNED:
    case COND_NE:
      error("JMP NE is not supported");
    break;

    case COND_LT:
      error("JMP LT is not supported");
      break;
    case COND_GT:
      error("JMP GT is not supported");
      break;
    case COND_LE:
      error("JMP LE is not supported");
      break;
    case COND_GE: {
        // Intro
        const char *label_1gtz = xasprintf("%s%" PRIu16, "label_", labelCount);
        ++labelCount;
        const char *label_1ez = xasprintf("%s%" PRIu16, "label_", labelCount);
        ++labelCount;
        const char *label_1ltz = xasprintf("%s%" PRIu16, "label_", labelCount);
        ++labelCount;

        BRP(opr1, label_1gtz);
        BRZ(opr1, label_1ez);
        BRZ(R0, label_1ltz);

        // opr1 > 0 cases:
        const char *label_1gtz_2gtz = xasprintf("%s%" PRIu16, "label_", labelCount);
        ++labelCount;

        EMIT_LABEL(label_1gtz);
        BRP(opr2, label_1gtz_2gtz);
        BRZ(R0, label_true);
        EMIT_LABEL(label_1gtz_2gtz);
        load_val(TMP_1_REG, 1);
        SUB(TMP_1_REG, TMP_1_REG, opr2);
        ADD(TMP_1_REG, TMP_1_REG, opr1);
        BRP(TMP_1_REG, label_true);
        BRZ(R0, label_false);

        // opr1 == 0 cases:
        EMIT_LABEL(label_1ez);
        BRP(opr2, label_false);
        BRZ(R0, label_true);

        // opr1 < 0 cases:
        EMIT_LABEL(label_1ltz);
        BRP(opr2, label_false);
        BRZ(opr2, label_false);
        SUB(TMP_1_REG, opr1, opr2);
        BRZ(TMP_1_REG, label_true);
        BRP(TMP_1_REG, label_true);

        free((char *)label_1gtz);
        free((char *)label_1ez);
        free((char *)label_1ltz);
        free((char *)label_1gtz_2gtz);
      }
      break;

    case COND_LT | COND_UNSIGNED:
      error("JMP Unsigned LT is not supported");
      break;
    case COND_GT | COND_UNSIGNED:
      error("JMP Unsigned GT is not supported");
      break;
    case COND_LE | COND_UNSIGNED:
      error("JMP Unsigned LE is not supported");
      break;
    case COND_GE | COND_UNSIGNED:
      error("JMP Unsigned GE is not supported");
      break;

    default:
      assert(false);
      break;
  }

  EMIT_LABEL(label_false);

  free((char *)label_false);
}

static void ei_tjmp(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not supported", __func__));
}

static void ei_precall(IR *ir) {
  // Living registers are not modified between preparing function arguments,
  // so safely saved before calculating argument values.
  ir->precall.caller_saves = push_caller_save_regs(ir->precall.living_pregs);
  ir->precall.stack_aligned = (MAX_ALIGN - (ir->precall.stack_args_size)) & (MAX_ALIGN - 1);
  // TODO: Is the SP sub from other targets required here?
}

static void ei_pusharg(IR *ir) {
  int pow = ir->opr1->vsize;
  assert(0 <= pow && pow < 1);
  const char *dst = kReg16s[ir->pusharg.index + ArchRegParamMapping[0]];
  if (ir->opr1->flag & VRF_CONST) {
    load_val(dst, ir->opr1->fixnum);
  }
  else if (ir->pusharg.index != ir->opr1->phys) {
    MOV(dst, kReg16s[ir->opr1->phys]);
  }
}

static void ei_call(IR *ir) {
  if (ir->call.label != NULL) {
    char *label = format_func_name(ir->call.label, ir->call.global);
    JSR(RET_ADDRESS_REG, label);
  } else {
    assert(!(ir->opr1->flag & VRF_CONST));
    error("Jumping to register address is not supported");
  }

  // TODO: Is the SP add from other targets required here?

  // Restore caller save registers.
  pop_caller_save_regs(ir->call.precall->precall.caller_saves);

  if (ir->dst != NULL && ir->dst->phys != INT_RET_REG_INDEX) {
    MOV(kReg16s[ir->dst->phys], kReg16s[INT_RET_REG_INDEX]);
  }
}

static void ei_result(IR *ir) {
  int pow = ir->opr1->vsize;
  assert(0 <= pow && pow < 1);
  int dstphys = ir->dst != NULL ? ir->dst->phys : INT_RET_REG_INDEX;
  const char *dst = kReg16s[dstphys];
  if (ir->opr1->flag & VRF_CONST) {
    load_val(dst, ir->opr1->fixnum);
  } else if (ir->opr1->phys != dstphys) {
    MOV(dst, kReg16s[ir->opr1->phys]);
  }
}

static void ei_subsp(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not supported", __func__));
}

static void ei_cast(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not supported", __func__));
}

static void ei_mov(IR *ir) {
  assert(!(ir->dst->flag & VRF_CONST));
  int pow = ir->dst->vsize;
  assert(0 <= pow && pow < 1);
  const char *dst = kReg16s[ir->dst->phys];
  if (ir->opr1->flag & VRF_CONST) {
    load_val(dst, ir->opr1->fixnum);
  } else if (ir->opr1->phys != ir->dst->phys) {
    MOV(dst, kReg16s[ir->opr1->phys]);
  }
}

static void ei_keep(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not supported", __func__));
}

static void ei_asm(IR *ir) {
  EMIT_ASM(ir->asm_.str);
  if (ir->dst != NULL) {
    assert(!(ir->dst->flag & VRF_CONST));
    int pow = ir->dst->vsize;
    assert(0 <= pow && pow < 1);
    if (INT_RET_REG_INDEX != ir->dst->phys) {
      MOV(kReg16s[ir->dst->phys], kReg16s[INT_RET_REG_INDEX]);
    }
  }
}

//

static int count_callee_save_regs(unsigned long used) {
  int count = 0;
  for (int i = 0; i < CALLEE_SAVE_REG_COUNT; ++i) {
    int ireg = kCalleeSaveRegs[i];
    if (used & (1 << ireg)) {
      ++count;
    }
  }
  return count;
}

int push_callee_save_regs(unsigned long used, unsigned long fused) {
  UNUSED(fused);
  int count = 0;
  for (int i = 0; i < CALLEE_SAVE_REG_COUNT; ++i) {
    int ireg = kCalleeSaveRegs[i];
    if (used & (1UL << ireg)) {
      const char *reg = kReg16s[ireg];
      if (count == 0) {
        START_PUSH(reg, TMP_1_REG);
      } else {
        CONTINUE_PUSH(reg, TMP_1_REG);
      }
      ++count;
    }
  }
  return count;
}

void pop_callee_save_regs(unsigned long used, unsigned long fused) {
  UNUSED(fused);
  bool no_moves = true;
  for (int i = CALLEE_SAVE_REG_COUNT; --i >= 0;) {
    int ireg = kCalleeSaveRegs[i];
    if (used & (1UL << ireg)) {
      const char *reg = kReg16s[ireg];
      if (no_moves) {
        START_POP(reg, TMP_1_REG);
        no_moves = false;
      } else {
        CONTINUE_POP(reg, TMP_1_REG);
      }
    }
  }
}

int calculate_func_param_bottom(Function *func) {
  FuncBackend *fnbe = func->extra;
  unsigned long used = fnbe->ra->used_reg_bits;
  int callee_save_count = count_callee_save_regs(used);

  return (callee_save_count * TARGET_POINTER_SIZE) + (TARGET_POINTER_SIZE * 2);  // Return address, saved base pointer.
}

static Vector *push_caller_save_regs(unsigned long living) {
  Vector *saves = new_vector();

  for (int i = 0; i < CALLER_SAVE_REG_COUNT; ++i) {
    int ireg = kCallerSaveRegs[i];
    if (living & (1UL << ireg)) {
      const char *reg = kReg16s[ireg];
      if (saves->len == 0) {
        START_PUSH(reg, TMP_1_REG);
      } else {
        CONTINUE_PUSH(reg, TMP_1_REG);
      }
      vec_push(saves, reg);
    }
  }

  return saves;
}

static void pop_caller_save_regs(Vector *saves) {
  bool no_moves = true;
  if (saves->len <= 0) {
    return;
  }
  for (int n = saves->len, i = n; i-- > 0; ) {
    const char *reg = saves->data[i];
    if (no_moves) {
      START_PUSH(reg, TMP_1_REG);
      no_moves = false;
    } else {
      CONTINUE_PUSH(reg, TMP_1_REG);
    }
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
    [IR_NEG] = ei_neg, [IR_BITNOT] = ei_bitnot,
    [IR_COND] = ei_cond, [IR_JMP] = ei_jmp, [IR_TJMP] = ei_tjmp,
    [IR_PRECALL] = ei_precall, [IR_PUSHARG] = ei_pusharg, [IR_CALL] = ei_call,
    [IR_RESULT] = ei_result, [IR_SUBSP] = ei_subsp, [IR_CAST] = ei_cast,
    [IR_MOV] = ei_mov, [IR_KEEP] = ei_keep, [IR_ASM] = ei_asm,
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

    if (is_fall_path_only(bbcon, i)) {
      emit_comment(NULL);
    } else {
      // TODO: Check what this does
      EMIT_LABEL(fmt_name(bb->label));
    }
    for (int j = 0; j < bb->irs->len; ++j) {
      IR *ir = bb->irs->data[j];
      assert(ir->kind < (int)ARRAY_SIZE(table));
      assert(table[ir->kind] != NULL);
      (*table[ir->kind])(ir);
    }
  }
}

void tweak_irs(FuncBackend *fnbe) {
  UNUSED(fnbe);
  // TODO: Add tweaks as required
}
