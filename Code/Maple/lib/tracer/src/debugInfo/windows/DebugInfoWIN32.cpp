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
//! \file DebugInfoWIN32.cpp

#include "tracerDef.hpp"
#include "DebugInfoWIN32.hpp"
#include <dbghelp.h>
#include <iostream>

using namespace tracer;
using namespace std;

DebugInfoWIN32::~DebugInfoWIN32() {}

bool DebugInfoWIN32::processFrames(vector<Frame> &frames) {
  HANDLE process = GetCurrentProcess();
  BOOL   result  = SymInitialize(process, nullptr, TRUE);

  if (result == 0) {
    cerr << "[TRACER] (Windows) Failed to initialize symbols (SymInitialize)" << endl;
    return false;
  }

  char buffer1[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
  // char buffer2[sizeof(IMAGEHLP_MODULE64) + MAX_SYM_NAME * sizeof(TCHAR)];
  PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer1;

  pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
  pSymbol->MaxNameLen   = MAX_SYM_NAME;

  for (auto &i : frames) {
    DWORD64           displacement;
    DWORD             displacement2;
    DWORD64           addr = static_cast<DWORD64>(i.frameAddr) - 4;
    IMAGEHLP_MODULE64 modInfo;
    IMAGEHLP_LINE64   lineInfo;
    modInfo.SizeOfStruct  = sizeof(IMAGEHLP_MODULE64);
    lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    if (SymFromAddr(process, addr, &displacement, pSymbol)) {
      if (pSymbol->Name) {
        i.flags |= FrameFlags::HAS_FUNC_NAME;
        i.funcName = pSymbol->Name;
      }
    }

    if (SymGetModuleInfo64(process, addr, &modInfo)) {
      if (modInfo.ImageName) {
        i.flags |= FrameFlags::HAS_MODULE_INFO;
        i.moduleName = modInfo.ImageName;
      }
    }

    if (SymGetLineFromAddr64(process, addr, &displacement2, &lineInfo)) {
      if (lineInfo.FileName) {
        i.flags |= FrameFlags::HAS_LINE_INFO;
        i.fileName = lineInfo.FileName;
        i.line     = lineInfo.LineNumber;
        i.column   = 0; // Column not supported :(
      }
    }
  }

  return true;
}
