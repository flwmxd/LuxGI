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
//! \file PrinterContainer.cpp

#include "tracerDef.hpp"
#include "PrinterContainer.hpp"

using namespace tracer;
using namespace std;


/*!
 * \brief Constructor sets the printer
 * \param p The printer to store
 */
PrinterContainer::PrinterContainer(AbstractPrinter *p) : printer(p) {}
PrinterContainer::~PrinterContainer() {
  if (printer)
    delete printer;
}

/*!
 * \brief Move constructor
 * \param moveFrom object to move from
 */
PrinterContainer::PrinterContainer(PrinterContainer &&moveFrom) {
  printer          = moveFrom.printer;
  moveFrom.printer = nullptr;
}

/*!
 * \brief Move assignment operator
 * \param moveFrom Object to move from
 */
PrinterContainer &PrinterContainer::operator=(PrinterContainer &&moveFrom) {
  if (this != &moveFrom) {
    printer          = moveFrom.printer;
    moveFrom.printer = nullptr;
  }
  return *this;
}

/*!
 * \brief Returns a container with a tracer::FancyPrinter object
 * \returns Returns the constructed container
 */
PrinterContainer PrinterContainer::fancy() { return PrinterContainer(new FancyPrinter); }

/*!
 * \brief Returns a container with a tracer::FilePrinter object
 * \returns Returns the constructed container
 */
PrinterContainer PrinterContainer::file() { return PrinterContainer(new FilePrinter); }

/*!
 * \brief Returns a container with a tracer::SystemInfoPrinter object
 * \returns Returns the constructed container
 */
PrinterContainer PrinterContainer::system() { return PrinterContainer(new SystemInfoPrinter); }

/*!
 * \brief Returns a container with a tracer::DefaultPrinter object
 * \returns Returns the constructed container
 */
PrinterContainer PrinterContainer::plain() { return PrinterContainer(new DefaultPrinter); }

AbstractPrinter *PrinterContainer::get() { return printer; }        //!< \returns the stored printer
AbstractPrinter *PrinterContainer::operator()() { return printer; } //!< \returns the stored printer
AbstractPrinter *PrinterContainer::operator->() { return printer; } //!< \returns the stored printer
