# Apollon 5.1.0-test1 — on-device verification

**Purpose.** Confirm the 11 recovered body-layer offsets for Minecraft Bedrock
**1.26.66.1 (arm64-v8a)** actually resolve to live engine code before we spend
effort recovering the remaining 66.

**Do not judge this build on features working.** Judge it on the logcat lines it
prints. A hook that resolves but does not visibly change gameplay is *normal* on
a first pass — these are method **bodies**, whereas the 1.21 table patched
dispatch thunks, so a `ret` written at an entry point does not cancel work that
already happened upstream.

---

## 1. Install

| file | md5 |
|---|---|
| `Apollon_ImGui_5.1.0-test1.levipack` | `cd8bd4257f3ed69d4b75efb9cae95172` |
| `libapollon_imgu.so` (inside the pack) | `d0cd184d93de72719c1575841d64ecf2` |

Import the `.levipack` in LeviLaunchroid, target **1.26.66.1**, and launch.
If the pack is rejected, verify the md5 first.

## 2. Capture the log

Start capturing **before** launching Minecraft, then launch and let the main menu
appear:

```bash
adb logcat -c
adb logcat -s ApollonOffset:V Apollon:V > apollon_test1.txt
```

Or grab everything if you are unsure of the tag:

```bash
adb logcat | grep -iE 'ApollonOffset|apollon' > apollon_test1.txt
```

Also worth having (crash evidence, if any):

```bash
adb logcat -b crash > crash.txt
```

## 3. What the lines mean

Every recovered hook prints one of:

```
PASS <Hook> @ 0x<rel>  first=<word>      entry decodes as a real function, hook installed
FAIL <Hook> @ 0x<rel>  first=<word>      entry did NOT decode as a function. Skipped. Safe.
HOOKED <Hook> @ 0x<rel>                  inline hook installed
REFUSED <Hook> @ 0x<rel>                 verification failed, hook deliberately not installed
PATCH-ok <Hook> @ 0x<rel>                hex patch applied
PATCH-skip <Hook> @ 0x<rel>              hex patch not applied (offset 0, or aliases another hook)
```

`@ 0x...` is the **relative** offset, so it should match this table exactly:

| hook | expected offset | body |
|---|---|---|
| Gravity | `0x0CD4C40C` | `MobMovementGravity::tickPlayerWaterGravity` |
| WaterDrown | `0x0CD4C40C` | *same body as Gravity — patch is skipped by design* |
| LavaDrown | `0x0CD4CCE0` | `MobMovementGravity::tickLavaGravity` |
| NoBoatRotation | `0x08A76564` | `CameraVehicleRotationLookSystem::tick` |
| PlaceCamera | `0x11EBC7BC` | `CameraBlendSystem::_tick` |
| XrayCameraThird | `0x11EB835C` | `CameraAvoidanceSystem::tickILb1` |
| SlowDownTriggers | `0x0C259410` | `BlockMovementSlowdownMultiplierSystemImpl::applySlowdownOnMove` |
| NoSlowDown | `0x0C12825C` | `ItemUseSlowdownSystemImpl::tickItemUseSlowdown` |
| WaterSpeed | `0x0CDC2F68` | `doWaterMoveSystem` |
| LavaSpeed | `0x0C1353EC` | `doLavaMoveSystem` |
| SpeedHack | `0x0C1CC8B0` | `DefaultMoveSystems::doDefaultMoveSystems` |

Expected: **11 PASS** (10 unique addresses; `WaterDrown` prints
`PATCH-skip ... (aliases Gravity)`), and **0 FAIL**.

## 4. How to read the result

- **All 11 PASS, no crash** → the body layer is sound. Green light to recover
  the remaining 66.
- **Some FAIL** → that offset is wrong; the name→body mapping for those entries
  needs another pass. Nothing is damaged, they are simply skipped.
- **Crash on launch** → capture `crash.txt`. The most likely culprits are a hook
  whose body is real but whose *calling convention* does not match Apollon's
  stub signature, not a bad address.
- **`unresolvable`/`cannot locate symbol` on dlopen** → the build's ABI is
  wrong; send the load error verbatim.

## 5. Note on the 66 zeros

Every hook still at `0` is **inert**: `A64HookSafe()` returns early and
`MemoryPatch::createWithHex()` refuses a 0 offset. They cannot corrupt the ELF
header or crash the client. Features behind them will simply do nothing. That is
the intended state of this test build — no fabricated addresses.

## 6. Suggested functional spot-checks (secondary)

Only after the log looks right. Toggle one at a time:

1. **SpeedHack / WaterSpeed / LavaSpeed** — movement feel changes.
2. **NoSlowDown / SlowDownTriggers** — eating and soul-sand slow less.
3. **NoBoatRotation / PlaceCamera / XrayCameraThird** — camera behaviour.
4. **Gravity** — jump/fall feel.
5. **LavaDrown** — standing in lava.

If a toggle does nothing, that is an expected possible outcome at this layer;
record which ones changed behaviour so we can correlate.
