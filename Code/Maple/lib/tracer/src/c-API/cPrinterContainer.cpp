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
//! \file cPrinterContainer.cpp

#include "tracer.h"
#include "tracerDef.hpp"
#include "tracerInternal.hpp"
#include <string.h>

using namespace tracer;
using namespace std;

extern "C" {

//! \brief Manages the Printer
tr_Printer_t *tr_getPrinter(void *printer) {
  if (!printer)
    return nullptr;

  tr_Printer_t *cont = new tr_Printer_t{PrinterContainer(static_cast<AbstractPrinter *>(printer))};
  return cont;
}

//! \brief Wrapper for tracer::PrinterContainer::fancy
tr_Printer_t *tr_getPrinter__fancy() { return new tr_Printer_t{PrinterContainer::fancy()}; }

//! \brief Wrapper for tracer::PrinterContainer::file
tr_Printer_t *tr_getPrinter__file() { return new tr_Printer_t{PrinterContainer::file()}; }

//! \brief Wrapper for tracer::PrinterContainer::system
tr_Printer_t *tr_getPrinter__system() { return new tr_Printer_t{PrinterContainer::system()}; }

//! \brief Wrapper for tracer::PrinterContainer::plain
tr_Printer_t *tr_getPrinter__plain() { return new tr_Printer_t{PrinterContainer::plain()}; }

//! \brief Wrapper for tracer::PrinterContainer::get
void *tr_PrinterContainer__get(tr_Printer_t *pContainer) {
  if (!pContainer)
    return nullptr;

  return pContainer->obj.get();
}

//! \brief Frees the Printer container
void tr_freePrinter(tr_Printer_t *pContainer) {
  if (pContainer)
    delete pContainer;
}
}
