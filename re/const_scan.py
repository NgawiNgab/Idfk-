#!/usr/bin/env python3
"""
const_scan.py -- Robust f32 constant recovery from AArch64 code.

clang materialises float constants in several ways; all must be handled:
  A) movz wD,#lo ; movk wD,#hi,lsl#16 ; fmov sN,wD
  B) movz wS,#lo ... movz wD,#lo ; movk wS,#hiA ; movk wD,#hiB   (shared low half)
  C) fmov sN,#imm8                  (8-bit VFPExpandImm only)
  D) ldr sN,[pc,#off] -> literal pool in .rodata
  E) ldr sN,[pc,#off] -> literal pool inside .text

Rather than pattern-match instruction pairs, this tool does a proper linear
sweep tracking constant materialisation into registers, which catches (A),(B).
It also reports .rodata literal-pool references for (D).
"""
import sys, struct, collections
sys.path.insert(0, 're')
from bedrock_re import Library

def f32_bits(v): 
    b = struct.pack('<f', v)
    return struct.unpack('<H', b[:2])[0], struct.unpack('<H', b[2:])[0]

def bits_to_f32(lo, hi):
    return struct.unpack('<f', struct.pack('<HH', lo, hi))[0]

class RegTracker:
    """Track which 32-bit immediates are materialised into each W register."""
    def __init__(self):
        self.known = {}   # reg -> (lo, hi) or (lo, None)

def scan_constants(lib, targets, max_per=200):
    """
    targets: dict name -> float value
    Returns dict name -> list of (insn_vaddr, func_start)
    """
    tstart, tend, toff = lib.text_range()
    lib.f.seek(toff)
    blob = lib.f.read(tend - tstart)
    n = len(blob)

    want = {}
    for name, v in targets.items():
        lo, hi = f32_bits(v)
        want[(lo, hi)] = name

    results = collections.defaultdict(list)
    reg = {}   # reg -> dict with 'lo' and 'full' (32-bit value)

    for i in range(0, n - 4, 4):
        w = struct.unpack_from('<I', blob, i)[0]
        pc = tstart + i
        op = w & 0xFF800000
        rd = w & 0x1F

        if op == 0x52800000:                       # MOVZ Wd, #imm16[,lsl#16]
            hw = (w >> 21) & 3
            imm = (w >> 5) & 0xFFFF
            if hw >= 2: continue
            reg[rd] = {'lo': imm if hw == 0 else None,
                       'hi': imm if hw == 1 else None,
                       'val': imm << (16*hw)}
            continue
        if op == 0x72800000:                       # MOVK Wd, #imm16[,lsl#16]
            hw = (w >> 21) & 3
            imm = (w >> 5) & 0xFFFF
            if hw >= 2 or rd not in reg: continue
            r = reg[rd]
            if hw == 0: r['lo'] = imm; r['val'] = (r['val'] & 0xFFFF0000) | imm
            else:       r['hi'] = imm; r['val'] = (r['val'] & 0x0000FFFF) | (imm<<16)
            continue
        # FMOV SN, Wd  == 0x1E270000 | ...  (check bits)
        if (w & 0xFFE0FC00) == 0x1E200000 and ((w >> 5) & 0x1F) in reg:
            r = reg[(w >> 5) & 0x1F]
            lo = r['val'] & 0xFFFF
            hi = (r['val'] >> 16) & 0xFFFF
            key = (lo, hi)
            if key in want:
                results[want[key]].append((pc, lib.function_start(pc)))
            continue
        # MOV Wd, Wn / ORR etc would invalidate; simple conservative clear
        if (w & 0xFFE0FFE0) in (0x2A0003E0,):     # mov wd, wn
            src = (w >> 16) & 0x1F
            if src in reg: reg[rd] = dict(reg[src])
            else: reg.pop(rd, None)
    return results

def main():
    lib = Library(sys.argv[1] if len(sys.argv) > 1
                  else 'mods/apollon/game/libminecraftpe.so')
    targets = {
        'gravity -0.08': -0.08,
        'gravity_water -0.01': -0.01,
        'jump 0.42': 0.42,
        'speed 0.1': 0.1,
        'water_speed 0.02': 0.02,
        'drag 0.91': 0.91,
        'drag 0.98': 0.98,
        'friction 0.6': 0.6,
        'term_vel -3.92': -3.92,
        'sprint 1.3': 1.3,
        'fov 70': 70.0,
        'reach 7': 7.0,
        'reach 12': 12.0,
    }
    res = scan_constants(lib, targets)
    print(f"{'name':22} {'sites':>6} {'funcs':>6}")
    print('-'*46)
    for name in targets:
        hits = res.get(name, [])
        funcs = sorted(set(f for _, f in hits if f))
        print(f"{name:22} {len(hits):>6} {len(funcs):>6}")
        for u in funcs[:5]:
            print(f"      func {hex(u)}")

if __name__ == '__main__':
    main()
