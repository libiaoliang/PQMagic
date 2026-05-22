#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

BUILD_TYPE="${BUILD_TYPE:-Debug}"
BUILD_DIR="${BUILD_DIR:-build-debug}"
JOBS="${JOBS:-}"
TARGET=""
CLEAN=1
EXTRA_CMAKE_ARGS=()

usage() {
    cat <<'USAGE'
Usage: ./build.sh [options]

Default:
  Build PQMagic libraries in Debug mode. Test and benchmark programs are not built.

Options:
  --release          Build with CMAKE_BUILD_TYPE=Release.
  --debug            Build with CMAKE_BUILD_TYPE=Debug. This is the default.
  --build-dir DIR    Use a custom build directory. Default: build-debug.
  --jobs N           Parallel build jobs. Default: number of CPU cores.
  --target NAME      Build one CMake target, for example pqmagic_static_target.
  --no-clean         Keep the old build directory and build incrementally.
  --shake            Use SHAKE instead of the default SM3 hash component.
  --help             Show this help.

Examples:
  ./build.sh
  ./build.sh --release --build-dir build-release
  ./build.sh --target pqmagic_static_target
USAGE
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --release)
            BUILD_TYPE="Release"
            if [ "$BUILD_DIR" = "build-debug" ]; then
                BUILD_DIR="build-release"
            fi
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
        --target)
            shift
            if [ "$#" -eq 0 ]; then
                echo "error: --target requires a target name" >&2
                exit 2
            fi
            TARGET="$1"
            ;;
        --no-clean)
            CLEAN=0
            ;;
        --shake)
            EXTRA_CMAKE_ARGS+=("-DUSE_SHAKE=ON")
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
    elif command -v sysctl >/dev/null 2>&1; then
        JOBS="$(sysctl -n hw.ncpu)"
    else
        JOBS="2"
    fi
fi

echo "==> Configuring PQMagic"
echo "    build type : $BUILD_TYPE"
echo "    build dir  : $BUILD_DIR"
echo "    jobs       : $JOBS"

if [ "$CLEAN" -eq 1 ]; then
    echo "==> Removing old build directory"
    rm -rf "$BUILD_DIR"
fi

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DENABLE_TEST=OFF \
    -DENABLE_BENCH=OFF \
    "${EXTRA_CMAKE_ARGS[@]}"

echo "==> Building PQMagic"
if [ -n "$TARGET" ]; then
    cmake --build "$BUILD_DIR" --target "$TARGET" --parallel "$JOBS"
else
    cmake --build "$BUILD_DIR" --parallel "$JOBS"
fi

echo "==> Done"
echo "    libraries are in: $BUILD_DIR"
