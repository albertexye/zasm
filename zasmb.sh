#!/bin/sh
# zasm build script
# Usage: ./zasmb <command> [args]
# Commands:
#   build         - Build the zasmb binary if needed and run it with args
#   force_build   - Always rebuild zasmb binary and run it with args
#   run <target>  - Build and run a debug executable for the given target
#   exe <target>  - Build and run a release executable for the given target
#
# This script manages building and running the zasm assembler and related tools.

set -eu

DIR_NAME=$(cd "$(dirname "$0")" && pwd)
BUILD_DIR="$DIR_NAME/out"      # Output directory for build artifacts
SRC_DIR="$DIR_NAME/src"        # Source code directory
ZASMB="$DIR_NAME/zasmb.c"  # Path to the zasmb source file

warnings="-Weverything -Werror -Wno-unsafe-buffer-usage -Wno-declaration-after-statement -Wno-covered-switch-default -Wno-pre-c23-compat -Wno-padded -Wno-format-nonliteral -Wno-c++98-compat -D_DEFAULT_SOURCE"

# Build the zasmb builder binary
_build_builder () {
  mkdir -p "$BUILD_DIR"
  clang -xc -std=c23 -o "$BUILD_DIR/zasmb" "$ZASMB" -O3 -ffast-math -flto -march=native $warnings "-DBUILD_DIR=\"$BUILD_DIR\"" "-DSRC_DIR=\"$SRC_DIR\"" "-DWARNINGS=\"$warnings\""
}

# Build zasmb if needed (if source is newer than binary), then run it
build () {
  src_time=$(stat -c '%Y' "$ZASMB")
  if [ -f "$BUILD_DIR/zasmb" ]; then
    exe_time=$(stat -c '%Y' "$BUILD_DIR/zasmb")
  else
    exe_time=0
  fi
  if [ "$src_time" -gt "$exe_time" ]; then
    _build_builder
  fi
  "$BUILD_DIR/zasmb" $@
}

# Always rebuild zasmb, then run it
force_build () {
  _build_builder
  "$BUILD_DIR/zasmb" $@ f
}

# Build and run a debug executable for the given target
run () {
  if [ "$#" -eq 0 ]; then
    echo "missing executable" >&2
    exit 1
  fi
  build d "$1"
  lowercase=$(printf "%s" "$1" | tr '[:upper:]' '[:lower:]')
  shift
  "$BUILD_DIR/debug/$lowercase/zasm$lowercase" "$@"
}

# Build and run a release executable for the given target
exe () {
  if [ "$#" -eq 0 ]; then
    echo "missing executable" >&2
    exit 1
  fi
  build r "$1"
  lowercase=$(printf "%s" "$1" | tr '[:upper:]' '[:lower:]')
  shift
  "$BUILD_DIR/release/$lowercase/zasm$lowercase" "$@"
}

# Main command dispatch
if [ "$#" -eq 0 ]; then
  echo "missing command" >&2
  echo "commands: build, force_build, run" >&2
  exit 1
fi

case "$1" in
  build|force_build|run|exe)
    "$@"
    ;;
  *)
    echo "'$1' is not a command" >&2
    echo "commands: build, force_build, run" >&2
    exit 1
    ;;
esac

