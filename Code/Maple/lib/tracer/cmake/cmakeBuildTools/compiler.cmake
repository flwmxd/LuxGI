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
#     * Neither the name of the <organization> nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Set compiler flags
#  1st parameter: the CMake compiler ID
# Sections:
# ALL:         c++ compiler options for all build types
# DEBUG:       c++ compiler options only for the DEBUG build type
# RELEASE:     c++ compiler options only for the RELEASE build type
# C_ALL:       c compiler options for all build types
# C_DEBUG:     c compiler options only for the DEBUG build type
# C_RELEASE:   c compiler options only for the RELEASE build type
# SANITIZE:    sanitizer option. Will only be enabled on DEBUG build type and when the variable ENABLE_SANITIZERS is set
# MIN_VERSION: the min. compiler version
function( add_compiler COMPILER )
  if( ${ARGC} LESS 1 )
    message( FATAL_ERROR "function 'add_compiler' needs at least 1 parameters" )
  endif( ${ARGC} LESS 1 )

  if( NOT CMAKE_CXX_COMPILER_ID STREQUAL COMPILER )
    return()
  endif( NOT CMAKE_CXX_COMPILER_ID STREQUAL COMPILER )

  message( STATUS "The detected compiler is: ${COMPILER}" )

  set( options )
  set( oneValueArgs   MIN_VERSION SANITIZE )
  set( multiValueArgs ALL DEBUG RELEASE C_ALL C_DEBUG C_RELEASE )
  cmake_parse_arguments( OPTS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

  if( DEFINED OPTS_MIN_VERSION )
    if( CMAKE_CXX_COMPILER_VERSION VERSION_LESS "${OPTS_MIN_VERSION}" )
        message( SEND_ERROR "Minimum compiler version is ${OPTS_MIN_VERSION} but the current version is ${CMAKE_CXX_COMPILER_VERSION}" )
    endif( CMAKE_CXX_COMPILER_VERSION VERSION_LESS "${OPTS_MIN_VERSION}" )
  endif( DEFINED OPTS_MIN_VERSION )

  if( ENABLE_SANITIZERS )
    set( OPTS_DEBUG "${OPTS_DEBUG};${OPTS_SANITIZE}" )
    message( STATUS "Using sanitizer(s) ${OPTS_SANITIZE} (change with -DENABLE_SANITIZERS)" )
  else( ENABLE_SANITIZERS )
    message( STATUS "Use no sanitizer(s) (change with -DENABLE_SANITIZERS)" )
  endif( ENABLE_SANITIZERS )

  # List --> string
  string( REGEX REPLACE ";" " " C_ALL     "${OPTS_C_ALL}"     )
  string( REGEX REPLACE ";" " " C_DEBUG   "${OPTS_C_DEBUG}"   )
  string( REGEX REPLACE ";" " " C_RELEASE "${OPTS_C_RELEASE}" )
  string( REGEX REPLACE ";" " " ALL       "${OPTS_ALL}"       )
  string( REGEX REPLACE ";" " " DEBUG     "${OPTS_DEBUG}"     )
  string( REGEX REPLACE ";" " " RELEASE   "${OPTS_RELEASE}"   )

  string( APPEND CMAKE_C_FLAGS           " ${C_ALL}"     )
  string( APPEND CMAKE_C_FLAGS_DEBUG     " ${C_DEBUG}"   )
  string( APPEND CMAKE_C_FLAGS_RELEASE   " ${C_RELEASE}" )
  string( APPEND CMAKE_CXX_FLAGS         " ${ALL}"       )
  string( APPEND CMAKE_CXX_FLAGS_DEBUG   " ${DEBUG}"     )
  string( APPEND CMAKE_CXX_FLAGS_RELEASE " ${RELEASE}"   )

  set( CMAKE_C_FLAGS           "${CMAKE_C_FLAGS}"           PARENT_SCOPE )
  set( CMAKE_C_FLAGS_DEBUG     "${CMAKE_C_FLAGS_DEBUG}"     PARENT_SCOPE )
  set( CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE}"   PARENT_SCOPE )
  set( CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS}"         PARENT_SCOPE )
  set( CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}"   PARENT_SCOPE )
  set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}" PARENT_SCOPE )

  message( "" )
endfunction( add_compiler )
