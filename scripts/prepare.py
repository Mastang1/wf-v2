#!/usr/bin/env python3
"""Apply only reviewed board overlays, refusing unknown local edits."""
from pathlib import Path
import hashlib,json,shutil
root=Path(__file__).resolve().parents[1]; boot=root/'vendor/wolfBoot'
manifest=json.loads((root/'patches/manifest.json').read_text())
for name,rec in manifest.items():
 p=boot/name; digest=hashlib.sha256(p.read_bytes()).hexdigest()
 if digest not in (rec['before'],rec['after']):
  raise SystemExit('Refuse to overwrite edited source: '+name)
for name,rec in manifest.items():
 shutil.copyfile(root/'patches/overlay'/name,boot/name)
config=(root/'config/explorer-f407.config').read_bytes()
if (root/'.config').read_bytes()!=config:
 raise SystemExit('Root .config differs; update both reviewed config files first')
if (boot/'.config').exists() and (boot/'.config').read_bytes()!=config:
 raise SystemExit('Refuse to replace a different vendor/wolfBoot/.config')
(boot/'.config').write_bytes(config)
if not (boot/'lib/wolfssl/wolfssl/wolfcrypt/settings.h').exists():
 raise SystemExit('Missing pinned wolfssl source; see sources.lock.json')
print('Board overlays and configuration ready (not hardware validation).')
