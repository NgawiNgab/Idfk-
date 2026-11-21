# Apollon 5.5.0-patchsync — the actual bug

**Download:** [`Apollon_5.5.0-patchsync.levipack`](https://drive.google.com/file/d/1cwnSxwBj79SeyNip2UBnlb8Nw7joR0mt/view)
md5 `a503eca236106f26c3639c3bfd8e108f`

## I was wrong last round, and here's the proof

I told you the six features were disabled because `Patches()` restored them
every tick due to `inMenu` being stuck `true`. **That was wrong.**

`Patches()` is defined at line 226 and **never called anywhere in the entire
codebase**. Not from `ClientInstance_onTick`, not from `NormalTick`, not from
`hack_thread`, not from the mod lifecycle. Nowhere.

Both hooks that would have called it are dead:

```
NormalTick            = 0   ← offsets.h, Offsets64
ClientInstanceOnTick  = 0
```

`A64HookSafe` skips any offset of `0`, so neither hook installed and nothing
ever drove the patches.

## And here's the part that makes it look like it works

In `hack_thread`, every one of the six features does this:

```cpp
hexPatches.NoBoatRotation = MemoryPatch::createWithHex(..., "C0 03 5F D6");
log("PATCH-ok NoBoatRotation @ 0x8A76564");     // ← says OK
                                                // ← and never calls .Modify()
```

`createWithHex` only **snapshots** the original bytes. Nothing is written to the
game until `Modify()` is called. Compare the next statement in the same function:

```cpp
hexPatches.NativeKeyHandler = MemoryPatch::createWithHex(...);
hexPatches.NativeKeyHandler.Modify();          // ← this one is applied
```

So the log cheerfully reported `PATCH-ok` for six features while the game was
completely untouched. **That is why only SpeedHack worked** — SpeedHack uses
`DobbyHook`, which patches itself at install time and needs no ticker.

## The fix

`applyPatches()` now exists and is called from two places:

1. **Once at the end of `hack_thread`**, after every patch object is constructed
   — establishes a known-clean baseline (all flags default false).
2. **On every Mod Menu toggle**, because there's no tick hook available to poll
   the flags.

Each call logs a per-feature line under tag `ApollonPatch`:

```
RESULT NoBoatRotation: ON -> applied
RESULT PlaceCamera: patch NOT CREATED in hack_thread (offset was 0 ...)
```

That log now distinguishes the two failure modes that used to look identical.

## About "still need to jump"

I kept that loop enabled — it's the only thing working, and disabling it would be
a regression. The jump dependency is real and documented: `0x0C1CC8B0` is a
method **body**, but 1.21 hooked a dispatch **thunk**. Re-entering a body restarts
half-finished work, which is why the duplicate only lands the frame after a jump.
Fixing it properly means finding the thunk, which needs the 1.26 binary.

## About Reach and Aura

Good instinct, and much closer to working than I expected — but they hit a wall
that isn't a gating bug:

**Reach** is nearly free. `GameMode_BlockReach()` is already written and already
hooked at line 1554:
```cpp
if (blockReach) return 12;
```
All it needs is `BlockReach` in `Offsets64`, which is `0` along with 66 other
names. One offset.

**Aura** is not savable cheaply. It lives in `NormalTick` — **also `0`** — and
depends on `Fields64`, the member-offset table, which is **entirely zeros**:
`GetPosition`, `ActorRotation`, `GetHurtTime`, `GetGameMode`, `GetLevel`. Every
one. Recovering them without the binary risks reading attacker-controlled memory
at wrong offsets, which is a crash or worse, not a feature.

**And it wouldn't have helped anyway** — the six features were never blocked by
their offsets. They were blocked by a missing function call. Reach and Aura would
have failed for the exact same reason.

## Test

Toggle **No Boat Rotation**, **Place Camera**, **Xray Camera**, **No SlowDown**,
**Off Drowning**. If a log is capturable, tag `ApollonPatch` now names which
feature applied and which was never constructed.

Two outcomes matter:
- **Features react** → the offsets are fine, gating was the only blocker.
- **`patch NOT CREATED`** → that row's offset is still `0`; tell me which and
  it's a targeted recovery job rather than a guessing game.

Offsets still 11/77. Build clean, 0 errors, 0 undefined `pl::` symbols, no
`libpreloader.so` dependency.
