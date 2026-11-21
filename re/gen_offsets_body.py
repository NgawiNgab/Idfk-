#!/usr/bin/env python3
"""gen_offsets_body.py -- BODY-LAYER offset table for 1.26.66.1.

Recovered addresses are the real method bodies (record slot[6]), not the
generic dispatch thunks the 1.21 table pointed at. Every entry must be:
  * named by an entt registry adapter string, and
  * validated by disassembling at least one plausible body instruction,
  * and has a documented reason when left at 0.
"""
import sys, re, json, collections
sys.path.insert(0,'re')
from symtable import SymDB
from systems import build, method_of
from diff_match import Bin, md

LIB='mods/apollon/game/libminecraftpe.so'
HDR='mods/apollon/source/Apollon_Client_ImGui/app/src/main/jni/offsets.h'

# hook -> (substring of registry method name, required if any)
WANT = {
 'Gravity':        'MobMovementGravity::tickPlayerWaterGravity',
 'WaterDrown':     'MobMovementGravity::tickPlayerWaterGravity',
 'LavaDrown':      'MobMovementGravity15tickLavaGravity',
 'NoBoatRotation': 'CameraVehicleRotationLookSystem::tick',
 'PlaceCamera':    'CameraBlendSystem::_tick',
 'XrayCameraThird':'CameraAvoidanceSystem::tickILb1',
 'SlowDownTriggers':'BlockMovementSlowdownMultiplierSystemImpl::applySlowdownOnMove',
 'NoSlowDown':     'ItemUseSlowdownSystemImpl::tickItemUseSlowdown',
 'WaterSpeed':     'doWaterMoveSystem',
 'LavaSpeed':      'doLavaMoveSystem',
 'SpeedHack':      'DefaultMoveSystems20doDefaultMoveSystems',
}

def load_hooks():
    src=open(HDR).read()
    blk=src[src.index('namespace Offsets64'):]; blk=blk[:blk.index('};')]
    return [(n,int(v,16)) for n,v in re.findall(r'(\w+)\s*=\s*(0x[0-9A-Fa-f]+)',blk)]

def main():
    db=SymDB(LIB); recs=build(db); b=Bin(LIB)
    name2body={}
    for r in recs:
        f=r['fns']
        if len(f)>=7 and f[6] and b.t0<=f[6]<b.t1:
            for m in method_of(r['adapter']):
                name2body.setdefault(m, f[6])
    hooks=load_hooks()
    out={}
    for name,old in hooks:
        want=WANT.get(name)
        if not want:
            out[name]=dict(addr=0,old=old,src=None,
                           note='no registry-named body; needs runtime/vtable anchor')
            continue
        cands=[(k,v) for k,v in name2body.items() if want in k]
        if not cands:
            out[name]=dict(addr=0,old=old,src=None,note=f'name {want} not in registry')
            continue
        k,v=cands[0]
        # validate: disassembles and is not a thunk
        ins=[i.mnemonic for i in md.disasm(b.insn_bytes(v,12),v)]
        if 'blr' in ins and 'adr' not in ins:
            out[name]=dict(addr=0,old=old,src=None,note=f'slot6 is thunk for {k[:50]}')
            continue
        out[name]=dict(addr=v,old=old,src=k,note='')
    json.dump(out,open('mods/apollon/out/offsets_body_1.26.66.1.json','w'),indent=1)
    ok=sum(1 for v in out.values() if v['addr'])
    print(f"body-layer recovered: {ok}/{len(hooks)}")
    for n,v in out.items():
        if v['addr']: print(f"  {n:24s} = {v['addr']:#x}   ({v['src'][:46]})")
    print("\nunresolved:", sum(1 for v in out.values() if not v['addr']))

if __name__=='__main__': main()
