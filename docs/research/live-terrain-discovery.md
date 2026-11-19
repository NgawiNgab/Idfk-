# Live passive discovery — Bedrock 1.26.62 ARM64

**Passed on device as a frozen report, not a live ore inventory.** Passive Terrain Probe **0.2.0** replaces the [two-point test](passive-terrain.md#device-result) with discovery: no target coordinates are configured. It reports examples of diamond and deepslate diamond keys returned by ordinary terrain builds. For discovery without traveling into fresh terrain, try the separate [Loaded Diamond Scan](loaded-diamond-scan.md).

## Run on the device

1. Download **passive-terrain-probe-arm64** from the successful **Build ARM64 HUD test** run on [PR #1](https://github.com/NgawiNgab/Idfk-/pull/1). Extract `passive-terrain-probe.levipack` from the downloaded ZIP. Check that the package is **0.2.0**, not the older fixed-coordinate probe.
2. Fully exit Minecraft. Replace the old **Passive Terrain Probe** in LeviLauncher; keep one copy enabled. Turn off the one-block behavior pack, other native mods, and memory editors. Use a disposable **local offline Overworld** on the same game installation. No ADB, log dump, or new Build ID report is needed.
3. Choose a short, fresh stretch of terrain around diamond depth (for example, Y ≈ −45). Note the render distance. In the Mod Menu, turn on **Passive Terrain Discovery** → **Start discovery**. Close the menu. Travel into new terrain for about 15–20 seconds so it builds normally. No ore coordinates are entered. If you want a controlled check, place your own diamonds in two different chunk columns before the run, then leave the area and approach it normally while recording. The old two points don't need repeating.
4. Before changing worlds, dimensions, render distance, or editing a sample, tap **Freeze and show report**. Close the menu and take a cropped screenshot of the report. It shows sampled columns/sections, sections with diamond keys, up to four example XYZs and a `Partial` warning if samples were dropped. If a sample is nearby, independently check the stated block at its coordinate **after** freeze (inspect it in Creative, or use `/testforblock` in the disposable world).
5. For the range check, change the render distance while stopped and use another fresh stretch of similar duration. Tap **Start discovery** again for a clean window. Get a second frozen report. Counts depend on what the engine actually rebuilds; a larger render distance needn't produce a larger count in every route. Turn the module off after the test.

**Stop conditions:** On an attach/restore warning, crash, or substantial hitching, turn the module off if possible, exit Minecraft, and report the warning or when it happened. Do not retry attachment in the same process after a detach warning. The probe is not automatically scoped to a world; don't leave it recording across a world change, dimension change, or resume. Do not use it in multiplayer.

## What to send back

A screenshot per pass plus its render distance is enough. If you checked example coordinates, note what block was there. Mention any hitching compared with traveling with the probe off. For a text report:

```text
Render distance: __ chunks
Frozen: __
Sampled: __ columns / __ sections
Diamond keys in __ sections
Example and verification: __
Partial warning or hitching: __
```

- A verified example at a coordinate **never supplied to the probe** is live discovery evidence. Examples in distinct chunk columns show it was not limited to the original section. More sampled columns on a longer or higher-distance pass are evidence of more ordinary visits, not a measurement of all resident chunks.
- `Frozen: no builder visits` means no qualifying calls were seen in the window. `Diamond keys in 0 sections` means no positive keys were retained. Neither result means there is no ore. `Partial` means the counts are lower bounds among sampled visits. Even without it, they are not an exhaustive scan.
- These are historical positives. A frozen example can remain in the report after you move away or replace the block. Do not interpret that as a stale-result detection pass. Native retention/removal, adoption, valid empty sections and projection are still gated by the [lifecycle work](render-section-lifecycle.md#next-validation-gate).

## Device result

**PASS, user-reported**, for the [0.2.0 ARM64 artifact](https://github.com/NgawiNgab/Idfk-/actions/runs/44446898996/artifacts/14493477499). The screenshot showed `Frozen: historical visits`, **328 sampled columns / 878 sampled sections**, **diamond keys in 133 sections**, and `Partial: worker contention`. It listed four deepslate diamond coordinates in four different chunk columns. One example was `(32, -32, 117)`; the screenshot at player position `(32, -32, 116)` showed a nearby exposed diamond vein. The user reported the probe worked, but that it required new terrain to discover the ores. Render distance and frame cost were not reported.

Contention means some visits were dropped, so the counts aren't an inventory. This confirms unannounced historical discovery in multiple columns, not stationary discovery, current membership, edit tracking, or projection. No further passive discovery test is needed before trying the [stationary scan](loaded-diamond-scan.md).

## What changed in the probe

- Same detour, caller, exact-build and code/RELRO checks, pin and detach policy as [the original probe](passive-terrain.md#attachment-and-lifetime). The original lookup runs exactly once; there are no extra engine queries, scheduling calls, executable patches, or world writes. Only the returned key and XYZ are copied at the already tested builder call site. The keys are the two [verified initializer pairs](xray-006-evidence.md#names-hashes-and-the-coordinate-fallback); other values, including fallback air, never count as positives.
- An ordinary visit at section-local `(0,0,0)` or a positive anywhere can contribute a section. This avoids a table operation for every cell, but clipped or skipped visits can omit sections. A bounded table holds at most 4,096 signed section XYZs and one first positive per section. A worker attempts the lock once, then at most 16 bucket checks; no worker allocation, waits, or HUD calls. Limited capacity, probe collisions, or lock contention are reported as partial. Examples favor different columns.
- Stop disarms before the slot is restored. Frozen reports synchronize only with already-running copies into the diagnostic's table, not with game workers finishing their builds. Starting again discards earlier samples and uses a new window token; callbacks that already took an old token can't populate it. The token is not a world epoch or a build ticket. No raw engine references are queued. HUD formatting and distinct-column counting are off the worker path.
- The [host tests](../../tests/terrain_capture_test.cpp) cover signed/stacked coordinates, unconfigured positives, duplicate visits, bounds and delayed writes across freeze/rearm. They do not replace the device trial. Performance is not yet measured on device. The synthetic HUD package, host lifecycle replay and existing-only scheduler are not fed from this report.
