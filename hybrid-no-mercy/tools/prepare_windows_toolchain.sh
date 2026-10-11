#!/usr/bin/env bash
# Keep all SDK files and compatibility links inside the cloud workspace.
set -euo pipefail
TASK_TOOLCHAIN=${HH_CLOUD_TOOLCHAIN:-/workspace/toolchain}
source "$TASK_TOOLCHAIN/activate.sh"
[[ -x "$TASK_TOOLCHAIN/bin/xwin" ]] || { echo 'Install xwin 0.10 before preparing the Windows SDK.' >&2; exit 1; }
if [[ ! -f "$TASK_TOOLCHAIN/msvc/crt/include/vector" ]]; then
  "$TASK_TOOLCHAIN/bin/xwin" --accept-license --cache-dir "$TASK_TOOLCHAIN/xwin-cache" --arch x86_64 splat --output "$TASK_TOOLCHAIN/msvc"
fi
ln -sfn x86_64 "$TASK_TOOLCHAIN/msvc/crt/lib/x64"
ln -sfn include "$TASK_TOOLCHAIN/msvc/sdk/Include"
ln -sfn lib "$TASK_TOOLCHAIN/msvc/sdk/Lib"
ln -sfn x86_64 "$TASK_TOOLCHAIN/msvc/sdk/lib/um/x64"
ln -sfn x86_64 "$TASK_TOOLCHAIN/msvc/sdk/lib/ucrt/x64"
# RT64 uses a Windows-tolerated casing variant; Linux SDK paths are sensitive.
ln -sfn ShlObj.h "$TASK_TOOLCHAIN/msvc/sdk/include/um/Shlobj.h"
ln -sfn ShellScalingApi.h "$TASK_TOOLCHAIN/msvc/sdk/include/um/ShellScalingAPI.h"
ln -sfn winmm.lib "$TASK_TOOLCHAIN/msvc/sdk/lib/um/x86_64/Winmm.lib"
ln -sfn shcore.lib "$TASK_TOOLCHAIN/msvc/sdk/lib/um/x86_64/Shcore.lib"
for TASK_BINARY in lld-link llvm-rc llvm-mt llvm-lib; do
  [[ -x "$TASK_TOOLCHAIN/sysroot/usr/lib/llvm-19/bin/$TASK_BINARY" ]] || { echo "Missing LLVM 19 tool: $TASK_BINARY" >&2; exit 1; }
  ln -sfn "../sysroot/usr/lib/llvm-19/bin/$TASK_BINARY" "$TASK_TOOLCHAIN/bin/$TASK_BINARY"
done
TASK_WRAPPER=$(mktemp "$TASK_TOOLCHAIN/bin/clang-cl.XXXXXX")
trap 'rm -f "$TASK_WRAPPER"' EXIT
cat > "$TASK_WRAPPER" <<'WRAPPER'
#!/usr/bin/env bash
TASK_BIN=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
exec "$TASK_BIN/clang" --driver-mode=cl "$@"
WRAPPER
if ! cmp -s "$TASK_WRAPPER" "$TASK_TOOLCHAIN/bin/clang-cl"; then
  mv "$TASK_WRAPPER" "$TASK_TOOLCHAIN/bin/clang-cl"
fi
chmod +x "$TASK_TOOLCHAIN/bin/clang-cl"
"$TASK_TOOLCHAIN/bin/clang-cl" --version
