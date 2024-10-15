#include "./arch_config.h"
#include "emit_code.h"

#include <assert.h>    // assert
#include <inttypes.h>  // PRIX8
#include <stdbool.h>
#include <string.h>    // strcmp

#include "ast.h"
#include "codegen.h"
#include "emit_util.h"
#include "regalloc.h"
#include "type.h"
#include "util.h"
#include "var.h"
#include "xtoy.h"

static bool no_emits = true;

//

const char *im(uint8_t x) {
  return fmt("%02" PRIX8, x);
}

////////////////////////////////////////////////

static bool is_asm(Stmt *stmt) {
  return stmt->kind == ST_ASM;
}

static void move_params_to_assigned(Function *func) {
  extern const char *kReg16s[];
  extern const int ArchRegParamMapping[];

  bool no_noves = true;
  RegParamInfo iparams[MAX_REG_ARGS];
  int iparam_count = 0;
  int fparam_count = 0;
  enumerate_register_params(func, iparams, MAX_REG_ARGS, NULL, MAX_FREG_ARGS,
                            &iparam_count, &fparam_count);

  // Generate code to store parameters to the destination.
  for (int i = 0; i < iparam_count; ++i) {
    RegParamInfo *p = &iparams[i];
    VReg *vreg = p->vreg;
    size_t size = type_size(p->type);
    int pow = most_significant_bit(size);
    assert(IS_POWER_OF_2(size) && pow < 4);
    const char *src = kReg16s[ArchRegParamMapping[p->index]];
    if (vreg->flag & VRF_SPILLED) {
      if (no_noves) {
        START_PUSH(src, TMP_REG);
        no_noves = false;
      } else {
        CONTINUE_PUSH(src, TMP_REG);
      }
    } else if (ArchRegParamMapping[p->index] != vreg->phys) {
      const char *dst = kReg16s[vreg->phys];
      MOV(dst, src);
    }
  }
}

void emit_defun(Function *func) {
  if (func->scopes == NULL ||  // Prototype definition.
      func->extra == NULL)     // Code emission is omitted.
    return;

  if (no_emits) {
    BRZ(R0, "$main");
    no_emits = false;
  }

  emit_comment(NULL);

  bool global = true;
  const VarInfo *varinfo = scope_find(global_scope, func->name, NULL);
  if (varinfo != NULL) {
    global = (varinfo->storage & VS_STATIC) == 0;
  }

  bool is_main = false;
  {
    char *label = format_func_name(func->name, global);
    is_main = strcmp("main", label) == 0;
    EMIT_LABEL(label);
  }

  bool no_stmt = true;
  if (func->body_block != NULL) {
    Vector *stmts = func->body_block->block.stmts;
    for (int i = 0; i < stmts->len; ++i) {
      Stmt *stmt = stmts->data[i];
      if (stmt == NULL)
        continue;
      if (!is_asm(stmt)) {
        no_stmt = false;
        break;
      }
    }
  }

  // Prologue
  // Allocate variable bufer.
  FuncBackend *fnbe = func->extra;
  size_t frame_size = ALIGN(fnbe->frame_size, MAX_ALIGN);
  bool fp_saved = false;  // Frame pointer saved?
  bool ra_saved = false;  // Return Address register saved?
  unsigned long used_reg_bits = fnbe->ra->used_reg_bits;
  int vaarg_params_saved = 0;
  if (!no_stmt) {
    // TODO: Only emit this if the stack is needed
    // func->flag & (FUNCF_HAS_FUNCALL | FUNCF_STACK_MODIFIED)?
    // fp_saved?
    if (is_main) {
      LDA(FRAME_PTR_REG, im(0xFF));
      LDA(STACK_PTR_REG, im(0xFF));
    }

    if (func->type->func.vaargs) {
      error("var arg functions are not supported");
    }

    fp_saved = frame_size > 0 || fnbe->ra->flag & RAF_STACK_FRAME;
    ra_saved = !is_main && (func->flag & FUNCF_HAS_FUNCALL) != 0;

    if (ra_saved) {
      START_PUSH(RET_ADDRESS_REG, TMP_REG);
    }

    if (fp_saved) {
      if (ra_saved) {
        CONTINUE_PUSH(FRAME_PTR_REG, TMP_REG);
      } else {
        START_PUSH(FRAME_PTR_REG, TMP_REG);
      }

      // FP is saved, so omit from callee save.
      used_reg_bits &= ~(1UL << GET_FPREG_INDEX());
    }

    // Callee save.
    push_callee_save_regs(used_reg_bits, fnbe->ra->used_freg_bits);

    if (fp_saved) {
      MOV(FRAME_PTR_REG, STACK_PTR_REG);
    }

    move_params_to_assigned(func);
  }

  emit_bb_irs(fnbe->bbcon);

  if (!function_not_returned(fnbe)) {
    // Epilogue
    if (!no_stmt) {
      if (fp_saved) {
        MOV(STACK_PTR_REG, FRAME_PTR_REG);
      }

      pop_callee_save_regs(used_reg_bits, fnbe->ra->used_freg_bits);

      if (fp_saved) {
        START_POP(FRAME_PTR_REG, TMP_REG);
      }

      if (ra_saved) {
        if (fp_saved) {
          CONTINUE_POP(RET_ADDRESS_REG, TMP_REG);
        } else {
          START_POP(RET_ADDRESS_REG, TMP_REG);
        }
      }
    }
    if (vaarg_params_saved > 0) {
      error("var arg functions are not supported");
    }

    if (is_main) {
      STR(R1, im(0xFF));
      HLT();
    } else {
      RET();
    }
  }
}
