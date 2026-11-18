# Ore outlines for LeviLauncher

Target: **Minecraft Bedrock 1.26.62** in an offline world. This package includes a native Android **synthetic HUD test** and an opt-in **loaded Build ID check**, not a working ore X-ray. There are no Minecraft hooks, native game bindings, camera/caves controls, FullBright, or forced terrain rebuilds.

## Build the LeviPack

On the pull request, open the **Build ARM64 HUD test** check. After it passes, download the **ore-outline-hud-test-arm64** artifact, extract the outer ZIP, and use `ore-outline-hud-test.levipack`. The workflow is also manually runnable after it is on the default branch.

To build locally on Linux, use CMake 3.22+, Ninja, Git, Python 3, GNU `readelf`, and Android NDK **r28c**:

```sh
bash scripts/build-levipack.sh /path/to/android-ndk-r28c
```

Output: `build/android-arm64/ore-outline-hud-test.levipack`. The build fetches the pinned Preloader SDK for linking. The package contains only the root manifest and `libore_outline.so`; it uses the launcher's own `libpreloader.so` and `libc++_shared.so`. No Drive files, Minecraft APK/library, or installed game are needed to build. See [how BedrockTools builds and reads the world](docs/research/bedrocktools-integration.md).

## Offline HUD smoke test

Import the `.levipack` in LeviLauncher and enable the package. In an offline test session, open the Mod Menu:

1. **Ore Outlines HUD Test** should start off, with no boxes. Turn it on; expect a cyan box on the left and a yellow box on the right. They are not aligned to real blocks.
2. Switch **Synthetic Diamond** or **Synthetic Nether Gold** off individually. Only the other box should remain. With both off, or the test module off, no lines should remain.
3. After resize or rotation, toggle the test off and back on to sample the new HUD size. Disabling/unloading the package should clear the batch and remove the menu entry.

**Observed on device:** The user saw both boxes and confirmed the follow-up sequence: disabling Synthetic Diamond hid cyan without hiding gold, then disabling the HUD test cleared gold. Resize, package unload, and the gold filter independently have not been checked. The test toggle and filters are session settings; the test does not exercise world data or the game camera.

## Read-only loaded Build ID check

1. On [PR #1](https://github.com/NgawiNgab/Idfk-/pull/1), open the latest successful **Build ARM64 HUD test** run and download **ore-outline-hud-test-arm64**. Extract `ore-outline-hud-test.levipack`. This check requires package version **0.1.1** or later; the original 0.1.0 artifact does not have it.
2. Exit Minecraft. In LeviLauncher's mod manager, replace the old **Ore Outlines HUD Test** package with the new `.levipack` and enable it. Keep only one copy enabled.
3. Launch the same Minecraft installation and enter an offline world. In the Mod Menu, turn on **Minecraft Build ID Check**, then close the menu to read the HUD. **Ore Outlines HUD Test** can remain off.
4. Report the status and, if shown, both rows of hex digits (or a cropped screenshot of the text). **Build ID: MATCH** means the digits match the separate Drive sample: `c60fa32908019eecae39ebfe7bac5755dc35cde2`. **DIFFERENT**, **Minecraft library not loaded**, **Multiple Minecraft libraries**, or **Build ID unavailable** are useful results too; report the exact message. Turn the check off when done.

The switch takes one snapshot when enabled. It enumerates already loaded objects and examines bounded GNU Build ID notes inside readable mappings. No root, ADB, local file hash, cache reads, Zaphkiel, or log dump is needed. A match corroborates only the loaded object's reported build fingerprint; it does not show full-file equality or enable a block reader or camera binding.

## Host-only outline core

On Linux, run `make test` with GNU Make, a C++20 compiler, and GNU binutils. The tests use synthetic values and tiny shared-library fixtures; no game library, launcher, or Android toolchain is required.

- The core is **off** by default, with no categories selected. It matches exact `minecraft:` names for coal, copper, iron, gold, redstone, lapis, diamond, emerald, quartz, and ancient debris, including the relevant deepslate ores, nether gold, and lit redstone. It excludes crafted storage blocks and other non-ore targets. RGB values follow the ore entries in [Apollon's reference trace](docs/research/apollon.md#acquisition--classification--rendering). All 21 names occur as strings in the [Drive sample](docs/research/intake.md#binary-intake); that does not prove a device name accessor.
- A caller supplies a world epoch, dimension, inclusive bounds, and copied observations. No game object or borrowed string is retained. An unavailable read (`std::nullopt`), read of air, or replacement with a non-ore removes the old ore. Range and filter changes prune the cache; disabling or leaving/replacing the world clears it. The caller must budget scanning and revalidate positions. There is no Minecraft reader or scan scheduler yet.
- Projection clips box edges before the perspective divide, skips shared edges already drawn, and returns at most 1,024 lines by default (hard maximum 3,072 across 256 blocks). The HUD test uses a synthetic matrix and a smaller 24-line cap. The core does not infer the game's projection convention.

## Evidence and remaining work

[XRAY-008](docs/research/xray-008-evidence.md) matched all eight requested windows (340 decoded instructions) to the separate Drive sample. The lookup can mutate the region's chunk cache and uses branch-specific status checks. Camera position and matrix-copy paths are corroborated, but the active pass and viewport are not. The [port plan](docs/research/xray-port-1.26.62.md) records what still gates real ore outlines: verify the loaded game module, verify a non-generating block read with a valid lifetime, and obtain coherent main-pass camera data. No further static batch is pending.
