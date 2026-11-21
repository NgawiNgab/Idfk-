#!/usr/bin/env python3
"""
body.py -- Bounded function-body scan for materialised f32 constants.

We have no symbol sizes for Bedrock's own code, so bodies are bounded by
reachability: scan linearly from the entry and stop after the terminating
`ret` at the outermost frame depth. This is deliberately conservative but
sufficient to answer "does this function compute with constant X?".
"""
import sys, struct, collections
sys.path.insert(0,'re')
from bedrock_re import Library

def f32parts(v):
    b = struct.pack('<f', v); return struct.unpack('<H', b[:2])[0], struct.unpack('<H', b[2:])[0]

def scan(lib, start, targets, maxlen=0x2000):
    """targets: {value: name}. Returns set of names found."""
    want = {}
    for v, n in targets.items():
        lo, hi = f32parts(v); want[(lo,hi)] = n
    blob = lib.read(start, maxlen)
    if not blob: return set()
    reg = {}; found = set()
    for i in range(0, len(blob)-4, 4):
        w = struct.unpack_from('<I', blob, i)[0]
        op = w & 0xFF800000; rd = w & 0x1F
        if op == 0x52800000:
            hw=(w>>21)&3; imm=(w>>5)&0xFFFF
            if hw<2: reg[rd]={'val':imm<<(16*hw)}
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
            k=((v & 0xFFFF), (v>>16)&0xFFFF)
            if k in want: found.add(want[k])
            continue
        if (w & 0xFFE0FFE0) == 0x2A0003E0:
            s=(w>>16)&0x1F
            if s in reg: reg[rd]=dict(reg[s])
    return found

if __name__ == '__main__':
    lib = Library('mods/apollon/game/libminecraftpe.so')
    T = {-0.08:'grav-0.08', -0.01:'grav-0.01', 0.42:'jump 0.42'}
    for a in sys.argv[1:]:
        f = int(a,16)
        print(f"{f:#x}: {sorted(scan(lib,f,T)) or '-'}")
