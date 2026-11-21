# Apollon offsets — Minecraft Bedrock 1.26.66.1 (arm64-v8a)

**Target:** `libminecraftpe.so`, 342,231,392 bytes, md5 `fa377b0e07e7558b16d3684984598cc2`
**Header:** `offsets_1.26.66.1.h`  ·  **Machine-readable:** `re/offsets_1.26.66.1.json`
**Toolkit:** `re/recover.py` (re-runnable on any build with entt names intact)

---

## 1. What "Ghidra or Radare2 to get the right offsets" turned into

Neither Ghidra, Radare2, nor a JVM is available in this sandbox — only `objdump`
and Python. That turned out not to matter, because **this binary does not need a
decompiler**: it ships its own symbol information in a place decompilers would
only find by hand.

### The discovery

`libminecraftpe.so` exports 91,693 `.dynsym` entries, but those are only
third-party code (cohtml, V8, WebRTC, Xbox services, OpenSSL) plus JNI entry
points. Every Bedrock class — `Actor`, `Player`, `GameMode`, `ClientInstance`,
`LevelRenderer`, `BlockSource` — is **fully stripped** from `.dynsym`.

However, Bedrock's ECS (entt) stores, for every registered system, a metadata
record in `.data.rel.ro` containing:

* a pointer to the system's **mangled C++ name** (e.g.
  `_ZN18MobMovementGravity22tickPlayerWaterGravityE…`),
* seven function pointers — the system's adapter thunks and implementation.

Because the library is a position-independent shared object, each of those
pointers is an `R_AARCH64_RELATIVE` relocation (868,396 of them) whose **addend
is the final virtual address**. So:

```
name string  --(reloc addend)-->  pointer slot  --(sibling relocs)-->  function pointers
```

That chain is exact — no heuristics, no signature scanning.

### Pipeline actually used

1. Index all NUL-terminated strings in `.rodata` → 190,777 strings, of which
   **7,504 are mangled C++ names**.
2. Read all `R_AARCH64_RELATIVE` relocations → `slot → addend`.
3. For each name string, find the slot whose addend equals its VA.
4. Sibling slots in the same record are the system's function pointers.
   → **707 systems resolved by name**; with the wider index, **5,163 methods**.
5. Interior pointers are normalised to true entry points: a candidate entry is
   accepted only if a **linear decode from it reaches the interior address**.
6. Constant-bearing hooks are then confirmed by scanning the body for the
   `movz/movk + fmov` materialisation of that hook's own constant.

---

## 2. Results

### `[A]` Gravity family — fully verified

```
Gravity = WaterDrown = LavaDrown = 0xCD2266C
```

The dispatcher inside `MobMovementGravity::tickPlayerWaterGravity` does
`adr x8, #0xcd226ec` and passes that as the per-entity lambda. `0xcd226ec`
resolves back to entry **`0xcd2266c`**, whose body contains **both −0.01 and
−0.08**:

```
bl   0x10104040                 ; travel-flag predicate -> w0
mov  w8, #0xd70a ; mov w9, #0xd70a
tst  w0, #1
movk w8, #0xbda3, lsl #16       ; -0.08  (land)
movk w9, #0xbc23, lsl #16       ; -0.01  (water)
fmov s0, w8 ; fmov s1, w9
fcsel s0, s1, s0, ne            ; water ? -0.01 : -0.08
fmul s1, s14, #0.75
fadd s1, #-1.0, s1
fneg s0, s0 ; fmul s0, s0, s1
ldr  s1, [x19, #0x1c] ; fadd s0, s1, s0 ; str s0, [x19, #0x1c]
```

This is precisely the construct Apollon's `Gravity` / `WaterDrown` / `LavaDrown`
hooks are written to patch, so those three hooks are correct on this build.

> **Why the earlier constant hunt failed and this succeeded:** the old table
> pointed at a deep interior site. Searching for `-0.08` as a raw float returns
> nothing, because it is only ever built at runtime from
> `mov w,#0xd70a; movk w,#0xbda3,lsl#16`. Searching for the *system by name*
> finds the function that contains that sequence.

### `[B]` Camera and movement systems — entry points verified

| Hook | Address | Bearer |
|---|---|---|
| `NoHurtCam` | `0x8A62080` | `CameraBobSystem::tick` |
| `NoCamDistortion` | `0x11E5C3D4` | `CameraPortalDistortionSystem::_tick` |
| `NoBoatRotation` | `0x8A74BE4` | `CameraVehicleRotationLookSystem::tick` |
| `PlaceCamera` | `0x11EBB97C` | `CameraBlendSystem::_tick` |
| `XrayCameraThird` | `0x11EB5E4C` | `CameraAvoidanceSystem::tick` |
| `SlowDownTriggers` | `0xC258480` | `BlockMovementSlowdownMultiplierSystemImpl::applySlowdownOnMove` |
| `NoSlowDown` | `0xC12522C` | `ItemUseSlowdownSystemImpl::doItemUseSlowdownSystem` |
| `WaterSpeed` | `0x88DB454` | `doWaterMoveSystem` |
| `LavaSpeed` | `0xC133388` | `doLavaMoveSystem` |
| `Noclip` | `0xC10FD44` | `MoveCollisionSystem` |
| `SpeedHack` | `0xC1CA408` | `DefaultMoveSystems::doDefaultMoveSystems` |

All twelve start with a valid AArch64 prologue (`stp x29,x30,[sp,#-N]!` or
`sub sp,sp,#N`) and were proven reachable by linear decode.

Because `NoHurtCam`, `NoCamDistortion`, `NoBoatRotation`, `PlaceCamera` and
`Noclip` are applied as **`ret` patches** (`C0 03 5F D6`), a correct entry point
is all they need — those five hooks should now behave.

### `[C]` String-xref only

| Hook | Address | Note |
|---|---|---|
| `ScreenName` | `0xBA308C8` | 2 candidate functions (`ClientInstance::getScreenName` is not symbolised) |

### `[X]` Not recoverable statically — 52 hooks

These classes appear in **no symbol source at all**: not in `.dynsym`, not as
entt names, not as RTTI type names (RTTI only covers the bundled libraries).

`Actor`, `Player`, `LocalPlayer`, `Mob`, `GameMode`, `Level`, `Block`,
`BlockSource`, `ClientInstance`, `LevelRenderer`, `LevelRendererPlayer`,
`HudPlayerRenderer`, `HudVignetteRenderer`, `FullScreenEffectRenderer`,
`AppPlatform`, `BaseOptions`, `Entitlement`, `MobJump`, `TileMessage`,
`GameplayUserManager`, `BaseActorRenderer`, `CameraAPI`, `NetherNet`.

Affected hooks: `Attack`, `GetHealth`, `Intersects`, `LerpMotion`, `Swing`,
`NormalTick`, `SetSprinting`, `SetClimbing`, `SetYHeadRot`, `SetYBodyRot`,
`SetInvisible`, `IsEmoting`, `GetAbilities`, `SetupFog`, `FogState`,
`FullBright`, `GetMaxPlayers`, `UnlockMarket`, `DisplayMessage`, `GetBlock`,
`IsAir`, `GetNameTag`, `ScreenName`(exact), and the rest of the `[X]` list.

**No address has been invented for any of these.** Reaching them requires a
vtable slot index, which is only obtainable from live process memory — that is
the runtime-discovery step described in §4.

### Field offsets — all `[X]`

Every `Fields64` value in the 1.21.x table (`GetGamemode 0x9C0`, `GetLevel 0x1D0`,
`GetPosition 0x208`, `GetCameraPos 0x65C`, `FovX 0xF28`, `FovY 0xF3C`, …) is a
**live-memory** offset. None of them can be derived from the file on disk, and
none were carried over.

---

## 3. Sanity check on the old table

`verify_offsets.py` found 20 of 99 entries from the 1.21.x table structurally
impossible for this build (e.g. `Font = 0x263C304` lands inside the string
`"aes-128-cfb8"`). That table is for a different major version and must not be
reused.

---

## 4. What remains, and how to finish it

Everything left needs **runtime** discovery, because the information is not in
the file. Two practical routes:

**A. vtable-slot discovery (recommended, no debugger needed).**
`Actor`, `Player`, `ClientInstance` etc. are polymorphic, so their methods are
reachable through vtable slots. In-game, capture a live `this` pointer (e.g. from
the already-verified `MobMovementGravity` body, which holds the `StateVector` at
`[x19,#0x1c]`), read its vptr, then diff the vtable against the 1.21.x slot
indices to recover the new slot numbers. This turns each `[X]` hook into a
`vtable[n]` lookup instead of a fixed address.

**B. Diff against a second build.**
If a 1.26.66.1 library with symbols (or a 1.26.66.x dev build) can be obtained,
a binary diff of the shared entt records yields exact addresses for the rest in
minutes.

Both are runtime steps; neither can be completed from the on-disk image alone.
