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
//! \file TracerHandler.cpp

#include "tracerDef.hpp"
#include "TracerHandler.hpp"
#include "FancyPrinter.hpp"
#include <iostream>

#ifndef _WIN32
#include <unistd.h>
#endif

using namespace tracer;
using namespace std;

TracerHandler::TracerHandler() : printer(nullptr) {} //!< \brief Initalizes the printer with nullptr
TracerHandler::~TracerHandler() { tracer = nullptr; }

/*!
 * \brief Resets the singleton
 */
void TracerHandler::reset() {
  if (tracer)
    delete tracer;

  tracer = nullptr;
}

//! \brief Returns a pointer to the TracerHandler object
TracerHandler *TracerHandler::getTracer() {
  if (!tracer)
    tracer = new TracerHandler();

  return tracer;
}


//! \brief The internal signal handler
void TracerHandler::sigHandler(int sigNum) {
  TracerHandler *th  = TracerHandler::getTracer();
  auto           cfg = th->getConfig();

  if (cfg.callDefultHandlerWhenDone) {
#if defined(__GNUC__) || defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif
    signal(sigNum, SIG_DFL);
#if defined(__GNUC__) || defined(__clang__)
#pragma clang diagnostic pop
#endif
  }

  auto tracerEngines   = Tracer::getAvailableEngines();
  auto debuggerEngines = Tracer::getAvailableDebuggers();

  TraceerEngines  tEngine  = tracerEngines[0];
  DebuggerEngines tDebuger = debuggerEngines[0];

  for (auto i : cfg.preferredTracerEngines) {
    for (auto j : tracerEngines) {
      if (i == j) {
        tEngine = i;
        break;
      }
    }
  }

  for (auto i : cfg.preferredDebuggerEngines) {
    for (auto j : debuggerEngines) {
      if (i == j) {
        tDebuger = i;
        break;
      }
    }
  }

  Tracer t(tEngine, tDebuger);

  t.trace();

  AbstractPrinter *  p    = th->printer.get();
  SystemInfoPrinter *sysP = nullptr;
  if (!p) {
    cerr << "[TRACER] (sigHandler) Fatal error: Printer not set (this code should never be executed)" << endl;
    goto error;
  }

  p->setTrace(&t);
  sysP = dynamic_cast<SystemInfoPrinter *>(p);
  if (sysP) {
    sysP->setSignum(sigNum);
  }

  if (cfg.autoPrintToStdErr) {
    p->printToStdErr();
  }

  if (cfg.autoPrintToFile) {
    p->printToFile(cfg.logFile, cfg.appendToFile);
  }

  if (cfg.callback)
    cfg.callback(&t, p, cfg.callbackData);

error:
  if (!cfg.callDefultHandlerWhenDone)
    return;

#ifndef _WIN32
  kill(getpid(), sigNum);
#endif
}


//! \brief Basic setup, sufficient for most use cases
bool TracerHandler::defaultSetup() { return setup(PrinterContainer::fancy()); }

//! \brief Sets everything up with a custom printer
bool TracerHandler::setup(PrinterContainer printerToUse) {
  printer = std::move(printerToUse);

  for (int i : cfg.signums) {
    signal(i, TracerHandler::sigHandler);
  }

  return true;
}


TracerHandler *TracerHandler::tracer = nullptr;
