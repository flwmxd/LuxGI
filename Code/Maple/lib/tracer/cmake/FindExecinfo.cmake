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

find_package( DLFCN )

find_path(
  Execinfo_INC
    NAMES execinfo.h
    HINTS ${Execinfo_ROOT}/include
)

find_library(
  Execinfo_LIB
    NAMES c
    HINTS
      /usr/lib
      ${LibBFD_ROOT}/lib
)

find_package_handle_standard_args(
  Execinfo
  REQUIRED_VARS Execinfo_INC Execinfo_LIB DLFCN_FOUND
)

if( Execinfo_FOUND )
  set( Execinfo_INCLUDE_DIRS ${Execinfo_INC} )
  set( Execinfo_LIBRARIES    ${Execinfo_LIB})

  if( NOT TARGET Execinfo::Execinfo )
    add_library(Execinfo::Execinfo UNKNOWN IMPORTED)
    set_target_properties( Execinfo::Execinfo
      PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${Execinfo_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES      "${DLFCN_LIBRARIES}"
        IMPORTED_LOCATION             "${Execinfo_LIBRARIES}"
    )
  endif( NOT TARGET Execinfo::Execinfo )
endif()
