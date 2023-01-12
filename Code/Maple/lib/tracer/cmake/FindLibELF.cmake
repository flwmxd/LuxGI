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
  LibELF_INC
    NAMES dwarf.h
    HINTS ${LibELF_ROOT}/include
)

find_path(
  LibELF_INC2
    NAMES libdwfl.h
    HINTS
      /usr/include/elfutils
      ${LibELF_ROOT}/include/elfutils
)

find_library(
  LibELF_DW_LIB
    NAMES dw
    HINTS
      /usr/lib
      ${LibELF_ROOT}/lib
)

find_library(
  LibELF_ELF_LIB
    NAMES elf
    HINTS
      /usr/lib
      ${LibELF_ROOT}/lib
)

# find_library(
#   LibELF_ASM_LIB
#     NAMES asm
#     HINTS
#       /usr/lib
#       ${LibELF_ROOT}/lib
# )

set( REPLACE_PREFIX ".*#define[ \t\n]+UNW_VERSION_" )
set( REPLACE_SUFFIX "[ \t\n]+([0-9\.]*).*" )

find_package_handle_standard_args(
  LibELF
  REQUIRED_VARS LibELF_INC LibELF_INC2 LibELF_DW_LIB LibELF_ELF_LIB # LibELF_ASM_LIB
)

if( LibELF_FOUND )
  set( LibELF_INCLUDE_DIRS ${LibELF_INC} ${LibELF_INC2} )
  set( LibELF_LIBRARIES    ${LibELF_ASM_LIB} ${LibELF_DW_LIB} ${LibELF_ELF_LIB} )

  foreach( I IN ITEMS DW ELF ) # ASM
    if( NOT TARGET LibELF::${I} )
      add_library(LibELF::${I} UNKNOWN IMPORTED)
      set_target_properties( LibELF::${I}
        PROPERTIES
          INTERFACE_INCLUDE_DIRECTORIES "${LibELF_INCLUDE_DIRS}"
          IMPORTED_LOCATION             "${LibELF_${I}_LIB}"
      )
    endif( NOT TARGET LibELF::${I} )
  endforeach( I IN ITEMS DW ELF )
endif()
