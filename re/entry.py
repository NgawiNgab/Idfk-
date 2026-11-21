#!/usr/bin/env python3
"""
entry.py -- Given an address INSIDE a function, recover the true entry point and
prove it by forward-decoding from the candidate entry to the interior address.

Bedrock's .text is stripped and functions are tightly packed, so `walk back to
the nearest stp x29,x30` is unreliable on its own. We accept a candidate entry
only if a linear disassembly from it reaches the interior address exactly.
"""
import sys, struct
sys.path.insert(0,'re')
from bedrock_re import Library
from capstone import Cs, CS_ARCH_ARM64, CS_MODE_ARM

MD = Cs(CS_ARCH_ARM64, CS_MODE_ARM)

def looks_like_entry(lib, a):
    w = lib.read(a, 4)
    if not w: return False
    w = struct.unpack('<I', w)[0]
    # stp x29,x30,[sp,#-N]!  |  sub sp,sp,#N  |  str xN,[sp,#-N]!  |  bti/ paciasp
    if (w & 0xFFC07FFF) == 0xA9807BFD: return True      # stp x29,x30,[sp,#-imm]!
    if (w & 0xFFC00000) == 0xD1000000 and ((w>>5)&0x1F) == 31: return True   # sub sp,sp,#
    if (w & 0xFFE00000) == 0xD1000000: return False
    if w == 0xD503233F: return True                      # paciasp
    if w == 0xD503245F: return True                      # bti c
    return False

def forward_reaches(lib, entry, target, limit=0x1200):
    blob = lib.read(entry, min(limit, target - entry + 0x40))
    if not blob: return False
    for ins in MD.disasm(blob, entry):
        if ins.address == target: return True
        if ins.address > target: return False
    return False

def entry_of(lib, inner, span=0x1800):
    """Find a verified function entry at or before `inner`."""
    t0 = lib.text_range()[0]
    a = inner
    while a > max(t0, inner - span):
        a -= 4
        if looks_like_entry(lib, a):
            if forward_reaches(lib, a, inner):
                return a
    return None

if __name__ == '__main__':
    lib = Library('mods/apollon/game/libminecraftpe.so')
    for s in sys.argv[1:]:
        v = int(s, 16)
        e = entry_of(lib, v)
        print(f"{v:#x} -> entry {e if e is None else hex(e)}")
