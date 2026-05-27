#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

BUILD_TYPE="${BUILD_TYPE:-Debug}"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/build-aigis-sig-minimal-test}"
JOBS="${JOBS:-}"
MODES="${AIGIS_SIG_MODES:-1;2;3}"
CLEAN=1
NO_BUILD=0
EXTRA_COMPILE_ARGS=()
ASAN=0
RANDOMBYTES_SOURCE=""

usage() {
    cat <<'USAGE'
Usage: aigis-sig-minimal/test.sh [options]

Default:
  Build the minimized Aigis-sig static library and run the original
  sig/aigis-sig/std/test/test_aigis.c test for modes 1,2,3.

Options:
  --mode MODE        Test one mode: 1, 2, or 3.
  --modes LIST       Test a CMake list of modes, e.g. "1;2;3".
  --release          Build with CMAKE_BUILD_TYPE=Release.
  --debug            Build with CMAKE_BUILD_TYPE=Debug. This is the default.
  --build-dir DIR    Use a custom build directory. Default: build-aigis-sig-minimal-test.
  --jobs N           Parallel build jobs. Default: number of CPU cores.
  --shake            Use SHAKE instead of the default SM3 hash component.
  --asan             Build and link tests with AddressSanitizer instrumentation.
  --randombytes-source FILE
                     Use FILE as the randombytes.c implementation.
  --no-clean         Keep the old build directory and build incrementally.
  --no-build         Do not build first; use an existing minimized library.
  --help             Show this help.

Examples:
  aigis-sig-minimal/test.sh
  aigis-sig-minimal/test.sh --mode 2
USAGE
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --mode)
            shift
            if [ "$#" -eq 0 ]; then
                echo "error: --mode requires 1, 2, or 3" >&2
                exit 2
            fi
            MODES="$1"
            ;;
        --modes)
            shift
            if [ "$#" -eq 0 ]; then
                echo "error: --modes requires a CMake list such as \"1;2;3\"" >&2
                exit 2
            fi
            MODES="$1"
            ;;
        --release)
            BUILD_TYPE="Release"
            ;;
        --debug)
            BUILD_TYPE="Debug"
            ;;
        --build-dir)
            shift
            if [ "$#" -eq 0 ]; then
                echo "error: --build-dir requires a directory" >&2
                exit 2
            fi
            BUILD_DIR="$1"
            ;;
        --jobs)
            shift
            if [ "$#" -eq 0 ]; then
                echo "error: --jobs requires a number" >&2
                exit 2
            fi
            JOBS="$1"
            ;;
        --shake)
            EXTRA_COMPILE_ARGS+=("--shake")
            ;;
        --asan)
            ASAN=1
            EXTRA_COMPILE_ARGS+=("--asan")
            ;;
        --randombytes-source)
            shift
            if [ "$#" -eq 0 ]; then
                echo "error: --randombytes-source requires a C file" >&2
                exit 2
            fi
            RANDOMBYTES_SOURCE="$1"
            EXTRA_COMPILE_ARGS+=("--randombytes-source" "$1")
            ;;
        --no-clean)
            CLEAN=0
            EXTRA_COMPILE_ARGS+=("--no-clean")
            ;;
        --no-build)
            NO_BUILD=1
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            echo "error: unknown option: $1" >&2
            usage >&2
            exit 2
            ;;
    esac
    shift
done

if [ -z "$JOBS" ]; then
    if command -v nproc >/dev/null 2>&1; then
        JOBS="$(nproc)"
    else
        JOBS="2"
    fi
fi

if [ "$NO_BUILD" -eq 0 ]; then
    compile_args=(
        "--build-dir" "$BUILD_DIR"
        "--modes" "$MODES"
        "--target" "pqmagic_aigis_sig_minimal_static"
        "--jobs" "$JOBS"
    )

    if [ "$BUILD_TYPE" = "Release" ]; then
        compile_args+=("--release")
    else
        compile_args+=("--debug")
    fi

    if [ "$CLEAN" -eq 0 ]; then
        compile_args+=("--no-clean")
    fi

    compile_args+=("${EXTRA_COMPILE_ARGS[@]}")
    "$SCRIPT_DIR/compile.sh" "${compile_args[@]}"
fi

LIB_PATH="$BUILD_DIR/libpqmagic_aigis_sig_minimal_std.a"
if [ ! -f "$LIB_PATH" ]; then
    echo "error: minimized static library not found: $LIB_PATH" >&2
    echo "hint: run aigis-sig-minimal/test.sh without --no-build first" >&2
    exit 2
fi

IFS=';' read -r -a MODE_ARRAY <<< "$MODES"
MODE_DEFINES=()
ASAN_FLAGS=()
if [ "$ASAN" -eq 1 ]; then
    ASAN_FLAGS=(-fsanitize=address -fno-omit-frame-pointer -g)
fi

for mode in "${MODE_ARRAY[@]}"; do
    case "$mode" in
        1)
            MODE_DEFINES+=("-DPQMAGIC_AIGIS_ENABLE_SIG1")
            ;;
        2)
            MODE_DEFINES+=("-DPQMAGIC_AIGIS_ENABLE_SIG2")
            ;;
        3)
            MODE_DEFINES+=("-DPQMAGIC_AIGIS_ENABLE_SIG3")
            ;;
        *)
            echo "error: unsupported AIGIS_SIG_MODE: $mode" >&2
            exit 2
            ;;
    esac
done

echo "==> Running minimized Aigis-sig tests"
for mode in "${MODE_ARRAY[@]}"; do
    case "$mode" in
        1|2|3)
            ;;
        *)
            echo "error: unsupported AIGIS_SIG_MODE: $mode" >&2
            exit 2
            ;;
    esac

    test_bin="$BUILD_DIR/test_aigis_sig_${mode}_minimal"
    cc -I"$ROOT_DIR" \
       -I"$ROOT_DIR/include" \
       -I"$ROOT_DIR/utils" \
       -I"$ROOT_DIR/sig/aigis-sig/std" \
       -DAIGIS_SIG_MODE="$mode" \
       "${ASAN_FLAGS[@]}" \
       "$ROOT_DIR/sig/aigis-sig/std/test/test_aigis.c" \
       "$LIB_PATH" \
       -o "$test_bin"

    "$test_bin"
    echo "PASS mode $mode"
done

provider_test_bin="$BUILD_DIR/test_aigis_provider_minimal"
cc -I"$SCRIPT_DIR/include" \
   -I"$SCRIPT_DIR/src/provider_compat" \
   -I"$ROOT_DIR" \
   -I"$ROOT_DIR/include" \
   -I"$ROOT_DIR/utils" \
   -I"$ROOT_DIR/sig/aigis-sig/std" \
   "${MODE_DEFINES[@]}" \
   "${ASAN_FLAGS[@]}" \
   "$SCRIPT_DIR/test/provider_smoke.c" \
   "$LIB_PATH" \
   -o "$provider_test_bin"

"$provider_test_bin"

echo "==> All minimized Aigis-sig tests passed"
