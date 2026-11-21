# Apollon 5.3.0-menu2 — why no module appeared, and what changed

**Download:** [`Apollon_5.3.0-menu2.levipack`](https://drive.google.com/file/d/15y68aGgU8Jgh2hYuL8MhbL1MxWwLUTQv/view)
md5 `e2262902dee53e2f4864f4319d84f7a6`

Thank you for the BedrockTools link — it was the right call. I cloned it *and*
`LiteLDev/preloader-android` (the actual launcher runtime) and read how a mod is
really accepted. There were **three** defects, any one of which alone produces
"no module at all".

---

## Defect 1 — a hard `DT_NEEDED` on `libpreloader.so` (the killer)

5.2.0 linked a stub with SONAME `libpreloader.so` to force the dependency. That
made the launcher's library a **load-time requirement**. If the installed
`libpreloader.so` exports a different symbol set than the SDK I compiled
against, `dlopen()` of *our own library* fails:

```
cannot locate symbol "_ZN2pl7modmenu14registerModuleERKNS0_10ModuleInfoE"
```

…and the mod doesn't load **at all** — no hooks, no modules, no log. Exactly
your symptom.

**Fix:** resolve it at runtime, `dlsym(RTLD_DEFAULT)` with a
`dlopen("libpreloader.so", RTLD_NOLOAD)` fallback. Verified: `libpreloader.so`
is gone from `DT_NEEDED`, **0** undefined `pl::` symbols remain. A missing or
older API now degrades to "menu unavailable, hooks still work" *and says so*.

## Defect 2 — registering in the wrong lifecycle phase

`preloader-android` wraps **only** the `load` callback in
`pl::runtime::ScopedModMenuOwner`. `RegisterCppModule` then checks:

```cpp
if (modId != ownerModId) {
    preloaderLogger.error("Rejected Mod Menu module {}: declared mod_id {} "
                          "does not match owning lifecycle mod {}", ...);
    return false;
}
```

I was registering from **`enable`** — outside that scope. Every module was
silently rejected.

**Fix:** registration moved into `load`. Hooks stay in `enable`, so a failed
hook can never stop the menu from appearing.

## Defect 3 — hardcoded `mod_id = "apollon"`

The launcher derives a mod's id from its **install directory name**
(`ModManifest.cpp`: `.id = modDirectory.filename().string()`). No manifest field
can influence it. So a hardcoded id mismatches unless you happened to extract it
into a folder named exactly `apollon`.

**Fix:** leave `mod_id` empty — the launcher fills it from the current owner.
This is verbatim what BedrockTools does (`mModId = context.id()`).

---

## Check it

```bash
adb logcat -c
adb logcat -s ApollonMenu:V ApollonOffset:V > apollon2.txt
```

Search for **`load phase: registered N modules`**. This line is new and
decisive:

| what you see | meaning |
|---|---|
| `load phase: registered 10 modules` | **fixed** — modules exist, open the menu |
| `load phase: registered 0 modules (0 = menu API unreachable or all rejected)` | the API was found but every module was rejected; send me the `Rejected Mod Menu module` lines above it |
| `menu API unavailable -- skipping registration` | `libpreloader.so` isn't mapped in your process |
| `libpreloader.so is mapped but does not export ...` | your launcher's API is older/different than the SDK I built against |
| **nothing at all** | the mod still isn't loading — send `adb logcat \| grep -iE 'apollon\|preloader\|dlopen'` |

That last case matters: Defect 1 means silence was possible. Now a failed load
can't be silent, because we no longer hard-link the launcher.

## Confirmed correct (so I didn't change them)

- our `.levipack` manifest shape matches BedrockTools exactly
  (`type: preload-native` + `entry` + `version` + `minecraft_versions`)
- ours passes `parseModDirectory` — I ran its validation rules against it
- `minecraft_versions` is **not** enforced by the launcher, so version matching
  isn't rejecting the pack
- `icon` is optional

## Unchanged

Offsets still 11/77. Once modules appear, the toggles finally become testable —
that was the whole point. If `registered 10` shows up, then turn on SpeedHack
and tell me if you move faster; that single data point tells me whether the
body-layer offsets are worth finishing.
