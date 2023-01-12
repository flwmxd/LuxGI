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
//! \file Tracer.cpp

#include "tracerDef.hpp"
#include "Tracer.hpp"
#include "DebugInfoDummy.hpp"
#include "DummyTracer.hpp"
#include <iostream>

#if USE_LIBUNWIND
#include "LibUnwindTracer.hpp"
#endif

#if USE_GLIBC
#include "GlibCTracer.hpp"
#endif

#if USE_DWFL
#include "DebugInfoDWLF.hpp"
#endif

#if USE_BFD
#include "DebugInfoBFD.hpp"
#endif

#if USE_FALLBACK
#include "DebugInfoExternalFallback.hpp"
#endif

#if USE_WINDOWS
#include "DebugInfoWIN32.hpp"
#include "Win32Tracer.hpp"
#endif

using namespace tracer;
using namespace std;

//! \brief Default constructor; uses the first entrie from getAvaliableEngines and getAvaliableDebuggers
Tracer::Tracer() : Tracer(getAvailableEngines()[0], getAvailableDebuggers()[0]) {}

/*!
 * \brief Constructor, Sets the engines
 * \param engine   The tracer engine to use
 * \param debugger The debugger engine to use
 *
 * \note Both engine and debugger must be in the list returned form getAvaliableEngines and getAvaliableDebuggers
 * \note If not the dummy engines will be used
 */
Tracer::Tracer(TraceerEngines engine, DebuggerEngines debugger) {
// Tracer
#if USE_LIBUNWIND
  if (engine == TraceerEngines::LIBUNWIND)
    tracerEngine = new LibUnwindTracer;
#endif

#if USE_GLIBC
  if (engine == TraceerEngines::GLIBC)
    tracerEngine = new GlibCTracer;
#endif

#if USE_WINDOWS
  if (engine == TraceerEngines::WIN32_TRACER)
    tracerEngine = new Win32Tracer;
#endif

  if (engine == TraceerEngines::DUMMY)
    tracerEngine = new DummyTracer;

// Debugger
#if USE_DWFL
  if (debugger == DebuggerEngines::LIBDWFL)
    debuggerEngine = new DebugInfoDWFL;
#endif

#if USE_BFD
  if (debugger == DebuggerEngines::LIBBFD)
    debuggerEngine = new DebugInfoBFD;
#endif

#if USE_WINDOWS
  if (debugger == DebuggerEngines::WIN32_INFO)
    debuggerEngine = new DebugInfoWIN32;
#endif

#if USE_FALLBACK
  if (debugger == DebuggerEngines::EXTERNAL_FALLBACK)
    debuggerEngine = new DebugInfoExternalFallback;
#endif

  if (debugger == DebuggerEngines::DUMMY)
    debuggerEngine = new DebugInfoDummy;


  if (!tracerEngine) {
    cerr << "[TRACER] Unable to initialize tracer engine; Falling back to the dummy engine" << endl;
    cerr << "[TRACER] No stack trace will be genereted!" << endl;
    tracerEngine = new DummyTracer;
    return;
  }

  if (!debuggerEngine) {
    cerr << "[TRACER] Unable to initialize debugger engine; Falling back to the dummy engine" << endl;
    cerr << "[TRACER] No debug information will be parsed!" << endl;
    debuggerEngine = new DebugInfoDummy;
    return;
  }
}

Tracer::~Tracer() {
  if (tracerEngine)
    delete tracerEngine;

  if (debuggerEngine)
    delete debuggerEngine;
}

/*!
 * \brief Generates the stack trace
 * \returns A pointer to the generated frames
 */
vector<Frame> *Tracer::trace() {
  if (!tracerEngine || !debuggerEngine) {
    cerr << "[TRACER] Can not generate a backtrace without a tracerEngine or debuggerEngine" << endl;
    return nullptr;
  }

  frames = tracerEngine->backtrace();
  debuggerEngine->processFrames(frames);

  return &frames;
}

vector<Frame> *Tracer::getFrames() { return &frames; } //!< \brief Returns a pointer to the stack frames


AbstractTracer *   Tracer::getTracerEngine() { return tracerEngine; }     //!< \brief Returns the internal tracer engine
AbstractDebugInfo *Tracer::getDebuggerEngine() { return debuggerEngine; } //!< \brief Returns the internal debug engine


//! \brief Returns a list of available tracer engines
vector<TraceerEngines> Tracer::getAvailableEngines() {
  vector<TraceerEngines> engines;

#if USE_LIBUNWIND
  engines.emplace_back(TraceerEngines::LIBUNWIND);
#endif
#if USE_GLIBC
  engines.emplace_back(TraceerEngines::GLIBC);
#endif
#if USE_WINDOWS
  engines.emplace_back(TraceerEngines::WIN32_TRACER);
#endif

  engines.emplace_back(TraceerEngines::DUMMY);

  return engines;
}

//! \brief Returns a list of available debug info engines
vector<DebuggerEngines> Tracer::getAvailableDebuggers() {
  vector<DebuggerEngines> engines;

#if USE_DWFL
  engines.emplace_back(DebuggerEngines::LIBDWFL);
#endif

#if USE_BFD
  engines.emplace_back(DebuggerEngines::LIBBFD);
#endif

#if USE_WINDOWS
  engines.emplace_back(DebuggerEngines::WIN32_INFO);
#endif

#if USE_FALLBACK
  engines.emplace_back(DebuggerEngines::EXTERNAL_FALLBACK);
#endif

  engines.emplace_back(DebuggerEngines::DUMMY);

  return engines;
}
