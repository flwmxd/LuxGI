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

cmake_minimum_required( VERSION 3.5 )
project( Tracer LANGUAGES CXX )

message( STATUS "Using CMake version: ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}\n" )
include( ${PROJECT_SOURCE_DIR}/cmake/cmakeBuildTools/cmakeBuildTools.cmake )
include( CMakePackageConfigHelpers )

######################
# Set some variables #
######################

# Set the binary output directories
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin )

set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake )

set( CMAKE_CXX_STANDARD          14 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_C_STANDARD            11 )
set( CMAKE_C_STANDARD_REQUIRED   ON )

set( CMAKE_POSITION_INDEPENDENT_CODE  ON )
set( CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON )

option( DISABLE_TESTS       "Disables all tests"                                      OFF )
option( AUTO_ENABLE_MODULES "Automatically enable modules when found"                 ON )
option( USE_CLANG_LIBCXX    "Build with libc++ instead of stdlibc++"                  OFF )
option( BUILD_STATIC        "Build and link tracer static"                            OFF )
option( DISABLE_FILESYSTEM  "Disables std::filesystem -- then only C++11 is required" OFF )
option( VERBOSE_DEBUG       "Some additional debug messages"                          OFF )
#option( PREFER_STATIC_LIBS  "Prefer linking to static libs"                           ON  )

#####################
# Check std C++ lib #
#####################

if( VERBOSE_DEBUG )
  set( CM_VERBOSE_DEBUG 1 )
else( VERBOSE_DEBUG )
  set( CM_VERBOSE_DEBUG 0 )
endif( VERBOSE_DEBUG )

if( DISABLE_FILESYSTEM )
  set( CM_DISABLE_FILESYSTEM 1  )
  set( CMAKE_CXX_STANDARD    11 ) # No need for c++17 without std::filesystem
else( DISABLE_FILESYSTEM )
  set( CM_DISABLE_FILESYSTEM 0 )
endif( DISABLE_FILESYSTEM )

if( UNIX AND NOT DISABLE_FILESYSTEM )
  if( USE_CLANG_LIBCXX )
    set( GCC_STDLIB_FLAGS   "-nodefaultlibs -lc++ -lc++abi" )
    set( CLANG_STDLIB_FLAGS "-stdlib=libc++" )
    set( DEPS c++experimental )
  else( USE_CLANG_LIBCXX )
    set( GCC_STDLIB_FLAGS   "" )
    set( CLANG_STDLIB_FLAGS "" )
    set( DEPS stdc++fs )
  endif( USE_CLANG_LIBCXX )
elseif( WIN32 )
  set( DEPS version Dbghelp )
else()
  set( DEPS )
endif()
set( ALL_MODULES )

if( EXISTS "${CMAKE_CURRENT_LIST_DIR}/private.cmake" )
  include( "${CMAKE_CURRENT_LIST_DIR}/private.cmake" )
endif( EXISTS "${CMAKE_CURRENT_LIST_DIR}/private.cmake" )

################################
# Set up the build environment #
################################

add_compiler(
  GNU MIN_VERSION "5.1"
  ALL        "-Wall -Wextra -Wfloat-equal -Wshadow -Wno-unknown-pragmas -fdiagnostics-color=always ${GCC_STDLIB_FLAGS}"
  DEBUG      "-Og -Werror -fno-omit-frame-pointer -gcolumn-info" ${COVERAGE_COMPILER_FLAG}
  RELEASE    "-O3 -fno-omit-frame-pointer -g -gcolumn-info"

  C_ALL      "-Wall -std=c11 -fdiagnostics-color=always"
  C_DEBUG    "-Og -Werror -fno-omit-frame-pointer -gcolumn-info" ${COVERAGE_COMPILER_FLAG}
  C_RELEASE  "-O3 -fno-omit-frame-pointer -g -gcolumn-info"

  SANITIZE   "-fsanitize=${SANITIZERS}"
)

add_compiler(
  Clang MIN_VERSION 3.9
  ALL        "-Weverything -ftemplate-backtrace-limit=0 -fcolor-diagnostics"
             "-Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded ${CLANG_STDLIB_FLAGS}"
             "-Wno-gnu-anonymous-struct -Wno-nested-anon-types -Wno-exit-time-destructors"
             "-Wno-global-constructors -Wno-switch-enum -Wno-reserved-id-macro"
             "-Wno-documentation-unknown-command -Wno-unknown-pragmas"
             "-Wno-disabled-macro-expansion -Wno-undef"
  DEBUG      "-Werror -gline-tables-only"
  RELEASE    "-O3 -gline-tables-only"

  C_ALL      "-Wall"
  C_DEBUG    ""
  C_RELEASE  "-O3 -gline-tables-only"

  SANITIZE "-fsanitize=${SANITIZERS}"
)

add_compiler(
  MSVC MIN_VERSION "19.10"
  ALL        "/W4"
  DEBUG      ""
  RELEASE    "/O2"

  C_ALL      "/W4"
  C_DEBUG    ""
  C_RELEASE  "/O2"
)

######################
# Check Dependencies #
######################

macro( ADD_MODULE_DEP NAME PL_TARGET DEP )
  find_package( ${NAME} )
  set( ${NAME}_MOD_DEP   ${${NAME}_MOD_DEP}   ${DEP} )
  set( ${NAME}_PL_TARGET ${${NAME}_PL_TARGET} ${PL_TARGET} )
  list( APPEND ALL_MODULES ${NAME} )
endmacro( ADD_MODULE_DEP )

ADD_MODULE_DEP( LibUnwind UNIX_LIBUNWIND        LibUnwind::LibUnwind )
ADD_MODULE_DEP( LibELF    UNIX_LIBDWLF          LibELF::DW )
ADD_MODULE_DEP( LibBFD    UNIX_LIBBFD           LibBFD::LibBFD )
ADD_MODULE_DEP( Execinfo  UNIX_GLIBC            Execinfo::Execinfo )
ADD_MODULE_DEP( Execinfo  UNIX_EXTERNALFALLBACK Execinfo::Execinfo )

run_git()

############################
# Setting build conditions #
############################

add_platform( OS WIN32 TARGET windows )

add_platform(
  OS UNIX
  TARGET
    libunwind glibc                 # Tracer
    libdwlf libbfd externalFallback # Debug Info
)

if( AUTO_ENABLE_MODULES )
  set( PLATFORM_TARGET "" )
endif( AUTO_ENABLE_MODULES )

if( WIN32 )
  list( APPEND PLATFORM_TARGET "WIN32_WINDOWS" )
endif( WIN32 )

foreach( I IN LISTS ALL_MODULES )
  if( AUTO_ENABLE_MODULES )
    if( ${I}_FOUND )
      list( APPEND PLATFORM_TARGET ${${I}_PL_TARGET} )
      list( APPEND DEPS            ${${I}_MOD_DEP} )
    endif( ${I}_FOUND )
  endif( AUTO_ENABLE_MODULES )

  foreach( J IN LISTS ${I}_PL_TARGET )
    if( ${J} IN_LIST PLATFORM_TARGET )
      if( NOT ${I}_FOUND )
        message( SEND_ERROR "${J} is in list PLATFORM_TARGET but ${I} is not found!" )
      endif( NOT ${I}_FOUND )
    endif( ${J} IN_LIST PLATFORM_TARGET )
  endforeach( J IN LISTS ${I}_PL_TARGET )
endforeach( I IN LISTS ALL_MODULES )

list(REMOVE_DUPLICATES PLATFORM_TARGET)
check_platform()

####################
# Generate subdirs #
####################

new_project_library(
  PATH         "${PROJECT_SOURCE_DIR}/src"
  NAME         "tracer"
  TEMPLATE     "${PROJECT_SOURCE_DIR}/cmake/templates/CMakeLists.lib.txt"
  DEPENDENCIES ${DEPS}
  EXCLUDE      ".*/whereami/example/.*"
)

if( NOT DISABLE_TESTS )
  file( GLOB  TEST_DIRS "${PROJECT_SOURCE_DIR}/tests/*/" )
  enable_testing()

  add_custom_target(
    check
      COMMAND ${CMAKE_COMMAND} -E env CTEST_OUTPUT_ON_FAILURE=1 ${CMAKE_CTEST_COMMAND} --force-new-ctest-process
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )

  foreach( I IN LISTS TEST_DIRS )
    file( RELATIVE_PATH J "${PROJECT_SOURCE_DIR}/tests" "${I}" )

    new_project_executable(
      PATH         "${I}"
      NAME         "${J}"
      TEMPLATE     "${PROJECT_SOURCE_DIR}/cmake/templates/CMakeLists.exe.txt"
      DEPENDENCIES ${DEPS} tracer
    )

    add_dependencies( check ${J} )
  endforeach( I IN LISTS TEST_DIRS )
endif( NOT DISABLE_TESTS )

##############################
# Update include directories #
##############################

include_directories( SYSTEM
  ${LibUnwind_INCLUDE_DIRS}
)

include_directories(
  ${Tracer_LIB_INCLUDE_DIRECTORIES}
  ${PROJECT_BINARY_DIR}
)

######################
# Add subdirectories #
######################

foreach( I IN LISTS Tracer_SUBDIR_LIST )
  add_subdirectory( "${I}" )
endforeach( I IN LISTS Tracer_SUBDIR_LIST )

# Fix include dirs for parent projects
foreach( I IN LISTS Tracer_LIB_INCLUDE_DIRECTORIES PROJECT_BINARY_DIR )
  target_include_directories(tracer PUBLIC
      $<BUILD_INTERFACE:${I}>
      $<INSTALL_INTERFACE:include/tracer>
  )
endforeach()

###########
# Doxygen #
###########

find_package( Doxygen COMPONENTS dot )
if( Doxygen_FOUND )
  if( NOT TARGET Doxygen::doxygen )
    add_executable(Doxygen::doxygen IMPORTED GLOBAL)
    set_target_properties(Doxygen::doxygen PROPERTIES
        IMPORTED_LOCATION "${DOXYGEN_EXECUTABLE}"
    )
  endif()

  set( DOXYGEN_TARGET_FILE "${PROJECT_SOURCE_DIR}/docs/doxygen.tmp" )
  configure_file( "${PROJECT_SOURCE_DIR}/Doxyfile.in" "${PROJECT_SOURCE_DIR}/Doxyfile" @ONLY )
  add_custom_command(
    OUTPUT          "${DOXYGEN_TARGET_FILE}"
    COMMAND          Doxygen::doxygen ARGS "${PROJECT_SOURCE_DIR}/Doxyfile"
    COMMAND          ${CMAKE_COMMAND} ARGS -E touch "${DOXYGEN_TARGET_FILE}"
    MAIN_DEPENDENCY "${PROJECT_SOURCE_DIR}/Doxyfile"
    VERBATIM
  )

  add_custom_target( doc DEPENDS "${DOXYGEN_TARGET_FILE}" COMMAND ${CMAKE_COMMAND} -E remove "${DOXYGEN_TARGET_FILE}" )
endif( Doxygen_FOUND )

##################
# Generate Files #
##################

configure_file( "${PROJECT_SOURCE_DIR}/cmake/templates/defines.in.hpp" "${PROJECT_BINARY_DIR}/tracerDef.hpp" @ONLY )

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Version.cmake"
    VERSION       ${CM_VERSION_MAJOR}.${CM_VERSION_MINOR}.${CM_VERSION_PATCH}
    COMPATIBILITY SameMajorVersion
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Version.cmake
    ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}Config.cmake
  DESTINATION lib/cmake/${PROJECT_NAME}
)

install( EXPORT ${PROJECT_NAME}Targets DESTINATION lib/cmake/${PROJECT_NAME} )
