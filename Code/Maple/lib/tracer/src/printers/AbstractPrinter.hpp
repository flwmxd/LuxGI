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
//! \file AbstractPrinter.hpp

#pragma once

#include "tracerDef.hpp"

namespace tracer {

class Tracer;

/*!
 * \brief Basic class for all printers
 */
class AbstractPrinter {
 protected:
  Tracer *trace         = nullptr; //!< \brief Pointer to the trace
  bool    disableColorB = false;   //!< \brief Whether to disable colored output or not

  virtual void setupTrace(); //!< \brief This function will be called when the trace is sets

  virtual std::string genStringPreFrameIMPL(size_t frameNum);     //!< \brief Generate string for the frame info prefix
  virtual std::string genStringForFrameIMPL(size_t frameNum) = 0; //!< \brief Generate string for the frame info
  virtual std::string genStringPostFrameIMPL(size_t frameNum);    //!< \brief Generate string for the frame info suffix

 public:
  virtual ~AbstractPrinter();
  AbstractPrinter();

  std::string genStringPreFrame(size_t frameNum);
  std::string genStringForFrame(size_t frameNum);
  std::string genStringPostFrame(size_t frameNum);

  std::string generateString();
  void printToFile(std::string file, bool append = true);
  void printToStdOut();
  void printToStdErr();

  void enableColor() { disableColorB = false; } //!< \brief Enables colored output (ANSI escape sequences)
  void disableColor() { disableColorB = true; } //!< \brief Disables colored output (ANSI escape sequences)

  void setTrace(Tracer *t);
};
}
