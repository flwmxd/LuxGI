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
//! \file DebugInfoBFD.hpp

#pragma once

#define PACKAGE // required by bfd.h

#include "tracerDef.hpp"
#include "AbstractDebugInfo.hpp"
#include <bfd.h>

namespace tracer {

//! \brief Some internally used classes to make things easiers
namespace internal {

//!< \brief Manages the bfd object (using RAII)
class abfdRAII {
 public:
  bfd *abfd = nullptr; //!< \brief The bfd object to manage

  ~abfdRAII() {
    if (abfd)
      bfd_close(abfd);
  }

  inline bfd *operator()() const noexcept { return abfd; } //!< \brief Returns the bfd object
};

//! \brief Manages the bfd_symbol(s)
class asymbolRAII {
 public:
  bfd_symbol **symbols = nullptr; //!< \brief The bfd_symbol to manage

  ~asymbolRAII() {
    if (symbols)
      free(symbols);
  }

  inline bfd_symbol **operator()() const noexcept { return symbols; } //!< \brief Returns the symbols
};
}

/*!
 * \brief Generates debug information using libbfd
 */
class DebugInfoBFD : public AbstractDebugInfo {
 private:
  static bool isBfdInit; //!< \brief Whether libbfd is init or not (default false)

  static void findInSection(bfd *abfd, bfd_section *sec, void *ctx);

  /*!
   * \brief Internal data structure for libbfd debug information generation
   */
  struct FindInSectionContext {
    bool found = false; //!< \brief Whether the line information in the DWARF section was already found

    Address addr     = 0; //!< \brief The address of the frame (the instruction pointer)
    Address baseAddr = 0; //!< \brief The base address in the library

    internal::asymbolRAII *secSym    = nullptr; //!< \brief Programm section symbols
    internal::asymbolRAII *dynSecSym = nullptr; //!< \brief Shared object symbols

    std::string fileName; //!< \brief The name of the source file [output]
    std::string funcName; //!< \brief The name of the function [output]
    int         line = 0; //!< \brief The line in the source file [output]
    int         col  = 0; //!< \brief The column in the source file [output]
  } ctx;

 public:
  DebugInfoBFD() = default;
  virtual ~DebugInfoBFD();

  bool processFrames(std::vector<Frame> &frames) override;
};
}
