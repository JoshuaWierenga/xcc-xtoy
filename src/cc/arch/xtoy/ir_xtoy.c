#include "../../../config.h"
#include "./arch_config.h"

#include <assert.h>
#include <stdbool.h>

#include "ast.h"
#include "emit_code.h"
#include "regalloc.h"
#include "util.h"
#include "xtoy.h"

// Register allocator

// X-Toy: Registers
//   R0:    Zero register
//   R1-R8: Integer argument registers (Caller saved), R1 is also the integer return register
//   R7-R8: Temporary registers, R8 is also the return address
//   R9-RD: Callee saved
//   RE:    Frame pointer (Callee saved)
//   RF:    Stack pointer (Globally saved)

const char *kReg16s[PHYSICAL_REG_MAX] = {
  R1, R2, R3, R4, R5, R6, // Caller saved
  R7, R8,                 // Temporary
  R9, RA, RB, RC, RD, RE  // Callee saved
};

#define GET_R1_INDEX() 0

#define CALLEE_SAVE_REG_COUNT  ((int)ARRAY_SIZE(kCalleeSaveRegs))
static const int kCalleeSaveRegs[] = {8, 9, 10, 11, 12, 13};

const int ArchRegParamMapping[] = {0, 1, 2, 3, 4, 5};

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

static void ei_bofs(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not support", __func__));
}

static void ei_iofs(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not support", __func__));
}

static void ei_sofs(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not support", __func__));
}

static void ei_load(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not support", __func__));
}

// TODO: Merge with ei_load
static void ei_load_s(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not support", __func__));
}

static void ei_store(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not support", __func__));
}

// TODO: Merge with ei_store
static void ei_store_s(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not support", __func__));
}

static void ei_add(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not support", __func__));
}

static void ei_sub(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not support", __func__));
}

static void ei_mul(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not support", __func__));
}

static void ei_div(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not support", __func__));
}

static void ei_mod(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not support", __func__));
}

static void ei_bitand(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not support", __func__));
}

static void ei_bitor(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not support", __func__));
}

static void ei_bitxor(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not support", __func__));
}

static void ei_lshift(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not support", __func__));
}

static void ei_rshift(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not support", __func__));
}

static void ei_neg(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not support", __func__));
}

static void ei_bitnot(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not support", __func__));
}

static void ei_cond(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not support", __func__));
}

static void ei_jmp(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not support", __func__));
}

static void ei_tjmp(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not support", __func__));
}

static void ei_precall(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not support", __func__));
}

static void ei_pusharg(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not support", __func__));
}

static void ei_call(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not support", __func__));
}

static void ei_result(IR *ir) {
  int pow = ir->opr1->vsize;
  assert(0 <= pow && pow < 4);
  int dstphys = ir->dst != NULL ? ir->dst->phys : GET_R1_INDEX();
  const char *dst = kReg16s[dstphys];
  if (ir->opr1->flag & VRF_CONST) {
    int16_t val = ir->opr1->fixnum;
    if ((uint16_t)val <= UINT8_MAX) {
      LDA(dst, im((uint8_t)val));
    } else {
      // TODO: Test this
      /* TODO: 3 words with 2 labels is possible:
          JMP(label2);
        label1:
          EMIT((uint16_t)val);
        label2:
          LOD(dst, label1);
      */
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
      uint8_t upper = val >> 8;
      uint8_t lower = val & 0xFF;
      LDA(dst, im(upper));
      LDA(R7, im(8));
      ASL(R7, dst, R7);
      LDA(dst, im(lower));
      ADD(dst, dst, R7);
    }
  } else if (ir->opr1->phys != dstphys) {
    MOV(dst, kReg16s[ir->opr1->phys]);
  }
}

static void ei_subsp(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not support", __func__));
}

static void ei_cast(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not support", __func__));
}

static void ei_mov(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not support", __func__));
}

static void ei_keep(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not support", __func__));
}

static void ei_asm(IR *ir) {
  UNUSED(ir);
  error(fmt("function %s is not support", __func__));
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
    if (used & (1 << ireg)) {
      if (count == 0) {
        START_PUSH(kReg16s[ireg], R7);
      } else {
        CONTINUE_PUSH(kReg16s[ireg], R7);
      }
      ++count;
    }
  }
  return count;
}

void pop_callee_save_regs(unsigned long used, unsigned long fused) {
  UNUSED(fused);
  bool noMoves = true;
  for (int i = CALLEE_SAVE_REG_COUNT; --i >= 0;) {
    int ireg = kCalleeSaveRegs[i];
    if (used & (1 << ireg)) {
      if (noMoves) {
        START_POP(kReg16s[ireg], R7);
        noMoves = false;
      } else {
        CONTINUE_POP(kReg16s[ireg], R7);
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