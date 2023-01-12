#!/bin/bash

BUILD_DIR=build2

msg() {
  echo -e "\x1b[1;33m[ MSG ]\x1b[0;33m $@\x1b[0m"
}

msg_ok() {
  echo -e "\x1b[1;32m[ OK  ]\x1b[0;32m $@\x1b[0m"
}

msg_error() {
  echo -e "\x1b[1;31m[ERROR]\x1b[0;31m $@\x1b[0m"
}

print_version() {
  msg "  - $1: \x1b[1m$(pacman -Qi $1 | grep Version | sed 's/[^:]*: //g')"
}

ERROR_COUNT=0

__EXEC__() {
  SUDO_EXEC=
  (( $1 == 1 )) && SUDO_EXEC="sudo -u nobody"

  echo ""
  $SUDO_EXEC "${@:3}"
  ERROR=$?

  echo -e "\n"
  if (( ERROR == 0 || ( $2 == 1 && ERROR != 0 ) )); then
    msg_ok "Command '${@:3}' returned $ERROR"
    return
  fi

  msg_error "Command '${@:3}' returned $ERROR"
  (( ERROR_COUNT++ ))
}

testExec()           { __EXEC__ 0 0 "$@"; }
testFail()           { __EXEC__ 0 1 "$@"; }
testExecNoRoot()     { __EXEC__ 1 0 "$@"; }
testExecNoRootFail() { __EXEC__ 1 1 "$@"; }

cd "$(dirname $0)"

if [ -z "$1" ]; then
  msg_error "Compiler not set! Usage: $0 <c++ compiler>"
  exit 1
fi

if [[ "$1" == "g++" ]]; then
  msg "Detected compiler \x1b[1;33mGCC"
  msg "Enabling Coverage data collection"
  export CXX=g++
  export CC=gcc
elif [[ "$1" == "clang++" ]]; then
  msg "Detected compiler \x1b[1;33mLLVM / CLANG"
  msg "Disabling Coverage data collection"
  export CXX=clang++
  export CC=clang
else
  msg_error "Unknown compiler '$1'"
  exit 1
fi

msg "Versions:"
print_version cmake
print_version gcc
print_version clang
print_version libunwind


msg "Setting up the build env"
testExec ./checkFormat.sh --only-check
testExec mkdir       $BUILD_DIR
testExec cd          $BUILD_DIR

msg "START BUILD"

testExec cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/tracer ..
testExec make
#testExec make check

exit $ERROR_COUNT
