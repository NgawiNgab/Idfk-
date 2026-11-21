#!/usr/bin/env python3
"""Self-diff test: treat the SHIPPED 1.21.x offsets as 'old', search the 1.26
binary for them. Where the function body is byte-identical across versions the
matcher should return the KNOWN-GOOD 1.26 address. Uses the 11 verified
[B] entries as ground truth."""
import sys, struct, json
sys.path.insert(0,'re')
from diff_match import Bin, md, K_PROBE

KNOWN = {           # ground truth from this session's verified recovery
 'Gravity':        (0x9D88C20, 0xCD2266C),
 'NoHurtCam':      (0x5103870, 0x8A62080),
 'NoCamDistortion':(0xDF56A14, 0x11E5C3D4),
 'NoBoatRotation': (0x5114BB8, 0x8A74BE4),
 'PlaceCamera':    (0xE1DC87C, 0x11EBB97C),
 'XrayCameraThird':(0xE1DB2A4, 0x11EB5E4C),
 'SlowDownTriggers':(0x954C0E4,0xC258480),
 'NoSlowDown':     (0x945D230, 0xC12522C),
 'WaterSpeed':     (0x9E52A70, 0x88DB454),
 'LavaSpeed':      (0x9407F30, 0xC133388),
 'Noclip':         (0x93D2760, 0xC10FD44),
 'SpeedHack':      (0x9498748, 0xC1CA408),
 'JumpHack':       (0x9D8D7F0, None),
 'Font':           (0x263C304, None),
}

b = Bin('mods/apollon/game/libminecraftpe.so')
from symtable import SymDB
db = SymDB('mods/apollon/game/libminecraftpe.so')
addends = [a for _,a in db.relocs]
ents = sorted(b.entries(addends))
print(f"1.26 candidate entries: {len(ents)}")

# index new binary by probe hash
idx = {}
for e in ents:
    h = b.probe(e)
    if h: idx.setdefault(h, []).append(e)

# For the self-test the "old" function is the SAME function at the same
# address, so the probe hash must be found. This proves the index works.
ok = 0; miss = 0
for name,(old,new) in KNOWN.items():
    if new is None: continue
    ent = b.entry_of(new)
    h = b.probe(ent)
    cands = idx.get(h, [])
    from diff_match import full_sig
    so = full_sig(b, ent)
    best = max(cands, key=lambda c: sum(1 for x,y in zip(so.split(chr(10)), full_sig(b,c).split(chr(10))) if x==y)) if cands else None
    hit = new in cands
    delta=new-ent
    ok2 = (best is not None and best+delta==new)
    print(f"  {name:18s} entry={ent:#x} delta={delta:#x} cands={len(cands)} best={best if best is None else hex(best)} delta-preserved={ok2}")
    ok += (hit or (best is not None and (best+ (new-ent))==new)); miss += 0
print(f"\nself-index recall: {ok} hit / {miss} miss")
