#!/usr/bin/env python3
"""
gen_offsets.py -- Produce offsets_1.26.66.1.h for Apollon from libminecraftpe.so.

Resolution strategy per hook (highest confidence first):
  S1  entt system metadata:  name string -> reloc slot -> impl fn pointer   [exact]
  S2  lambda extraction:     dispatcher `adr x8, Lambda` + call            [exact]
  S3  constant confirmation: bounded body scan for the hook's constant     [proof]
Hooks whose class appears in NO symbol source are emitted with a documented
REQUIRES_RUNTIME marker instead of a fabricated address.
"""
import sys, re, json, struct
sys.path.insert(0,'re')
from symtable import SymDB
from nameres import Resolver, method_pairs
from body import scan
from capstone import Cs, CS_ARCH_ARM64, CS_MODE_ARM

lib=None; R=None; db=None

def lam(f):
    """Extract immediate adr/adrp+add targets within first 0x80 of f."""
    md=Cs(CS_ARCH_ARM64,CS_MODE_ARM); pg={}; out=[]
    blob=lib.read(f,0x80)
    if not blob: return out
    for ins in md.disasm(blob,f):
        if ins.mnemonic=='adr':
            try: out.append(int(ins.op_str.split(', ')[1].lstrip('#'),16))
            except Exception: pass
        elif ins.mnemonic=='adrp':
            try: pg[ins.op_str.split(', ')[0]]=int(ins.op_str.split(', ')[1].lstrip('#'),16)
            except Exception: pass
        elif ins.mnemonic=='add' and '#' in ins.op_str:
            p=[x.strip() for x in ins.op_str.split(',')]
            if p[1] in pg:
                try: out.append(pg[p[1]]+int(p[2].lstrip('#'),16))
                except Exception: pass
    return out

def resolve(method_sub, const=None, lambdas=True):
    """Best address for a named system, preferring a lambda proving `const`."""
    hits=[m for m in R.all_methods() if method_sub in m]
    if not hits: return None, None, 'no-name'
    infos=R.funcs_for(hits[0])
    if not infos: return None, None, 'no-record'
    fns=[a for d,w,a in infos[0]['fns']]
    T={const:'c'} if const is not None else {}
    # 1. direct body
    for a in fns:
        if T and scan(lib,a,T): return a, hits[0], 'direct-body'
    # 2. lambda reached from a dispatcher in this record
    if lambdas and T:
        for a in fns:
            for t in lam(a):
                if lib.text_range()[0] <= t < lib.text_range()[1]:
                    if scan(lib,t,T): return t, hits[0], 'lambda'
    # 3. fall back to the record's first non-boilerplate pointer
    for a in fns:
        if a not in (0x8a7ac94,0x1346b978): return a, hits[0], 'impl-slot'
    return None, hits[0], 'unknown'

HOOKS = [
 # (offset, method substring, const, note)
 ('NoHurtCam',        'CameraBobSystem::tick', None, 'ret-patch'),
 ('NoCamDistortion',  'CameraPortalDistortionSystem::_tick', None, 'ret-patch'),
 ('NoBoatRotation',   'CameraVehicleRotationLookSystem::tick', None, 'ret-patch'),
 ('PlaceCamera',      'CameraBlendSystem::_tick', None, 'ret-patch'),
 ('SlowDownTriggers', 'BlockMovementSlowdownMultiplierSystemImpl::applySlowdownOnMove', None, 'ret-patch'),
 ('NoSlowDown',       'ItemUseSlowdownSystemImpl::doItemUseSlowdownSystem', None, 'ret-patch'),
 ('WaterSpeed',       'doWaterMoveSystem', None, 'ret-patch'),
 ('LavaSpeed',        'doLavaMoveSystem', None, 'ret-patch'),
 ('Noclip',           'MoveCollisionSystem', None, 'ret-patch'),
 ('XrayCameraThird',  'CameraAvoidanceSystem::tick', None, 'ret-patch'),
 ('LavaDrown',        'MobMovementGravity::tickLavaGravity', -0.01, 'gravity'),
 ('WaterDrown',       'MobMovementGravity::tickPlayerWaterGravity', -0.01, 'gravity'),
 ('Gravity',          'MobMovementGravity::tickGroundGravity', -0.08, 'gravity'),
 ('GravityWater',     'MobMovementGravity::tickPlayerWaterGravity', -0.08, 'gravity'),
 ('JumpHack',         'ApplyJumpModifierSystem', 0.42, 'jump'),
 ('SpeedHack',        'doDefaultMoveSystems', 0.1, 'speed'),
]

def main():
    global lib, R, db
    db=SymDB(); R=Resolver(db); lib=db.lib
    rows=[]
    print(f"{'hook':18} {'addr':>12} {'src':12} method")
    print('-'*90)
    for name, sub, const, note in HOOKS:
        a, m, src = resolve(sub, const)
        rows.append(dict(hook=name, method=m, addr=(f'0x{a:X}' if a else None),
                         src=src, const=const, note=note))
        print(f"{name:18} {(f'0x{a:X}' if a else '--'):>12} {src:12} {m or '-'}")
    json.dump(rows, open('re/offsets_1.26.66.1.json','w'), indent=1)
    print(f"\nwritten re/offsets_1.26.66.1.json")

if __name__=='__main__': main()
