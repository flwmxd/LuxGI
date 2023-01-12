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

set( __generate_format_command__DIR "${CMAKE_CURRENT_LIST_DIR}" )

# Adds a new make target CMD_NAME that formats the entire source code with clang-format
function( generate_format_command CMD_NAME CM_CLANG_FORMAT_VER )
  set( FILE_IN  "${__generate_format_command__DIR}/templates/cmake_format.cmake.in" )
  set( FILE_OUT "${PROJECT_BINARY_DIR}/cmake_format.cmake" )

  list( APPEND SOURCES_RAW ${${PROJECT_NAME}_ALL_UNASIGNED_HPP} )
  list( APPEND SOURCES_RAW ${${PROJECT_NAME}_ALL_UNASIGNED_CPP} )
  list( SORT SOURCES_RAW )

  foreach( I IN LISTS SOURCES_RAW )
    string( APPEND CM_ALL_SOURCE_FILES "   ${I}\n" )
  endforeach( I IN LISTS SOURCES_RAW )

  configure_file( "${FILE_IN}" "${FILE_OUT}" @ONLY )
  add_custom_target( ${CMD_NAME} COMMAND ${CMAKE_COMMAND} -P ${FILE_OUT} )
endfunction( generate_format_command )
