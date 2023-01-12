#!/bin/bash

# Test clang-format formating

########################
# BEGIN CONFIG SECTION #
########################

CLANG_FORMAT_VERSIONS=( "4.0.0" "4.0.1" )
CLANG_FORMAT_DEFAULT_CMD="clang-format"
EXTENSIONS=( cpp hpp c h )
SOURCE_DIRS=( src tests )

########################
#  END  CONFIG SECTION #
########################

VERBOSE=0
GIT_MODE=0
ONLY_CHECK=0

for I in "$@"; do
  case "$I" in
    -v|--verbose)    VERBOSE=1    ;;
    -g|--git)        GIT_MODE=1   ;;
    -c|--only-check) ONLY_CHECK=1 ;;
  esac
done

error() {
  echo -e '        \x1b[1;31m______ ______________  ___  ___ _____   _________________ ___________  \x1b[0m'
  echo -e '        \x1b[1;31m|  ___|  _  | ___ \  \/  | / _ \_   _| |  ___| ___ \ ___ \  _  | ___ \ \x1b[0m'
  echo -e '        \x1b[1;31m| |_  | | | | |_/ / .  . |/ /_\ \| |   | |__ | |_/ / |_/ / | | | |_/ / \x1b[0m'
  echo -e '        \x1b[1;31m|  _| | | | |    /| |\/| ||  _  || |   |  __||    /|    /| | | |    /  \x1b[0m'
  echo -e '        \x1b[1;31m| |   \ \_/ / |\ \| |  | || | | || |   | |___| |\ \| |\ \\\\ \_/ / |\ \  \x1b[0m'
  echo -e '        \x1b[1;31m\_|    \___/\_| \_\_|  |_/\_| |_/\_/   \____/\_| \_\_| \_|\___/\_| \_| \x1b[0m'
  echo -e '        \x1b[1;31m                                                                       \x1b[0m'
  echo -e "  \x1b[1;31m==> \x1b[0;31m$*\x1b[0m"
}

verbose() {
  (( VERBOSE == 0 )) && return
  echo -e "$*"
}


cd "$(dirname "$0")"

[ -z "$CLANG_FORMAT_CMD" ] && CLANG_FORMAT_CMD="$CLANG_FORMAT_DEFAULT_CMD"

# Check if the command exists
CLANG_FORMAT_EXEC=$(which "$CLANG_FORMAT_CMD" 2> /dev/null)
if (( $? != 0 )); then
  error "clang-format ($CLANG_FORMAT_CMD) not found. Try setting the CLANG_FORMAT_CMD environment variable"
  exit 1
fi

verbose "clang-format command: $CLANG_FORMAT_EXEC"


# Check the version
CURRENT_VERSION="$($CLANG_FORMAT_EXEC --version | sed 's/^[^0-9]*//g')"
CURRENT_VERSION="$(echo "$CURRENT_VERSION" | sed 's/^\([0-9.]*\).*/\1/g')"
FOUND_VERSION=0

for I in "${CLANG_FORMAT_VERSIONS[@]}"; do
  [[ "$I" == "$CURRENT_VERSION" ]] && (( FOUND_VERSION++ ))
done

if (( FOUND_VERSION == 0 )); then
  error "Invalid clang-format version! ${CLANG_FORMAT_VERSIONS[@]} versions required but $CURRENT_VERSION provided"
  exit 2
fi

verbose "clang-format version: $CURRENT_VERSION\n"


#########################
# Checking source files #
#########################

ERROR=0
NUM_FILES=0
TO_GIT_ADD=()

checkFormat() {
  local I J
  NUM_FILES=0
  ERROR=0

  verbose " ==> Checking files"

  if (( GIT_MODE == 1 )); then
    SOURCE_LIST=()
    for I in $(git diff --cached --name-only); do
      for J in "${SOURCE_DIRS[@]}"; do
        echo "$I" | grep -E "^$J" &> /dev/null
        if (( $? == 0 )); then
          SOURCE_LIST+=( "$I" )
          break
        fi
      done
    done
  else
    SOURCE_LIST=( $(find "${SOURCE_DIRS[@]}" -type f) )
  fi

  for I in "${SOURCE_LIST[@]}"; do
    for J in "${EXTENSIONS[@]}"; do
      [[ ! "$I" =~ \.$J$ ]] && continue

      (( NUM_FILES++ ))

      if [[ "$1" == "format" ]]; then
        verbose "   --> Fixing $I"
        $CLANG_FORMAT_EXEC -i "$I"
        TO_GIT_ADD+=( "$I" )
      fi

      verbose "   --> Checking $I"

      RES=$( $CLANG_FORMAT_EXEC -output-replacements-xml "$I" | grep "<replacement " )
      if [ -n "$RES" ]; then
        echo -e " \x1b[1;31mFormat check error: $I"
        (( ERROR++ ))
      fi

      break
    done
  done
}

checkFormat check

if (( ERROR != 0 )); then
  error "$ERROR out of $NUM_FILES files are not formatted"
  (( ONLY_CHECK == 1 )) && exit $ERROR

  read -p "  ==> Try formatting source files [Y/n]? " -n 1 INPUT
  [ -z "$INPUT" ] && INPUT=y || echo ""

  if [[ "$INPUT" == "Y" || "$INPUT" == "y" ]]; then
    checkFormat format
    if (( ERROR != 0 )); then
      error "$ERROR out of $NUM_FILES files are not formatted -- CLANG FORMAT FAIL"
      exit $ERROR
    fi

    if (( GIT_MODE == 1 )); then
      read -p "  ==> Show formate updates diff [y/N]? " -n 1 INPUT
      [ -z "$INPUT" ] && INPUT=n || echo ""
      [[ "$INPUT" == "Y" || "$INPUT" == "y" ]] && git diff
    fi
  else
    exit $ERROR
  fi
fi

if (( GIT_MODE == 0 )); then
  echo -e '        \x1b[1;32m______ ______________  ___  ___ _____   _____ _   __ \x1b[0m'
  echo -e '        \x1b[1;32m|  ___|  _  | ___ \  \/  | / _ \_   _| |  _  | | / / \x1b[0m'
  echo -e '        \x1b[1;32m| |_  | | | | |_/ / .  . |/ /_\ \| |   | | | | |/ /  \x1b[0m'
  echo -e '        \x1b[1;32m|  _| | | | |    /| |\/| ||  _  || |   | | | |    \  \x1b[0m'
  echo -e '        \x1b[1;32m| |   \ \_/ / |\ \| |  | || | | || |   \ \_/ / |\  \ \x1b[0m'
  echo -e '        \x1b[1;32m\_|    \___/\_| \_\_|  |_/\_| |_/\_/    \___/\_| \_/ \x1b[0m'
  echo -e '        \x1b[1;32m                                                     \x1b[0m'
else
  echo -e '        \x1b[1;32m _____ ________  ______  ________ _____   _____ _   __ \x1b[0m'
  echo -e '        \x1b[1;32m/  __ \  _  |  \/  ||  \/  |_   _|_   _| |  _  | | / / \x1b[0m'
  echo -e '        \x1b[1;32m| /  \/ | | | .  . || .  . | | |   | |   | | | | |/ /  \x1b[0m'
  echo -e '        \x1b[1;32m| |   | | | | |\/| || |\/| | | |   | |   | | | |    \  \x1b[0m'
  echo -e '        \x1b[1;32m| \__/\ \_/ / |  | || |  | |_| |_  | |   \ \_/ / |\  \ \x1b[0m'
  echo -e '        \x1b[1;32m \____/\___/\_|  |_/\_|  |_/\___/  \_/    \___/\_| \_/ \x1b[0m'
  echo -e '        \x1b[1;32m                                                       \x1b[0m'
fi
echo -e "  \x1b[1;32m==>\x1b[0m All $NUM_FILES files are OK"

# Add changes
if (( GIT_MODE == 1 )); then
  for I in "${TO_GIT_ADD[@]}"; do
    echo "$I"
    git add "$I"
  done
fi

exit $ERROR
