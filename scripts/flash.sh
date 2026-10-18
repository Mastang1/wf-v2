#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname -- "${BASH_SOURCE[0]}")/.."
kind=${1:-}; image=${2:-}
[[ "${BOARD_CONFIRMED:-}" == 'STM32F407ZGT6-1MiB-8MHz' ]] || {
 echo 'Check chip marking, 1024KiB flash, 8MHz HSE, 3.3V and backup first.' >&2
 echo 'Then export BOARD_CONFIRMED=STM32F407ZGT6-1MiB-8MHz' >&2;exit 1;
}
[[ "$image" =~ ^build/[A-Za-z0-9_.-]+\.bin$ && -s "$image" ]] || { echo 'Use a generated build/*.bin path' >&2;exit 1; }
case "$kind" in
 factory)
  [[ "$image" == build/factory.bin ]] || exit 1
  # Recreate factory with guards so stale or foreign factory binaries cannot bypass checks.
  python3 scripts/images.py factory build/wolfboot.bin build/image_v1_signed.bin build/factory.bin
  address=0x08000000
  ;;
 update)
  python3 scripts/images.py inspect "$image"
  address=0x08060000
  ;;
 *) echo 'Usage: bash scripts/flash.sh factory build/factory.bin | update build/image_v2_signed.bin';exit 1;;
esac
# Probe and enforce silicon family plus 1024KiB flash before every write.
openocd -f config/openocd-explorer.cfg -c "init; reset halt; set ident [mrw 0xE0042000]; if {[expr {\$ident & 0xfff}] != 0x413} {error {not STM32F405/407 family}}; set cap [mrh 0x1FFF7A22]; if {\$cap != 1024} {error {not 1024KiB flash}}; program $image $address verify; reset halt; shutdown"
echo 'Written and verified; CPU halted. UPDATE write alone does not trigger upgrade. See docs/02-debug.md.'
