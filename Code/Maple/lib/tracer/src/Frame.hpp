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
//! \file Frame.hpp

#pragma once

#include "tracerDef.hpp"

namespace tracer {

//! \brief Flags that describe which fields of the Frame struct are set
enum class FrameFlags : uint16_t {
  HAS_ADDRESS     = 0b0001, // Should always be set
  HAS_FUNC_NAME   = 0b0010,
  HAS_LINE_INFO   = 0b0100,
  HAS_MODULE_INFO = 0b1000,

  NONE = 0
};

// clang-format off
inline FrameFlags operator~(FrameFlags a)                  { return static_cast<FrameFlags>(~static_cast<uint16_t>(a));                                          } //!< \brief operator for FrameFlags
inline FrameFlags operator|(FrameFlags a, FrameFlags b)    { return static_cast<FrameFlags>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));                } //!< \brief operator for FrameFlags
inline FrameFlags operator&(FrameFlags a, FrameFlags b)    { return static_cast<FrameFlags>(static_cast<uint16_t>(a) & static_cast<uint16_t>(b));                } //!< \brief operator for FrameFlags
inline FrameFlags operator^(FrameFlags a, FrameFlags b)    { return static_cast<FrameFlags>(static_cast<uint16_t>(a) ^ static_cast<uint16_t>(b));                } //!< \brief operator for FrameFlags
inline FrameFlags &operator|=(FrameFlags &a, FrameFlags b) { return reinterpret_cast<FrameFlags &>(reinterpret_cast<uint16_t &>(a) |= static_cast<uint16_t>(b)); } //!< \brief operator for FrameFlags
inline FrameFlags &operator&=(FrameFlags &a, FrameFlags b) { return reinterpret_cast<FrameFlags &>(reinterpret_cast<uint16_t &>(a) &= static_cast<uint16_t>(b)); } //!< \brief operator for FrameFlags
inline FrameFlags &operator^=(FrameFlags &a, FrameFlags b) { return reinterpret_cast<FrameFlags &>(reinterpret_cast<uint16_t &>(a) ^= static_cast<uint16_t>(b)); } //!< \brief operator for FrameFlags
// clang-format on

//! \brief Describes a frame
struct Frame {
  FrameFlags flags = FrameFlags::NONE; //!< \brief Describes which fields are set

  Address     frameAddr = 0; //!< \brief The instruction pointer of the stack frame
  std::string funcName;      //!< \brief The name of the function
  std::string moduleName;    //!< \brief The name / path of the binary file
  std::string fileName;      //!< \brief The name / path of the source file
  int         line   = 0;    //!< \brief The line number in the source file
  int         column = 0;    //!< \brief The column in the source file
};
}
