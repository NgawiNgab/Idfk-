# Apollon 5.2.0-menu1 — Mod Menu build

**Download:** [`Apollon_5.2.0-menu1.levipack`](https://drive.google.com/file/d/1KMz8F6eXWk3I_hoiA8YUpDnlWLa7kf5T/view)
md5 `5bc1acc1ff553c0f76c9bcfbd233691e`

---

## Why the last build had no GUI

Not ImGui's fault. The ImGui surface had exactly one frame pump:

```c
Java_com_mojang_minecraftpe_platforms_Platform2_step(...)
```

That is a **JNI symbol the game has to call by name**. LeviLaunchpad `dlopen()`s
this `.so` and never runs `JNI_OnLoad`, so the game never called it,
`g_Initialized` stayed `false`, and `ImGui::Render()` never ran. Hence
"No GUI detected".

## What this build does instead

Features register as **LeviLaunchpad Mod Menu modules**. The launcher owns the
UI and persistence; we only supply metadata and a callback that writes the same
globals the old ImGui checkbox wrote. The existing tick thread already polls
those globals, so a toggle reaches the feature through the *exact* previous code
path — panic guards and `hexPatches.Modify()/Restore()` included.

### 10 modules (every one backed by a recovered offset)

| Module | Type | Drives |
|---|---|---|
| SpeedHack | slider 1–50 | `speedValue` |
| Water Speed | slider 1–50 | `waterSpeedValue` |
| Lava Speed | slider 1–50 | `lavaSpeedValue` |
| Gravity | slider −0.08…0.08 | `gravityValue` |
| No SlowDown | toggle | `noSlowDown` |
| No Web / Slowdown Trigger | toggle | `offSlowdownTrigger` |
| No Boat Rotation Limit | toggle | `noBoatRotation` |
| Xray Camera (Third Person) | toggle | `xrayCam` |
| Place Camera (Third Person) | toggle | `placeCam` |
| Off Drowning | toggle | `offDrown` |

Only features with a recovered 1.26.66.1 offset are offered. Registering a
toggle that does nothing would be worse than omitting it.

---

## Install and check

Import the `.levipack` into LeviLaunchpad, target **1.26.66.1**, launch.

### 1. Confirm the menu registered

```bash
adb logcat -c
adb logcat -s ApollonMenu:V ApollonOffset:V > apollon_menu.txt
```

You want:

```
mod menu ready: 10/10 modules registered
registered 10 modules (0 = launcher menu API unreachable)
```

**If you see `registered 0`** — the launcher's `libpreloader.so` did not expose
`pl::modmenu`, and this build will not have loaded at all (it declares
`libpreloader.so` as `DT_NEEDED`). Send the full `dlopen` error.

### 2. Confirm the offsets are live

Same log, tag `ApollonOffset`. Expected:

```
PASS <Hook> @ 0x<rel>  first=<word>     x11 (10 unique; WaterDrown aliases Gravity)
PATCH-skip WaterDrown @ 0x0cd4c40c (aliases Gravity)
```

**0 FAIL** is the goal.

### 3. Now actually toggle things

This is what was impossible before. Turn on one at a time:

1. **SpeedHack** → slider up → walk faster
2. **Water Speed** → swim faster
3. **Lava Speed** → move faster in lava
4. **No SlowDown** → eat while walking, no slowdown
5. **Off Drowning** → stand in deep water, no drown damage
6. **No Boat Rotation Limit** → boat camera frees up
7. **Xray Camera / Place Camera** → third-person camera changes
8. **Gravity** → jump/fall feel

Report which ones visibly change behaviour. A module that resolves but changes
nothing is *expected* at this layer (body patch vs. thunk patch — see
`OFFSET_RECOVERY_STATUS.md` ROUND 4), and knowing which ones work is exactly
what decides whether recovering the remaining 66 is worth it.

---

## If it crashes or won't load

```bash
adb logcat -b crash > crash.txt
adb logcat | grep -iE 'apollon|preloader|dlopen' > load.txt
```

Send both. The most likely failures:

| symptom | cause |
|---|---|
| `cannot locate symbol _ZN2pl7modmenu14registerModule...` | launcher's libpreloader older than the SDK this was built against |
| `library "libpreloader.so" not found` | pack installed outside LeviLaunchpad |
| loads, but `registered 0` | launcher ignores the module registry |
| crash after enabling a toggle | that specific hook's calling convention, not the address |

---

## What is NOT in this build

- No ImGui window (removed by design — it could never be pumped)
- 66 of 77 offsets still `0` → inert, refused by `A64HookSafe`, never patched
- No ESP / killaura / noclip modules offered, because those have no recovered
  offset yet and a dead toggle is a lie
