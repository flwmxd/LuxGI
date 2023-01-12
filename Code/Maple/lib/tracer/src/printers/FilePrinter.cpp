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
//! \file FilePrinter.cpp

#include "tracerDef.hpp"
#include "FilePrinter.hpp"
#include "Tracer.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace tracer;
using namespace std;

FilePrinter::~FilePrinter() {}

#if !DISABLE_STD_FILESYSTEM
bool FilePrinter::findPath(unsigned int depth, fs::path current, fs::path &out, fs::path const &file) {
  if (depth > fCFG.maxRecursionDepth)
    return false;

  try {
    if (fs::exists(current / file)) {
      out = current;
      return true;
    }

    for (auto &i : fs::directory_iterator(current)) {
      if (fs::is_directory(i) && !fs::is_symlink(i)) {
        if (findPath(depth + 1, i, out, file)) {
          return true;
        }
      }
    }
  } catch (...) { return false; }

  return false;
}


/*!
 * \brief Searches a file
 * \param file The file to find
 *
 * Searches the file recursively
 *
 * \returns The file if found or an empty path if not
 */
fs::path FilePrinter::findFile(string file) {
  // Absolute paths are easy
  fs::path filePath(file);
  if (filePath.is_absolute()) {
    if (fs::exists(filePath)) {
      return fs::canonical(filePath).native();
    }
  }

  // Check the cache
  fs::path tempPath;
  for (auto &i : pathCache) {
    tempPath = i / filePath;
    if (fs::exists(tempPath)) {
      return fs::canonical(tempPath).native();
    }
  }

  // Search for the file in the filesystem
  fs::path dir;
  if (findPath(0, fs::current_path(), dir, filePath)) {
    pathCache.emplace_back(dir);
    tempPath = dir / filePath;
    return fs::canonical(tempPath).native();
  }

  // Descend from current path to the root
  dir              = fs::current_path();
  fs::path rootDir = dir.root_directory();
  while (dir != rootDir) {
    tempPath = dir / filePath;
    if (fs::exists(tempPath)) {
      return fs::canonical(tempPath).native();
    }

    dir = dir.parent_path();
  }

  // Give up :(

  return "";
}
#endif


string FilePrinter::genStringPostFrameIMPL(size_t frameNum) {
  string fileStr;
  auto * frames = trace->getFrames();

  if (frameNum >= frames->size())
    return "";

  Frame &f = frames->at(frameNum);
  if ((f.flags & FrameFlags::HAS_LINE_INFO) != FrameFlags::HAS_LINE_INFO)
    return "";

  fileStr = f.fileName;

#if !DISABLE_STD_FILESYSTEM
  fs::path file;

  try {
    file = findFile(f.fileName);
  } catch (...) { return ""; }

  if (file.empty() || !fs::is_regular_file(file))
    return "";

  fileStr = file.string();
#endif

  ifstream fileStream(fileStr, ios::in);
  if (!fileStream.is_open())
    return "";

  char         buff[2048];
  stringstream outStream;
  outStream << setfill(' ') << left;
  for (uint32_t line = 1; !fileStream.eof(); ++line) {
    fileStream.getline(buff, sizeof(buff));
    if (line >= static_cast<uint32_t>(f.line) - fCFG.linesBefore) {
      char   marker = ' ';
      string color  = "";

      if (line == static_cast<uint32_t>(f.line)) {
        marker = '>';
        color  = fCFG.lineHighlightColor;
      }

      outStream << color << "  |  #" << setw(5) << line << marker << " | " << buff << "\x1b[0m" << endl;

      if (line == static_cast<uint32_t>(f.line) && f.column != 0) {
        outStream << "  | <MARKER> | \x1b[1;33m" << right << setw(f.column) << '^' << left << "\x1b[0m" << endl;
      }


      if (line >= static_cast<uint32_t>(f.line) + fCFG.linesAfter) {
        break;
      }
    }
  }

  return outStream.str();
}
