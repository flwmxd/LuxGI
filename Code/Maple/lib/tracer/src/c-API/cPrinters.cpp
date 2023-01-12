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
//! \file cPrinters.cpp

#include "tracer.h"
#include "tracerDef.hpp"
#include "tracerInternal.hpp"
#include <string.h>

//! \brief Makes sure that the printer can be casted and sets cOK
#define PRINTER_CASTER(Target)                      \
  TR_BOOL_t localCastOK = TR_TRUE;                  \
  if (!castOK)                                      \
    castOK = &localCastOK;                          \
                                                    \
  TR_BOOL_t &cOK = *castOK;                         \
                                                    \
  cOK             = TR_FALSE;                       \
  Target *printer = nullptr;                        \
  if (p) {                                          \
    printer = dynamic_cast<Target *>(p->obj.get()); \
    if (printer) {                                  \
      cOK = TR_TRUE;                                \
    }                                               \
  }

using namespace tracer;
using namespace std;

extern "C" {

//! \brief Wrapper for tracer::AbstractPrinter::genStringPreFrame
void tr_Printer__genStringPreFrame(tr_Printer_t *p, size_t frameNum, char **outStr, TR_BOOL_t *castOK) {
  PRINTER_CASTER(AbstractPrinter)

  if (!outStr || cOK != TR_TRUE)
    return;

  *outStr = nullptr;

  string str = printer->genStringPreFrame(frameNum);
  if (!str.empty()) {
    *outStr = reinterpret_cast<char *>(malloc(sizeof(char) * (str.length() + 1)));
    strncpy(*outStr, str.c_str(), str.length() + 1);
  }
}

//! \brief Wrapper for tracer::AbstractPrinter::genStringForFrame
void tr_Printer__genStringForFrame(tr_Printer_t *p, size_t frameNum, char **outStr, TR_BOOL_t *castOK) {
  PRINTER_CASTER(AbstractPrinter)

  if (!outStr || cOK != TR_TRUE)
    return;

  *outStr = nullptr;

  string str = printer->genStringForFrame(frameNum);
  if (!str.empty()) {
    *outStr = reinterpret_cast<char *>(malloc(sizeof(char) * (str.length() + 1)));
    strncpy(*outStr, str.c_str(), str.length() + 1);
  }
}

//! \brief Wrapper for tracer::AbstractPrinter::genStringPostFrame
void tr_Printer__genStringPostFrame(tr_Printer_t *p, size_t frameNum, char **outStr, TR_BOOL_t *castOK) {
  PRINTER_CASTER(AbstractPrinter)

  if (!outStr || cOK != TR_TRUE)
    return;

  *outStr = nullptr;

  string str = printer->genStringPostFrame(frameNum);
  if (!str.empty()) {
    *outStr = reinterpret_cast<char *>(malloc(sizeof(char) * (str.length() + 1)));
    strncpy(*outStr, str.c_str(), str.length() + 1);
  }
}

//! \brief Wrapper for tracer::AbstractPrinter::generateString
void tr_Printer__generateString(tr_Printer_t *p, char **outStr, TR_BOOL_t *castOK) {
  PRINTER_CASTER(AbstractPrinter)

  if (!outStr || cOK != TR_TRUE)
    return;

  *outStr = nullptr;

  string str = printer->generateString();
  if (!str.empty()) {
    *outStr = reinterpret_cast<char *>(malloc(sizeof(char) * (str.length() + 1)));
    strncpy(*outStr, str.c_str(), str.length() + 1);
  }
}

//! \brief Wrapper for tracer::AbstractPrinter::printToFile
void tr_Printer__printToFile(tr_Printer_t *p, const char *file, TR_BOOL_t append, TR_BOOL_t *castOK) {
  PRINTER_CASTER(AbstractPrinter)

  if (!file || cOK != TR_TRUE)
    return;

  printer->printToFile(file, append == TR_TRUE);
}

//! \brief Wrapper for tracer::AbstractPrinter::printToStdOut
void tr_Printer__printToStdOut(tr_Printer_t *p, TR_BOOL_t *castOK) {
  PRINTER_CASTER(AbstractPrinter)

  if (cOK != TR_TRUE)
    return;

  printer->printToStdOut();
}

//! \brief Wrapper for tracer::AbstractPrinter::printToStdErr
void tr_Printer__printToStdErr(tr_Printer_t *p, TR_BOOL_t *castOK) {
  PRINTER_CASTER(AbstractPrinter)

  if (cOK != TR_TRUE)
    return;

  printer->printToStdErr();
}

//! \brief Wrapper for tracer::AbstractPrinter::enableColor
void tr_Printer__enableColor(tr_Printer_t *p, TR_BOOL_t *castOK) {

  PRINTER_CASTER(AbstractPrinter)

  if (cOK != TR_TRUE)
    return;

  printer->enableColor();
}

//! \brief Wrapper for tracer::AbstractPrinter::disableColor
void tr_Printer__disableColor(tr_Printer_t *p, TR_BOOL_t *castOK) {
  PRINTER_CASTER(AbstractPrinter)

  if (cOK != TR_TRUE)
    return;

  printer->disableColor();
}

//! \brief Wrapper for tracer::AbstractPrinter::setTrace
void tr_Printer__setTrace(tr_Printer_t *p, tr_Tracer_t *t, TR_BOOL_t *castOK) {
  PRINTER_CASTER(AbstractPrinter)

  if (!t || cOK != TR_TRUE)
    return;

  printer->setTrace(t->tPTR);
}


//! \brief Wrapper for tracer::DefaultPrinter::getConfig
tr_DefaultPrinter_Config_t tr_Printer__getConfig(tr_Printer_t *p, TR_BOOL_t *castOK) {
  tr_DefaultPrinter_Config_t cfg;
  memset(&cfg, 0, sizeof(cfg));

  PRINTER_CASTER(DefaultPrinter)

  if (cOK != TR_TRUE)
    return cfg;

  auto pCFG = printer->getConfig();

  strncpy(cfg.prefix, pCFG.prefix.c_str(), 64);
  strncpy(cfg.seper1, pCFG.seper1.c_str(), 64);
  strncpy(cfg.seper2, pCFG.seper2.c_str(), 64);
  strncpy(cfg.seper3, pCFG.seper3.c_str(), 64);
  strncpy(cfg.suffix, pCFG.suffix.c_str(), 64);
  strncpy(cfg.colorFrameNum, pCFG.colorFrameNum.c_str(), 64);
  strncpy(cfg.colorNotFound, pCFG.colorNotFound.c_str(), 64);
  strncpy(cfg.colorAddress, pCFG.colorAddress.c_str(), 64);
  strncpy(cfg.colorFuncName, pCFG.colorFuncName.c_str(), 64);
  strncpy(cfg.colorLineInfo, pCFG.colorLineInfo.c_str(), 64);
  strncpy(cfg.colorModule, pCFG.colorModule.c_str(), 64);

  cfg.shortenFiles      = pCFG.shortenFiles ? TR_TRUE : TR_FALSE;
  cfg.shortenModules    = pCFG.shortenModules ? TR_TRUE : TR_FALSE;
  cfg.canonicalizePaths = pCFG.canonicalizePaths ? TR_TRUE : TR_FALSE;

  return cfg;
}

//! \brief Wrapper for tracer::DefaultPrinter::setConfig
void tr_Printer__setConfig(tr_Printer_t *p, tr_DefaultPrinter_Config_t cfg, TR_BOOL_t *castOK) {
  PRINTER_CASTER(DefaultPrinter)

  if (cOK != TR_TRUE)
    return;

  DefaultPrinter::Config pCFG;
  pCFG.prefix        = cfg.prefix;
  pCFG.seper1        = cfg.seper1;
  pCFG.seper2        = cfg.seper2;
  pCFG.seper3        = cfg.seper3;
  pCFG.suffix        = cfg.suffix;
  pCFG.colorFrameNum = cfg.colorFrameNum;
  pCFG.colorNotFound = cfg.colorNotFound;
  pCFG.colorAddress  = cfg.colorAddress;
  pCFG.colorFuncName = cfg.colorFuncName;
  pCFG.colorLineInfo = cfg.colorLineInfo;
  pCFG.colorModule   = cfg.colorModule;

  pCFG.shortenFiles      = cfg.shortenFiles == TR_TRUE;
  pCFG.shortenModules    = cfg.shortenModules == TR_TRUE;
  pCFG.canonicalizePaths = cfg.canonicalizePaths == TR_TRUE;

  printer->setConfig(pCFG);
}

//! \brief Wrapper for tracer::SystemInfoPrinter::setSignum
void tr_Printer__setSignum(tr_Printer_t *p, int signum, TR_BOOL_t *castOK) {
  PRINTER_CASTER(SystemInfoPrinter)

  if (cOK != TR_TRUE)
    return;

  printer->setSignum(signum);
}

//! \brief Wrapper for tracer::SystemInfoPrinter::addSystemEntry
void tr_Printer__addSystemEntry(tr_Printer_t *p, const char *name, const char *value, TR_BOOL_t *castOK) {
  PRINTER_CASTER(SystemInfoPrinter)

  if (cOK != TR_TRUE || !name || !value)
    return;

  printer->addSystemEntry({name, value});
}
}
