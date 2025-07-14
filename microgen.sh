#!/bin/sh
# zasm microcode generation script

set -eu

DIR_NAME=$(cd "$(dirname "$0")" && pwd)
ZASMB="$DIR_NAME/zasmb.sh"
OUT_DIR="$DIR_NAME/out/microgen"

mkdir -p "$OUT_DIR"

# microcode
"$ZASMB" exe m "$OUT_DIR/m0.bin" 0
"$ZASMB" exe m "$OUT_DIR/m1.bin" 1
"$ZASMB" exe m "$OUT_DIR/m2.bin" 2
"$ZASMB" exe p "$OUT_DIR/m0.bin" "$OUT_DIR/m0" m
"$ZASMB" exe p "$OUT_DIR/m1.bin" "$OUT_DIR/m1" m
"$ZASMB" exe p "$OUT_DIR/m2.bin" "$OUT_DIR/m2" m

# display
"$ZASMB" exe n "$OUT_DIR/n0.bin" 0
"$ZASMB" exe n "$OUT_DIR/n1.bin" 1
"$ZASMB" exe p "$OUT_DIR/n0.bin" "$OUT_DIR/n0" n
"$ZASMB" exe p "$OUT_DIR/n1.bin" "$OUT_DIR/n1" n
