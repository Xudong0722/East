/*
 * @Author: Xudong0722 
 * @Date: 2025-03-24 18:05:54 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-24 21:35:07
 */

#pragma once

#include <assert.h>
#include <string>
#include "util.h"

#define EAST_ASSERT(x)                                                   \
  if (!(x)) {                                                            \
    ELOG_ERROR(ELOG_ROOT()) << "EAST ASSERT: " #x << "\nBacktrace:\n" \
                            << East::BacktraceToStr(100, 2, "    ");     \
    assert(x);                                                           \
  }
#define EAST_ASSERT2(x, ex_info)                                     \
  if (!(x)) {                                                        \
    ELOG_ERROR(ELOG_ROOT()) << "EAST ASSERT: " #x << "\n"            \
                            << ex_info << "\nBacktrace:\n"           \
                            << East::BacktraceToStr(100, 2, "    "); \
    assert(x);                                                       \
  }
  