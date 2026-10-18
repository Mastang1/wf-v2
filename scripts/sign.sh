#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$ROOT"
version=${1:?Usage: bash scripts/sign.sh VERSION}
[[ "$version" =~ ^[1-9][0-9]*$ ]] || { echo 'Version must be positive decimal' >&2; exit 1; }
python3 scripts/images.py raw build/image.bin
BOOT="$ROOT/vendor/wolfBoot"
[[ -s "$BOOT/wolfboot_signing_private_key.der" ]] || exit 1
"$BOOT/tools/keytools/sign" --ed25519 --sha256 build/image.bin "$BOOT/wolfboot_signing_private_key.der" "$version"
python3 scripts/images.py inspect "build/image_v${version}_signed.bin"
