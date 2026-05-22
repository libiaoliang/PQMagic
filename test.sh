#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

BUILD_TYPE="${BUILD_TYPE:-Debug}"
BUILD_DIR="${BUILD_DIR:-build-debug}"
JOBS="${JOBS:-}"
CLEAN=1
NO_BUILD=0
LIST_ONLY=0
MODE="all"
FILTER=""

SMOKE_TESTS=(
    test_ml_kem_512
    test_ml_dsa_44
    test_aigis_enc_1
    test_slh_dsa_sm3_128f_simple
)

usage() {
    cat <<'USAGE'
Usage: ./test.sh [options]

Default:
  Build Debug test executables, skip benchmark executables, and run all test_* programs.

Options:
  --all             Run all generated test_* programs. This is the default.
  --smoke           Run a small representative test set.
  --list            Build if needed, then list selected tests without running them.
  --filter TEXT     Run/list tests whose names contain TEXT.
  --debug           Build with CMAKE_BUILD_TYPE=Debug. This is the default.
  --release         Build with CMAKE_BUILD_TYPE=Release.
  --build-dir DIR   Use a custom build directory. Default: build-debug.
  --jobs N          Parallel build jobs. Default: number of CPU cores.
  --no-clean        Keep the old build directory and build incrementally.
  --no-build        Do not build first; only use existing test executables.
  --help            Show this help.

Examples:
  ./test.sh
  ./test.sh --smoke
  ./test.sh --filter ml_kem
  ./test.sh --list
  ./test.sh --release --build-dir build-release
USAGE
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --all)
            MODE="all"
            ;;
        --smoke)
            MODE="smoke"
            ;;
        --list)
            LIST_ONLY=1
            ;;
        --filter)
            shift
            if [ "$#" -eq 0 ]; then
                echo "error: --filter requires text" >&2
                exit 2
            fi
            FILTER="$1"
            ;;
        --debug)
            BUILD_TYPE="Debug"
            ;;
        --release)
            BUILD_TYPE="Release"
            if [ "$BUILD_DIR" = "build-debug" ]; then
                BUILD_DIR="build-release"
            fi
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
        --no-clean)
            CLEAN=0
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

if [ "$NO_BUILD" -eq 0 ]; then
    if [ "$CLEAN" -eq 1 ]; then
        echo "==> Removing old build directory"
        rm -rf "$BUILD_DIR"
    fi

    if [ -z "$JOBS" ]; then
        if command -v nproc >/dev/null 2>&1; then
            JOBS="$(nproc)"
        elif command -v sysctl >/dev/null 2>&1; then
            JOBS="$(sysctl -n hw.ncpu)"
        else
            JOBS="2"
        fi
    fi

    echo "==> Configuring PQMagic tests"
    echo "    build type : $BUILD_TYPE"
    echo "    build dir  : $BUILD_DIR"
    echo "    jobs       : $JOBS"

    cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DENABLE_TEST=ON \
        -DENABLE_BENCH=OFF

    echo "==> Building PQMagic tests"
    cmake --build "$BUILD_DIR" --parallel "$JOBS"
fi

if [ ! -d "$BUILD_DIR" ]; then
    echo "error: build directory does not exist: $BUILD_DIR" >&2
    echo "hint: run ./test.sh without --no-build first" >&2
    exit 2
fi

SELECTED_TESTS=()
if [ "$MODE" = "smoke" ]; then
    SELECTED_TESTS=("${SMOKE_TESTS[@]}")
else
    while IFS= read -r test_path; do
        SELECTED_TESTS+=("$(basename "$test_path")")
    done < <(find "$BUILD_DIR" -maxdepth 1 -type f -executable -name 'test_*' | sort)
fi

if [ -n "$FILTER" ]; then
    FILTERED_TESTS=()
    for test_name in "${SELECTED_TESTS[@]}"; do
        if [[ "$test_name" == *"$FILTER"* ]]; then
            FILTERED_TESTS+=("$test_name")
        fi
    done
    SELECTED_TESTS=("${FILTERED_TESTS[@]}")
fi

if [ "${#SELECTED_TESTS[@]}" -eq 0 ]; then
    echo "error: no tests selected" >&2
    exit 2
fi

if [ "$LIST_ONLY" -eq 1 ]; then
    printf '%s\n' "${SELECTED_TESTS[@]}"
    echo "Total: ${#SELECTED_TESTS[@]}"
    exit 0
fi

echo "==> Running ${#SELECTED_TESTS[@]} test(s) from $BUILD_DIR"
FAILED_TESTS=()

for test_name in "${SELECTED_TESTS[@]}"; do
    test_path="$BUILD_DIR/$test_name"
    if [ ! -x "$test_path" ]; then
        echo "FAIL $test_name (missing executable)"
        FAILED_TESTS+=("$test_name")
        continue
    fi

    echo "==> $test_name"
    if "$test_path"; then
        echo "PASS $test_name"
    else
        echo "FAIL $test_name"
        FAILED_TESTS+=("$test_name")
    fi
done

if [ "${#FAILED_TESTS[@]}" -ne 0 ]; then
    echo "==> ${#FAILED_TESTS[@]} test(s) failed:"
    printf '  %s\n' "${FAILED_TESTS[@]}"
    exit 1
fi

echo "==> All selected tests passed"
