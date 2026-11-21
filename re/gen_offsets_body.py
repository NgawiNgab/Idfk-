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

def real_body(b, addr, min_words=4):
    """True if `addr` is a plausible non-trivial function entry.

    A name can be attached to several slots in one registry record; some of them
    are empty `ret` stubs (CameraBlendSystem::_tick at 0x11EBC844 is literally a
    single `ret`, preceded by a run of other empty functions). Patching a bare
    `ret` is a no-op at best, so require the body to actually do something.
    """
    if not (b.t0 <= addr < b.t1): return False
    w = b.word(addr)
    if w == 0xd65f03c0: return False                      # ret
    if (w & 0xFC000000) == 0x14000000: return False       # b <veneer>
    ms = [i.mnemonic for i in md.disasm(b.insn_bytes(addr, 64), addr)]
    if not ms: return False
    if ms[0] not in ('sub','stp','str','sturb','movi','mov','ldr','add'): return False
    # count real work before the first ret
    real = 0
    for m in ms:
        if m in ('ret','br'): break
        if m not in ('nop','bti','paciasp'): real += 1
    return real >= min_words

# The 77 hook names are FIXED (they must match every Offsets64::X reference in
# main.cpp). Do not read them back from the generated header -- that would make
# the generator consume its own output and collapse to 0 entries on a rerun.
HOOKS = ['Font','NoHurtCam','NoCamDistortion','NoBoatRotation','NoCamSleep','PlaceCamera',
 'WaterTrigger1','WaterTrigger2','WaterTrigger3','SlowDownTriggers','NoSlowDown','WaterSpeed',
 'LavaSpeed','Noclip','XrayCameraThird','LavaDrown','WaterDrown','Fov','SpeedHack','JumpHack',
 'Gravity','AutoClickMine','FastBridge','BlockReach','PlayerAuthInput','NativeKeyHandler',
 'InMainMenu','InLanGame1','InLanGame2','FastDrop','UnlockMarket','GetMaxPlayers','FogState',
 'SetupFog','FullBright','ForEachActivePlayer','GetRuntimeActor','IsAttackable','Attack',
 'LerpMotion','NormalTick','Swing','SetSprinting','GetEntityTypeId','GetHealth','SetClimbing',
 'SetYHeadRot','SetYBodyRot','SetInvisible','Intersects','GetYHeadRot','InitMaxAutoStep',
 'GetAABBDim','IsEmoting','NoBlur1','NoBlur2','ShowPaperdoll','Subtitle','GetAbilities',
 'GetLayer','SetAbility','ClientInstanceOnTick','HandleDestroyOrAttack','PlayUI',
 'SetNoJumpDelay','SetOnGround','MobIsJumping','DisplayMessage','Block_getRenderLayer',
 'RebuildAllChunkGeometry','VignetteRenderer','NoNametags','ScreenName','GetNameTag',
 'GetDimensionBlockSource','GetBlock','IsAir']

def load_hooks():
    return [(n, 0) for n in HOOKS]

def write_header(out):
    """Emit the standalone reference header (out/), mirroring the enum in
    jni/offsets.h. These two must never drift: the .h is the artifact quoted in
    the status docs, the enum is what actually compiles."""
    L=["#if defined(__aarch64__)",
       "namespace Offsets64 {",
       "\tenum Offsets64 {",
       "\t\t// ===== 1.26.66.1 BODY-LAYER table =====",
       "\t\t// Non-zero = real entt system method body, resolved by name from the",
       "\t\t// 1.26.66.1 registry (highest valid record slot = slot[6] = body) and",
       "\t\t// machine-validated: inside .text, decodes to a function prologue, and",
       "\t\t// contains real work before the first ret.",
       "\t\t// 0 = unrecovered and therefore inert.",
       "\t\t//",
       "\t\t// WaterDrown and Gravity are the SAME body in this build",
       "\t\t// (MobMovementGravity::tickPlayerWaterGravity); main.cpp lets the Gravity",
       "\t\t// inline hook own it and skips the WaterDrown hex patch.",
       "\t\t//"]
    for n in HOOKS:
        v=out.get(n) or {}
        a=v.get('addr') or 0
        if a: L.append(f"\t\t{n} = {a:#X},// RECOVERED: {(v.get('src') or '')[:70]}")
        else: L.append(f"\t\t{n} = 0,")
    L+=["\t};","}","#endif",""]
    open('mods/apollon/out/offsets_body_1.26.66.1.h','w').write('\n'.join(L))

def main():
    db=SymDB(LIB); recs=build(db); b=Bin(LIB)
    # A record exposes the same method at several slots: slot3/slot5 are the
    # +0x30/+0x38 dispatch thunks (they `blr` through a vtable slot), slot6 is
    # the real method body. Prefer the highest valid slot index = slot6, so we
    # hook the body rather than a thunk that would re-dispatch to itself.
    name2body={}
    for r in recs:
        for idx,f in enumerate(r['fns']):
            if not f or not real_body(b, f): continue
            for m in method_of(r['adapter']):
                cur=name2body.get(m)
                if cur is None or idx > cur[0]:
                    name2body[m]=(idx,f)
    hooks=load_hooks()
    out={}
    for name,old in hooks:
        want=WANT.get(name)
        if not want:
            out[name]=dict(addr=0,old=old,src=None,
                           note='no registry-named body; needs runtime/vtable anchor')
            continue
        cands=[(k,v[1]) for k,v in name2body.items() if want in k]
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
    write_header(out)
    ok=sum(1 for v in out.values() if v['addr'])
    print(f"body-layer recovered: {ok}/{len(hooks)}")
    for n,v in out.items():
        if v['addr']: print(f"  {n:24s} = {v['addr']:#x}   ({v['src'][:46]})")
    print("\nunresolved:", sum(1 for v in out.values() if not v['addr']))

if __name__=='__main__': main()
