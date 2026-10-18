#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$ROOT"
for tool in make gcc arm-none-eabi-gcc arm-none-eabi-objcopy arm-none-eabi-nm python3; do
 command -v "$tool" >/dev/null || { echo "Missing existing tool: $tool" >&2; exit 1; }
done
python3 scripts/prepare.py
mkdir -p build/logs
BOOT="$ROOT/vendor/wolfBoot"
# A changed board configuration must never reuse stale objects.
make -C "$BOOT" WOLFBOOT_ROOT="$BOOT" clean >build/logs/clean.log 2>&1
make -C "$BOOT" WOLFBOOT_ROOT="$BOOT" keytools 2>&1 | tee build/logs/keytools.log
# Generate once. Never silently rotate the trust root on a rebuild.
if [[ ! -f "$BOOT/wolfboot_signing_private_key.der" ]]; then
 (cd "$BOOT" && tools/keytools/keygen --ed25519 -g wolfboot_signing_private_key.der)
 chmod 600 "$BOOT/wolfboot_signing_private_key.der"
fi
[[ -s "$BOOT/src/keystore.c" ]] || { echo 'Missing matching keystore; restore key and keystore together' >&2; exit 1; }
make -C "$BOOT" WOLFBOOT_ROOT="$BOOT" -j"${JOBS:-2}" wolfboot.bin test-app/image.bin 2>&1 | tee build/logs/target.log
cp "$BOOT/wolfboot.bin" "$BOOT/wolfboot.elf" "$BOOT/test-app/image.bin" "$BOOT/test-app/image.elf" build/
[[ ! -f "$BOOT/wolfboot.map" ]] || cp "$BOOT/wolfboot.map" build/
cp "$BOOT/test-app/image.map" build/app.map
arm-none-eabi-nm -n build/wolfboot.elf >build/boot-symbols.txt
arm-none-eabi-nm -n build/image.elf >build/app-symbols.txt
bash scripts/sign.sh 1
bash scripts/sign.sh 2
bash scripts/sign.sh 3
python3 scripts/images.py factory build/wolfboot.bin build/image_v1_signed.bin build/factory.bin
python3 scripts/images.py inspect build/image_v2_signed.bin
{
 date -u; arm-none-eabi-gcc --version; gcc --version; make --version
 sha256sum .config build/*.bin
} >build/logs/versions-and-hashes.txt
echo 'Build complete. No flash operation was performed. Read docs/02-debug.md before flashing.'
