#!/usr/bin/env python3
"""
resolve_all.py -- Resolve every Apollon offset that is statically recoverable
from libminecraftpe.so 1.26.66.1, and classify the rest.

Recoverable sources:
  1. entt/system metadata tables  -> named systems  (exact)
  2. .dynsym                      -> third-party + JNI (exact, not game classes)
  3. string xrefs                 -> named string users (approximate)
Not recoverable statically:
  - methods of core game classes (Actor/Player/GameMode/ClientInstance/...) --
    stripped from every symbol source; reachable only via vtable slot index,
    which requires runtime discovery.
"""
import sys, re, json, collections, struct
sys.path.insert(0,'re')
from symtable import SymDB
from nameres import Resolver, method_pairs
from body import scan, f32parts
from bedrock_re import Library

# The 1.21.x offsets.h names -> the exact C++ entity each must resolve to.
TARGETS = [
 # offset name            comment/candidates from offsets.h
 ('NoHurtCam',        ['CameraBobSystem::tick']),
 ('NoCamDistortion',  ['CameraPortalDistortionSystem::_tick']),
 ('NoBoatRotation',   ['CameraVehicleRotationLookSystem::tick']),
 ('NoCamSleep',       ['CameraSleepFadeSystem::_tickSleepFade']),
 ('PlaceCamera',      ['CameraBlendSystem::_tick']),
 ('SlowDownTriggers', ['BlockMovementSlowdownMultiplierSystemImpl::applySlowdownOnMove']),
 ('NoSlowDown',       ['ItemUseSlowdownSystemImpl::doItemUseSlowdownSystem']),
 ('WaterSpeed',       ['doWaterMoveSystem']),
 ('LavaSpeed',        ['doLavaMoveSystem']),
 ('Noclip',           ['MoveCollisionSystem']),
 ('XrayCameraThird',  ['CameraAvoidanceSystem::tick']),
 ('LavaDrown',        ['MobMovementGravity::tickLavaGravity']),
 ('WaterDrown',       ['MobMovementGravity::tickPlayerWaterGravity']),
 ('Gravity',          ['MobMovementGravity::tickGroundGravity']),
 ('JumpHack',         ['ApplyJumpModifierSystem::_tickApplyJumpModifierSystem']),
]

def main():
    db = SymDB(); R = Resolver(db); lib = db.lib
    allm = R.all_methods()
    out = []
    for name, cands in TARGETS:
        rec = dict(offset=name, candidates=cands, method=None, fns=[], note='')
        for c in cands:
            hits = [m for m in allm if c in m]
            if hits:
                rec['method'] = hits[0]
                infos = R.funcs_for(hits[0])
                if infos:
                    rec['fns'] = [a for d,w,a in infos[0]['fns']]
                break
        if not rec['method']:
            rec['note'] = 'no named system in binary'
        out.append(rec)
    ok = sum(1 for r in out if r['fns'])
    print(f"[resolve] {ok}/{len(out)} offsets mapped to named systems\n")
    for r in out:
        st = 'OK  ' if r['fns'] else 'FAIL'
        print(f"{st} {r['offset']:16} -> {r['method'] or r['note']}")
        if r['fns']:
            print(f"      {[hex(a) for a in r['fns']]}")
    json.dump(out, open('re/resolve_all.json','w'), indent=1)

if __name__ == '__main__':
    main()
