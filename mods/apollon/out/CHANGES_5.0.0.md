# Apollon Reborn 5.0.1 — what changed and why

Target: **Minecraft Bedrock 1.26.66.1**, arm64-v8a, LeviLauncher native mod.

---

## 1. The `dlopen` failure — root cause and fix

You saw:

```
Failed to load mod Apollon ImGui:
dlopen failed: cannot locate symbol "_ZNSt3__113random_deviceD1Ev"
referenced by "/data/data/org.levimc.launcher/cache/native_mods/
com.mojang.minecraftpe/mods/Apollon ImGui/libapollon_imgu.so"
```

That mangled name decodes to `std::__1::random_device::~random_device`.

**`std::__1` is upstream libc++'s ABI namespace. Android uses `std::__ndk1`.**

The previous build passed

```
-D_LIBCPP_ABI_NAMESPACE=__1 -D_LIBCPP_ABI_NAMESPACE_LEN=3
```

so every libc++ symbol in the object files was emitted as `St3__1...`. Android's
`libc++_shared.so` only exports `St6__ndk1...`, the linker could not match them,
and `dlopen` aborted before `JNI_OnLoad` was ever reached.

Measured on the old `.so`:

| | count |
|---|---|
| undefined symbols in `St3__1` | **65** |
| undefined symbols in `St6__ndk1` | **0** |

After changing the build to `__ndk1` / length `6`:

| | count |
|---|---|
| undefined symbols in `St3__1` | **0** |
| undefined symbols in `St6__ndk1` | **65** |

All 65 now resolve, because `libminecraftpe.so` itself exports the `__ndk1`
namespace (verified: it exports `_ZNSt6__ndk113random_deviceD1Ev`,
`_ZNSt6__ndk113random_deviceclEv`, and the `basic_string` methods we need).

## 2. A second, independent load failure (silent)

While auditing the symbol table we found 2 more unresolved symbols that would
have failed *after* the ABI fix:

```
_ZN10KittyUtils17validateHexStringERNSt6__ndk112basic_string...
_ZN10KittyUtils7fromHexERKNSt6__ndk112basic_string...Pv
```

Cause: Apollon contains **two different things called `KittyUtils`** —

* a **class** `KittyUtils` in `KittyMemory/KittyUtils.h`
* a **namespace** `KittyUtils` in `KittyScanner/KittyUtils.hpp`

The build script named object files with `basename`, so both produced
`KittyUtils.o`. The second one silently overwrote the first, and the class
member definitions vanished.

Fix: object names are now derived from the full relative source path
(`KittyMemory_KittyUtils.o`, `KittyScanner_KittyUtils.o`). Object count went
19 → 20 and both symbols resolve.

## 3. It would never have started under LeviLauncher

Even with the load fixed, the old build only implemented `JNI_OnLoad`.

LeviLauncher loads a native mod with `dlopen` and drives it through the
`PLGetModRegistration()` dispatch table — **`JNI_OnLoad` is never called on
that path**, so `cheatThread` was never created and no hook was ever installed.

This build is *Levi native mod only*, as requested:

* `JNI_OnLoad` / `JNI_OnUnload` removed
* new exported lifecycle:

```c
void *PLGetModRegistration();      // pl::mod::ModRegistration-compatible
void  ApollonNativeMod_Start();    // starts the overlay/cheat thread
void  ApollonNativeMod_Stop();     // stops it
```

The registration table exposes `load` / `enable` / `disable` / `unload`.
`load` and `enable` both call `ApollonNativeMod_Start()`, which is idempotent.

Verified in the built `.so`:

```
PLGetModRegistration  : True
ApollonNativeMod_Start: True
JNI_OnLoad            : False
```

## 4. New menu — "Aurora"

The old menu is replaced. Widget bodies were carried over **verbatim** from the
upstream sources, so no feature was lost:

| widget | original | new |
|---|---|---|
| `Checkbox` | 124 | **124** |
| `Combo` | 45 | **45** |
| `DragFloat` | 9 | **9** |
| `SliderFloat` | 2 | **2** |
| `SliderInt` | 1 | **1** |
| `DragInt` | 8 | 7 |

The one missing `DragInt` is `ActorId`, which was inside a commented-out block
on the old Info page — nothing functional was dropped.

What changed visually:

* flat sidebar with an active-state pill instead of borderless buttons
* one violet accent (`#7860FF`) replacing the boxed magenta look
* rounded window, accent hairline, custom header with drag-and-close
* every page scrolls, so long pages (Combat, Visuals, Hotkeys) no longer
  overflow off-screen on phones
* branding reads **APOLLON REBORN / 1.26.66.1** instead of the stale 1.21.111
* the old Russian/English disclaimer gate is gone
* new **Status** page reports which modules can actually work on this build

Files: `aurora_theme.h` (new theme + helpers), `menu.h` (new shell; the old one
is kept as `menu.h.orig`).

## 5. Honest status of the module set

The 11 verified hooks from the offset work are wired and should work:

`NoHurtCam`, `NoCamDistortion`, `NoBoatRotation`, `PlaceCamera`,
`XrayCameraThird`, `SlowDownTriggers`, `NoSlowDown`, `WaterSpeed`,
`LavaSpeed`, `Noclip`, `SpeedHack`, plus the gravity family.

The remaining modules (Combat, ESP, Xray, Players, Hotkeys) depend on
`Actor` / `Player` / `GameMode` methods which are **absent from every symbol
source in this build**. They still need the runtime vtable pass. The new
Status page says so in-game rather than silently doing nothing.


---

## 6. compiler-rt: `_aarch64_cas4_acq_rel` (5.0.1)

After the ABI and object-collision fixes the mod still failed to load:

```
dlopen failed: cannot locate symbol "__aarch64_cas4_acq_rel"
```

That is an **out-of-line atomic** emitted by clang when it decides an
`std::atomic` CAS/RMW is better out-of-line than inline. The functions live in
**compiler-rt**, which is normally linked automatically — but this build uses
`-nostdlib`, so nothing pulled it in.

Four were missing, from two objects:

| object | symbol |
|---|---|
| `Arm64_And64InlineHook.o` | `__aarch64_cas4_acq_rel` |
| `Arm64_And64InlineHook.o` | `__aarch64_ldadd4_acq_rel` |
| `main.o` | `__aarch64_ldadd8_acq_rel` |
| `main.o` | `__aarch64_ldadd8_relax` |

Fix: build compiler-rt for the target once and link the archive.

```sh
zig build-lib -target aarch64-linux-android -O ReleaseFast -fcompiler-rt \
    --name crt_a64 -femit-bin=/tmp/crt/libcrt_a64.a
```

The build script now requires `$CRT_A64` and links it next to libdobby.a.

Verified in an **unstripped** test link (the shipping binary is `--strip-all`,
which hides definitions):

```
__aarch64_cas4_acq_rel    -> 0x1b8500   DEFINED
__aarch64_ldadd4_acq_rel  -> 0x1b84d0   DEFINED
__aarch64_ldadd8_acq_rel  -> 0x1b8470   DEFINED
__aarch64_ldadd8_relax    -> 0x1b84a0   DEFINED
```

Undefined atomic symbols in the shipping binary: **0**.

### Cumulative symbol health

| check | 4.62-opt (original) | 5.0.0 | 5.0.1 |
|---|---|---|---|
| `St3__1` undefined | 65 | 0 | 0 |
| `KittyUtils` undefined | 2 | 0 | 0 |
| `__aarch64_*` undefined | 4 | 4 | **0** |

---

## Artifacts

| file | md5 |
|---|---|
| `libapollon_imgu.so` | `2460a792102df704b5ac049507a802e3` |
| `Apollon_ImGui.levipack` | `73d43cdf02f9777110494e96528e062b` |
| `Apollon_ImGui.apk` | `71d91db27cab1fbb1e572eaa99424d26` |
| `manifest.json` | `14595f3541e44e386975a5f93507fb0f` |

ELF checks: `EM_AARCH64`, `PT_LOAD p_align 0x4000` (16 KB pages),
`DT_SONAME libapollon_imgu.so`, `DF_BIND_NOW`, `DF_1_NOW`.
