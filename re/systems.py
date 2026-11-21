#!/usr/bin/env python3
"""
systems.py -- Parse Bedrock's entt system metadata records and resolve each
named system to its candidate implementation functions.

Record layout (0x80 stride, verified):
  +0x00  const char*  filter/type-list name
  +0x18  const char*  adapter name (embeds the mangled lambda <method>)
  +0x38 .. +0x70  seven function pointers (system vtable bodies)

The adapter string names the underlying method; the seven pointers are the
thunks/virtuals for that system. Disassembly of the non-trivial ones reveals
the real maths.
"""
import sys, re, struct, collections
sys.path.insert(0,'re')
from symtable import SymDB
from capstone import Cs, CS_ARCH_ARM64, CS_MODE_ARM

REC = 0x80
SLOTS = range(0x38, 0x78, 8)

def build(db):
    rel = db.relocs
    slotmap = dict(rel)              # where -> addend
    recs = []
    # a record starts where slot +0x00 holds a rodata ptr AND +0x38 holds a text ptr
    for where, addend in rel:
        if not (db.ro_start <= addend < db.ro_end): continue
        need = [where + o for o in (0x18,) + tuple(SLOTS)]
        if not all(n in slotmap for n in need): continue
        if not (db.ro_start <= slotmap[where+0x18] < db.ro_end): continue
        if not all(db.t_start <= slotmap[n] < db.t_end for n in need if n != where+0x18):
            continue
        fname = db.lib.read(addend, 300).split(b'\0')[0].decode('latin1','replace')
        aname = db.lib.read(slotmap[where+0x18], 400).split(b'\0')[0].decode('latin1','replace')
        if 'StrictTickingSystem' not in fname and 'System' not in fname: continue
        fns = [slotmap[where+o] for o in SLOTS]
        recs.append(dict(va=where, filter=fname, adapter=aname, fns=fns))
    return recs

def method_of(adapter):
    """Pull '<Class>::<method>' out of a mangled adapter name."""
    m = re.search(r'_ZN(?:\d+)?(\w*?)\.?(\d+)([A-Za-z_]\w*)', adapter)
    # general: find ...\d+Class\d+Method
    hits = re.findall(r'L_ZN(\d+)([A-Za-z_]\w*?)(\d+)([A-Za-z_]\w{2,40}?)E', adapter)
    if hits:
        return [f"{c}::{mt}" for _,c,_,mt in hits]
    hits2 = re.findall(r'(\d+)([A-Z][A-Za-z0-9_]{3,40})(\d+)([a-z]\w{2,40})', adapter)
    return [f"{c}::{mt}" for _,c,_,mt in hits2]

if __name__ == '__main__':
    db = SymDB()
    recs = build(db)
    print(f"[sys] {len(recs)} system records")
    # index by method text
    idx = collections.defaultdict(list)
    for r in recs:
        for m in method_of(r['adapter']):
            idx[m].append(r)
    queries = sys.argv[1:]
    if not queries:
        queries = sorted(k for k in idx if any(c in k for c in
            ('MobMovementGravity','CameraBobSystem','CameraPortal','CameraVehicle',
             'CameraBlend','CameraAvoidance','MoveCollision')))
    for q in queries:
        print(f"\n===== {q}  ({len(idx.get(q,[]))} recs)")
        for r in idx.get(q, [])[:3]:
            print(f"  rec {r['va']:#x}")
            print(f"    filter: {r['filter'][:110]}")
            print(f"    fns: {[hex(f) for f in r['fns']]}")
