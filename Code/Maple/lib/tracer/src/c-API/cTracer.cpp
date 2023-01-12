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
//! \file cTracer.cpp

#include "tracer.h"
#include "tracerDef.hpp"
#include "tracerInternal.hpp"
#include <limits>
#include <stddef.h>
#include <string.h>

using namespace tracer;
using namespace std;

extern "C" {

//! \brief Wrapper for tracer::Tracer::Tracer
tr_Tracer_t *tr_getTracer() {
  tr_Tracer_t *t = new tr_Tracer_t;
  t->tPTR        = &t->tracer;
  return t;
}

//! \brief Wrapper for tracer::Tracer::Tracer(TraceerEngines, DebuggerEngines)
tr_Tracer_t *tr_getTracerWithParam(TR_TraceerEngines_t tracer, TR_DebuggerEngines_t debugInfo) {
  tr_Tracer_t *t = new tr_Tracer_t(static_cast<TraceerEngines>(tracer), static_cast<DebuggerEngines>(debugInfo));
  t->tPTR        = &t->tracer;
  return t;
}

//! \brief Creates the priavet C struct form a void pointer to the C++ Tracer object
tr_Tracer_t *tr_getTracerFromVoid(void *tVoid) {
  tr_Tracer_t *t = new tr_Tracer_t;
  t->tPTR        = reinterpret_cast<Tracer *>(tVoid);
  return t;
}

//! \brief Wrapper for tracer::Tracer::trace
void tr_Tracer__trace(tr_Tracer_t *tracer) {
  if (!tracer)
    return;

  tracer->tPTR->trace();
}

/*!
 * \brief (Indirect) Wrapper for tracer::Tracer::getFrames
 * \sa tr_Tracer__getFrame
 */
size_t tr_Tracer__getNumFrames(tr_Tracer_t *tracer) {
  if (!tracer)
    return numeric_limits<size_t>::max();

  return tracer->tPTR->getFrames()->size();
}

/*!
 * \brief (Indirect) Wrapper for tracer::Tracer::getFrames
 * \sa tr_Tracer__getNumFrames
 */
tr_Frame_t tr_Tracer__getFrame(tr_Tracer_t *tracer, size_t frameNum) {
  tr_Frame_t frame;
  memset(&frame, 0, sizeof(frame));

  if (!tracer)
    return frame;

  auto *frames = tracer->tPTR->getFrames();

  if (frameNum >= frames->size())
    return frame;

  auto &f          = frames->at(frameNum);
  frame.frameAddr  = f.frameAddr;
  frame.funcName   = f.funcName.c_str();
  frame.moduleName = f.moduleName.c_str();
  frame.fileName   = f.fileName.c_str();
  frame.line       = f.line;
  frame.column     = f.column;

  if ((f.flags & FrameFlags::HAS_FUNC_NAME) != FrameFlags::HAS_FUNC_NAME)
    frame.funcName = nullptr;

  if ((f.flags & FrameFlags::HAS_MODULE_INFO) != FrameFlags::HAS_MODULE_INFO)
    frame.moduleName = nullptr;

  if ((f.flags & FrameFlags::HAS_LINE_INFO) != FrameFlags::HAS_LINE_INFO)
    frame.fileName = nullptr;

  return frame;
}

/*!
 * \brief (Indirect) Wrapper for tracer::Tracer::getAvailableEngines and tracer::Tracer::getAvailableDebuggers
 */
tr_Tracer_AvailableEngines_t tr_Tracer__getAvailableEngines() {
  tr_Tracer_AvailableEngines_t result;
  memset(&result, 0, sizeof(result));

  int c = 0;
  for (auto i : tracer::Tracer::getAvailableEngines()) {
    result.tracer[c++] = static_cast<TR_TraceerEngines_t>(i);

    if (c >= 8)
      break;
  }

  c = 0;
  for (auto i : tracer::Tracer::getAvailableDebuggers()) {
    result.debuggers[c++] = static_cast<TR_DebuggerEngines_t>(i);

    if (c >= 8)
      break;
  }

  return result;
}

//! \brief Destroyes the private C object and the Tracer C++ object UNLESS tr_getTracerFromVoid was used
void tr_freeTracer(tr_Tracer_t *tracer) {
  if (tracer)
    delete tracer;
}
}
