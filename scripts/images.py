#!/usr/bin/env python3
"""Layout guards; inspect is NOT cryptographic signature verification."""
import sys,struct,hashlib
from pathlib import Path
BASE=0x08000000; BOOT=0x08020000; UPDATE=0x08060000; SWAP=0x080A0000
SLOT=0x40000; SECTOR=0x20000; HEADER=256; MAX_RAW=SLOT-SECTOR-HEADER
def raw(data):
 if not 8<=len(data)<=MAX_RAW:raise ValueError('Raw APP exceeds reserved-trailer layout')
 sp,pc=struct.unpack_from('<II',data)
 if not (0x20000000<sp<=0x20020000 and sp%8==0):raise ValueError('APP MSP outside expected normal SRAM')
 if not (pc&1 and BOOT+HEADER<=pc-1<BOOT+HEADER+len(data)):raise ValueError('APP reset vector not in BOOT payload')
 return sp,pc
def inspect(data):
 if len(data)<HEADER or data[:4]!=b'WOLF':raise ValueError('Missing wolfBoot WOLF header')
 size=struct.unpack_from('<I',data,4)[0]
 if size+HEADER!=len(data):raise ValueError('Header size does not match file length')
 sp,pc=raw(data[HEADER:]);tags={};pos=8
 while pos+4<=HEADER:
  if data[pos]==255:pos+=1;continue
  typ,n=struct.unpack_from('<HH',data,pos)
  if typ==0:break
  if pos+4+n>HEADER:raise ValueError('TLV exceeds header')
  if typ in tags:raise ValueError('Duplicate header tag')
  tags[typ]=data[pos+4:pos+4+n];pos+=4+n
 version=struct.unpack('<I',tags[1])[0]
 if len(tags.get(0x20,b''))!=64:raise ValueError('Expected Ed25519 signature TLV')
 print(f'version={version} payload={size} MSP=0x{sp:08x} reset=0x{pc:08x} sha256(file)={hashlib.sha256(data).hexdigest()}')
 print('Layout/TLV check only; target wolfBoot performs cryptographic verification.')
 return size
def run():
 cmd=sys.argv[1];data=Path(sys.argv[2]).read_bytes()
 if cmd=='raw':print(raw(data))
 elif cmd=='inspect':inspect(data)
 elif cmd=='factory':
  signed=Path(sys.argv[3]).read_bytes();inspect(signed)
  if not 8<=len(data)<=BOOT-BASE:raise ValueError('Bootloader overlaps BOOT slot')
  sp,pc=struct.unpack_from('<II',data)
  if not (0x20000000<sp<=0x20020000 and pc&1 and BASE<=pc-1<BASE+len(data)):
   raise ValueError('Bad bootloader vectors')
  # Whole BOOT/UPDATE/SWAP including trailers set to erased FF, bounded at C0000.
  image=bytearray(b'\xff'*(SWAP+SECTOR-BASE))
  image[:len(data)]=data;image[BOOT-BASE:BOOT-BASE+len(signed)]=signed
  Path(sys.argv[4]).write_bytes(image);print('Factory: 0x08000000..0x080BFFFF; erases sectors 0..9')
 else:raise ValueError('Unknown command')
if __name__=='__main__':run()
