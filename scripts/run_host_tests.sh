#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "$0")/.."
test_build_dir=$(mktemp -d)
g++ -std=c++11 -Wall -Wextra -Werror -I tests/stubs tests/software_clock_test.cpp -o "$test_build_dir/software_clock_test"
g++ -std=c++11 -Wall -Wextra -Werror -I tests/stubs tests/hardware_test.cpp -o "$test_build_dir/hardware_test"
"$test_build_dir/software_clock_test"
"$test_build_dir/hardware_test"
