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
//! \file tracerInternal.hpp

#pragma once

#include "tracerDef.hpp"
#include "TracerHandler.hpp"

//! \brief Internal C wrapper structure
struct tr_TracerHandler {
  tracer::TracerHandler *       handler = nullptr; //!< \brief TracerHandler pointer
  tracer::TracerHandler::Config cfg;               //!< \brief TracerHandler config
};

//! \brief Internal C wrapper structure
struct tr_PrinterContainer {
  tracer::PrinterContainer obj; //!< PrinterContainer object
};

//! \brief Internal C wrapper structure
struct tr_Tracer {
  tracer::Tracer  tracer; //!< \brief Tracer object
  tracer::Tracer *tPTR;   //!< \brief Pointer to the tracer object to use

  tr_Tracer() {}                                                                   //!< \brief constructor
  tr_Tracer(tracer::TraceerEngines e, tracer::DebuggerEngines d) : tracer(e, d) {} //!< \brief constructor
};
