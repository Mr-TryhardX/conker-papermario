#!/usr/bin/env bash
set -euo pipefail
TASK_ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
if [[ -f /workspace/toolchain/activate.sh ]]; then source /workspace/toolchain/activate.sh; fi
TASK_GAME_DIR="$TASK_ROOT/native/HybridHeaven"
TASK_MODEL_PATH="${HH_KANE_MODEL_PATH:-$TASK_ROOT/asset-import/kane-displaylists/kane-displaylists.bin}"
[[ -f "$TASK_MODEL_PATH" ]] || { echo "Missing generated Kane archive: $TASK_MODEL_PATH" >&2; exit 1; }
[[ -x "$TASK_GAME_DIR/build-rendered/HybridHeavenRecompiled" ]] || { echo "Build the native rendered target first." >&2; exit 1; }
export HH_KANE_IN_GAME=1 HH_KANE_MODEL_PATH="$TASK_MODEL_PATH"
export HH_AUTOSTART="${HH_AUTOSTART:-1}"
cd -- "$TASK_GAME_DIR"
exec ./build-rendered/HybridHeavenRecompiled "$@"
