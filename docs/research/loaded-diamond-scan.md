# Loaded Diamond Scan — stationary device trial

**New standalone behavior pack, not a LeviPack.** This trial searches nearby chunks that are already loaded for scripting in a local Overworld, even when you stand still. It refreshes copied diamond coordinates in the action bar and chat. It does not change blocks, request chunk loads, set a ticking area, force a mesh rebuild, or draw outlines. The previously tested [passive LeviPack](live-terrain-discovery.md#device-result) is unchanged.

## Run on the device

1. Download **loaded-diamond-scan** from the [PR build](https://github.com/NgawiNgab/Idfk-/pull/1). Extract `loaded-diamond-scan.mcpack` from the artifact ZIP. Import the `.mcpack` via LeviLauncher **Content Management → Import Content**, not the native Mod Manager.
2. Use a disposable copy of a local **Overworld**, with **Multiplayer off** and **Cheats on**. Activate **Loaded Diamond Scan** in the world's Behavior Packs. Keep the passive native probe and one-block pack off for this trial. The package requests stable Script API 2.3.0, as in the [one-block probe](one-block-world-probe.md); no experiments are required by the manifest.
3. Go to an existing cave near diamond depth, then stay put and let the area settle before starting. Do **not** travel into fresh terrain or change render distance after starting. Enter in chat:

   ```text
   /scriptevent idfk:loaded_diamonds start
   ```

   Close chat. The action bar reports progress or a diamond coordinate and the number of chunks with samples. Run the following after one sweep (about 4 seconds at normal tick rate; wait longer if the game is behind). No ore coordinates need to be supplied.

   ```text
   /scriptevent idfk:loaded_diamonds status
   ```

4. Send the status screenshot and whether scanning caused hitching. If a reported block is within reach, check and mine it without leaving the scan window. After another sweep, check that its coordinate is no longer shown. The number of positive chunks can stay the same if another ore is found in that chunk. You do not need to plant any blocks to do the stationary discovery test.
5. Stop with `/scriptevent idfk:loaded_diamonds stop`. If the script produces no output, check that the pack is active in this world and send any import or activation error. If it hitches substantially, stop it or leave the world.

## Why this works without moving

The [rechecked Apollon archive](apollon.md#acquisition--classification--rendering) discovers new ores through its render-layer hook. The master toggle can request a rebuild of existing render geometry; the tick loop itself only rechecks previously found locations. Rather than port a private rebuild call or the unverified native block lookup, this trial calls the documented Script API on simulation ticks. It starts from coordinate ranges around the player, not from terrain visits or known ore positions.

- **Range:** The current chunk column and its eight neighbors, up to 32 Y levels around the player's four-block Y band. The window follows the player and clips at Overworld build limits. This is not a scan of the entire render distance.
- **Budget:** Visit one `16 × 4 × 16` slice per active tick. `Dimension.isChunkLoaded()` must return true before `Dimension.getBlocks()` requests a diamond/deepslate-diamond filter in that single chunk, with `allowUnloadedChunks=false`. At most one query covering 1,024 cells per tick and 72 slice visits per stationary sweep. The bounds limit work requested, not engine milliseconds; device cost is unmeasured.
- **Refresh:** Repeat the sweep, including slices that had no matches. A successful re-read replaces the slice sample. If the gate fails, the result is unavailable, not air; a failed volume read discards the affected slice. Dropped chunks are also checked before display. Leaving the window or aging out removes samples; stop, leaving the world, a dimension change, or detecting another player discards the session. The one-player guard is not proof the world is offline: use a local world with Multiplayer off.
- **Report:** Keep at most one copied example per slice, looking at up to 32 matches plus one to detect a dense result. The action bar refreshes every ten ticks; `status` shows up to four examples in distinct columns, coverage for the **latest** samples, and query failures. These are sampled at different ticks, not an instantaneous inventory. A missing sample is not proof that the unscanned or unavailable area has no ore. Neither the Script API loaded gate nor the report proves client render-ready membership.

The stable [`@minecraft/server` 2.3.0 declarations](https://registry.npmjs.org/@minecraft/server/-/server-2.3.0.tgz) document `Dimension.getBlocks` and its unloaded-volume behavior (`index.d.ts:6145–6170`), `isChunkLoaded` (`6345–6353`), and the match iterator (`3868–3885`, `4666–4674`). `make test-world-probe loaded-diamond-scan` runs the fixtures and builds `build/world-probe/loaded-diamond-scan.mcpack`. No new native binding is used; a standalone LeviPack still needs a verified existing-only world reader and an owning update path, and real outlines need coherent projection. This pack has not yet run on the phone.
