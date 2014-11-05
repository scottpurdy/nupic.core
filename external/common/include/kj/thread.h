// Copyright (c) 2013, Kenton Varda <temporal@gmail.com>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef KJ_THREAD_H_
#define KJ_THREAD_H_

#include "common.h"
#include "function.h"
#include "exception.h"

namespace kj {

class Thread {
  // A thread!  Pass a lambda to the constructor, and it runs in the thread.  The destructor joins
  // the thread.  If the function throws an exception, it is rethrown from the thread's destructor
  // (if not unwinding from another exception).

public:
  explicit Thread(Function<void()> func);

  ~Thread() noexcept(false);

  void sendSignal(int signo);
  // Send a Unix signal to the given thread, using pthread_kill or an equivalent.

  void detach();
  // Don't join the thread in ~Thread().

private:
  Function<void()> func;
  unsigned long long threadId;  // actually pthread_t
  kj::Maybe<kj::Exception> exception;
  bool detached = false;

  static void* runThread(void* ptr);
};

}  // namespace kj

#endif  // KJ_THREAD_H_
