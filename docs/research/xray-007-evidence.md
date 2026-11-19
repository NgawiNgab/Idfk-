# XRAY-007 — What the coordinate fallback means

**“Failed lookup” is not a failed Zaphkiel run.** It means the game's normal block-at-position path did not read a block from the requested coordinate and took a fallback branch instead. Examples in the static code are a Y coordinate outside the lookup bounds, a null chunk-like result, or a missing first storage pointer (`zaphkiel_out.txt:39–55`, `69–84`; `zaphkiel_out-4.txt:39–56`; `zaphkiel_out-5.txt:794–802`). These are possible paths, not failures observed in the user's world. A valid read can also return air if the coordinate really contains air.

**Startup timing is not the explanation established here.** These `range<>` queries read code and string literals from the scanned library file; they do not ask a live world for block data or check when the game's registry becomes ready. XRAY-006's `fn<>` rejection at `0x134ef010` was a disassembly heuristic. XRAY-007 decoded the same address as code and printed block-name strings. Neither result measures live registry initialization or the time Zaphkiel started.

## Report and comparison scope

User-run `zaphkiel_out-7.txt`: 8,117 bytes, 215 logical lines; SHA-256 `79dea26a15745982cda14d7fed72ef945b53cd95661fd43f00086dba6485f18d` (**report only**). All eight sections match the [batch input](xray-007-batch/zInput.txt); the cache metadata matches XRAY-001–006 (`zaphkiel_out-7.txt:3–18`). All 157 printed instructions matched the same selected windows in the [separate Drive ELF](intake.md#binary-intake) after normalizing instruction formatting. No new download or agent-run Zaphkiel. This corroborates those windows, not the identity of the installed or loaded module.

## What the eight windows show

| Evidence in the scanned file | What it does and does not establish |
| --- | --- |
| Status and ownership (`zaphkiel_out-7.txt:30–63`, `79–81`) | `0x10bece70` returns a zero pair if the indirect result is null or a status byte is below `0x0b`. On the other path the count adjustment and cleanup match the holder pattern seen in [XRAY-006's sample check](xray-006-evidence.md#uniform-storage-and-a-possible-acquisition-gate). The status helper uses an acquire byte load. This does not name the status or identify a safe thread, caller context, or storage lifetime for the mod. |
| Air key and fallback setup (`zaphkiel_out-7.txt:87–97`, `103–126`) | The initializer passes `minecraft:air` and the corresponding key value for `0x141f3f58`. The writer resolves that key and stores the result in a holder at registry `+0x130`, the field read by the [coordinate fallback](xray-004-evidence.md). This corroborates the air-derived fallback in the earlier sample. It does not show what any coordinate returned during the run. |
| Ore names (`zaphkiel_out-7.txt:132–141`, `147–156`) | `minecraft:diamond_ore` and `minecraft:deepslate_diamond_ore` appear as distinct literal inputs to key setup, with distinct first words that match the [sample findings](xray-006-evidence.md#names-hashes-and-the-coordinate-fallback). These are names in code, not live block results or runtime block IDs. The sample's `minecraft:end_stone` initializer was not part of this batch. |
| Name-copy path and helper (`zaphkiel_out-7.txt:162–179`, `185–192`, `208–215`) | The copy path passes the key reached through a block-like input to the string accessor. `0x134ef010` tests the length in a tagged short/long representation; a `ret` ends this helper before the next entry. This is code that could test a key, **not** a log that the key was empty. No string-layout binding is established. |

**Port implication:** Keep `unavailable` distinct from a successfully read `minecraft:air` block. Installed identity, usable world context and thread/ownership, camera sampling, and controlled offline validation are still open. No startup diagnosis, config change, runtime probe, or additional Zaphkiel run is required to answer this question.
