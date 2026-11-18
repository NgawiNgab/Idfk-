# XRAY-003 — The earlier position predicate

**Question:** Does the call preceding the [XRAY-001 lookup](xray-001-evidence.md) establish that the requested world data is loaded?

## Scope and provenance

- **CONFIRMED (report):** The user-run `zaphkiel_out-3.txt` is 24,768 bytes. Its SHA-256 is `ea27754cfeab9a4102b04ae45ae321e1b567cc12bc09877d2253794916ba5505` (**report only**, not a library hash). Lines 18–23 show one target, `fn<0x10371d70>`. The agent did not run Zaphkiel.
- **CONFIRMED (cross-report observations):** The scanned path, file size, and segment/range metadata agree across the three reports (`zaphkiel_out-3.txt:3–15`). XRAY-003 lists `0x9004964` from the XRAY-001 call site (`zaphkiel_out-3.txt:517`; `zaphkiel_out.txt:127`) and reproduces the two-instruction accessor from [XRAY-002](xray-002-evidence.md) (`zaphkiel_out-3.txt:49–50`; `zaphkiel_out-2.txt:27–28`). These agreements are useful evidence about the scanned AArch64 code, not proof of identical binaries or of the library loaded by the game. Minecraft `1.26.62` remains user-reported; installed target binary identity is **UNVERIFIED**. No new Drive analysis is used here.

## What the function checks

| Classification | Evidence and interpretation |
| --- | --- |
| **CONFIRMED — indirect call first** | At `0x10371d7c–0x10371d84`, it reads the table pointer at `[x0]`, loads the entry at byte offset `0x10`, and calls it. The wrapper does not overwrite `x1` before the call; the shown callers pass an address for a position-like tuple there (`zaphkiel_out-3.txt:26–34`, `116–125`; `zaphkiel_out.txt:119–128`). The `x0=this` heuristic is not a complete argument contract or evidence of the receiver class. |
| **CONFIRMED — equality result** | At `0x10371d88–0x10371da8`, it dereferences the indirect call's result at `+0x68`, adds `0xc8` to the loaded address and reads a 64-bit word via `0x134ef05c`. It uses that helper again at the statically addressed `0x141f3000 + 0xf58 = 0x141f3f58`. `cmp x19, x0` and `cset w0, eq` return `1` for equality and `0` otherwise (`zaphkiel_out-3.txt:29–50`). There is no explicit null test of the indirect result before the field read. |
| **INFERRED — block-type-like test, not a loadedness gate** | The wrapper performs the same field-and-key comparison seen in XRAY-001/002, against a different static address than XRAY-001's `0x142231b8`. Distinct addresses do not prove distinct values. The field arithmetic fits the [BedrockTools name-object interpretation](xray-002-evidence.md#result) up to the key field, but is not evidence of a string or a verified block name. The predicate performs the indirect call **before** deciding equality. Its body cannot be used as an independent check that a position is available to read. What the indirect call itself does on missing data is still unknown. |
| **CONFIRMED — separate caller context** | In the shown caller, `0x8b9f9e4` calls this predicate. When false, `0x8b9f9f4` calls `0x10371db8` with the same receiver and tuple. The passed middle field is later decremented, with a separate comparison against a value returned by another indirect call (`zaphkiel_out-3.txt:116–147`). The first analysis also showed that the earlier predicate and subsequent lookup there take separately assembled tuples (`zaphkiel_out.txt:119–135`). Neither sequence establishes a reusable availability guard. |

**Decision:** Reject `0x10371d70` as a loaded-position precheck for the port. Its return reports equality of two stored values, not whether the lookup is safe. **UNKNOWN:** The target and lifetime contract of the indirect call, what block or state the fixed value denotes, the paired predicate's meaning, and actual unloaded/out-of-range behavior. A cached name hash remains a **HYPOTHESIS**, not a classifier. Finding a producer tied to known identifiers would address identity; a separate, corroborated availability check and offline validation would be needed for acquisition. Do not install a native binding from these findings.

## Next investigation — XRAY-004

The original `fn<0x10bab9e4>` probe became the first target in [XRAY-004-BATCH](xray-004-batch.md). [The batch report](xray-004-evidence.md) shows coordinate packing, inner indexing, and a shared fallback path.
