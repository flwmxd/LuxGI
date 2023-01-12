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

function( run_git )
  set( CM_VERSION_GIT "GIT_FAILED" )
  find_package( Git REQUIRED )

  # Set the version of this project
  set( CM_VERSION_MAJOR    0 )
  set( CM_VERSION_MINOR    0 )
  set( CM_VERSION_SUBMINOR 0 )
  set( CM_VERSION_PATCH    0 )
  set( CM_TAG_DIFF         0 )

  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
    OUTPUT_VARIABLE CM_VERSION_GIT
    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
    RESULT_VARIABLE GIT_RESULT
  )

  # Remove newlines added by Git
  string( REGEX REPLACE "\n" "" CM_VERSION_GIT ${CM_VERSION_GIT} )

  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --abbrev=0 --tags
    OUTPUT_VARIABLE GIT_TAG_1
    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
    ERROR_QUIET
  )

  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags
    OUTPUT_VARIABLE GIT_TAG_2
    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
    RESULT_VARIABLE GIT_HAS_TAGS
    ERROR_QUIET
  )

  if( ${GIT_HAS_TAGS} EQUAL 0 )
    string( REGEX REPLACE "\n" "" GIT_TAG_1 "${GIT_TAG_1}" )
    string( REGEX REPLACE "\n" "" GIT_TAG_2 "${GIT_TAG_2}" )

    string( REGEX REPLACE "^v([0-9]+)[\\.0-9]+$"          "\\1" CM_VERSION_MAJOR "${GIT_TAG_1}" )
    string( REGEX REPLACE "^v[\\.0-9]+([0-9]+)[\\.0-9]+$" "\\1" CM_VERSION_MINOR "${GIT_TAG_1}" )
    string( REGEX REPLACE "^v[\\.0-9]+([0-9]+)$"          "\\1" CM_VERSION_PATCH "${GIT_TAG_1}" )

    if( "${GIT_TAG_1}" STREQUAL "${GIT_TAG_2}" )
      set( CM_TAG_DIFF 0 )
      message( STATUS "${PROJECT_NAME} version: ${GIT_TAG_1} [RELEASE] ${CM_VERSION_GIT}" )
    else()
      string( REGEX REPLACE "^v[\\.0-9]+\\-([0-9]*)\\-[a-z0-9]*$" "\\1" CM_TAG_DIFF "${GIT_TAG_2}" )
      message( STATUS "${PROJECT_NAME} version: ${GIT_TAG_1} +${CM_TAG_DIFF} ${CM_VERSION_GIT}" )
    endif( "${GIT_TAG_1}" STREQUAL "${GIT_TAG_2}" )
  else( ${GIT_HAS_TAGS} EQUAL 0 )
    message( STATUS "Can not access tags ==> version is v0.0.1 +1" )
    set( CM_VERSION_MAJOR 0 )
    set( CM_VERSION_MINOR 0 )
    set( CM_VERSION_PATCH 1 )
    set( CM_TAG_DIFF      1 )
  endif( ${GIT_HAS_TAGS} EQUAL 0 )


  # Only set CMAKE_BUILD_TYPE when not defined
  if( NOT CMAKE_BUILD_TYPE )
    set( CMAKE_BUILD_TYPE DEBUG )

    # Is this tagged ==> Release?
    if( "${CM_TAG_DIFF}" STREQUAL "0" )
      set( CMAKE_BUILD_TYPE RELEASE )
    endif( "${CM_TAG_DIFF}" STREQUAL "0" )
  endif( NOT CMAKE_BUILD_TYPE )

  message( STATUS "Build type: ${CMAKE_BUILD_TYPE}\n" )

  set( CM_VERSION_SUBMINOR ${CM_VERSION_PATCH} )

  set( CMAKE_BUILD_TYPE    ${CMAKE_BUILD_TYPE}    PARENT_SCOPE )
  set( CM_VERSION_MAJOR    ${CM_VERSION_MAJOR}    PARENT_SCOPE )
  set( CM_VERSION_MINOR    ${CM_VERSION_MINOR}    PARENT_SCOPE )
  set( CM_VERSION_PATCH    ${CM_VERSION_PATCH}    PARENT_SCOPE )
  set( CM_VERSION_SUBMINOR ${CM_VERSION_SUBMINOR} PARENT_SCOPE )
  set( CM_TAG_DIFF         ${CM_TAG_DIFF}         PARENT_SCOPE )
  set( CM_VERSION_GIT      ${CM_VERSION_GIT}      PARENT_SCOPE )
endfunction( run_git )
