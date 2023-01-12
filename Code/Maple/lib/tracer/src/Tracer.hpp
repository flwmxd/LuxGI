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
//! \file Tracer.hpp

#pragma once

#include "tracerDef.hpp"
#include "AbstractDebugInfo.hpp"
#include "AbstractTracer.hpp"

namespace tracer {

//! \brief All available tracer backends
enum class TraceerEngines { LIBUNWIND = 1, GLIBC, WIN32_TRACER, DUMMY };

//! \brief All available debug information backends
enum class DebuggerEngines { LIBDWFL = 1, LIBBFD, WIN32_INFO, EXTERNAL_FALLBACK, DUMMY };

/*!
 * \brief Backtrace generator
 *
 * This class generates a Backtrace using different backends and stores it in
 * the platform independent Frame struct
 */
class Tracer {
 private:
  AbstractTracer *   tracerEngine   = nullptr; //!< \brief The tracer engine
  AbstractDebugInfo *debuggerEngine = nullptr; //!< \brief The debug information engine

  std::vector<Frame> frames; //!< \brief The generated frames

 public:
  Tracer(TraceerEngines engine, DebuggerEngines debugger);
  Tracer();
  virtual ~Tracer();

  Tracer(const Tracer &) = delete;
  Tracer(Tracer &&)      = delete;

  Tracer &operator=(const Tracer &) = delete;
  Tracer &operator=(Tracer &&) = delete;

  std::vector<Frame> *trace();
  std::vector<Frame> *getFrames();

  std::vector<Frame> *operator()() { return trace(); } //!< \brief Wrapper for trace

  AbstractTracer *   getTracerEngine();
  AbstractDebugInfo *getDebuggerEngine();

  static std::vector<TraceerEngines>  getAvailableEngines();
  static std::vector<DebuggerEngines> getAvailableDebuggers();
};
}
