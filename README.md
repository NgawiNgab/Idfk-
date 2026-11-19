# Ore outlines for LeviLauncher

Target: **Minecraft Bedrock 1.26.62** in an offline world. The main LeviPack is a **synthetic HUD test** and **loaded Build ID check**, not working ore outlines. A separate [Passive Terrain Probe](#passive-terrain-probe) reports diamond samples from normal terrain builds. The [Loaded Diamond Scan](#loaded-diamond-scan) behavior pack tests continuous discovery in nearby, already-loaded terrain without using the renderer. None of the packages changes terrain or forces rebuilds.

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

The [user-reported loaded Build ID](docs/research/intake.md#loaded-build-id) matched the Drive sample. No game bindings are enabled.

## Passive terrain probe

**Try 0.2.0 on-device:** Use the separate **passive-terrain-probe-arm64** artifact from the [PR check](https://github.com/NgawiNgab/Idfk-/pull/1) and follow the [live discovery runbook](docs/research/live-terrain-discovery.md#run-on-the-device). Replace the old probe. In a disposable offline world, tap **Start discovery**, travel into fresh terrain, then **Freeze and show report**. The probe reports diamond samples at coordinates you did not enter, with sampled column/section counts. It forwards the builder's existing reads unchanged. This is a frozen report, not a live ore map or outline overlay.

The [0.2.0 phone screenshot](docs/research/live-terrain-discovery.md#device-result) showed 328 sampled columns, 878 sampled sections and diamond keys in 133 sections, with partial worker contention. The [original 0.1.0 device run](docs/research/passive-terrain.md#device-result) saw both the exposed and stone-enclosed markers. Neither needs repeating. The [renderer lifecycle trace](docs/research/render-section-lifecycle.md) and host replay model adoption and removal, but those events aren't wired to Minecraft.

To include the diagnostic in a local ARM64 build, set `BUILD_TERRAIN_DIAGNOSTIC=1` when running `scripts/build-levipack.sh`. This also produces `build/android-arm64/passive-terrain-probe.levipack`.

## Loaded Diamond Scan

**Try discovery while staying still.** Import the **loaded-diamond-scan** build artifact's `.mcpack` through LeviLauncher's Content Management, not the Mod Manager. In a disposable offline Overworld with cheats on, activate **Loaded Diamond Scan**, let the terrain settle, then enter `/scriptevent idfk:loaded_diamonds start`. The pack sweeps a small area around the player on script ticks, shows sampled diamonds in the action bar, and repeats without needing a new terrain build. Use `status` in place of `start` for a coverage report, or `stop` to stop. See the [stationary test runbook](docs/research/loaded-diamond-scan.md#run-on-the-device).

This is a report-only behavior pack using the world Script API, not a client-side outline or a new native reader. It targets the current chunk and its eight neighbors, up to 32 Y levels, rather than every chunk in render distance.

## One-block offline test

The [one-block world probe](docs/research/one-block-world-probe.md#run-on-the-device) is a separate `.mcpack` test fixture, uploaded by CI as **one-block-world-probe**. The [user-reported device run passed](docs/research/one-block-world-probe.md#device-result): diamond ore → stone → air at the same loaded position, plus an unloaded guard-only control. This is a reference for testing a future native reader, not a LeviPack dependency or validation of native world or camera access.

## Host-only outline core

On Linux, run `make test` with GNU Make, a C++20 compiler, and GNU binutils. The tests use synthetic values and tiny shared-library fixtures; no game library, launcher, or Android toolchain is required.

- The core is **off** by default, with no categories selected. It matches exact `minecraft:` names for coal, copper, iron, gold, redstone, lapis, diamond, emerald, quartz, and ancient debris, including the relevant deepslate ores, nether gold, and lit redstone. It excludes crafted storage blocks and other non-ore targets. RGB values follow the ore entries in [Apollon's reference trace](docs/research/apollon.md#acquisition--classification--rendering). All 21 names occur as strings in the [Drive sample](docs/research/intake.md#binary-intake); that does not prove a device name accessor.
- A caller supplies a world epoch, dimension, inclusive bounds, and copied observations. No game object or borrowed string is retained. An unavailable read (`std::nullopt`), read of air, or replacement with a non-ore removes the old ore. Range and filter changes prune the cache; disabling or leaving/replacing the world clears it. The native game reader that would supply those observations is not wired yet.
- Projection clips box edges before the perspective divide, skips shared edges already drawn, and returns at most 1,024 lines by default (hard maximum 3,072 across 256 blocks). The HUD test uses a synthetic matrix and a smaller 24-line cap. The core does not infer the game's projection convention.

## Native chunk scheduler (fixture-tested)

The [C++ scheduler](docs/research/chunk-scanner.md) takes render-ready section coordinates, derives its own block positions, and scans the nearby sections with a per-update budget. Host tests discover ores across two adjacent chunks without supplying ore coordinates, skip non-ready/out-of-range sections, and clear on revision, range and world changes. It is compiled in the ARM64 target, but the LeviPack does not run it against Minecraft. No new install is needed for this milestone.

## Evidence and remaining work

[XRAY-008](docs/research/xray-008-evidence.md) matched all eight requested windows (340 decoded instructions) to the separate Drive sample. The lookup can mutate the region's chunk cache and uses branch-specific status checks, which is why the passive probe observes only the game's existing calls. The [port plan](docs/research/xray-port-1.26.62.md) tracks the remaining gates: native coverage, world and section lifetime, stale-result removal, and coherent main-pass projection at a measured cost. No further static batch is pending.
