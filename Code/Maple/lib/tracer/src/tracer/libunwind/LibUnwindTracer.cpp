/* Copyright (c) 2017, Daniel Mensinger
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Daniel Mensinger nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Daniel Mensinger BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
//! \file LibUnwindTracer.cpp

#include "tracerDef.hpp"
#include "LibUnwindTracer.hpp"
#include <iostream>

using namespace tracer;

LibUnwindTracer::~LibUnwindTracer() {}

/*!
 * \brief Generates the backtrace with libunwind
 * \returns The frames with the address set
 */
std::vector<Frame> LibUnwindTracer::backtrace() {
  std::vector<Frame> frames;
  unw_cursor_t       cursor;

  if (!custonContext) {
    if (unw_getcontext(&context) != 0) {
      std::cerr << "[TRACER] (libunwind) Failed to get context" << std::endl;
      return frames;
    }
  }

  if (unw_init_local(&cursor, &context) != 0) {
    std::cerr << "[TRACER] (libunwind) Failed to initialize" << std::endl;
    return frames;
  }

  Frame temp;
  temp.flags |= FrameFlags::HAS_ADDRESS;

  while (unw_step(&cursor) > 0) {
    unw_word_t ip;
    unw_get_reg(&cursor, UNW_REG_IP, &ip);

    temp.frameAddr = static_cast<Address>(ip);
    frames.push_back(temp);
  }

  return frames;
}

/*!
 * \brief Sets the execution context
 * \param ctx The context to set (must point to a ucontext_t struct)
 */
void LibUnwindTracer::setContext(void *ctx) {
  if (typeid(unw_context_t) == typeid(ucontext_t)) {
    context       = *reinterpret_cast<unw_context_t *>(ctx);
    custonContext = true;
  }
}
