#!/usr/bin/env python3
"""callers.py -- find `bl` call sites and adrp+add pointers to given targets."""
import sys, struct
sys.path.insert(0,'re')
from bedrock_re import Library

def bl_callers(lib, targets):
    t0,t1,toff = lib.text_range()
    lib.f.seek(toff); blob = lib.f.read(t1-t0)
    n = len(blob)//4
    w = struct.unpack_from('<%dI'%n, blob, 0)
    want = set(targets)
    out = {t: [] for t in targets}
    for i in range(n):
        x = w[i]
        if (x & 0xFC000000) != 0x94000000:      # BL
            continue
        imm = x & 0x3FFFFFF
        if imm & (1<<25): imm -= (1<<26)
        tgt = t0 + i*4 + (imm<<2)
        if tgt in want:
            out[tgt].append(t0 + i*4)
    return out

if __name__ == '__main__':
    lib = Library(sys.argv[1])
    targets = [int(a,0) for a in sys.argv[2:]]
    res = bl_callers(lib, targets)
    for t in targets:
        print(f"\nbl callers of {t:#x}: {len(res[t])}")
        for c in res[t][:20]:
            print(f"   {c:#x}")
