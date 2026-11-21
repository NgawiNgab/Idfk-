#!/usr/bin/env python3
"""getter_scan.py -- find float-constant GETTERS (`return <const>`).

Covers all three ways clang emits `return C;` for a float on AArch64:
  A) fmov s0, #imm8 ; ret                    (0x1E201000|imm8<<13)
  B) ldr  s0, [pc, #off] ; ret               (0x1C000000|imm19<<5|0)  literal in .text/.rodata
  C) movz/movk wN,#imm ; fmov s0,wN ; ret
Also optionally requires the *preceding* instruction to be a ret, i.e. the
candidate starts a function and is a pure 1-2 instruction getter.
"""
import sys, struct
sys.path.insert(0, 're')
from bedrock_re import Library

def f32_imm8_encodings():
    """all 256 imm8 -> float"""
    out = {}
    for imm8 in range(256):
        sign = (imm8 >> 7) & 1
        exp  = (imm8 >> 4) & 7
        frac = imm8 & 0xF
        if exp == 0:
            val = (frac/16.0) * (2.0**-2)
        else:
            val = (1.0 + frac/16.0) * (2.0**(exp-3))
        out[imm8] = -val if sign else val
    return out

def bits(v):
    return struct.unpack('<I', struct.pack('<f', v))[0]

def scan(lib, value):
    target = value
    t0, t1, toff = lib.text_range()
    lib.f.seek(toff); blob = lib.f.read(t1 - t0)
    n = len(blob)//4
    w = struct.unpack_from('<%dI'%n, blob, 0)

    def prev_is_ret(i):
        return i > 0 and w[i-1] == 0xD65F03C0

    def read_u32(va):
        if t0 <= va < t1:
            return struct.unpack_from('<I', blob, (va-t0))[0]
        try:
            lib.f.seek(va - 0)  # not used
        except Exception:
            return None
        return None

    hits = []
    # A) fmov s0,#imm8 ; ret
    for imm8, fv in f32_imm8_encodings().items():
        if abs(fv - value) < 1e-9:
            enc = 0x1E201000 | (imm8 << 13)
            for i in range(n-1):
                if w[i] == enc and w[i+1] == 0xD65F03C0:
                    va = t0 + i*4
                    hits.append(('A', va, lib.function_start(va), prev_is_ret(i)))
    # B) ldr s0,[pc,#off] ; ret  where literal == value
    tb = bits(value)
    for i in range(n-1):
        x = w[i]
        if (x & 0xFF000000) != 0x1C000000:   # LDR (literal) SIMD&FP 32-bit
            continue
        if (x & 0x1F) != 0:                  # Rt must be s0
            continue
        if w[i+1] != 0xD65F03C0: continue
        imm19 = (x >> 5) & 0x7FFFF
        if imm19 & (1<<18): imm19 -= (1<<19)
        va = t0 + i*4
        lit_va = va + (imm19 << 2)
        if t0 <= lit_va < t1:
            lw = struct.unpack_from('<I', blob, lit_va - t0)[0]
        else:
            continue
        if lw == tb:
            hits.append(('B', va, lib.function_start(va), prev_is_ret(i)))
    # C) movz/movk ; fmov s0,wN ; ret
    lo, hi = tb & 0xFFFF, (tb >> 16) & 0xFFFF
    for i in range(n-3):
        if (w[i] & 0xFF800000) != 0x52800000: continue
        if (w[i] >> 21) & 3: continue
        if ((w[i] >> 5) & 0xFFFF) != lo: continue
        d = w[i] & 0x1F
        if (w[i+1] & 0xFF800000) != 0x72800000: continue
        if (w[i+1] >> 21) & 3 != 2: continue
        if ((w[i+1] >> 5) & 0xFFFF) != hi: continue
        if (w[i+1] & 0x1F) != d: continue
        if (w[i+2] & 0xFFFFFC00) != 0x1E270000: continue
        if ((w[i+2] >> 5) & 0x1F) != d or (w[i+2] & 0x1F) != 0: continue
        if w[i+3] != 0xD65F03C0: continue
        va = t0 + i*4
        hits.append(('C', va, lib.function_start(va), prev_is_ret(i)))
    return hits

if __name__ == '__main__':
    lib = Library(sys.argv[1] if len(sys.argv) > 1 else 'mods/apollon/game/libminecraftpe.so')
    vals = [('jump 0.42',0.42), ('reach 7.0',7.0), ('reach 12.0',12.0), ('fov 70',70.0)]
    if len(sys.argv) > 2: vals = [('query', float(sys.argv[2]))]
    for name, v in vals:
        h = scan(lib, v)
        print(f"\n{name}: {len(h)} getter sites")
        for kind, site, fn, clean in h:
            tag = 'clean-entry' if clean else ''
            print(f"   [{kind}] site {site:#x}  func {fn:#x}  {tag}")
