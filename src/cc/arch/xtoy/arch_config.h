#pragma once

#include "../../../config.h"

// Configuration for xtoy

#define MAX_REG_ARGS  (6)
#define MAX_FREG_ARGS (0)

#define PHYSICAL_REG_TEMPORARY   (8)
#define PHYSICAL_REG_MAX         (PHYSICAL_REG_TEMPORARY + 6)

#define GET_FPREG_INDEX() 13

// Byte is 16 bit, word/ptr is 16 bit, 16/16 = 1
#define MAX_ALIGN TARGET_POINTER_SIZE
