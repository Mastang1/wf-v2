#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname -- "${BASH_SOURCE[0]}")/.."
mkdir -p build/logs
bash scripts/openocd.sh -c 'init; reset halt; mdw 0xE0042000 1; mdh 0x1FFF7A22 1; flash banks; flash info 0; shutdown' 2>&1 | tee build/logs/probe.log
