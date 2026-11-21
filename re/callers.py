#!/usr/bin/env python3
"""callers.py -- find all `bl target` sites, plus the containing function."""
import sys, struct
sys.path.insert(0,'re')
from bedrock_re import Library
from capstone import Cs, CS_ARCH_ARM64, CS_MODE_ARM

def find_callers(lib, target, max_back=0x2000):
    t_start, t_end, t_off = lib.text_range()
    lib.f.seek(t_off); blob = lib.f.read(t_end - t_start)
    out = []
    n = len(blob)
    for i in range(0, n - 4, 4):
        w = struct.unpack_from('<I', blob, i)[0]
        if (w & 0xFC000000) != 0x94000000: continue
        imm = w & 0x03FFFFFF
        if imm & 0x02000000: imm -= 0x04000000
        if (t_start + i) + imm*4 == target:
            out.append(t_start + i)
    return out

if __name__ == '__main__':
    lib = Library('mods/apollon/game/libminecraftpe.so')
    for a in sys.argv[2:]:
        t = int(a, 16)
        cs = find_callers(lib, t)
        print(f"\n### callers of {t:#x}: {len(cs)}")
        fns = []
        for c in cs[:40]:
            f = lib.function_start(c)
            fns.append(f)
            print(f"   site {c:#x}   func {f:#x}   off +{c-f:#x}")
        print(f"  distinct funcs: {[hex(f) for f in sorted(set(fns))]}")
