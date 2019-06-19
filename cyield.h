// Copyright 2019 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#if !defined(__cyield_h__)
#define __cyield_h__

#include <csetjmp>
#include <functional>

// A class interface that a running |function| will receive to call functions
// to realize "yield".
class CYieldClientAPI {
 public:
  virtual void Yield() = 0;
};

// A class to run a function on a separated stack with a functionality of
// "yield". Since it runs on a separated stack, both call sites and the function
// can run alternately without breaking information on each stack.
class CYield : CYieldClientAPI {
 public:
  CYield(size_t stack_size);
  ~CYield();

  // Runs the passed |function| on the newly created stack, until
  // CYieldClientAPI::Yield() is called inside |function|, or it finishes its
  // execution.
  // Returns true if its execution is suspended by the Yield() call. Otherwise
  // returns faslse.
  bool Run(std::function<void(CYieldClientAPI*)> function);

  // Continues to run the yielded |function| until the next Yield() call, or the
  // end of its execution.
  // Returns true if its execution is suspended by the Yield() call. Otherwise
  // returns faslse.
  bool Continue();

 private:
  void Yield() override;

  size_t stack_size = 0;
  uint8_t* stack_pointer = nullptr;
  std::jmp_buf call_env[2];
  int env = 0;
};

#endif // !defined(__cyield_h__)

