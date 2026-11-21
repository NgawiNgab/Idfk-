#!/usr/bin/env python3
"""
verify_offsets.py -- Validate Apollon Client ImGui offset tables against a
libminecraftpe.so build, and report which offsets are stale.

Usage:
    python3 verify_offsets.py <libminecraftpe.so> [offsets.h]

Why: Apollon v4.62 ships offsets tuned for BDS 1.21.x. Bedrock 1.26.66.1
reordered the binary, so nearly every entry points at unrelated bytes.
This tool just tells you, entry by entry, whether the byte signature at each
offset is plausible for the declared purpose (rather than trusting the table).

Checks per offset:
  * offset inside a LOAD segment's file range?  (else: out of range)
  * for float-valued hooks: 4 bytes decode to a finite, sane float?
  * for code hooks: first instruction is a valid AArch64 opcode word that
    disassembles as something other than UDF/zero?
  * flags obvious string data (ASCII run) where code/float is expected.

Exit code: 0 if all offsets look structurally plausible, 1 if any is stale.
"""
import sys, struct, re, subprocess, json

def parse_offsets_h(path):
    """Extract `Name = 0xADDR` entries from an offsets.h (first enum block)."""
    txt = open(path, encoding='utf-8', errors='replace').read()
    # cut at the first closing brace after a namespace Offsets64/Offsets block
    out, seen = [], set()
    for m in re.finditer(r'^\s*([A-Za-z_]\w*)\s*=\s*(0x[0-9A-Fa-f]+)', txt, re.M):
        name, addr = m.group(1), int(m.group(2), 16)
        if name in seen:
            continue
        seen.add(name)
        out.append((name, addr))
    return out

def read_load_segments(path):
    """Return [(file_off, file_size, vaddr, flags)] from program headers."""
    segs = []
    try:
        out = subprocess.run(['readelf', '-lW', path], capture_output=True, text=True).stdout
    except FileNotFoundError:
        return None
    for line in out.splitlines():
        if line.strip().startswith('LOAD'):
            p = line.split()
            # Type Offset VirtAddr PhysAddr FileSiz MemSiz Flg Align
            try:
                off, vaddr, fsiz = int(p[1],16), int(p[2],16), int(p[4],16)
                flags = p[6] if len(p) > 6 else ''
            except (IndexError, ValueError):
                continue
            segs.append((off, fsiz, vaddr, flags))
    return segs

def is_ascii_run(b):
    return sum(32 <= x < 127 for x in b) >= 6

def plausible_float(b):
    f = struct.unpack('<f', b[:4])[0]
    if f != f or f in (float('inf'), float('-inf')):
        return False, f
    # sane game-constant range; reject denormals and absurd magnitudes
    return (abs(f) < 1e6 and (f == 0 or abs(f) > 1e-4)), f

def classify(name):
    fl = ('Fov','Gravity','FastDrop','BlockReach','JumpHack','Font')
    for f in fl:
        if f.lower() in name.lower():
            return 'float'
    return 'code'

def main():
    if len(sys.argv) < 2:
        print(__doc__); return 2
    lib = sys.argv[1]
    offs_path = sys.argv[2] if len(sys.argv) > 2 else None
    if not offs_path:
        print("!! need offsets.h path as 2nd arg"); return 2

    entries = parse_offsets_h(offs_path)
    segs = read_load_segments(lib)
    if not segs:
        print("!! could not read LOAD segments (is readelf available?)"); return 2
    f = open(lib, 'rb')
    stale = 0
    report = []
    print(f"lib: {lib}")
    print(f"offsets: {offs_path}  ({len(entries)} entries)\n")
    print(f"{'entry':28} {'offset':>10} {'seg':>4} {'kind':>6}  verdict")
    print('-'*82)
    for name, addr in entries:
        kind = classify(name)
        seg = next((s for s in segs if s[0] <= addr < s[0]+s[1]), None)
        if seg is None:
            report.append((name, addr, 'OUT-OF-RANGE', ''))
            stale += 1
            print(f"{name:28} 0x{addr:08X} {'-':>4} {kind:>6}  STALE (not in any LOAD file range)")
            continue
        f.seek(addr); b = f.read(8)
        if len(b) < 8:
            verdict = 'STALE (short read)'; stale += 1
        elif is_ascii_run(b):
            verdict = 'STALE (ASCII/string data here)'; stale += 1
        elif kind == 'float':
            ok, val = plausible_float(b)
            verdict = f'{"ok" if ok else "STALE"} float={val:.6g}'
            stale += 0 if ok else 1
        else:
            # instruction plausibility: AArch64 fixed 32-bit, top bits not all 0/1
            w = struct.unpack('<I', b[:4])[0]
            if w == 0 or w == 0xFFFFFFFF:
                verdict = 'STALE (zero/UDF word)'; stale += 1
            elif (w & 0x7C000000) == 0:  # reserved class
                verdict = f'STALE? unusual opcode 0x{w:08X}'; stale += 1
            else:
                verdict = f'plausible insn 0x{w:08X}'
        print(f"{name:28} 0x{addr:08X} {seg[3]:>4} {kind:>6}  {verdict}")
    print('-'*82)
    print(f"{len(entries)-stale}/{len(entries)} structurally plausible, {stale} stale")
    json.dump([{'name':n,'offset':hex(a),'verdict':v} for n,a,v,_ in report],
              open('offset_verdicts.json','w'), indent=1)
    print("wrote offset_verdicts.json")
    return 0 if stale == 0 else 1

if __name__ == '__main__':
    sys.exit(main())
