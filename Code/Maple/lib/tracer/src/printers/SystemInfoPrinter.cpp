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
//! \file SystemInfoPrinter.cpp

#include "tracerDef.hpp"
#include "SystemInfoPrinter.hpp"
#include <fstream>
#include <iostream>
#include <regex>

#if _WIN32
#include <VersionHelpers.h>
#include <strsafe.h>
#endif

#if USE_DWFL
#include <libdwfl.h>
#endif

#if USE_LIBUNWIND
#include <libunwind.h>
#endif

#if __linux__ || __unix__
#include <sys/utsname.h>
#endif

#if !DISABLE_STD_FILESYSTEM
#if __cplusplus <= 201402L
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif
#endif

using namespace tracer;
using namespace std;

SystemInfoPrinter::SystemInfoPrinter() { setupOsInfo(); }

void tracer::SystemInfoPrinter::setupOsInfo() {
#ifdef _WIN32
// clang-format off
#if 0
  if (false) {}
  else if (IsWindows10OrGreater())        { OS = "Windows 10";          }
  else if (IsWindows8Point1OrGreater())   { OS = "Windows 8.1";         }
  else if (IsWindows8OrGreater())         { OS = "Windows 8";           }
  else if (IsWindows7SP1OrGreater())      { OS = "Windows 7 SP1";       }
  else if (IsWindows7OrGreater())         { OS = "Windows 7";           }
  else if (IsWindowsVistaSP2OrGreater())  { OS = "Windows Vista SP2";   }
  else if (IsWindowsVistaSP1OrGreater())  { OS = "Windows Vista SP1";   }
  else if (IsWindowsVistaOrGreater())     { OS = "Windows Vista";       }
  else if (IsWindowsXPSP3OrGreater())     { OS = "Windows XP SP3";      }
  else if (IsWindowsXPSP2OrGreater())     { OS = "Windows XP SP2";      }
  else if (IsWindowsXPSP1OrGreater())     { OS = "Windows XP SP1";      }
  else if (IsWindowsXPOrGreater())        { OS = "Windows XP";          }
  else { OS = "Windows (older than Windows XP)"; }
#endif

  // This shit is required because MS thought it was a good idea to deprecate all
  // other useful version info functions
  DWORD length = GetFileVersionInfoSize("Kernel32.dll", nullptr);
  LPVOID rawData = malloc(length);

  struct LANGANDCODEPAGE {
    WORD wLanguage;
    WORD wCodePage;
  } *lpTranslate;

  UINT cbTranslate;

  BOOL retVal = GetFileVersionInfo("Kernel32.dll", 0, length, rawData);
  if (retVal != 0) {
    BOOL retVal2 = VerQueryValue(rawData, TEXT("\\VarFileInfo\\Translation"), (LPVOID*)&lpTranslate, &cbTranslate);

    if (retVal2) {
      for (unsigned int i = 0; i < (cbTranslate / sizeof(struct LANGANDCODEPAGE)); i++) {
        char SubBlock[100];
        char *outStr = nullptr;
        UINT len;

        StringCchPrintf(SubBlock, 100,
          TEXT("\\StringFileInfo\\%04x%04x\\ProductVersion"),
          lpTranslate[i].wLanguage,
          lpTranslate[i].wCodePage);

        VerQueryValue(rawData, SubBlock, (LPVOID *)&outStr, &len);
        if (outStr) {
          OS = string("Windows ") + outStr;
          break;
        }
      }
    }
  }


  if(IsWindowsServer()) { OS += " Server"; }
// clang-format on
#elif __APPLE__
#include "TargetConditionals.h"
#if TARGET_IPHONE_SIMULATOR
  OS = "iOS Simulator"; // iOS Simulator
#elif TARGET_OS_IPHONE
  OS = "iOS Device"; // iOS device
#elif TARGET_OS_MAC
  OS = "Mac OS"; // Other kinds of Mac OS
#else
  OS = "Unknown Apple platform";
#endif
#elif __linux__ || __unix__
  OS                    = "";
  bool releaseInfoFound = false;

  // 1st try output of lsb_release -a
  FILE *fd = popen("lsb_release -a 2>&1", "r");
  if (fd) {
    char   buff[1024];
    string lsbOut;

    while (fgets(buff, sizeof(buff), fd)) {
      lsbOut += buff;
    }

    pclose(fd);

    regex  descReg("Description:[ \t]*([^\n]*)");
    regex  codenameReg("Codename:[ \t]*([^\n]*)");
    string desc;

    smatch match;

    if (regex_search(lsbOut, match, descReg)) {
      OS += regex_replace(match.str(), descReg, "$1") + "";
      releaseInfoFound = true;
      if (regex_search(lsbOut, match, codenameReg)) {
        string codename = regex_replace(match.str(), codenameReg, "$1");
        if (codename != "n/a" && codename != "N/A") {
          OS += " " + codename;
        }
      }
    }
  }

// Try /etc/os-release if this fails
#if !DISABLE_STD_FILESYSTEM
  fs::path relInfoP("/etc/os-release");
  if (fs::exists(relInfoP) && !releaseInfoFound) {
#else
  if (!releaseInfoFound) {
#endif
    ifstream relInfoFile("/etc/os-release", ios::in);
    if (relInfoFile) {
      regex  nameReg("NAME=\"([^\"]*)\"");
      regex  prettyNameReg("PRETTY_NAME=\"([^\"]*)\"");
      string content((std::istreambuf_iterator<char>(relInfoFile)), std::istreambuf_iterator<char>());

      smatch match;
      if (regex_search(content, match, prettyNameReg)) {
        OS += regex_replace(match.str(), prettyNameReg, "$1");
        releaseInfoFound = true;
      } else if (regex_search(content, match, nameReg)) {
        OS += regex_replace(match.str(), nameReg, "$1");
        releaseInfoFound = true;
      }
    }
  }

  // Add Uname Data
  utsname unameData;
  uname(&unameData);

  if (releaseInfoFound)
    OS += " (";

  OS += unameData.sysname;
  OS += " ";
  OS += unameData.release;
  OS += " ";
  OS += unameData.machine;

  if (releaseInfoFound)
    OS += ")";
#elif defined(_POSIX_VERSION)
  OS = "Some POSIX OS";
#endif

  addSystemEntry({"Operating System", OS});

#if USE_DWFL
  addSystemEntry({"LibDWFL", string(dwfl_version(nullptr))});
#endif

#if USE_LIBUNWIND
#define TO_STR2(x) #x
#define TO_STR1(x) TO_STR2(x)
#define UNW_DOT .
#define UNW_VERS_RAW1 UNW_PASTE(UNW_VERSION_MAJOR, UNW_DOT)
#define UNW_VERS_RAW2 UNW_PASTE(UNW_VERS_RAW1, UNW_VERSION_MINOR)
#define UNW_VERS_RAW3 TO_STR1(UNW_VERS_RAW2)
  addSystemEntry({"LibUnwind", UNW_VERS_RAW3});
#endif
}

/*!
 * \brief Converts a signal number to a string
 * \param sNum The number to convert
 * \returns The resulting string
 */
string SystemInfoPrinter::sigNum2Str(int sNum) {
  switch (sNum) {
#if _WIN32
    case SIGINT: return "SIGINT";
    case SIGILL: return "SIGILL";
    case SIGABRT: return "SIGABRT";
    case SIGFPE: return "SIGFPE";
    case SIGSEGV: return "SIGSEGV";
    case SIGTERM: return "SIGTERM";
#else
    case SIGHUP: return "SIGHUP";
    case SIGINT: return "SIGINT";
    case SIGQUIT: return "SIGQUIT";
    case SIGILL: return "SIGILL";
    case SIGTRAP: return "SIGTRAP";
    case SIGABRT: return "SIGABRT";
    case SIGBUS: return "SIGBUS";
    case SIGFPE: return "SIGFPE";
    case SIGKILL: return "SIGKILL";
    case SIGUSR1: return "SIGUSR1";
    case SIGSEGV: return "SIGSEGV";
    case SIGUSR2: return "SIGUSR2";
    case SIGPIPE: return "SIGPIPE";
    case SIGALRM: return "SIGALRM";
    case SIGTERM: return "SIGTERM";
    case SIGCHLD: return "SIGCHLD";
    case SIGCONT: return "SIGCONT";
    case SIGSTOP: return "SIGSTOP";
    case SIGTSTP: return "SIGTSTP";
    case SIGTTIN: return "SIGTTIN";
    case SIGTTOU: return "SIGTTOU";
    case SIGURG: return "SIGURG";
    case SIGXCPU: return "SIGXCPU";
    case SIGXFSZ: return "SIGXFSZ";
    case SIGVTALRM: return "SIGVTALRM";
    case SIGPROF: return "SIGPROF";
    case SIGWINCH: return "SIGWINCH";
    case SIGIO: return "SIGIO";
    case SIGSYS: return "SIGSYS";
#endif
    default: return "Invalid signal number";
  }
}

string SystemInfoPrinter::genStringPreFrameIMPL(size_t frameNum) {
  if (frameNum != 0)
    return "";

  string outStr = "\x1b[33mGenerating backtrace\x1b[0m";

  if (sigNum != _NSIG)
    outStr += " \x1b[1m[Received Signal \x1b[31;1m" + sigNum2Str(sigNum) + "\x1b[39;1m]\x1b[0m";

  outStr += "\n\n\x1b[33mSystem Information:\x1b[0m\n";

  size_t maxNameLength = 0;
  for (auto &i : entries)
    if (i.name.length() > maxNameLength)
      maxNameLength = i.name.length();


  for (auto &i : entries) {
    outStr += "  \x1b[1;34m-->\x1b[0m \x1b[32m" + i.name + ":";
    outStr.append(maxNameLength - i.name.length() + 3, ' ');
    if (i.value.empty()) {
      outStr += "\x1b[1;33m<UNKNOWN>\x1b[0m\n";
    } else {
      outStr += "\x1b[1;36m" + i.value + "\x1b[0m\n";
    }
  }

  outStr += "\n\x1b[33mBacktrace:\x1b[0m\n";

  return outStr;
}
