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

#include "tracerDef.hpp"
#include "TracerHandler.hpp"
#include <iostream>
#include <signal.h>

#ifndef _WIN32
#include <unistd.h>
#endif

using namespace std;
using namespace tracer;


int f1(int x);
int f2(int x);
int f3(int x);
int f4(int x);
int f5(int x);

int f1(int x) { return f2(++x); }
int f2(int x) { return f3(++x); }
int f3(int x) { return f4(++x); }
int f4(int x) { return f5(++x); }
int f5(int x) {
  int  i = 0;
  int *p = &i;
  p      = nullptr;
  *p     = x; // SEGFAULT here
  return i;
}

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  //   TracerHandler::getTracer()->defaultSetup();

  auto *tHandler = TracerHandler::getTracer(); // Get the singelton
  auto  cfg      = tHandler->getConfig();      // Get the current config
  // Edit cfg
  tHandler->setConfig(cfg);                 // Update the config
  auto printer = PrinterContainer::fancy(); // Generates a printer
  // Edit printer config
  tHandler->setup(std::move(printer)); // Sets things up. Now the signal handler is setup

  f1(42);

  return 0;
}
