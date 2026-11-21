# Apollon ImGui + Bedrock engine work — results

Target: **Minecraft Bedrock 1.26.66.1, arm64-v8a**.
Mod: **Apollon Client ImGui v4.62** (source you supplied: `1.21.111` leak).

---

## 1. What was actually built and verified

| Deliverable | File | Verified |
|---|---|---|
| Native mod, aarch64 Android | `mods/apollon/out/libapollon_imgu.so` | ELF64/AArch64 DYN, 16 KB pages, BIND_NOW, no TEXTREL, `JNI_OnLoad` + 8 `Java_` exports |
| Launcher pack | `mods/apollon/out/Apollon_ImGui.levipack` | zip integrity OK |
| Package (per spec name) | `mods/apollon/out/Apollon_ImGui.apk` | zip integrity OK |
| Manifest | `mods/apollon/out/manifest.json` | `preload-native`, `minecraft_versions: ["1.26.66.1"]` |
| Port patch | `mods/apollon/out/PatchApollon.patch` | `patch --dry-run` applies cleanly (5 files) |
| Offset analysis + tool | `mods/apollon/out/OFFSETS.md`, `out/verify_offsets.py` | ran against the real 342 MB lib |
| Reproducible build | `mods/apollon/build/build_apollon_android.sh` | ran end-to-end, rebuilt the .so |
| Engine library, size-optimized | `out/libminecraftpe_optimized.so.xz` (59 MB, from 342 MB) | `xz -t` passes |
| Engine deploy helper | `out/install_optimized_lib.sh` | — |
| Source archive | `out/Apollon_ImGui_source.zip` (8.2 MB, 1408 files) | — |

### Toolchain (why it's not just "NDK build")

Zig 0.17 no longer ships an Android libc, and the NDK zip is not fetchable here.
The build therefore uses:
 * **real bionic r28 headers** (`gubaojian/sysroot` — includes `jni.h`, `GLES3`, `EGL`, `android/*`),
 * **libc++ headers** configured for Android (`_LIBCPP_HAS_MUSL_LIBC=0`, ns `__1`),
 * `-nostdlib` linking with explicit **DT_NEEDED** for `libGLESv2/EGL/log/android/z/dl/m/libc++_shared`,
 * static **libdobby.a** from the mod's own `libraries/arm64-v8a/`,
 * `-Wl,-z,max-page-size=16384` (Android 15 16 KB page requirement).

Result: all 19 sources compile and link, 236 dynamic symbols, all resolvable on a
real device (`gl*`, `egl*`, `AInputEvent_*`, `__android_log_*`,
`__gnu_strerror_r` all exist in bionic; `_Z*` resolve from `libc++_shared.so`).

---

## 2. The honest part: gameplay hooks do NOT work on 1.26.66.1

Apollon's `offsets.h` is a **1.21.x** table. I verified it against your real
library (md5 `fa377b0e07e7558b16d3684984598cc2`, contains string `1.26.66.01`):

```
Font = 0x263C304  ->  61 65 73 2d 31 32 38 2d   "aes-128-"
                      63 66 62 38 00 41 45 53   "cfb8.AES"
```

That is a string, not a float. `out/verify_offsets.py` classifies all 99 entries:
20 are structurally impossible; the rest only "look like code" because any
offset inside a contiguous `.text` decodes as *some* instruction.

Additional blockers for a blind rebase:
 * the library is fully **stripped** — `readelf -Ws` shows **0** exported game
   functions, so there are no symbols to re-anchor by;
 * `.text` is ~211 MB (vs ~150 MB in 1.21), i.e. code was reordered massively;
 * it links **`libpairipcore.so`** (Play Integrity virtualization) — editing
   `.text` on disk trips integrity checks. Runtime hooks (Dobby, which is
   linked in) are the viable path, but they still need correct addresses.

**What that means:** the ImGui overlay, menu, input, config persistence and all
UI code are intact and will run; modules that hook game functions will hook the
wrong addresses and either no-op or crash. This is stated in `OFFSETS.md` with a
concrete procedure (string-xref → function start, vtable/RTTI recovery, float
signature scan for leaf hooks) to finish the port in Ghidra.

I did **not** fabricate a "working" offset table. A wrong address that happens to
be inside `.text` will silently corrupt gameplay.

---

## 3. "Optimize the game engine"

Your `libminecraftpe.zip` from Drive is **Stored (0 %)** — the archive carries the
full 342,231,392 bytes.

| | original | this work |
|---|---|---|
| transport size | 342,231,392 B | **59,028,196 B** (`xz -6`, 5.8×) |
| ELF validity | ok | ok, byte-identical after `xz -d` |
| 16 KB pages | already `p_align=0x4000` | unchanged (already compliant) |
| code changes | — | **none** (pairip would reject them) |

Real AOT/strip work is impossible here: the binary is already stripped
(28 sections, no symtab, no debug), and `objcopy`/`zig objcopy --strip-*` both
return `unimplemented` on this aarch64 ELF. What is genuinely deliverable and
safe is the payload-size reduction above, plus the deploy script.

### On "varaddress"

Your earlier varaddress vectors `(3132,90206,10197)` / `(3001,90862,11013)`
describe a 3-axis offset scheme inside the world/mesh data, not ELF addresses.
They cannot be applied to a stripped, reordered 1.26.66.1 binary: the layout they
index is exactly what changed. I did not fake this either — see `OFFSETS.md`.

---

## 4. Repro

```bash
# one-time sysroot + libc++ setup (both public)
git clone --depth 1 https://github.com/gubaojian/sysroot /tmp/sysroot
# libc++ headers: copy zig's, add __config_site, or use any NDK r28 libc++

# build the mod
cd mods/apollon/build && ./build_apollon_android.sh

# pack
cd ../out && zip -9 Apollon_ImGui.levipack manifest.json libapollon_imgu.so
```

## 5. Next steps that would actually finish the port
1. Get an **unstripped or symbol-mapped** 1.26.66.1 lib (or the Arm64 `.dSYM`
   equivalent) — then rebasing is mechanical.
2. Otherwise: Ghidra pass per `OFFSETS.md`, ~99 hooks.
3. Only after offsets: enable individual modules behind flags so a bad offset
   costs one toggle, not a crash.
