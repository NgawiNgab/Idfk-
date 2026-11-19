# Loaded Diamond Scan — stationary device trial

**Standalone behavior pack, not a LeviPack.** The scan searches nearby chunks that are already loaded for scripting in a local Overworld, even when you stand still. The [coordinate-only 0.1.0 trial](#device-result) worked on the phone. It does not change blocks, request chunk loads, set a ticking area or force a mesh rebuild. The [passive LeviPack](live-terrain-discovery.md#device-result) is unchanged.

## Run on the device

For **0.2.0**, follow the [cyan tracer runbook](diamond-tracer.md#try-it-on-the-phone). The scan commands are unchanged; the nearest-coordinate action bar is no longer used.

## Device result

**PASS, user-reported**, for [0.1.0](https://github.com/NgawiNgab/Idfk-/actions/runs/44449548642/artifacts/14494273464). Two screenshots showed deepslate diamond ore at the reported nearby positions `(5, -45, 96)` and `(-10, -19, 101)`, with samples in six and nine chunk columns, respectively. The chat report showed **72/72 slices** sampled in **nine loaded columns**, Y **−64 to −33**, two sweeps in that window and **4,243 queries, zero failures** since start. The user reported that nearest coordinates switched while moving. Mining/removal timing and frame cost were not reported. The screenshots do not test 0.2.0's tracer.

## Why this works without moving

The [rechecked Apollon archive](apollon.md#acquisition--classification--rendering) discovers new ores through its render-layer hook. The master toggle can request a rebuild of existing render geometry; the tick loop itself only rechecks previously found locations. Rather than port a private rebuild call or the unverified native block lookup, this trial calls the documented Script API on simulation ticks. It starts from coordinate ranges around the player, not from terrain visits or known ore positions.

- **Range:** The current chunk column and its eight neighbors, up to 32 Y levels around the player's four-block Y band. The window follows the player and clips at Overworld build limits. This is not a scan of the entire render distance.
- **Budget:** Visit one `16 × 4 × 16` slice per active tick. `Dimension.isChunkLoaded()` must return true before `Dimension.getBlocks()` requests a diamond/deepslate-diamond filter in that single chunk, with `allowUnloadedChunks=false`. At most one query covering 1,024 cells per tick and 72 slice visits per stationary sweep. The bounds limit work requested, not engine milliseconds; device cost is unmeasured.
- **Refresh:** Repeat the sweep, including slices that had no matches. A successful re-read replaces the slice sample. If the gate fails, the result is unavailable, not air; a failed volume read discards the affected slice. Dropped chunks are also checked before display. Leaving the window or aging out removes samples; stop, leaving the world, a dimension change, or detecting another player discards the session. The one-player guard is not proof the world is offline: use a local world with Multiplayer off.
- **Report:** Keep at most one copied example per slice, looking at up to 32 matches plus one to detect a dense result. `status` shows up to four examples in distinct columns, coverage for the **latest** samples, and volume query failures. These are sampled at different ticks, not an instantaneous inventory. A missing sample is not proof that the unscanned or unavailable area has no ore. Neither the Script API loaded gate nor the report proves client render-ready membership. The tracer verifies the exact block separately before pointing at it.

The stable [`@minecraft/server` declarations](https://registry.npmjs.org/@minecraft/server/-/server-2.11.0.tgz) document `Dimension.getBlocks`, `isChunkLoaded`, and the match iterator. Version 0.1.0 used Script API 2.3.0 for scanning; 0.2.0 uses 2.11.0 for the same scan calls plus [`TextPrimitive`](diamond-tracer.md#selection-and-drawing). `make test-world-probe loaded-diamond-scan` builds `build/world-probe/loaded-diamond-scan.mcpack`. A standalone LeviPack still needs a verified existing-only reader, owning update path, and coherent native projection.
