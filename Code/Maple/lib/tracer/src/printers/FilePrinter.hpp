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
//! \file FilePrinter.hpp

#pragma once

#include "tracerDef.hpp"
#include "DefaultPrinter.hpp"

#if !DISABLE_STD_FILESYSTEM
#if __cplusplus <= 201402L
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif
#endif

namespace tracer {

/*!
 * \brief Prints file contents when line information is avaliable in the frame
 */
class FilePrinter : virtual public DefaultPrinter {
 public:
  //! \brief User configuration
  struct FileConfig {
    unsigned int maxRecursionDepth = 4; //!< \brief Search for relative files recursively for maxRecursionDepth steps
    unsigned int linesBefore       = 4; //!< \brief Number of lines to print before the actual line
    unsigned int linesAfter        = 4; //!< \brief Number of lines to print after the actual line

    std::string lineHighlightColor = "\x1b[1;31m"; //!< \brief ANSI escape sequence color for the line
  };

 private:
  FileConfig fCFG;

#if !DISABLE_STD_FILESYSTEM
  std::vector<fs::path> pathCache;
  bool findPath(unsigned int depth, fs::path current, fs::path &out, fs::path const &file);
#endif

 protected:
  std::string genStringPostFrameIMPL(size_t frameNum) override;

 public:
  FilePrinter() = default;
  virtual ~FilePrinter();

#if !DISABLE_STD_FILESYSTEM
  fs::path findFile(std::string file);
#endif

  void setFilePrinterConfig(FileConfig d) { fCFG = d; } //!< \brief Sets the new configuration

  FileConfig getFilePrinterConfig() { return fCFG; } //!< \brief Returns the current configuration
};
}
