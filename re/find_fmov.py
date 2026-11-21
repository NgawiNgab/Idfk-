#!/usr/bin/env python3
"""
find_fmov.py -- Find float constants materialised via FMOV (immediate) in
AArch64 code, and walk back to the enclosing function.

Why this is needed: clang on AArch64 does not embed most float constants as
literal pools in .text. It emits
    fmov s0, #<8-bit immediate>
The 8-bit encoding (VFPExpandImm) covers a limited set: values of the form
    +/- (16+f)/16 * 2^(e-3)  for e in 0..15
so e.g. -0.08 is representable, but 0.42 is not (0.42 needs a literal pool,
which lives in .rodata, hence the 3 hits we saw there).
"""
import sys, struct, collections
sys.path.insert(0, 're')
from bedrock_re import Library

def decode_fmov8(imm8):
    """AArch64 VFPExpandImm for 32-bit FMOV."""
    sign = (imm8 >> 7) & 1
    exp  = (imm8 >> 4) & 0x7
    frac = imm8 & 0xF
    if exp == 0 and frac == 0:
        return -0.0 if sign else 0.0
    if exp == 0:                      # subnormal-ish encoding
        val = (frac / 16.0) * (2.0 ** -2)
    else:
        val = (1.0 + frac / 16.0) * (2.0 ** (exp - 3))
    return -val if sign else val

def main():
    lib = Library(sys.argv[1] if len(sys.argv) > 1
                  else 'mods/apollon/game/libminecraftpe.so')
    tstart, tend, toff = lib.text_range()
    lib.f.seek(toff)
    blob = lib.f.read(tend - tstart)
    print(f".text {hex(tstart)}..{hex(tend)}  ({len(blob):,} bytes)")

    # collect fmov-immediate occurrences
    hits = collections.defaultdict(list)
    for i in range(0, len(blob) - 4, 4):
        w = struct.unpack_from('<I', blob, i)[0]
        if (w & 0xFFE01FE0) != 0x1E201000:      # FMOV Sd,#imm8
            continue
        imm8 = (w >> 13) & 0xFF
        val = decode_fmov8(imm8)
        hits[round(val, 8)].append(tstart + i)

    print(f"\ndistinct FMOV immediates: {len(hits)}")
    print(f"{'value':>14}  {'count':>6}")
    for v in sorted(hits, key=lambda x: -len(hits[x]))[:25]:
        print(f"{v:>14.6g}  {len(hits[v]):>6}")

    # look for specific physics constants
    print("\n=== target constants ===")
    for target in (-0.08, -0.0784, -0.0784000015258789, 0.42, 0.5, -0.9800000,
                   -0.98, 0.08, 3.92, 0.6000000238418579):
        found = [v for v in hits if abs(v - target) < 1e-7]
        n = sum(len(hits[v]) for v in found)
        print(f"  {target:>10} -> {n} site(s)" +
              (f"  as {found}" if found else ""))

if __name__ == '__main__':
    main()
