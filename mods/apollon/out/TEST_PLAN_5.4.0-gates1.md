# Apollon 5.4.0-gates1 — fixes from the first working test

**Download:** [`Apollon_5.4.0-gates1.levipack`](https://drive.google.com/file/d/1ZPLJn0udL4mmD44PQ1hbkYqfxac_MZsW/view)
md5 `a026b5db38db96b46fd9a56cf9381bd8`

Your three observations were exactly enough to find two real bugs. Here's what
each one meant.

---

## Bug 1 — every patch-backed feature was permanently switched off

`Patches()` opened like this:

```cpp
if (inMenu) { Restore every patch }
else        { apply each feature's flag }
```

`inMenu` means *"an ImGui window is covering the screen."* It is declared
**`true`** in `variables.h` and is only ever cleared by `ClientInstance_onTick`
— and `ClientInstanceOnTick` is **offset 0** in my 1.26 table, so that hook never
installs and `inMenu` never becomes false.

**The `else` branch was unreachable.** Every hex-patch feature — NoBoatRotation,
PlaceCamera, XrayCameraThird, SlowDownTriggers, NoSlowDown, Off Drowning — was
`Restore()`d forever, whatever you toggled.

That is precisely why **only SpeedHack worked**: SpeedHack/WaterSpeed/LavaSpeed/
Gravity use `DobbyHook`, which `hexPatches.Restore()` cannot undo.

**Fix:** the condition is now `kImGuiSurfaceActive` (a `constexpr false`).
`inMenu` describes the *game's* screen; it must not decide whether *our* features
are on.

## Bug 2 — the speed hooks had no module gate at all

```cpp
for (int i = 0; i < speedValue - 1; ++i) old_SpeedHack(...);
```

No check of any module state. The hook installs whenever the offset resolves, so
it ran with the module **off** — exactly what you saw.

**Fix:** all three now require `speedModuleOn && multiplier > 1`. Each of the
three modules gained a real **Toggle** (it only had a slider before — there was
nothing to switch on), and the slider minimum moved 1 → 2 because a multiplier of
1 is a no-op.

## About needing a jump

With `speedValue = 1` the loop ran **zero** times, so the hook was a
pass-through — yet movement changed. That's consistent with the body-vs-thunk
difference I documented earlier: the 1.21 table pointed at a dispatch **thunk**,
where an extra call cleanly re-ran the system; I'm now hooking a method **body**
(record slot 6), so an extra call restarts that body's work *mid-tick*. The
effect landing on the frame after a jump is the signature of that.

I've made it deterministic rather than claiming it's fixed: with the gate and
multiplier > 1 it should behave consistently. A proper multiplier would need a
different hook site.

---

## What to test

1. Open the menu → **SpeedHack** should now show both an **Enabled** toggle and a
   **SpeedHack** slider.
2. Leave **Enabled off**, multiplier high → nothing should happen. *(This is the
   regression test for Bug 2.)*
3. Turn **Enabled on**, multiplier 2–3 → should speed up without needing a jump.
4. Then check the previously-dead ones: **No Boat Rotation**, **Place Camera**,
   **Xray Camera**, **No SlowDown**, **Off Drowning**.

Expected: the six patch-backed features can now actually do something for the
first time. Watch for these two possibilities:

| result | meaning |
|---|---|
| patch features now react | gating was the only blocker — offsets are good |
| still nothing, but no crash | the `"C0 03 5F D6"` (`ret`) patch doesn't cancel work already done upstream at these body sites (see ROUND 4) |
| crash when toggling one | that feature's patch lands somewhere unsafe — tell me which and I'll pull that row |

Log tag is still `ApollonOffset` / `ApollonMenu` if you can capture it.

## Housekeeping

The workspace was wiped again mid-session (repo back to the initial commit, the
whole `/tmp` toolchain and the game `.so` gone). I restored the branch from
`origin` and rebuilt the toolchain from scratch to produce this binary. The
game library is gone, so I could not re-run the offline dlopen checker this
round — the build itself was verified clean (0 errors, 0 undefined `pl::`
symbols, no `libpreloader.so` dependency).

Offsets unchanged: still 11/77.
