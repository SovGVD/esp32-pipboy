#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
python3 scripts/build_inventory.py --check
build_dir=$(mktemp -d /tmp/pipboy-tests.XXXXXX)
trap 'rm -rf "$build_dir"' EXIT
"${CXX:-g++}" -std=c++11 -Wall -Wextra -Werror -g -fsanitize=address,undefined \
    -fno-omit-frame-pointer -I lib/Id3Metadata/src \
    test/host/id3_test.cpp lib/Id3Metadata/src/Id3Metadata.cpp -o "$build_dir/id3_test"
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$build_dir/id3_test"
"${CXX:-g++}" -std=c++11 -Wall -Wextra -Werror -g -fsanitize=address,undefined \
    -fno-omit-frame-pointer -I lib/AudioBoost/src \
    test/host/audio_boost_test.cpp -o "$build_dir/audio_boost_test"
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$build_dir/audio_boost_test"
"${CXX:-g++}" -std=c++17 -Wall -Wextra -Werror -g -pthread -fsanitize=address,undefined \
    -fno-omit-frame-pointer -I test/host/stubs -I include -I lib/Id3Metadata/src -I lib/AudioBoost/src \
    test/host/music_test.cpp src/MusicPlayer.cpp lib/Id3Metadata/src/Id3Metadata.cpp \
    -o "$build_dir/music_test"
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$build_dir/music_test"
"${CXX:-g++}" -std=c++11 -Wall -Wextra -Werror -g -fsanitize=address,undefined \
    -fno-omit-frame-pointer -I include -I lib/Inventory/src \
    test/host/inventory_test.cpp lib/Inventory/src/Inventory.cpp -o "$build_dir/inventory_test"
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$build_dir/inventory_test"
