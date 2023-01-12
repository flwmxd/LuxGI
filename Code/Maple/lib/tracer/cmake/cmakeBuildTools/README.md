# cmakeBuildTools
Useful functions for CMake projects

# function `add_compiler( COMPILER ARG1 ARG2 ... )`

Set compiler flags
1st parameter: the CMake compiler ID
Sections:
```
ALL:         c++ compiler options for all build types
DEBUG:       c++ compiler options only for the DEBUG build type
RELEASE:     c++ compiler options only for the RELEASE build type
C_ALL:       c compiler options for all build types
C_DEBUG:     c compiler options only for the DEBUG build type
C_RELEASE:   c compiler options only for the RELEASE build type
SANITIZE:    sanitizer option. Will only be enabled on DEBUG build type and when the variable ENABLE_SANITIZERS is set
MIN_VERSION: the min. compiler version
```

# function `enum2str_generate( ... )`
 Usage:
```
 enum2str_generate
    PATH           <path to generate files in>
    CLASS_NAME     <name of the class (file names will be PATH/CLASS_NAME.{hpp,cpp})>
    FUNC_NAME      <the name of the function>
    INDENT_STR     <a string used for one level of indentation>
    INCLUDES       <files to include (where the enums are)>
    NAMESPACE      <namespace to use>
    ENUMS          <list of enums to generate>
    BLACKLIST      <blacklist for enum constants>
    USE_CONSTEXPR  <whether to use constexpr or not (default: off)>
    USE_C_STRINGS  <whether to use c strings instead of std::string or not (default: off)>
```

# function `find_source_files(...)`

Searches the a directory recursively for source files

```
Usage:
 find_source_files
    PATH    <root search path>
    EXT_CPP <cpp file extensions (without the first '.')>
    EXT_HPP <hpp file extensions (without the first '.')>
    EXCLUDE <list of regex to exclude (optional)>

Generated variables:
  ${PROJECT_NAME}_UNASIGNED_{C,H}PP  # All source files
  ${PROJECT_NAME}_ALL_{C,H}PP        # Source files independent of OS / target
  ${PROJECT_NAME}_${TARGET}_{C,H}PP  # Source files of target ${TARGET}
```

ALL sections must be set!

# macro `export_found_files(ROOT_PATH)`

Exports all lists from find_source_files with an additional ALL prefix

# function `add_platform(...)`

Adds a operating system with (multiple) targets. Each target can be enabled and disabled. Only enabled targets will be
found by find_source_files (= folders with the same name as disabled targets will be excluded).

```
Usage:
  add_platform
    OS     <the operating system>
    TARGET <supported targets (aka subdirectories in the source tree) of the OS>

 Variables:
    PLATFORM_TARGET               - the secondary target of one OS
    ${PROJECT_NAME}_PLATFORM_LIST - list of all platforms added so far (output)
```

# check_platform()

Checks the the `PLATFORM_TARGET` list. Also generates a `CM_${I}` variable (= 0/1) for every target of
each platform / OS.

This function must be run to finalize the platform setup.

# function `select_sources()`

Sets `CM_CURRENT_SRC_CPP`, `CM_CURRENT_SRC_HPP` and `CURRENT_INCLUDE_DIRS` for the current platform

# function `run_git()`

Collects version information about the current git repository

Output variables:
```
 - CMAKE_BUILD_TYPE (if not already set)
 - DEBUG_LOGGING
 - CM_VERSION_MAJOR
 - CM_VERSION_MINOR
 - CM_VERSION_PATCH
 - CM_TAG_DIFF
 - CM_VERSION_GIT
```

# function `generate_format_command(CMD_NAME CM_CLANG_FORMAT_VER)`

Adds a new make target `CMD_NAME` that formats the entire source code with clang-format

# function `new_project_library(...)`

```cmake
new_project_library searches for source files in PATH and generates a CMakeLists.txt.
The (optional) CMakeScript.cmake in PATH will be executed first

Usage:
new_project_library
   NAME         <Name of the library>
   PATH         <path to the source dir>
   TEMPLATE     <path to the CMake template file>
   DEPENDENCIES <dependencies (optional)>
   EXCLUDE      <list of regex to exclude (optional)>
   EXT_CPP      <list of valid c++ extensions (optinal [default: cpp;cxx;C;c])
   EXT_HPP      <list of valid c++ extensions (optinal [default: hpp;hxx;H;h])

Variables available in the template:
   CM_CURRENT_SRC_CPP
   CM_CURRENT_SRC_HPP
   CM_CURRENT_LIB_SRC
   CM_CURRENT_LIB_INC
   CM_CURRENT_LIB_LC
   CM_CURRENT_LIB_UC
   CURRENT_INCLUDE_DIRS

    Exported variables (multiple calls to new_project_library will extend these lists)
   ${PROJECT_NAME}_LIB_INCLUDE_DIRECTORIES  <List of all directories>
   ${PROJECT_NAME}_SUBDIR_LIST              <List of all generated subdirectories>
   ${PROJECT_NAME}_ALL_UNASIGNED_<H,C>PP    <List of ALL source files (has a CPP and HPP version)>
   ${PROJECT_NAME}_ALL_SRC_${I}_<H,C>PP     <List of source files for platform target ${I} (has a CPP and HPP version)>
   ${PROJECT_NAME}_ALL_SRC_ALL_<H,C>PP      <List of source files for all platform targets (has a CPP and HPP version)>
```

# function `new_project_executable`

```cmake
new_project_executable searches for source files in PATH and generates a CMakeLists.txt.
The (optional) CMakeScript.cmake in PATH will be executed first

Usage:
new_project_executable
   NAME         <Name of the library>
   PATH         <path to the source dir>
   TEMPLATE     <path to the CMake template file>
   DEPENDENCIES <dependencies (optional)>
   EXCLUDE      <list of regex to exclude (optional)>
   EXT_CPP      <list of valid c++ extensions (optinal [default: cpp;cxx;C;c])
   EXT_HPP      <list of valid c++ extensions (optinal [default: hpp;hxx;H;h])

Variables available in the template:
   CM_CURRENT_SRC_CPP
   CM_CURRENT_SRC_HPP
   CM_CURRENT_EXE_SRC
   CM_CURRENT_EXE_INC
   CM_CURRENT_EXE_LC
   CM_CURRENT_EXE_UC
   CURRENT_INCLUDE_DIRS

Exported variables (multiple calls to new_project_executable will extend these lists)
   ${PROJECT_NAME}_SUBDIR_LIST              <List of all generated subdirectories>
   ${PROJECT_NAME}_ALL_UNASIGNED_<H,C>PP    <List of ALL source files (has a CPP and HPP version)>
   ${PROJECT_NAME}_ALL_SRC_${I}_<H,C>PP     <List of source files for platform target ${I} (has a CPP and HPP version)>
   ${PROJECT_NAME}_ALL_SRC_ALL_<H,C>PP      <List of source files for all platform targets (has a CPP and HPP version)>
```
