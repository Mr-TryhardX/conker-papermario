#!/usr/bin/env bash
# Configure the original native port with the cloud's xwin SDK and host tools.
# Run tools/apply_native_integration.py and build the Linux host tools first.
set -euo pipefail
TASK_ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
TASK_TOOLCHAIN=${HH_CLOUD_TOOLCHAIN:-/workspace/toolchain}
source "$TASK_TOOLCHAIN/activate.sh"
TASK_NATIVE="$TASK_ROOT/native/HybridHeaven"
TASK_BUILD=${HH_WINDOWS_BUILD_DIR:-"$TASK_NATIVE/build-win-ready"}
TASK_HOST_CONVERTER="$TASK_NATIVE/build-rendered/rt64/src/tools/file_to_c/file_to_c"
for TASK_REQUIRED in "$TASK_TOOLCHAIN/bin/clang-cl" "$TASK_TOOLCHAIN/bin/lld-link" "$TASK_TOOLCHAIN/bin/llvm-lib" "$TASK_TOOLCHAIN/bin/llvm-mt" "$TASK_TOOLCHAIN/bin/llvm-rc" "$TASK_HOST_CONVERTER"; do
  [[ -x "$TASK_REQUIRED" ]] || { echo "Missing host tool: $TASK_REQUIRED" >&2; exit 1; }
done
[[ -d "$TASK_NATIVE/lib/freetype-windows-binaries/include" ]] || { echo 'Initialize the upstream FreeType Windows dependency (v2.13.2).' >&2; exit 1; }
cmake -S "$TASK_NATIVE" -B "$TASK_BUILD" -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_TOOLCHAIN_FILE="$TASK_NATIVE/cmake/toolchain-windows-clang-cl.cmake" \
  -DHH_WIN_VCTOOLS="$TASK_TOOLCHAIN/msvc/crt" \
  -DHH_WIN_SDK="$TASK_TOOLCHAIN/msvc/sdk" \
  -DHH_WIN_SDK_VERSION=10.0.26100 \
  '-DCMAKE_TRY_COMPILE_PLATFORM_VARIABLES=HH_WIN_VCTOOLS;HH_WIN_SDK;HH_WIN_SDK_VERSION' \
  -DCMAKE_TRY_COMPILE_CONFIGURATION=Release \
  -DCMAKE_AR="$TASK_TOOLCHAIN/bin/llvm-lib" \
  -DCMAKE_MT="$TASK_TOOLCHAIN/bin/llvm-mt" \
  -DRT64_FILE_TO_C="$TASK_HOST_CONVERTER" "$@"
echo "Build with: cmake --build $TASK_BUILD -j 6"
echo 'Windows runtime must select the Vulkan graphics backend.'
