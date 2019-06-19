// Copyright 2019 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "cyield.h"

#include <assert.h>

namespace {

void run(CYieldClientAPI* self, std::function<void(CYieldClientAPI*)> function,
    void* stack, std::jmp_buf return_env) {
  // Can not store anything in this existing stack because it will get broken on
  // Yield().
  // This inline assembly modifies the stack pointer to point the newly created
  // stack, and store the passed |return_env| in the new stack.
#ifdef __x86_64
  __asm__ volatile (
      "mov %0, %%rsp;"
      "push %1;"
      "push %1;"  // Redundant code to adjust 8B stack alignment for macOS.
  :: "r"(stack), "r"(return_env));
#else
#error
#endif

  function(self);

  // Restores |return_env| just in case becuase it should be stored in a caller
  // save register, or in the original stack that may be already disposed here.
#ifdef __x86_64
  __asm__ volatile (
      "pop %0;"
  : "=r"(return_env));
#else
#error
#endif

  // Never return, but jumps to the lastly saved environent at Run() or
  // Continue().
  longjmp(return_env, 2);
}

}  // namespace

CYield::CYield(size_t stack_size)
  : stack_size(stack_size),
    stack_pointer(static_cast<uint8_t*>(malloc(stack_size))) {}

CYield::~CYield() {
  free(stack_pointer);
}

bool CYield::Run(std::function<void(CYieldClientAPI*)> function) {
  // Save the call site environment.
  switch (setjmp(call_env[env])) {
   case 0:
    // The setjmp call will come here to run |function| on another stack.
    // run() never return, but jumps the point to return from setjmp() with 2.
    run(this, function, &stack_pointer[stack_size], call_env[env]);
    assert(false);  // not reached
    return false;
   case 1:
    // If the |function| calls Yield() first, it reaches here.
    return true;
   case 2:
    // If the |function| finishes without calling Yield(), it reaches here.
    return false;
  }
  assert(false);  // not reached
  return false;
}

bool CYield::Continue() {
  int now = env;
  env = (env + 1) & 1;
  // Save the call site environment.
  switch (setjmp(call_env[env])) {
   case 0:
    // The setjmp call will come here to continue the yielded |function| call.
    // This never returns as well.
    // If Continue() is called from Yield(), it saves running |function|
    // environment, and come to here to go back to the Run() or Continue() call
    // site. Folloing longjmp() returns from setjmp() with 1.
    longjmp(call_env[now], 1);
    assert(false);  // not reached
    return false;
   case 1:
    // If the |function| calls Yield(), it reaches here to go back the caller.
    // If the caller calls Continue(), it also reaches here to continue to run
    // |function|.
    return true;
   case 2:
    // If the |function| finishes after several Yield() calls, it reaches here.
    return false;
  }
  assert(false);  // not reached
  return false;
}

void CYield::Yield() {
  Continue();
}

