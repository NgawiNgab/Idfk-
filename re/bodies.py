#!/usr/bin/env python3
"""bodies.py -- resolve hook names to REAL method bodies via the entt registry.

The registry record's +0x38..+0x70 pointer slots hold, in order, the adapter
thunk and the actual system method body. We keep only slots that start a
plausible function body (sub/stp/str/movi), skipping `b <stub>` veneers and
canary-check wrappers.
"""
import sys, json, collections
sys.path.insert(0,'re')
from symtable import SymDB
from systems import build, method_of
from diff_match import Bin, md

def is_body(b, addr):
    if not (b.t0 <= addr < b.t1): return False
    w = b.word(addr)
    if (w & 0xFC000000) == 0x14000000: return False   # b <veneer>
    ms = [i.mnemonic for i in md.disasm(b.insn_bytes(addr,1), addr)]
    return bool(ms) and ms[0] in ('sub','stp','str','sturb','movi','mov','ldr','add')

def build_bodies(path):
    db = SymDB(path); recs = build(db); b = Bin(path)
    name2bodies = collections.defaultdict(list)
    for r in recs:
        for m in method_of(r['adapter']):
            for f in r['fns']:
                if f and is_body(b, f):
                    name2bodies[m].append((f, r['va']))
    return b, name2bodies
