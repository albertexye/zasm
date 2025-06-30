#!/bin/sh
# zasm build script
# Usage: ./zasmb <command> [args]
# Commands:
#   build         - Build the zasmb binary if needed and run it with args
#   force_build   - Always rebuild zasmb binary and run it with args
#   run <target>  - Build and run a debug executable for the given target
#
# This script manages building and running the zasm assembler and related tools.

set -eu

build_dir="out"      # Output directory for build artifacts
src_dir="src"        # Source code directory

warnings="-Weverything -Werror -Wno-unsafe-buffer-usage -Wno-declaration-after-statement -Wno-covered-switch-default -Wno-pre-c23-compat -Wno-padded -Wno-format-nonliteral -Wno-c++98-compat -D_DEFAULT_SOURCE"

# Build the zasmb builder binary
_build_builder () {
  mkdir -p "$build_dir"
  clang -xc -std=c23 -o "$build_dir/zasmb" zasmb.c -O3 -ffast-math -flto -march=native $warnings "-DBUILD_DIR=\"$build_dir\"" "-DSRC_DIR=\"$src_dir\"" "-DWARNINGS=\"$warnings\""
}

# Build zasmb if needed (if source is newer than binary), then run it
build () {
  src_time=$(stat -c '%Y' zasmb.c)
  if [ -f "$build_dir/zasmb" ]; then
    exe_time=$(stat -c '%Y' "$build_dir/zasmb")
  else
    exe_time=0
  fi
  if (( src_time > exe_time )); then
    _build_builder
  fi
  "$build_dir/zasmb" $@
}

# Always rebuild zasmb, then run it
force_build () {
  _build_builder
  "$build_dir/zasmb" $@ f
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
  "$build_dir/debug/$lowercase/zasm$lowercase" "$@"
}

# Main command dispatch
if [ "$#" -eq 0 ]; then
  echo "missing command" >&2
  echo "commands: build, force_build, run" >&2
  exit 1
fi

case "$1" in
  build|force_build|run)
    "$@"
    ;;
  *)
    echo "'$1' is not a command" >&2
    echo "commands: build, force_build, run" >&2
    exit 1
    ;;
esac

