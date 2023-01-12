#!/bin/bash

GIT_HOOKS_DIR="gitHooks"

die() {
  echo "$@"
  exit 1
}

cd "$(dirname "$0")"
ROOT_DIR="$PWD"

[ ! -d $GIT_HOOKS_DIR ] && die "Unable to find git hooks dir"

cd $GIT_HOOKS_DIR

for I in *; do
  echo "Updating hook $I"
  [ ! -x $I ] && die "$I is not executable"
  [ -e "$ROOT_DIR/.git/hooks/$I" ] && rm "$ROOT_DIR/.git/hooks/$I"
  ln -s "../../$GIT_HOOKS_DIR/$I" "$ROOT_DIR/.git/hooks/$I"
done

echo "DONE"
