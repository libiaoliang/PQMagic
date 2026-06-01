#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/build-aigis-sig1-demo}"
JOBS="${JOBS:-}"
CC_BIN="${CC:-cc}"
DEMO_SRC="$ROOT_DIR/aigis_sig1_sign_10s_demo.c"
DEMO_BIN="$BUILD_DIR/aigis_sig1_sign_10s_demo"

if [ -z "$JOBS" ]; then
    if command -v nproc >/dev/null 2>&1; then
        JOBS="$(nproc)"
    else
        JOBS="2"
    fi
fi

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_ML_KEM=OFF \
    -DENABLE_KYBER=OFF \
    -DENABLE_AIGIS_ENC=OFF \
    -DENABLE_ML_DSA=OFF \
    -DENABLE_DILITHIUM=OFF \
    -DENABLE_AIGIS_SIG=ON \
    -DAIGIS_SIG_MODES=1 \
    -DENABLE_SLH_DSA=OFF \
    -DENABLE_SPHINCS_A=OFF \
    -DENABLE_TEST=OFF \
    -DENABLE_BENCH=OFF

cmake --build "$BUILD_DIR" --target pqmagic_static_target --parallel "$JOBS"

"$CC_BIN" -O2 -std=c11 \
    -I"$ROOT_DIR/include" \
    "$DEMO_SRC" "$BUILD_DIR/libpqmagic_std.a" \
    -o "$DEMO_BIN"

echo "Built: $DEMO_BIN"
