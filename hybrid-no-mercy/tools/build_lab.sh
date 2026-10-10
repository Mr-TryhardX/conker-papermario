#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p artifacts dist/CareerLab
g++ -std=c++20 -O2 -Wall -Wextra -Werror -Isrc -Ithird_party/json src/career.cpp tests/career_test.cpp -o artifacts/career-test
artifacts/career-test artifacts/test-career.json
"${MINGW_CXX:-x86_64-w64-mingw32-g++-posix}" -std=c++20 -O2 -static -municode -mwindows -Wall -Wextra -Werror -Isrc -Ithird_party/json src/career.cpp src/windows_lab.cpp -lgdi32 -luser32 -lwinmm -lshell32 -lcomdlg32 -o dist/CareerLab/CareerLab.exe
if [[ -d assets/audio ]]; then
  mkdir -p dist/CareerLab/assets
  cp -r assets/audio dist/CareerLab/assets/
fi
echo 'Windows CareerLab.exe built; career engine tests passed.'
