#!/bin/sh

set -eu

CC=${CC:-cc}
XLLM_DIR=${XLLM_DIR:-../xllm}
XRT_DIR=${XRT_DIR:-../xrt}
BUILD_DIR=${BUILD_DIR:-build}
RELEASE_DIR=${RELEASE_DIR:-release}
RUN_TESTS=${RUN_TESTS:-1}
CFLAGS=${CFLAGS:-"-D_GNU_SOURCE -std=c11 -Wall -Wextra -Werror -O2 -ffunction-sections -fdata-sections"}
XRT_CFLAGS=${XRT_CFLAGS:-"$CFLAGS -Wno-pointer-sign -Wno-unused-function"}
LDFLAGS=${LDFLAGS:-"-Wl,--gc-sections"}
LIBS=${LIBS:-"-pthread -ldl -lm"}

mkdir -p "$BUILD_DIR" "$RELEASE_DIR"

$CC $CFLAGS -I. -I"$XLLM_DIR" -I"$XRT_DIR" -c xwork.c -o "$RELEASE_DIR/xwork.o"
$CC $XRT_CFLAGS $LDFLAGS -I. -I"$XLLM_DIR" -I"$XRT_DIR" \
    tests/test_xwork.c "$XRT_DIR/xrt.c" $LIBS -o "$BUILD_DIR/test_xwork"

if [ "$RUN_TESTS" = "1" ]; then
    "$BUILD_DIR/test_xwork"
fi

printf '\nxwork build: PASS\n'
