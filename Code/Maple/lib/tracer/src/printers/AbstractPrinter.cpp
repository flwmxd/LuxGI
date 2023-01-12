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
//! \file AbstractPrinter.cpp

#include "tracerDef.hpp"
#include "AbstractPrinter.hpp"
#include "Tracer.hpp"
#include <fstream>
#include <iostream>
#include <regex>

using namespace tracer;
using namespace std;

AbstractPrinter::AbstractPrinter() {} //!< \brief Default constructor
AbstractPrinter::~AbstractPrinter() {}

void AbstractPrinter::setupTrace() {}

/*!
 * \brief Generates a string for the complete stack trace
 * \returns The generated string
 */
std::string tracer::AbstractPrinter::generateString() {
  if (!trace)
    return "";

  std::string outSTR;
  auto *      frames = trace->getFrames();

  for (size_t i = 0; i < frames->size(); ++i) {
    outSTR += genStringPreFrameIMPL(i);
    outSTR += genStringForFrameIMPL(i);
    outSTR += genStringPostFrameIMPL(i);
  }

  if (disableColorB) {
    regex escRemover("\x1b\\[[0-9;]*m");
    outSTR = regex_replace(outSTR, escRemover, "");
  }

  return outSTR;
}

string AbstractPrinter::genStringPreFrameIMPL(size_t) { return ""; }  //!< \brief Default implementation
string AbstractPrinter::genStringPostFrameIMPL(size_t) { return ""; } //!< \brief Default implementation

/*!
 * \brief Generates a string for the frame information (prefix)
 * \param frameNum The number off the frame to process
 * \returns The generated string
 */
std::string tracer::AbstractPrinter::genStringPreFrame(size_t frameNum) {
  if (!trace)
    return "";

  return genStringPreFrameIMPL(frameNum);
}

/*!
 * \brief Generates a string for the frame information
 * \param frameNum The number off the frame to process
 * \returns The generated string
 */
std::string tracer::AbstractPrinter::genStringForFrame(size_t frameNum) {
  if (!trace)
    return "";

  return genStringForFrameIMPL(frameNum);
}

/*!
 * \brief Generates a string for the frame information (suffix)
 * \param frameNum The number off the frame to process
 * \returns The generated string
 */
std::string tracer::AbstractPrinter::genStringPostFrame(size_t frameNum) {
  if (!trace)
    return "";

  return genStringPostFrameIMPL(frameNum);
}


/*!
 * \brief Prints the stack trace to a file
 * \param file   The file to print to
 * \param append Whether to append the stack trace to the file or not
 *
 * \note The contents of file will be removed when append is false.
 */
void tracer::AbstractPrinter::printToFile(std::string file, bool append) {
  if (!trace)
    return;

  auto mode = ios_base::app | ios_base::out;

  if (!append)
    mode |= ios_base::trunc;

  ofstream outStream(file, mode);

  if (!outStream.is_open())
    return;

  outStream << generateString() << endl;
  outStream.close();
}

//! \brief prints the stack trace to STDERR
void tracer::AbstractPrinter::printToStdErr() { cerr << generateString() << endl; }

//! \brief prints the stack trace to STDOUT
void tracer::AbstractPrinter::printToStdOut() { cout << generateString() << endl; }

/*!
 * \brief Sets the trace to print
 * \param t The trace to print
 *
 * \warning This function MUST be called before a trace string can be generated!
 */
void tracer::AbstractPrinter::setTrace(tracer::Tracer *t) {
  trace = t;
  setupTrace();
}
