#ifndef BUILD_CONFIG_H
#define BUILD_CONFIG_H

#define DEBUG_LEVEL @DEBUG_LEVEL@
#define DEBUG_TRACE 3
#define DEBUG_ERROR 0

#define ENABLE_TESTS @ENABLE_TESTS@

#if DEBUG_LEVEL > DEBUG_TRACE
#include "ecall.h"
#endif

#endif
