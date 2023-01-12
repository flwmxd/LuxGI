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

#include "tracer.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
  p      = NULL;
  *p     = x; // SEGFAULT here
  return i;
}

void callback(void *tracer, void *printer, void *ctx) {
  (void)tracer;
  (void)printer;

  tr_Tracer_t * t       = tr_getTracerFromVoid(tracer);
  tr_Printer_t *p       = tr_getPrinter(printer);
  size_t        nFrames = tr_Tracer__getNumFrames(t);

  printf("SEGFAULT!!! %li -- FRAMES: %li\n", (uint64_t)ctx, nFrames);

  if (nFrames >= 4) {
    tr_Frame_t f = tr_Tracer__getFrame(t, 3);
    if (f.funcName)
      printf("%s\n", f.funcName);
  }

  TR_BOOL_t stat[5];
  tr_Printer__setTrace(p, t, &stat[0]);

  tr_DefaultPrinter_Config_t cfg = tr_Printer__getConfig(p, &stat[1]);
  strcat(cfg.colorNotFound, "\x1b[41m");
  cfg.shortenFiles = TR_TRUE;
  tr_Printer__setConfig(p, cfg, &stat[2]);
  tr_Printer__addSystemEntry(p, "Test (segfault)", "v0.0.1", &stat[3]);
  tr_Printer__printToStdOut(p, &stat[4]);

  printf("Printer cast status: %i %i %i %i %i\n\n", stat[0], stat[1], stat[2], stat[3], stat[4]);
}

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  tr_TracerHandler_t *      trTH = tr_getTracerHandler();
  tr_TracerHandler_Config_t cfg  = tr_TracerHandler__getConfig(trTH);

  cfg.callback                  = callback;
  cfg.callbackData              = (void *)42;
  cfg.callDefultHandlerWhenDone = TR_TRUE;
  tr_TracerHandler__setConfig(trTH, cfg);

  tr_Printer_t *systemPrinter = tr_getPrinter__system();
  tr_TracerHandler__setup(trTH, systemPrinter);
  tr_freePrinter(systemPrinter);
  tr_freeTracerHandler(trTH);

  f1(42);

  return 0;
}
