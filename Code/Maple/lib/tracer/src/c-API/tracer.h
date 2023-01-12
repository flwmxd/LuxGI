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
//! \file tracer.h

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if UINTPTR_MAX == 0xffffffff // 32-bit
typedef uint32_t Address;
#elif UINTPTR_MAX == 0xffffffffffffffff // 64-bit
typedef uint64_t Address;
#else                                   // wtf
#error "Could not detect address legth"
#endif

struct tr_TracerHandler;    //!< \brief Private struct to manage TracerHandler
struct tr_Tracer;           //!< \brief Private struct to manage Tracer
struct tr_PrinterContainer; //!< \brief Private struct to manage PrinterContainer

//! \brief Define some our own boolean values
enum TR_BOOL { TR_FALSE, TR_TRUE };

//! \brief Wrapper for tracer::TracerEngines
enum TR_TraceerEngines { TR_E_END = 0, TR_LIBUNWIND, TR_GLIBC, TR_WIN32_TRACER, TR_DUMMY_T };

//! \brief Wrapper for tracer::DebuggerEngines
enum TR_DebuggerEngines { TR_D_END = 0, TR_LIBDWFL, TR_LIBBFD, TR_WIN32_INFO, TR_EXTERNAL_FALLBACK, TR_DUMMY_D };

typedef enum TR_BOOL            TR_BOOL_t;            //!< \brief Define some our own boolean values
typedef enum TR_TraceerEngines  TR_TraceerEngines_t;  //!< \brief Wrapper for tracer::TracerEngines
typedef enum TR_DebuggerEngines TR_DebuggerEngines_t; //!< \brief Wrapper for tracer::DebuggerEngines

//! \brief C user callback for the signal handler
typedef void (*tr_callBackPTR)(void *tracer, void *printer, void *userData);

//! \brief Configuration
struct tr_TracerHandler_Config {
  TR_TraceerEngines_t  tracer[8];    //!< List of preferred engines; First try them for the Tracer (0 marks the end)
  TR_DebuggerEngines_t debuggers[8]; //!< List of preferred engines; First try them for the Tracer (0 marks the end)
  TR_BOOL_t            autoPrintToStdErr; //!< Prints the stack trace to stderr when enabled
  TR_BOOL_t            autoPrintToFile;   //!< Automatically writes the stack trace to a file when enabled
  char                 logFile[1024];     //!< The file to automatically print to; Requires autoPrintToFile
  TR_BOOL_t            appendToFile;      //!< Overides file co; Requires autoPrintToFile

  tr_callBackPTR callback;     //!< Function pointer to be called in the internal signal handler (MUST return)
  void *         callbackData; //!< User defined data to be send to the callback function

  TR_BOOL_t callDefultHandlerWhenDone; //!< The signal handler will call the default signal handler when done

  int signums[64]; //!< List of signals to handle (0 marks the end of the list)
};

//! \brief User configuration structure
struct tr_DefaultPrinter_Config {
  char prefix[64]; //!< \brief Perefix (prefix [functionName])
  char seper1[64]; //!< \brief 1st seperator
  char seper2[64]; //!< \brief 2nd seperator
  char seper3[64]; //!< \brief 3rd seperator
  char suffix[64]; //!< \brief suffix

  char colorFrameNum[64]; //!< \brief ANSI escape sequence for the Frame number color
  char colorNotFound[64]; //!< \brief ANSI escape sequence for the "Not Found" color
  char colorAddress[64];  //!< \brief ANSI escape sequence for the Address color
  char colorFuncName[64]; //!< \brief ANSI escape sequence for the Function Name color
  char colorLineInfo[64]; //!< \brief ANSI escape sequence for the Line information color
  char colorModule[64];   //!< \brief ANSI escape sequence for the Frame number color

  TR_BOOL_t shortenFiles;      //!< \brief The source file path
  TR_BOOL_t shortenModules;    //!< \brief The executable module (.so/.dll/.exe)
  TR_BOOL_t canonicalizePaths; //!< \brief Fixes path names if they contain "/../" or are relative
};

//! \brief Represents a stack frame
struct tr_Frame {
  Address     frameAddr;  //!< \brief The instruction pointer of the stack frame
  const char *funcName;   //!< \brief The name of the function
  const char *moduleName; //!< \brief The name / path of the binary file
  const char *fileName;   //!< \brief The name / path of the source file
  int         line;       //!< \brief The line number in the source file
  int         column;     //!< \brief The column in the source file
};

//! \brief Helper structure for wrapping the small std::vector's
struct tr_Tracer_AvailableEngines {
  TR_TraceerEngines_t  tracer[8];    //!< List of available engines; First try them for the Tracer (0 marks the end)
  TR_DebuggerEngines_t debuggers[8]; //!< List of available engines; First try them for the Tracer (0 marks the end)
};

typedef struct tr_TracerHandler           tr_TracerHandler_t;           //!< \brief Manages TracerHandler
typedef struct tr_TracerHandler_Config    tr_TracerHandler_Config_t;    //!< \brief Configuration
typedef struct tr_Tracer                  tr_Tracer_t;                  //!< \brief Private struct to manage Tracer
typedef struct tr_Tracer_AvailableEngines tr_Tracer_AvailableEngines_t; //!< \brief Typedef to make things easier in c
typedef struct tr_PrinterContainer        tr_Printer_t;                 //!< \brief Manages PrinterContainer
typedef struct tr_Frame                   tr_Frame_t;                   //!< \brief Represents a stack frame
typedef struct tr_DefaultPrinter_Config   tr_DefaultPrinter_Config_t;   //!< \brief User configuration structure

void tr_defaultSetup();

/* TracerHandler */

tr_TracerHandler_t *tr_getTracerHandler();
void tr_freeTracerHandler(tr_TracerHandler_t *trTH);
void tr_TracerHandler__reset();

TR_BOOL_t tr_TracerHandler__defaultSetup(tr_TracerHandler_t *trTH);
TR_BOOL_t tr_TracerHandler__setup(tr_TracerHandler_t *trTH, tr_Printer_t *pContainer);

tr_TracerHandler_Config_t tr_TracerHandler__getConfig(tr_TracerHandler_t *trTH);
void tr_TracerHandler__setConfig(tr_TracerHandler_t *trTH, tr_TracerHandler_Config_t cfg);

/* Tracer */

tr_Tracer_t *tr_getTracer();
tr_Tracer_t *tr_getTracerWithParam(TR_TraceerEngines_t tracer, TR_DebuggerEngines_t debugInfo);
tr_Tracer_t *tr_getTracerFromVoid(void *tVoid);

void tr_Tracer__trace(tr_Tracer_t *tracer);
size_t tr_Tracer__getNumFrames(tr_Tracer_t *tracer);
tr_Frame_t tr_Tracer__getFrame(tr_Tracer_t *tracer, size_t frameNum);

tr_Tracer_AvailableEngines_t tr_Tracer__getAvailableEngines();

void tr_freeTracer(tr_Tracer_t *tracer);

/* PrinterContainer */

tr_Printer_t *tr_getPrinter(void *printer);
tr_Printer_t *tr_getPrinter__fancy();
tr_Printer_t *tr_getPrinter__file();
tr_Printer_t *tr_getPrinter__system();
tr_Printer_t *tr_getPrinter__plain();

void *tr_PrinterContainer__get(tr_Printer_t *pContainer);

void tr_freePrinter(tr_Printer_t *pContainer);

/* Printer */

/*
 * ALL Printers are accessed with tr_Printer functions.
 * If a function is not supported by a printer then castOK is set to TR_FALSE
 */

void tr_Printer__genStringPreFrame(tr_Printer_t *p, size_t frameNum, char **outStr, TR_BOOL_t *castOK);
void tr_Printer__genStringForFrame(tr_Printer_t *p, size_t frameNum, char **outStr, TR_BOOL_t *castOK);
void tr_Printer__genStringPostFrame(tr_Printer_t *p, size_t frameNum, char **outStr, TR_BOOL_t *castOK);
void tr_Printer__generateString(tr_Printer_t *p, char **outStr, TR_BOOL_t *castOK);

void tr_Printer__printToFile(tr_Printer_t *p, const char *file, TR_BOOL_t append, TR_BOOL_t *castOK);
void tr_Printer__printToStdOut(tr_Printer_t *p, TR_BOOL_t *castOK);
void tr_Printer__printToStdErr(tr_Printer_t *p, TR_BOOL_t *castOK);

void tr_Printer__enableColor(tr_Printer_t *p, TR_BOOL_t *castOK);
void tr_Printer__disableColor(tr_Printer_t *p, TR_BOOL_t *castOK);

void tr_Printer__setTrace(tr_Printer_t *p, tr_Tracer_t *t, TR_BOOL_t *castOK);

tr_DefaultPrinter_Config_t tr_Printer__getConfig(tr_Printer_t *p, TR_BOOL_t *castOK);
void tr_Printer__setConfig(tr_Printer_t *p, tr_DefaultPrinter_Config_t cfg, TR_BOOL_t *castOK);

void tr_Printer__setSignum(tr_Printer_t *p, int signum, TR_BOOL_t *castOK);
void tr_Printer__addSystemEntry(tr_Printer_t *p, const char *name, const char *value, TR_BOOL_t *castOK);

#ifdef __cplusplus
}
#endif
