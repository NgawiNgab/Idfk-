#!/usr/bin/env python3
"""
confirms.py -- Cross-check named ECS systems against known float constants.
For each system record, scan the bodies of its function slots for a target
constant materialised via movz/movk+fmov, and report which slot has it.
"""
import sys, re, struct, collections
sys.path.insert(0,'re')
from symtable import SymDB
from systems import build, method_of
from capstone import Cs, CS_ARCH_ARM64, CS_MODE_ARM

def f32parts(v):
    b = struct.pack('<f', v)
    return struct.unpack('<H', b[:2])[0], struct.unpack('<H', b[2:])[0]

def body_has(lib, start, target, maxlen=0x1800):
    """Linear scan a function body for materialised f32 constant."""
    lo_t, hi_t, _ = lib.text_range()
    lo, hi = f32parts(target)
    blob = lib.read(start, maxlen)
    if not blob: return False
    reg = {}
    for i in range(0, len(blob) - 4, 4):
        w = struct.unpack_from('<I', blob, i)[0]
        op = w & 0xFF800000; rd = w & 0x1F
        if op == 0x52800000:
            hw=(w>>21)&3; imm=(w>>5)&0xFFFF
            if hw<2: reg[rd]={'lo':imm if hw==0 else None,'hi':imm if hw==1 else None,
                              'val':imm<<(16*hw)}
            continue
        if op == 0x72800000:
            hw=(w>>21)&3; imm=(w>>5)&0xFFFF
            if hw<2 and rd in reg:
                r=reg[rd]
                if hw==0: r['val']=(r['val']&0xFFFF0000)|imm
                else: r['val']=(r['val']&0xFFFF)|(imm<<16)
            continue
        if (w & 0xFFE0FC00) == 0x1E200000 and ((w>>5)&0x1F) in reg:
            v=reg[(w>>5)&0x1F]['val']
            if (v&0xFFFF)==lo and (v>>16)==hi: return True
            continue
        if (w & 0xFFE0FFE0) == 0x2A0003E0:
            s=(w>>16)&0x1F
            if s in reg: reg[rd]=dict(reg[s])
    return False

if __name__ == '__main__':
    db = SymDB(); recs = build(db)
    targets = {float(a): n for a,n in zip(sys.argv[1::2], sys.argv[2::2])}
    if not targets: targets = {-0.08:'gravity', 0.42:'jump', 0.91:'drag', 12.0:'reach'}
    for val, name in targets.items():
        print(f"\n##### {name} = {val}")
        seen=set()
        for r in recs:
            ms = method_of(r['adapter'])
            for f in r['fns']:
                if f in seen: continue
                if body_has(db.lib, f, val):
                    seen.add(f)
                    print(f"  {f:#x}  {r['va']:#x}  {ms[:2]}")
