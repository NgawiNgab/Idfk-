# One-block offline world probe

**Purpose:** Establish the on-device reference observations for one position before enabling any private native reader. This is a separate Minecraft behavior pack, not a replacement for the LeviPack, and not evidence that a native lookup or camera binding is safe. The existing HUD/Build ID LeviPack is unchanged.

## Why this path

The [loaded Build ID](intake.md#loaded-build-id) matches. The [native lookup](xray-008-evidence.md#what-the-report-establishes) can still update its chunk cache and branch to a producer whose loading behavior is unverified. The pinned BedrockTools [`BlockSource` wrapper](https://github.com/NgawiNgab/BedrockTools/blob/c03d3ae0c9022557de0acdf0d94e373e0b9e0579/include/bedrocktools/sdk/world/BlockSource.hpp#L8-L13) does not expose an existing-only read. Do not call those offsets in this probe.

The [stable `@minecraft/server` 2.3.0 declarations](https://registry.npmjs.org/@minecraft/server/-/server-2.3.0.tgz) define `Dimension.isChunkLoaded(location)` as loaded **and valid for scripting** (`index.d.ts:6345–6353`). `Dimension.getBlock(location)` may return `undefined` for an unloaded chunk or throw for invalid access (`6082–6105`). The pack checks the gate and copies `Block.typeId` in one synchronous `/scriptevent` after-event callback. A false, missing, or failed gate never calls the block reader; a failed read is unavailable, never air. It does not store block or dimension objects, loop, set blocks, request a ticking area, or call the camera. Only the local test command triggers the check. The player-count guard is not proof the world is offline; the operator must use a local world.

## Run on the device

1. Download **one-block-world-probe** from the latest PR build's artifacts and extract the outer ZIP. In LeviLauncher, select the same Minecraft installation/storage and use **Content Management → Import Content** to select **`ore-outline-one-block-probe.mcpack`**. It should appear under Behavior Packs. Do not import it in the native Mod Manager or unpack the `.mcpack` itself.
2. Make a **new disposable Creative Overworld** with **Multiplayer off** and **Cheats on**. In **Behavior Packs → My Packs**, activate **Ore Outlines One-Block Probe** for that world only. No experimental toggle or new LeviPack is needed. If import or activation fails, report that message rather than changing the game build.
3. Enter the world and run these in chat **one at a time**. Stay in the `0–15` X/Z chunk after teleporting. Creative flight is fine. The `/setblock` commands deliberately change only the test position; the probe itself does not edit the world.

```text
/tp @s 2.5 105 2.5
/setblock 0 100 0 minecraft:diamond_ore
/scriptevent idfk:one_block_probe sample
/setblock 0 100 0 minecraft:stone
/scriptevent idfk:one_block_probe sample
/setblock 0 100 0 minecraft:air
/scriptevent idfk:one_block_probe sample
```

4. Send the three `[One Block] 0 100 0:` lines and one `[One Block] Control:` line. Expected:

```text
[One Block] 0 100 0: minecraft:diamond_ore; chunk=loaded; getBlock=1
[One Block] 0 100 0: minecraft:stone; chunk=loaded; getBlock=1
[One Block] 0 100 0: minecraft:air; chunk=loaded; getBlock=1
[One Block] Control: chunk=unloaded; getBlock=0
```

The control checks `(524288, 100, 524288)` but **never calls `getBlock` there**, even if unexpectedly loaded. Do not teleport there, set a block there, or add a ticking area. If it says `loaded` or `unknown`, the unloaded case is not confirmed; send the exact output. If `/scriptevent` produces no `[One Block]` output, first confirm the behavior pack is active in that world and the command was entered in player chat.

## Device result

**PASS, user-reported.** At `(0, 100, 0)` the probe returned the following in order. Each invocation also printed `Control: chunk=unloaded; getBlock=0`.

| Observed `Block.typeId` | Probe status | Expected result in the native outline core |
| --- | --- | --- |
| `minecraft:diamond_ore` | `chunk=loaded; getBlock=1` | `Ore / Diamond`; track it when enabled and selected. |
| `minecraft:stone` | `chunk=loaded; getBlock=1` | `Other`; remove any old outline there. |
| `minecraft:air` | `chunk=loaded; getBlock=1` | `Air`; remove any old outline there. |
| No value at the distant control | `chunk=unloaded; getBlock=0` | `Unavailable`, not air; no lookup or outline. |

This is a reference result for the native reader, not a data bridge. The [C++ core](../../README.md#host-only-outline-core) already handles these copied names and `std::nullopt` in host tests. An eventual native diagnostic must independently reproduce the results in the same offline world, with the pack disabled, on a verified update path and without scanning or using the camera. An air-like native fallback cannot stand in for the loaded-air result.

## Build and evidence limits

`make test-world-probe world-probe` runs Node fixture tests and packages the root manifest and two scripts as `build/world-probe/ore-outline-one-block-probe.mcpack`. The CI job uploads it separately from the ARM64 LeviPack. No game, client assets, or native libraries go into this behavior pack.

The tests cover the script's call counts, guard behavior, and formatting with fixtures. The device output establishes the reference values **through the supported Script API**. The counter is the script's `getBlock` call count, not instrumentation of engine generation or cache internals. This does not certify an independent native loadedness check, object lifetime, or game-to-HUD projection.
