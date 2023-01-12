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
//! \file DefaultPrinter.hpp

#pragma once

#include "tracerDef.hpp"
#include "AbstractPrinter.hpp"

namespace tracer {

/*!
 * \brief Gnerates strings from the frame structure
 *
 * The generated string has the form (by default)
 *
 * #[numOfFrame] in functionName at fileLocation -- binaryFile [address]
 *
 * This cam be configured with the DefaultPrinter::Config struct
 */
class DefaultPrinter : virtual public AbstractPrinter {
 public:
  //! \brief User configuration structure
  struct Config {
    std::string prefix = " \x1b[0;33min ";     //!< \brief Perefix (prefix [functionName])
    std::string seper1 = " \x1b[0;33mat ";     //!< \brief 1st seperator
    std::string seper2 = " \x1b[0;33m-- ";     //!< \brief 2nd seperator
    std::string seper3 = " \x1b[0;33m[";       //!< \brief 3rd seperator
    std::string suffix = "\x1b[0;33m]\x1b[0m"; //!< \brief suffix

    std::string colorFrameNum = "\x1b[1;36m"; //!< \brief ANSI escape sequence for the Frame number color
    std::string colorNotFound = "\x1b[1;33m"; //!< \brief ANSI escape sequence for the "Not Found" color
    std::string colorAddress  = "\x1b[0;36m"; //!< \brief ANSI escape sequence for the Address color
    std::string colorFuncName = "\x1b[1;31m"; //!< \brief ANSI escape sequence for the Function Name color
    std::string colorLineInfo = "\x1b[1;32m"; //!< \brief ANSI escape sequence for the Line information color
    std::string colorModule   = "\x1b[1;35m"; //!< \brief ANSI escape sequence for the Frame number color

    bool shortenFiles      = false; //!< \brief The source file path
    bool shortenModules    = true;  //!< \brief The executable module (.so/.dll/.exe)
    bool canonicalizePaths = true;  //!< \brief Fixes path names if they contain "/../" or are relative
  };

 private:
  size_t maxModuleNameLegth = 5;
  size_t maxLineInfoLength  = 5;
  size_t maxAddressLength   = 5;
  size_t maxFuncNameLegth   = 5;

  bool calculatedMaxLengths = false;

  Config cfg;

  void calcMaxNameLengths();

 protected:
  std::string genStringForFrameIMPL(size_t frameNum) override;
  void setupTrace() override;

 public:
  DefaultPrinter();
  virtual ~DefaultPrinter();

  void setConfig(Config newCfg) { cfg = newCfg; } //!< \brief Sets the new config

  Config getConfig() const { return cfg; } //!< \brief Returns the current
};
}
