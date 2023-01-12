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
//! \file cTracerHandler.cpp

#include "tracer.h"
#include "tracerDef.hpp"
#include "tracerInternal.hpp"
#include <string.h>

using namespace tracer;
using namespace std;

extern "C" {

//! \brief Runs tracer::TracerHandler::getTracer()->defaultSetup();
void tr_defaultSetup() { TracerHandler::getTracer()->defaultSetup(); }

//! \brief Retunrns the handler as a private c struct
tr_TracerHandler_t *tr_getTracerHandler() {
  tr_TracerHandler_t *trTH = new tr_TracerHandler_t;

  if (!trTH)
    return nullptr;

  trTH->handler = TracerHandler::getTracer();
  trTH->cfg     = trTH->handler->getConfig();

  return trTH;
}

/*!
 * \brief Frees the handler
 * \note This does not effect the C++ class
 */
void tr_freeTracerHandler(tr_TracerHandler_t *trTH) {
  if (trTH)
    delete trTH;
}

//! \brief Wrapper for tracer::TracerHandler::defaultSetup
TR_BOOL_t tr_TracerHandler__defaultSetup(tr_TracerHandler_t *trTH) {
  if (!trTH)
    return TR_FALSE;

  return trTH->handler->defaultSetup() == true ? TR_TRUE : TR_FALSE;
}

//! \brief Wrapper for tracer::TracerHandler::setup
TR_BOOL_t tr_TracerHandler__setup(tr_TracerHandler_t *trTH, tr_Printer_t *pContainer) {
  if (!pContainer || !trTH)
    return TR_FALSE;

  return trTH->handler->setup(move(pContainer->obj)) == true ? TR_TRUE : TR_FALSE;
}

//! \brief Wrapper for tracer::TracerHandler::reset
void tr_TracerHandler__reset() { TracerHandler::reset(); }


//! \brief Wrapper for tracer::TracerHandler::getConfig
tr_TracerHandler_Config_t tr_TracerHandler__getConfig(tr_TracerHandler_t *trTH) {
  tr_TracerHandler_Config_t cfg;
  memset(&cfg, 0, sizeof(cfg));

  if (!trTH)
    return cfg;

  trTH->cfg = trTH->handler->getConfig();

  for (size_t i = 0; i < 8 && i < trTH->cfg.preferredTracerEngines.size(); ++i) {
    cfg.tracer[i] = static_cast<TR_TraceerEngines_t>(trTH->cfg.preferredTracerEngines[i]);
  }

  for (size_t i = 0; i < 8 && i < trTH->cfg.preferredDebuggerEngines.size(); ++i) {
    cfg.debuggers[i] = static_cast<TR_DebuggerEngines_t>(trTH->cfg.preferredDebuggerEngines[i]);
  }

  cfg.autoPrintToStdErr         = trTH->cfg.autoPrintToStdErr ? TR_TRUE : TR_FALSE;
  cfg.autoPrintToFile           = trTH->cfg.autoPrintToFile ? TR_TRUE : TR_FALSE;
  cfg.appendToFile              = trTH->cfg.appendToFile ? TR_TRUE : TR_FALSE;
  cfg.callDefultHandlerWhenDone = trTH->cfg.callDefultHandlerWhenDone ? TR_TRUE : TR_FALSE;

  strncpy(cfg.logFile, trTH->cfg.logFile.c_str(), 1024);

  cfg.callback     = reinterpret_cast<tr_callBackPTR>(trTH->cfg.callback);
  cfg.callbackData = trTH->cfg.callbackData;


  for (size_t i = 0; i < 64 && i < trTH->cfg.signums.size(); ++i) {
    cfg.signums[i] = trTH->cfg.signums[i];
  }

  return cfg;
}

//! \brief Wrapper for tracer::TracerHandler::setConfig
void tr_TracerHandler__setConfig(tr_TracerHandler_t *trTH, tr_TracerHandler_Config_t cfg) {
  if (!trTH)
    return;

  trTH->cfg.preferredTracerEngines.clear();
  for (size_t i = 0; i < 8; ++i) {
    if (cfg.tracer[i] == 0)
      break;

    trTH->cfg.preferredTracerEngines.push_back(static_cast<TraceerEngines>(cfg.tracer[i]));
  }

  trTH->cfg.preferredDebuggerEngines.clear();
  for (size_t i = 0; i < 8; ++i) {
    if (cfg.tracer[i] == 0)
      break;

    trTH->cfg.preferredDebuggerEngines.push_back(static_cast<DebuggerEngines>(cfg.tracer[i]));
  }

  trTH->cfg.autoPrintToStdErr         = cfg.autoPrintToStdErr == TR_TRUE;
  trTH->cfg.autoPrintToFile           = cfg.autoPrintToFile == TR_TRUE;
  trTH->cfg.appendToFile              = cfg.appendToFile == TR_TRUE;
  trTH->cfg.callDefultHandlerWhenDone = cfg.callDefultHandlerWhenDone == TR_TRUE;

  trTH->cfg.logFile = cfg.logFile;

  trTH->cfg.callback     = reinterpret_cast<TracerHandler::callBackPTR>(cfg.callback);
  trTH->cfg.callbackData = cfg.callbackData;

  trTH->cfg.signums.clear();
  for (size_t i = 0; i < 64; ++i) {
    if (cfg.signums[i] == 0)
      break;

    trTH->cfg.signums.push_back(cfg.signums[i]);
  }

  trTH->handler->setConfig(trTH->cfg);
}
}
