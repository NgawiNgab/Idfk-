# Cyan diamond tracer — Loaded Diamond Scan 0.2.0

**First-person, offline Overworld only.** This replaces 0.1.0's constantly changing action-bar coordinates with a dotted cyan line ending in a `+` at the center of one discovered diamond ore block. Discovery is the same bounded loaded-chunk sweep that [worked on the phone](loaded-diamond-scan.md#device-result). For the separate native implementation, see the [LeviPack plan and phone checkpoints](native-diamond-tracer.md).

## Try it on the phone

1. Download **loaded-diamond-scan** from [PR #1's build](https://github.com/NgawiNgab/Idfk-/pull/1) and extract `loaded-diamond-scan.mcpack`. Exit the world, import it via LeviLauncher **Content Management → Import Content**, and make sure **Loaded Diamond Scan 0.2.0** is the enabled world behavior pack. Disable the old 0.1.0 pack and other probes. Use the same disposable local Overworld with Multiplayer off and Cheats on. No Beta APIs experiment or native mod is required.
2. In first person near diamond depth, run `/scriptevent idfk:loaded_diamonds start`. Wait one sweep (up to 72 simulation ticks, about four seconds at normal speed) and look around. The line approaches the target from just below the center of the view. It won't draw if the target is behind you. Coordinates are available only on demand through `/scriptevent idfk:loaded_diamonds status`.
3. With stone between you and the target, look toward it and move a little side to side. Look for the same endpoint without the old rapid switching. If you can reach the targeted ore, mine it: the line should clear or point to another known diamond within a few ticks. No need to plant markers or load new chunks. Run `/scriptevent idfk:loaded_diamonds stop` to check that the tracer clears.

Send a screenshot or short clip and report any hitching. If the line doesn't appear after turning around, include the exact `status` output. `Tracer: locked on …` means a point read validated a block, not that the device displayed the markers. `Tracer: unavailable` means rendering raised an error; the scan itself can still be queried. Stop if it hitches badly.

## Device result

**User-reported on 0.2.0.** The cyan tracer was visible through stone, moved with the player's view, and disappeared when they mined its target. This confirms the Script API tracer in that trial, not native HUD projection. Stop, world exit, dimension changes, and frame cost were not reported.

## Selection and drawing

- The scan still visits one `16 × 4 × 16` slice of a loaded chunk per active tick. The range is still the surrounding 3 × 3 chunk columns and up to 32 levels around the player. The old cached nearest examples are only candidates, not proof of a current block.
- Keep the previous target even if it is no longer the closest example in its slice. A new candidate must be **more than three blocks closer** to switch while the old one is valid. Every six ticks, recheck the locked/candidate types with at most two guarded `getBlock` attempts in total. Both diamond types are eligible; stone, air, failed reads and missing chunks are not. Leaving the window drops the old line without looking the block up.
- The endpoint is `(x + 0.5, y + 0.5, z + 0.5)`. The start follows the player's head and look direction, offset below the forward axis. The intervening markers are spaced for perspective and placed in actual world space. This is not a guessed projection to HUD pixels; third-person and custom cameras aren't aligned to the player's head and aren't supported by this trial.
- The [stable 2.11.0 declarations](https://registry.npmjs.org/@minecraft/server/-/server-2.11.0.tgz) expose `world.primitiveShapesManager` and [`TextPrimitive.depthTest`](https://learn.microsoft.com/en-us/minecraft/creator/scriptapi/minecraft/server/textprimitive?view=minecraft-bedrock-stable#depthtest). Up to 12 cyan glyphs, with no background plate and depth testing off, form the line so it can be seen through terrain. These are world-space text primitives, not solid debug lines, particles or an ore mesh change.
- Reuse the same marker handles when moving; update at most every two script ticks, except for an immediate redraw on a target change. Only show to the local player in their Overworld. Remove owned markers on stop, world/dimension loss, or a draw failure; a one-second lease is a cleanup backstop. Never clear unrelated shapes. No chunk generation, terrain rebuild, block writes, or game offsets are involved.

`make test-world-probe loaded-diamond-scan` exercises target hysteresis, loadedness, edit replacement, placement math, marker reuse and teardown, then packages the `.mcpack`. Neither these fixtures nor the device result certify a standalone LeviPack's camera or block reader.
