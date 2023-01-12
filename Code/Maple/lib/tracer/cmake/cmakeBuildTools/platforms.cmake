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

# Adds a operating system with (multiple) graphic API's
#
# Usage:
# add_platform
#    OS     <the operating system>
#    TARGET <supported targets (aka subdirectories in the source tree) of the OS>
#
# Variables:
#  PLATFORM_TARGET               - the secondary target of one OS
#  ${PROJECT_NAME}_PLATFORM_LIST - list of all platforms added so far (output)
function( add_platform )
  set( options )
  set( oneValueArgs   OS )
  set( multiValueArgs TARGET  )
  cmake_parse_arguments( OPTS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

  if( "${OPTS_OS}" STREQUAL "" )
    set( OPTS_OS "ALL" )
  endif( "${OPTS_OS}" STREQUAL "" )

  if( ${OPTS_OS} OR "${OPTS_OS}" STREQUAL "ALL" )
    set( CURRENT_OS_STRING " (current)" )
  endif( ${OPTS_OS} OR "${OPTS_OS}" STREQUAL "ALL" )

  # Generate the platform list
  message( STATUS "Adding platform support for ${OPTS_OS}${CURRENT_OS_STRING}. Supported targets are:" )

  # Calculate filler whitespaces
  set( MAX_LENGTH 0 )
  foreach( I IN LISTS OPTS_TARGET )
    string( LENGTH "${I}" CUR_LENGTH )
    if( CUR_LENGTH GREATER MAX_LENGTH )
      set( MAX_LENGTH ${CUR_LENGTH} )
    endif( CUR_LENGTH GREATER MAX_LENGTH )
  endforeach( I IN LISTS OPTS_TARGET )

  foreach( I IN LISTS OPTS_TARGET )
    string( LENGTH "${I}" CUR_LENGTH )
    set( SPACES "" )
    math( EXPR NSPACES "${MAX_LENGTH} - ${CUR_LENGTH}" )
    foreach( J RANGE 0 ${NSPACES} )
      string( APPEND SPACES " " )
    endforeach( J RANGE 0 ${NSPACES} )

    string( TOUPPER "${OPTS_OS}_${I}" VAR_NAME )
    message( STATUS " - ${I}: ${SPACES} ${VAR_NAME}" )
    list( APPEND PLATFORM_LIST ${VAR_NAME} )
    set( ${VAR_NAME} "${I}" PARENT_SCOPE ) # store <api> in <OPTS_OS>_<API> for the find sources script
  endforeach( I IN LISTS OPTS_TARGET )

  # Set default target
  if( ${OPTS_OS} )
    if( NOT PLATFORM_TARGET )
      list( GET OPTS_TARGET 0 TEMP )
      string( TOUPPER "${OPTS_OS}_${TEMP}" VAR_NAME )
      set( PLATFORM_TARGET ${VAR_NAME} PARENT_SCOPE )
    endif( NOT PLATFORM_TARGET )
  endif( ${OPTS_OS} )

  # Export to parent scope
  list( APPEND ${PROJECT_NAME}_PLATFORM_LIST "${PLATFORM_LIST}" )
  set( ${PROJECT_NAME}_PLATFORM_LIST ${${PROJECT_NAME}_PLATFORM_LIST} PARENT_SCOPE )
endfunction( add_platform )


function( check_platform )
  message( "" )
  message( STATUS "Checking enabled platforms / targets (change with -DPLATFORM_TARGET):" )

  foreach( I IN LISTS ${PROJECT_NAME}_PLATFORM_LIST )
    set( CM_${I} 0 PARENT_SCOPE )
  endforeach( I IN LISTS ${PROJECT_NAME}_PLATFORM_LIST )

  foreach( I IN LISTS PLATFORM_TARGET )
    if( I IN_LIST ${PROJECT_NAME}_PLATFORM_LIST )
      set( CM_${I} 1 PARENT_SCOPE )
      message( STATUS " - Using target ${I}" )
    else( I IN_LIST ${PROJECT_NAME}_PLATFORM_LIST )
      message( FATAL_ERROR " - Unknown target ${I}" )
    endif( I IN_LIST ${PROJECT_NAME}_PLATFORM_LIST )
  endforeach( I IN LISTS PLATFORM_TARGET )

  message( "" )
endfunction( check_platform )
