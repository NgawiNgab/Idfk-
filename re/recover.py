#!/usr/bin/env python3
"""
recover.py -- One-shot re-derivation of Apollon's offsets for any
libminecraftpe.so that still carries entt system names in .data.rel.ro.

  usage:  recover.py <lib.so> [--json out.json] [--header out.h]

Pipeline
  1. index .rodata NUL-terminated strings
  2. read R_AARCH64_RELATIVE relocations
  3. name string --(reloc addend)--> pointer slot
  4. sibling reloc slots in the same record -> function pointers
  5. normalise each interior pointer to a verified entry point
  6. confirm constant-bearing hooks by body scan for movz/movk+fmov

This is deliberately dependency-light: pyelftools + capstone only.
"""
import sys, json, struct, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from symtable import SymDB
from nameres import Resolver, method_pairs
from body import scan
from entry import entry_of

SPEC = [
    # (Apollon name, name-substring to match, constant to confirm or None)
    ('Gravity',          'MobMovementGravity::tickPlayerWaterGravity', -0.08),
    ('WaterDrown',       'MobMovementGravity::tickPlayerWaterGravity', -0.01),
    ('LavaDrown',        'MobMovementGravity::tickLavaGravity', -0.01),
    ('NoHurtCam',        'CameraBobSystem::tick', None),
    ('NoCamDistortion',  'CameraPortalDistortionSystem::_tick', None),
    ('NoBoatRotation',   'CameraVehicleRotationLookSystem::tick', None),
    ('PlaceCamera',      'CameraBlendSystem::_tick', None),
    ('XrayCameraThird',  'CameraAvoidanceSystem::tick', None),
    ('SlowDownTriggers', 'BlockMovementSlowdownMultiplierSystemImpl::applySlowdownOnMove', None),
    ('NoSlowDown',       'ItemUseSlowdownSystemImpl::doItemUseSlowdownSystem', None),
    ('WaterSpeed',       'doWaterMoveSystem', None),
    ('LavaSpeed',        'doLavaMoveSystem', None),
    ('Noclip',           'MoveCollisionSystem', None),
    ('SpeedHack',        'DefaultMoveSystems::doDefaultMoveSystems', None),
]

def imp_targets(lib, f, n=0xC0):
    """immediate adr / adrp+add targets in the first n bytes of f."""
    from capstone import Cs, CS_ARCH_ARM64, CS_MODE_ARM
    md = Cs(CS_ARCH_ARM64, CS_MODE_ARM); pg = {}; out = []
    blob = lib.read(f, n)
    if not blob: return out
    for ins in md.disasm(blob, f):
        if ins.mnemonic == 'adr':
            try: out.append(int(ins.op_str.split(', ')[1].lstrip('#'), 16))
            except Exception: pass
        elif ins.mnemonic == 'adrp':
            try: pg[ins.op_str.split(', ')[0]] = int(ins.op_str.split(', ')[1].lstrip('#'), 16)
            except Exception: pass
        elif ins.mnemonic == 'add' and '#' in ins.op_str:
            p = [x.strip() for x in ins.op_str.split(',')]
            if p[1] in pg:
                try: out.append(pg[p[1]] + int(p[2].lstrip('#'), 16))
                except Exception: pass
    return out

def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'mods/apollon/game/libminecraftpe.so'
    db = SymDB(path); R = Resolver(db); lib = db.lib
    t0, t1, _ = lib.text_range()
    allm = R.all_methods()
    rows = []
    print(f"{'hook':18} {'addr':>11} {'src':12} method")
    print('-' * 92)
    for name, sub, const in SPEC:
        rec = dict(hook=name, addr=None, src='MISSING', method=None)
        hits = [m for m in allm if sub in m]
        if not hits:
            rows.append(rec); print(f"{name:18} {'--':>11} MISSING"); continue
        rec['method'] = hits[0]
        infos = R.funcs_for(hits[0])
        fns = [a for d, w, a in infos[0]['fns']] if infos else []
        T = {const: 'c'} if const is not None else {}
        best = None; src = 'impl-slot'
        for a in fns:
            if T and scan(lib, a, T):
                best, src = a, 'direct'; break
        if best is None and const is not None:
            for a in fns:
                for t in imp_targets(lib, a):
                    if t0 <= t < t1 and scan(lib, t, T):
                        best, src = t, 'lambda'; break
                if best: break
        if best is None:
            for a in fns:
                if a not in (0x8a7ac94, 0x1346b978):
                    best, src = a, 'impl-slot'; break
        if best:
            e = entry_of(lib, best)
            if e: best, src = e, src + '+entry'
        rec['addr'] = best; rec['src'] = src
        rows.append(rec)
        print(f"{name:18} {best:#11x} {src:12} {hits[0]}")
    if '--json' in sys.argv:
        out = sys.argv[sys.argv.index('--json') + 1]
        json.dump(rows, open(out, 'w'), indent=1)
        print(f"\nwrote {out}")

if __name__ == '__main__':
    main()
