/**
 * Copyright (c) 2021 OceanBase
 * OceanBase CE is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */

#include "common/ob_smart_call.h"

namespace oceanbase
{
namespace common
{


#if defined(__x86_64__)
OB_NOINLINE int jump_call(void * arg_, int(*func_) (void*), void* stack_addr)
{
  int ret = 0;
  __asm__ __volatile__ (
    "leaq  -0x10(%3), %3\n\t"                      /* reserve space for old RSP on new stack, 0x10 for align */
    "movq  %%rsp, (%3)\n\t"                        /* store RSP in new stack */
    "movq  %3, %%rsp\n\t"                          /* jump to new stack */
    "call *%2\n\t"                                 /* run the second arg func_ */
    "popq  %%rsp\n\t"                              /* jump back to old stack */
    :"=a" (ret)                                    /* specify rax assigned to ret */
    :"D"(arg_), "r"(func_), "r"(stack_addr)        /* specify rdi assigned to arg_ */
    :"memory"
  );
  return ret;
}
#elif defined(__aarch64__)
OB_NOINLINE int jump_call(void * arg_, int(*func_) (void*), void* stack_addr)
{
  register int64_t ret __asm__("x0") = 0;          /* specify x0 assigned to ret */
  register void* x0 __asm__("x0") = arg_;          /* specify x0 assigned to arg_ */
  __asm__ __volatile__ (
    "sub  %3, %3, #0x20\n\t"                       /* reserve space for old sp on new stack, 0x10 for align */
    "stp  x29, x30, [%3, #0x10]\n\t"               /* x29, x30 may not be stored in the innest function. */
    "mov  x9, sp\n\t"                              /* transit SP by x9 */
    "str  x9, [%3, #0x00]\n\t"                     /* store SP in new stack */
    "mov  sp, %3\n\t"                              /* jump SP to new stack */
    "blr  %2\n\t"                                  /* run the second arg func_ */
    "ldp  x29, x30, [sp, #0x10]\n\t"               /* restore x29, x30 */
    "ldr  x9, [sp, #0x00]\n\t"                     /* jump back to old stack */
    "mov  sp, x9\n\t"                              /* transit SP by x9 */
    :"=r" (ret)                                    /* output */
    :"r"(x0), "r"(func_), "r"(stack_addr)          /* input*/
    :"x9", "memory"                                /* specify x9 is used */
  );
  return (int) ret;
}
#elif defined(__powerpc64__)
OB_NOINLINE int jump_call(void * arg_, int(*func_) (void*), void* stack_addr)
{
  register int64_t ret __asm__ ("r3") = 0;
  register void* r3 __asm__ ("r3") = arg_;
  __asm__ __volatile__ (
    "mflr  %%r0\n"                                  /* move from link register to r0, store old lr */
    "std   %%r31, -0x8(%%r1)\n"                     /* store old stack pointer to r31 */
    "std   %%r0, 0x10(%%r1)\n"                      /* store old lr on stack */
    "stdu   %%r1, -0xa0(%%r1)\n"                    /* reserve space for old SP on stack */
    "std    %%r2, 24(%%r1)\n"                       /* store old toc on stack */
    "subi  %3, %3, 160\n"                           /* reserve space for old sp on new stack */
    "clrrdi %3, %3, 4\n\t"                          /* clear low 4 bits for address alignment */
    "std   %%r1, 0x0(%3)\n"                         /* store input stack on r1 */
    "mr    %%r1, %3\n"                              /* transit SP to new stack pointer */
    "mr    %%r3, %1\n"                              /* transit first argument of arg_ to r3 register */
    "mtctr %2\n"                                    /* put func_ to SPR count register */
    "mr    %%r12, %2\n"                             /* transit func_ value to exception and glink register */
    "bctrl\n\t"                                     /* branch to func_ and pc to link register lr*/
    "mr    %0, %%r3\n"                              /* transit return value from r3 */
    "ld    %%r31, 0x0(%%r1)\n"                      /* restore old SP from stored r31 */
    "mr    %%r1, %%r31\n"                           /* transit SP from stored r31 */
    "ld    %%r2, 24(%%r1)\n"                        /* restore old toc pointer*/
    "addi   %%r1, %%r1, 0xa0\n"                     /* restore  old sp from old stack */
    "ld    %%r0, 0x10(%%r1)\n"                      /* restore lr content to r0 */
    "ld    %%r31, -0x8(%%r1)\n"                     /* restore r31 from stack */
    "mtlr  %%r0\n"                                  /* move to link register from r0, restore old lr */
    :"=&b" (ret)                                    /* output */
    :"b"(arg_), "b"(func_), "b"(stack_addr)         /* input */
    :"r31", "memory"                                /* specify r31 is used */
   );
  return (int) ret;
}

#endif

_RLOCAL(int64_t, all_stack_size);

} // namespace common
} // namespace oceanbase
