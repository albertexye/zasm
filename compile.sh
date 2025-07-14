#!/bin/sh
# zasm compile script

set -eu

DIR_NAME=$(cd "$(dirname "$0")" && pwd)
ZASMB="$DIR_NAME/zasmb.sh"
OUT_DIR="$DIR_NAME/out/programs"

mkdir -p "$OUT_DIR"

NAME=$(basename "$1" | cut -d. -f1)

"$ZASMB" exe c "$1" "$OUT_DIR/$NAME.bin"
"$ZASMB" exe p "$OUT_DIR/$NAME.bin" "$OUT_DIR/$NAME" i
