#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

BUILD_TYPE="${BUILD_TYPE:-Release}"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/build-aigis-sig-minimal}"
JOBS="${JOBS:-}"
MODES="${AIGIS_SIG_MODES:-1;2;3}"
CLEAN=1
TARGET=""
EXTRA_CMAKE_ARGS=()

usage() {
    cat <<'USAGE'
Usage: aigis-sig-minimal/compile.sh [options]

Default:
  Build the minimized Aigis-sig static and shared libraries for modes 1,2,3.

Options:
  --mode MODE        Build one mode: 1, 2, or 3.
  --modes LIST       Build a CMake list of modes, e.g. "1;2;3".
  --release          Build with CMAKE_BUILD_TYPE=Release. This is the default.
  --debug            Build with CMAKE_BUILD_TYPE=Debug.
  --build-dir DIR    Use a custom build directory. Default: build-aigis-sig-minimal.
  --target NAME      Build one CMake target, e.g. pqmagic_aigis_sig_minimal_static.
  --jobs N           Parallel build jobs. Default: number of CPU cores.
  --shake            Use SHAKE instead of the default SM3 hash component.
  --no-clean         Keep the old build directory and build incrementally.
  --help             Show this help.

Examples:
  aigis-sig-minimal/compile.sh
  aigis-sig-minimal/compile.sh --mode 2
  aigis-sig-minimal/compile.sh --target pqmagic_aigis_sig_minimal_static
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
        --target)
            shift
            if [ "$#" -eq 0 ]; then
                echo "error: --target requires a target name" >&2
                exit 2
            fi
            TARGET="$1"
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
            EXTRA_CMAKE_ARGS+=("-DUSE_SHAKE=ON")
            ;;
        --no-clean)
            CLEAN=0
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

if [ "$CLEAN" -eq 1 ]; then
    echo "==> Removing old build directory: $BUILD_DIR"
    rm -rf "$BUILD_DIR"
fi

echo "==> Configuring minimized Aigis-sig"
echo "    build type : $BUILD_TYPE"
echo "    build dir  : $BUILD_DIR"
echo "    modes      : $MODES"
echo "    jobs       : $JOBS"

cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DAIGIS_SIG_MODES="$MODES" \
    "${EXTRA_CMAKE_ARGS[@]}"

echo "==> Building minimized Aigis-sig"
if [ -n "$TARGET" ]; then
    cmake --build "$BUILD_DIR" --target "$TARGET" --parallel "$JOBS"
else
    cmake --build "$BUILD_DIR" --parallel "$JOBS"
fi

echo "==> Done"
echo "    static library: $BUILD_DIR/libpqmagic_aigis_sig_minimal_std.a"
echo "    shared library: $BUILD_DIR/libpqmagic_aigis_sig_minimal_std.so"

