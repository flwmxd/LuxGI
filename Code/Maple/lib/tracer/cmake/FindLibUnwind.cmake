# Copyright (c) 2017, Daniel Mensinger
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the Daniel Mensinger nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL Daniel Mensinger LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

include(FindPackageHandleStandardArgs)

find_path(
  LibUnwind_INC
    NAMES libunwind-common.h
    HINTS ${LibUnwind_ROOT}/include
)

find_library(
  LibUnwind_LIB
    NAMES unwind
    HINTS
      /usr/lib
      ${LibUnwind_ROOT}/lib
)

set( REPLACE_PREFIX ".*#define[ \t\n]+UNW_VERSION_" )
set( REPLACE_SUFFIX "[ \t\n]+([0-9\.]*).*" )

if( LibUnwind_INC AND EXISTS "${LibUnwind_INC}/libunwind-common.h" )
  file( READ "${LibUnwind_INC}/libunwind-common.h" LUW_INC_FILE )
  string( REGEX REPLACE "${REPLACE_PREFIX}MAJOR${REPLACE_SUFFIX}" "\\1" LibUnwind_VERSION_MAJOR "${LUW_INC_FILE}" )
  string( REGEX REPLACE "${REPLACE_PREFIX}MINOR${REPLACE_SUFFIX}" "\\1" LibUnwind_VERSION_MINOR "${LUW_INC_FILE}" )
  string( REGEX REPLACE "${REPLACE_PREFIX}EXTRA${REPLACE_SUFFIX}" "\\1" LibUnwind_VERSION_PATCH "${LUW_INC_FILE}" )

  # Work around version bug
  if( "${LibUnwind_VERSION_PATCH}" STREQUAL "" )

  endif( "${LibUnwind_VERSION_PATCH}" STREQUAL "" )
    string( REGEX REPLACE "[0-9]\." "" LibUnwind_VERSION_PATCH "${LibUnwind_VERSION_MINOR}" )
    string( REGEX REPLACE "\.[0-9]" "" LibUnwind_VERSION_MINOR "${LibUnwind_VERSION_MINOR}" )
  set( LibUnwind_VERSION "${LibUnwind_VERSION_MAJOR}.${LibUnwind_VERSION_MINOR}.${LibUnwind_VERSION_PATCH}" )
endif( LibUnwind_INC AND EXISTS "${LibUnwind_INC}/libunwind-common.h" )

find_package_handle_standard_args(
  LibUnwind
  VERSION_VAR LibUnwind_VERSION
  REQUIRED_VARS LibUnwind_INC LibUnwind_LIB
)

if( LibUnwind_FOUND )
  set( LibUnwind_INCLUDE_DIRS ${LibUnwind_INC} )
  set( LibUnwind_LIBRARIES    ${LibUnwind_LIB} )

  if( NOT TARGET LibUnwind::LibUnwind )
    add_library(LibUnwind::LibUnwind UNKNOWN IMPORTED)
    set_target_properties( LibUnwind::LibUnwind
      PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${LibUnwind_INCLUDE_DIRS}"
        IMPORTED_LOCATION             "${LibUnwind_LIBRARIES}"
    )
  endif( NOT TARGET LibUnwind::LibUnwind)
endif()
