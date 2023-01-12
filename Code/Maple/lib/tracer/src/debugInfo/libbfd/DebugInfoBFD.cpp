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
//! \file DebugInfoBFD.cpp

#include "tracerDef.hpp"
#include "DebugInfoBFD.hpp"
#include <cxxabi.h>
#include <dlfcn.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>

using namespace tracer;
using namespace tracer::internal;
using namespace std;

DebugInfoBFD::~DebugInfoBFD() {}

bool DebugInfoBFD::isBfdInit = false;

/*!
 * \brief Callback for libbfd (bfd_map_over_sections)
 *
 * Checks whether the section and the address matches and extracts the information.
 *
 * \param abfd    The current bfd context
 * \param section The current processed section
 * \param ctx     Used supplied data (here a pointer to the DebugInfoBFD object)
 */
void DebugInfoBFD::findInSection(bfd *abfd, bfd_section *section, void *ctx) {
  DebugInfoBFD *obj = reinterpret_cast<DebugInfoBFD *>(ctx);

  if (obj->ctx.found)
    return; // Nothing left to do

  if ((bfd_get_section_flags(abfd, section) & SEC_ALLOC) == 0)
    return;

  bfd_vma       vma  = bfd_get_section_vma(abfd, section);
  bfd_size_type size = bfd_get_section_size(section);

  auto addressToUse = obj->ctx.addr;

  // Check boundaries
  if (addressToUse < vma || addressToUse >= vma + size) {
    addressToUse = obj->ctx.addr - obj->ctx.baseAddr; // Try again with adjusted values
    if (addressToUse < vma || addressToUse >= vma + size) {
      return;
    }
  }

  const char * fileName_CSTR = nullptr;
  const char * funcName_CSTR = nullptr;
  unsigned int lineNumber    = 0;
  unsigned int column        = 0;


  if (bfd_find_nearest_line_discriminator(abfd,
                                          section,
                                          obj->ctx.secSym->symbols,
                                          addressToUse - vma,
                                          &fileName_CSTR,
                                          &funcName_CSTR,
                                          &lineNumber,
                                          &column) == 0) {
    if (bfd_find_nearest_line_discriminator(abfd,
                                            section,
                                            obj->ctx.dynSecSym->symbols,
                                            addressToUse - vma,
                                            &fileName_CSTR,
                                            &funcName_CSTR,
                                            &lineNumber,
                                            &column) != 0) {
      return;
    }
  }

  obj->ctx.found = true;

  obj->ctx.line = static_cast<int>(lineNumber);
  obj->ctx.col  = static_cast<int>(column);

  if (fileName_CSTR)
    obj->ctx.fileName = fileName_CSTR;

  if (funcName_CSTR) {
    char   demangled[constants::MAX_FUNC_NAME];
    int    status = 0;
    size_t legth  = sizeof(demangled);
    abi::__cxa_demangle(funcName_CSTR, demangled, &legth, &status);

    if (status == 0) {
      obj->ctx.funcName = demangled;
    } else {
      obj->ctx.funcName = funcName_CSTR;
    }
  }
}



/*!
 * \brief Generates the debug information with libbfd
 * \param frames The frames to process
 */
bool DebugInfoBFD::processFrames(std::vector<Frame> &frames) {
  if (!isBfdInit) {
    bfd_init();
    isBfdInit = true;
  }

  for (auto &i : frames) {
    Dl_info dlInfo;

    if (dladdr(reinterpret_cast<void *>(i.frameAddr), &dlInfo) == 0) {
      continue;
    }

    if (dlInfo.dli_sname) {
      char   demangled[constants::MAX_FUNC_NAME];
      int    status = 0;
      size_t legth  = sizeof(demangled);
      abi::__cxa_demangle(dlInfo.dli_sname, demangled, &legth, &status);

      i.flags |= FrameFlags::HAS_FUNC_NAME;

      if (status == 0) {
        i.funcName = demangled;
      } else {
        i.funcName = dlInfo.dli_sname;
      }
    }

    if (dlInfo.dli_fname) {
      i.flags |= FrameFlags::HAS_MODULE_INFO;
      i.moduleName = dlInfo.dli_fname;
    } else {
      continue; // dlInfo.dli_fname is required for the next step
    }

    // Start getting line info
    abfdRAII    abfd;
    asymbolRAII symbols;
    asymbolRAII dynSymbols;

    abfd.abfd = bfd_openr(dlInfo.dli_fname, nullptr);
    if (abfd() == nullptr)
      continue;

    abfd()->flags |= BFD_DECOMPRESS;

    if (bfd_check_format(abfd(), bfd_object) != TRUE)
      continue;

    if ((bfd_get_file_flags(abfd()) & HAS_SYMS) == 0)
      continue; // No debug symbols?

    auto storageSize        = bfd_get_symtab_upper_bound(abfd());
    auto dynamicStorageSize = bfd_get_dynamic_symtab_upper_bound(abfd());

    if (storageSize <= 0 && dynamicStorageSize <= 0)
      continue;

    if (storageSize > 0) {
      symbols.symbols = static_cast<bfd_symbol **>(malloc(static_cast<size_t>(storageSize)));
      bfd_canonicalize_symtab(abfd(), symbols());
    }

    if (dynamicStorageSize > 0) {
      dynSymbols.symbols = static_cast<bfd_symbol **>(malloc(static_cast<size_t>(dynamicStorageSize)));
      bfd_canonicalize_dynamic_symtab(abfd(), dynSymbols());
    }

    ctx = FindInSectionContext();

    ctx.addr      = i.frameAddr - 4;
    ctx.baseAddr  = reinterpret_cast<Address>(dlInfo.dli_fbase);
    ctx.secSym    = &symbols;
    ctx.dynSecSym = &dynSymbols;

    bfd_map_over_sections(abfd(), &findInSection, reinterpret_cast<void *>(this));

    if (ctx.found) {
      if (!ctx.fileName.empty()) {
        i.flags |= FrameFlags::HAS_LINE_INFO;
        i.fileName = ctx.fileName;
        i.line     = ctx.line;
        i.column   = ctx.col;
      }

      if (!ctx.funcName.empty()) {
        i.flags |= FrameFlags::HAS_FUNC_NAME;
        i.funcName = ctx.funcName;
      }
    }
  }

  return true;
}
