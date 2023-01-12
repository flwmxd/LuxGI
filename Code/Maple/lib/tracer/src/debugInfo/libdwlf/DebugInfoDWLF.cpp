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
//! \file DebugInfoDWLF.cpp

#include "tracerDef.hpp"
#include "DebugInfoDWLF.hpp"
#include <cxxabi.h>
#include <dwarf.h>
#include <iostream>
#include <libdwfl.h>
#include <unistd.h>

using namespace tracer;
using namespace std;

DebugInfoDWFL::~DebugInfoDWFL() {}


/*!
 * \brief Generates debug information with libdwfl
 * \param frames The frames to process
 */
bool DebugInfoDWFL::processFrames(vector<Frame> &frames) {
  char *debuginfo_path = nullptr;

  Dwfl_Callbacks callbacks;
  callbacks.find_elf        = dwfl_linux_proc_find_elf;
  callbacks.find_debuginfo  = dwfl_standard_find_debuginfo;
  callbacks.section_address = dwfl_offline_section_address;
  callbacks.debuginfo_path  = &debuginfo_path;

  Dwfl *dwfl = dwfl_begin(&callbacks);
  if (!dwfl) {
    cerr << "[TRACER] (libdwfl) Failed to initialize libdwfl" << endl;
    return false;
  }

  dwfl_linux_proc_report(dwfl, getpid());
  dwfl_report_end(dwfl, nullptr, nullptr);

  for (auto &i : frames) {
    Dwarf_Addr   addr = reinterpret_cast<Dwarf_Addr>(i.frameAddr) - 4;
    GElf_Sym     sym;
    GElf_Word    shndx;
    Dwfl_Module *module       = nullptr;
    const char * functionName = nullptr;
    const char * moduleName   = nullptr;

    module = dwfl_addrmodule(dwfl, addr);
    if (!module)
      continue;

    moduleName = dwfl_module_info(module, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    if (moduleName) {
      i.flags |= FrameFlags::HAS_MODULE_INFO;
      i.moduleName = moduleName;
    }

    functionName = dwfl_module_addrsym(module, addr, &sym, &shndx);
    if (functionName) {
      char   demangled[constants::MAX_FUNC_NAME];
      int    status = 0;
      size_t legth  = sizeof(demangled);
      abi::__cxa_demangle(functionName, demangled, &legth, &status);

      i.flags |= FrameFlags::HAS_FUNC_NAME;

      if (status == 0) {
        i.funcName = demangled;
      } else {
        i.funcName = functionName;
      }
    }

    Dwarf_Addr mod_bias = 0;
    Dwarf_Die *cudie    = dwfl_module_addrdie(module, addr, &mod_bias);

    if (!cudie) {
      // Fix fow clang suboptimal DWARF generator
      // https://github.com/bombela/backward-cpp/blob/master/backward.hpp#L1216
      // http://elfutils-devel.fedorahosted.narkive.com/QRASwB7u/dwfl-module-addrdie-fails-for-binaries-built-with-clang

      bool found = false;

      // Iterate over all dies
      while ((cudie = dwfl_module_nextcu(module, cudie, &mod_bias))) {
        Dwarf_Line *l = dwarf_getsrc_die(cudie, addr - mod_bias);
        if (l) {
          found = true;
          break;
        }
      }

      if (!found)
        cudie = nullptr;
    }

    Dwarf_Line *line = dwarf_getsrc_die(cudie, addr - mod_bias);
    if (line) {
      i.fileName = dwarf_linesrc(line, nullptr, nullptr);
      dwarf_lineno(line, &i.line);
      dwarf_linecol(line, &i.column);
      i.flags |= FrameFlags::HAS_LINE_INFO;
    }
  }

  dwfl_end(dwfl);

  return true;
}
