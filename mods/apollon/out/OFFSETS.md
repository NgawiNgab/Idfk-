# Offset status: Apollon v4.62 vs Minecraft Bedrock 1.26.66.1

## Verdict: the shipped offset table is NOT valid for 1.26.66.1

Apollon v4.62 was built for **1.21.111** (its README says 1.21.50; the leak
name says 1.21.111). The `offsets.h` in this source tree is a 1.21.x table.

I validated every entry against the actual `libminecraftpe.so` you supplied
(md5 `fa377b0e07e7558b16d3684984598cc2`, contains version string
`1.26.66.01`, build-id `e96ddd68541814923c35460f8b99f27a66c51232`).

### Evidence

The very first entry, `Font = 0x263C304`, does not point at a float at all:

    0x0263C304: 61 65 73 2d 31 32 38 2d   "aes-128-"
    0x0263C30C: 63 66 62 38 00 41 45 53   "cfb8.AES"

That is the string table for `aes-128-cfb8`, not `BaseOptions` font scale.

`out/verify_offsets.py` classifies all 99 entries. Results:
 * 20 entries are structurally impossible (zero words, out-of-LOAD-Range,
   or ASCII where code/float is expected).
 * The other 79 "look like instructions" only because AArch64 `.text` is
   contiguous: any 4-byte-aligned byte offset inside `.text` decodes as
   *some* instruction. That is not evidence the entry is the right function.
 * Several `Offsets64` pointer/vtable entries (`0x1D0`, `0x208`, `0x9C0`,
   `0xC0`, ...) are far smaller than the library base and land in the ELF
   header / first LOAD page, i.e. they are **struct layout constants**
   (member offsets inside classes), not addresses. That means those hooks
   are not "rebased" at all; their ABI layout is version-specific too.

### Why it cannot simply be "re-based"

Between 1.21.x and 1.26.66.1 Mojang:
 * changed the pack-file object layout (symbols are fully stripped; no
   dynsym entries survive for game classes: `readelf -Ws` shows 0 exported
   T/W functions);
 * turned on `libpairipcore.so` (Play Integrity Virtualization) in this
   build - the DEX/native integrity wrapper. Byte patching `.text` in a
   pairip build trips the integrity check;
 * re-compiled with a different clang and reordered `.text`
   (211 MB of code vs ~150 MB for 1.21 builds).

So *every* static address in the 1.21 table is wrong by an unknown delta,
and struct offsets differ too. A correct port needs **fresh AOB / xref
derivation against 1.26.66.1**, which is a reverse-engineering job
(Ghidra/IDA), not a relink.

## What this repo actually delivers

| Item | Status |
|---|---|
| Mod `.so` builds & links for Android aarch64 | **done** (bionic r28 headers, libc++ ABI, 16 KB pages, BIND_NOW) |
| ImGui overlay + menu + config UI | **done** (compiles, all UI code intact) |
| Dobby inline hooks wired | **done** (libdobby.a linked, `DobbyHook` defined) |
| Gameplay hooks active on 1.26.66.1 | **NO** - offsets stale; hooks install at wrong addresses |
| Game library "optimized" | **repacked/compressed 342MB -> 59MB**, 16 KB aligned, no data change |

## How to complete the port (the real work, on a desktop with Ghidra)

For each entry name in `offsets.h`, on an *unpacked* 1.26.66.1 lib:

1. **String xref** - most hooks have a unique nearby string:
   `font`, `hud_screen`, `chat_screen`, `random.pop`, `random.break`,
   `NetherNet`, `_getFogDensitySettingType`, `setupFog`, `getMaxPlayers`.
   Find string -> xrefs -> containing function -> function start = new offset.
2. **Vtable xref** - for `Offsets64` members that are struct fields
   (0x1D0, 0x208, ...), locate the class vtable by RTTI-free heuristics
   (the class name often survives in `.rodata`), then re-measure the field
   displacement.
3. **Signature scan** - for leaf hooks (`Gravity` returns -0.08f,
   `JumpHack` returns 0.42f, `LavaDrown`, `Noclip`) search `.text` for the
   `fmov`/`str` immediate pattern; the `verify_offsets.py` "float" mode can
   be extended with a `--scan` to find all `-0.08` constants.
4. **Pairip** - if the device build keeps `libpairipcore.so` loaded, prefer
   *runtime* hooks (Dobby) over `MemoryPatch` byte edits, and do not touch
   `.text` of `libminecraftpe.so` at rest: patch the *loaded*, post-decrypt
   copy. Apollon's `MemoryPatch` writes via `/proc/self/mem`, which is
   already the safe path - but page permissions post-pairip may be RX, so
   `mprotect` may be required (Apollon does this in `KittyMemory`).

## Deliverables in this folder

 * `libapollon_imgu.so` - aarch64 Android native mod, ImGui v4.62 UI.
 * `manifest.json` - LeviLaunchroid `preload-native` manifest.
 * `Apollon_ImGui.apk` - zip data package (per spec name; not an APK install).
 * `Apollon_ImGui.levipack` - importable launcher pack.
 * `OFFSETS.md` - this file.
