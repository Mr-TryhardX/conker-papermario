#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p artifacts dist/AnimatedDemo
g++ -std=c++20 -O2 -Wall -Wextra -Werror -Isrc -Ithird_party/json src/career.cpp tests/career_test.cpp -o artifacts/career-test
artifacts/career-test artifacts/test-career.json
"${MINGW_CXX:-x86_64-w64-mingw32-g++-posix}" -std=c++20 -O2 -static -municode -mwindows -Wall -Wextra -Werror -Isrc -Ithird_party/json src/career.cpp src/animated_demo.cpp -lopengl32 -lglu32 -lgdi32 -luser32 -lwinmm -o dist/AnimatedDemo/AnimatedDemo.exe
if [[ -d assets ]]; then
  cp -r assets dist/AnimatedDemo/
elif [[ -d ../assets ]]; then
  cp -r ../assets dist/AnimatedDemo/
else
  echo 'Music assets absent from source checkout; copy them from your extracted demo or capture your supplied ROM.' >&2
fi
echo 'Animated Windows prototype built; career tests passed.'
