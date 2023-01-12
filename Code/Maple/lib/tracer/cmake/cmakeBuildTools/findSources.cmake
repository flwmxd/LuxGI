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

macro( find_helper_make_relative INPUT OUT )
  foreach( I IN LISTS ${INPUT} )
    file( TO_CMAKE_PATH "${I}" I )
    file( RELATIVE_PATH I "${OPTS_PATH}" "${I}" )
    list( APPEND ${OUT} "${I}" )
  endforeach( I IN LISTS ${INPUT} )
endmacro( find_helper_make_relative )

macro( find_helper_asign_and_export FILE_LIST PLT_LIST SUFFIX )
  foreach( I IN LISTS ${FILE_LIST} )
    set( MATCH_FOUND OFF )

    foreach( J IN LISTS ${PLT_LIST} )
      string( REGEX MATCH "^([^/]+/)*${${J}}/" REG_OUT "${I}" )

      if( NOT "${REG_OUT}" STREQUAL "" )
        list( APPEND SRC_${J}_${SUFFIX} "${I}" )
        set( MATCH_FOUND ON )
        break()
      endif( NOT "${REG_OUT}" STREQUAL "" )
    endforeach( J IN LISTS ${PLT_LIST} )

    if( MATCH_FOUND )
      continue()
    endif( MATCH_FOUND )

    list( APPEND SRC_ALL_${SUFFIX} "${I}" )
  endforeach( I IN LISTS ${FILE_LIST} )

  # Export variables
  foreach( J IN LISTS ${PLT_LIST} )
    set( ${PROJECT_NAME}_SRC_${J}_${SUFFIX} ${SRC_${J}_${SUFFIX}} PARENT_SCOPE )
  endforeach( J IN LISTS ${PLT_LIST} )

  set( ${PROJECT_NAME}_SRC_ALL_${SUFFIX} ${SRC_ALL_${SUFFIX}} PARENT_SCOPE )
endmacro( find_helper_asign_and_export )


# Usage:
# find_source_files
#    PATH    <root search path>
#    EXT_CPP <cpp file extensions (without the first '.')>
#    EXT_HPP <hpp file extensions (without the first '.')>
#    EXCLUDE <list of regex to exclude (optional)>
#
# Searches the a directory recursively for source files
# ALL sections must be set!
#
# Generated variables:
#   ${PROJECT_NAME}_UNASIGNED_{C,H}PP  # All source files
#   ${PROJECT_NAME}_ALL_{C,H}PP        # Source files independent of OS / target
#   ${PROJECT_NAME}_${TARGET}_{C,H}PP  # Source files of target ${TARGET}
function( find_source_files )
  set( options )
  set( oneValueArgs   PATH )
  set( multiValueArgs EXT_CPP EXT_HPP EXCLUDE )
  cmake_parse_arguments( OPTS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

  foreach( I IN LISTS oneValueArgs multiValueArgs )
    if( "${OPTS_${I}}" STREQUAL "" AND NOT "${I}" STREQUAL "EXCLUDE" )
      message( ERROR "Invalid use of find_source_files: Section ${I} not defined!" )
    endif( "${OPTS_${I}}" STREQUAL "" AND NOT "${I}" STREQUAL "EXCLUDE" )
  endforeach( I IN LISTS TARGET_LIST )

  foreach( I IN ITEMS CPP HPP )
    set( ${I}_LIST_RAW "" )

    foreach( J IN LISTS OPTS_EXT_${I} )
      file( GLOB_RECURSE TEMP_LIST "${OPTS_PATH}/*.${J}" )
      if( TEMP_LIST STREQUAL "" )
        continue()
      endif( TEMP_LIST STREQUAL "" )
      foreach( K IN LISTS OPTS_EXCLUDE )
        if( "${CMAKE_VERSION}" VERSION_LESS "3.6" )
          message( WARNING "CMake version is ${CMAKE_VERSION} but at least 3.6 is recommended. EXCLUDE feature disabled" )
        else( "${CMAKE_VERSION}" VERSION_LESS "3.6" )
          list( FILTER TEMP_LIST EXCLUDE REGEX "${K}" )
        endif( "${CMAKE_VERSION}" VERSION_LESS "3.6" )
      endforeach( K IN LISTS OPTS_EXCLUDE )
      list( APPEND ${I}_LIST_RAW "${TEMP_LIST}" )
    endforeach( J IN LISTS OPTS_EXT_${I} )
  endforeach()

  find_helper_make_relative( CPP_LIST_RAW CPP_LIST )
  find_helper_make_relative( HPP_LIST_RAW HPP_LIST )

  find_helper_asign_and_export( CPP_LIST ${PROJECT_NAME}_PLATFORM_LIST "CPP" )
  find_helper_asign_and_export( HPP_LIST ${PROJECT_NAME}_PLATFORM_LIST "HPP" )

  set( ${PROJECT_NAME}_UNASIGNED_CPP "${CPP_LIST}" PARENT_SCOPE )
  set( ${PROJECT_NAME}_UNASIGNED_HPP "${HPP_LIST}" PARENT_SCOPE )
endfunction( find_source_files )


macro( export_helper_append_and_sort LIST_ALL TO_APPEND ROOT_DIR )
  if( NOT DEFINED ${LIST_ALL} )
    set( ${LIST_ALL} "" )
  endif( NOT DEFINED ${LIST_ALL} )

  if( DEFINED ${TO_APPEND} AND DEFINED ${LIST_ALL} )
    list( SORT ${TO_APPEND} )
    foreach( I IN LISTS ${TO_APPEND} )
      list( APPEND ${LIST_ALL} "${ROOT_DIR}/${I}" )
    endforeach( I IN LISTS ${TO_APPEND} )
    list( SORT ${LIST_ALL} )
    set( ${LIST_ALL} "${${LIST_ALL}}" PARENT_SCOPE )
  endif( DEFINED ${TO_APPEND} AND DEFINED ${LIST_ALL} )
endmacro( export_helper_append_and_sort )

# Exports all lists from find_source_files with an additional ALL prefix
macro( export_found_files ROOT_DIR )
  foreach( I IN LISTS PLATFORM_LIST )
    export_helper_append_and_sort( ${PROJECT_NAME}_ALL_SRC_${I}_CPP ${PROJECT_NAME}_SRC_${I}_CPP ${ROOT_DIR} )
    export_helper_append_and_sort( ${PROJECT_NAME}_ALL_SRC_${I}_HPP ${PROJECT_NAME}_SRC_${I}_HPP ${ROOT_DIR} )
  endforeach( I IN LISTS PLATFORM_LIST )

  export_helper_append_and_sort( ${PROJECT_NAME}_ALL_SRC_ALL_CPP ${PROJECT_NAME}_SRC_ALL_CPP ${ROOT_DIR} )
  export_helper_append_and_sort( ${PROJECT_NAME}_ALL_SRC_ALL_HPP ${PROJECT_NAME}_SRC_ALL_HPP ${ROOT_DIR} )

  export_helper_append_and_sort( ${PROJECT_NAME}_ALL_UNASIGNED_CPP ${PROJECT_NAME}_UNASIGNED_CPP ${ROOT_DIR} )
  export_helper_append_and_sort( ${PROJECT_NAME}_ALL_UNASIGNED_HPP ${PROJECT_NAME}_UNASIGNED_HPP ${ROOT_DIR} )
endmacro( export_found_files )
