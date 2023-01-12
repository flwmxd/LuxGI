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

macro( select_sources_helper_list_to_string IN_LIST OUT_STRING )
  list( SORT ${IN_LIST} )
  foreach( I IN LISTS ${IN_LIST} )
    set( ${OUT_STRING} "${${OUT_STRING}}\n   \${CMAKE_CURRENT_SOURCE_DIR}/${I}" )
  endforeach( I IN LISTS ${IN_LIST} )

  set( ${OUT_STRING} "${${OUT_STRING}}" PARENT_SCOPE )
endmacro( select_sources_helper_list_to_string )

function( select_sources_helper_add_dir FILE_NAME )
  string( REGEX REPLACE "[^/]+$" "" DIR_NAME "${FILE_NAME}" )

  if( NOT DEFINED CURRENT_INCLUDE_DIRS )
    set( CURRENT_INCLUDE_DIRS "${DIR_NAME}" PARENT_SCOPE )
    return()
  endif( NOT DEFINED CURRENT_INCLUDE_DIRS )

  foreach( X IN LISTS CURRENT_INCLUDE_DIRS )
    if( "${X}" STREQUAL "${DIR_NAME}" )
      return()
    endif( "${X}" STREQUAL "${DIR_NAME}" )
  endforeach()

  set( CURRENT_INCLUDE_DIRS ${CURRENT_INCLUDE_DIRS} "${DIR_NAME}" PARENT_SCOPE )
endfunction( select_sources_helper_add_dir )

# Sets CM_CURRENT_SRC_CPP, CM_CURRENT_SRC_HPP and CURRENT_INCLUDE_DIRS for the current platform
function( select_sources )
  set( CURRENT_SRC_CPP "" )
  set( CURRENT_SRC_HPP "" )

  # Calculate filler whitespaces
  set( MAX_LENGTH 0 )
  foreach( I IN LISTS ${PROJECT_NAME}_PLATFORM_LIST )
    string( LENGTH "${${I}}" CUR_LENGTH )
    if( CUR_LENGTH GREATER MAX_LENGTH )
      set( MAX_LENGTH ${CUR_LENGTH} )
    endif( CUR_LENGTH GREATER MAX_LENGTH )
  endforeach( I IN LISTS ${PROJECT_NAME}_PLATFORM_LIST )

  foreach( I IN LISTS ${PROJECT_NAME}_PLATFORM_LIST )
    set( BUILD_STR "" )
    if( DEFINED ${PROJECT_NAME}_SRC_${I}_CPP )
      if( I IN_LIST PLATFORM_TARGET )
        set( BUILD_STR "[build]" )
        list( APPEND CURRENT_SRC_CPP "${${PROJECT_NAME}_SRC_${I}_CPP}" )
        list( APPEND CURRENT_SRC_HPP "${${PROJECT_NAME}_SRC_${I}_HPP}" )

        foreach( J IN LISTS ${PROJECT_NAME}_SRC_${I}_HPP )
          select_sources_helper_add_dir( "${J}" )
        endforeach( J IN LISTS ${PROJECT_NAME}_SRC_${I}_HPP )
      endif( I IN_LIST PLATFORM_TARGET )

      list( LENGTH ${PROJECT_NAME}_SRC_${I}_CPP CPP_LENGTH )
      list( LENGTH ${PROJECT_NAME}_SRC_${I}_HPP HPP_LENGTH )

      string( LENGTH "${${I}}" CUR_LENGTH )
      set( SPACES "" )
      math( EXPR NSPACES "${MAX_LENGTH} - ${CUR_LENGTH}" )
      foreach( J RANGE 0 ${NSPACES} )
        string( APPEND SPACES " " )
      endforeach( J RANGE 0 ${NSPACES} )

      math( EXPR LENGTH_ALL "${CPP_LENGTH} + ${HPP_LENGTH}" )
      message( STATUS "  - Target ${${I}}:${SPACES}${LENGTH_ALL} files ${BUILD_STR}" )
    endif( DEFINED ${PROJECT_NAME}_SRC_${I}_CPP )
  endforeach( I IN LISTS ${PROJECT_NAME}_PLATFORM_LIST )

  list( LENGTH ${PROJECT_NAME}_SRC_ALL_CPP CPP_LENGTH )
  list( LENGTH ${PROJECT_NAME}_SRC_ALL_HPP HPP_LENGTH )

  set( SPACES "" )
  foreach( J RANGE 3 ${MAX_LENGTH} )
    string( APPEND SPACES " " )
  endforeach( J RANGE 3 ${MAX_LENGTH} )
  math( EXPR LENGTH_ALL "${CPP_LENGTH} + ${HPP_LENGTH}" )
  message( STATUS "  - Target ALL:${SPACES}${LENGTH_ALL} files [build]\n" )

  list( APPEND CURRENT_SRC_CPP "${${PROJECT_NAME}_SRC_ALL_CPP}" )
  list( APPEND CURRENT_SRC_HPP "${${PROJECT_NAME}_SRC_ALL_HPP}" )

  select_sources_helper_list_to_string( CURRENT_SRC_CPP CM_CURRENT_SRC_CPP )
  select_sources_helper_list_to_string( CURRENT_SRC_HPP CM_CURRENT_SRC_HPP )

  foreach( I IN LISTS CURRENT_SRC_HPP )
    select_sources_helper_add_dir( "${I}" )
  endforeach( I IN LISTS CURRENT_SRC_HPP )

  set( CURRENT_INCLUDE_DIRS ${CURRENT_INCLUDE_DIRS} PARENT_SCOPE )
endfunction( select_sources )
