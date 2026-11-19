# XRAY-004 — Coordinate packing, inner reader, and fallback

**Question:** What does the lookup read after obtaining a non-null object, and can its result distinguish missing data from a block?

## Scope

- **CONFIRMED (report):** User-run `zaphkiel_out-4.txt`, 291,967 bytes; SHA-256 `8a7761125f4ca72aba20553ba87a507b7d5b643374becd0a631a5634e78ac69d` (**report only**). The header reports eight targets, in the order of the [original input](xray-004-batch/zInput.txt). Seven have direct disassembly, and the position transform appears as a callee despite a direct-query warning. The agent did not run Zaphkiel or use new Drive binary evidence.
- **CONFIRMED (cross-report scope):** The launcher-cache path, size, and segment metadata match XRAY-001 through XRAY-003 (`zaphkiel_out-4.txt:3–18`). These are findings about scanned AArch64 code. The Minecraft `1.26.62` version is user-reported; installed target binary identity is **UNVERIFIED**. Matching report metadata does not establish the loaded module or byte equivalence to Drive.

## Non-null read path

**CONFIRMED — position transform.** The direct `fn<0x11f0e5e8>` query says “looks like string data” for bytes `28 00 40 39` (`zaphkiel_out-4.txt:1511–1513`). Those bytes are the little-endian AArch64 word `0x39400028`, which decodes as `ldrb w8, [x1]`. The callee section for `0x10373a68` shows that same entry instruction and a complete straight-line body through `ret` (`zaphkiel_out-4.txt:5441–5447`, `5460–5474`). It writes:

| Output position | Instruction-level derivation |
| --- | --- |
| Byte at `+0` | Input byte at `+0`, masked with `0xf`. |
| Byte at `+1` | Input byte at `+8`, masked with `0xf`. |
| Halfword at `+2` | Low 16 bits of `(input halfword at +4) - w2`. |

**INFERRED — local block coordinates.** [XRAY-001](xray-001-evidence.md) shifts the first and third position fields right by four for its initial indirect lookup, bounds-checks the middle field, and passes the lower-bound halfword in `w2` to the transform (`zaphkiel_out.txt:39–63`, `99–105`). Together, the reports support local horizontal coordinates in `[0, 15]` and a vertical coordinate relative to that lower bound. This does not establish a portable C++ type or authorize reusing the reported offsets.

**CONFIRMED — inner indexing and delegation.** The entry at `0x10bab9e4` loads two values from `x0+0x180`, uses their difference in a count-like calculation, and derives a record index from the sign-extended `+2` halfword shifted right by four. When the unsigned index comparison succeeds, it selects a record at `begin + index × 0x68` and calls `0x10b70a74` with `w1 = 0`. If the tested bit is clear, it tail-calls `0x10b69120` with that record and index `((byte0 << 8) | (halfword & 15)) + (byte1 << 4)` (`zaphkiel_out-4.txt:30–66`). With the observed 4-bit horizontal inputs, this index is within `0..4095`.

**INFERRED — subchunk-like records, not a verified palette format.** The difference is shifted by three and multiplied by `0x4ec4ec4ec4ec4ec5`, the inverse of 13 modulo `2^64`. For a nonnegative difference that is a multiple of `0x68`, this gives the count of `0x68`-byte entries. The 16-position vertical grouping, record stride, and 12-bit local index fit a reader over `16 × 16 × 16` cells. The report stops short of the final storage read. The `0x10b70a74` excerpt shows a body for selector `2`, but the reader supplies `0`, which branches to the unshown `0x10b70a94` (`zaphkiel_out-4.txt:43–49`, `686–696`). Neither the gate's meaning nor the palette/entry representation is established. The printed block after `0x10baba84` starts a new prologue at `0x10baba88` after both shown entry paths tail-branch; do not attribute that later code's writes to the reader (`zaphkiel_out-4.txt:50–75`).

## Missing-data result

**CONFIRMED — shared fallback mechanism.** An out-of-range inner index or a set gate bit reaches the same helper chain as the bounds-failure and null-result paths of the outer lookup (`zaphkiel_out-4.txt:39–56`; `zaphkiel_out.txt:69–84`). The supplied bodies show:

| Target | Observed behavior |
| --- | --- |
| `0x10cc9f90` | Returns the statically addressed `0x14208000 + 0x858 = 0x14208858` in report coordinates (`zaphkiel_out-4.txt:1518–1523`). |
| `0x10cdad5c` | Reads `[x0+0x130]` and returns it when non-null; the zero branch at `0x10cdad68` is not shown (`zaphkiel_out-4.txt:2770–2776`). |
| `0x1091a034` | Returns the 64-bit value at `[x0+0x200]`. The caller dereferences the preceding helper's return once before passing it here (`zaphkiel_out-4.txt:50–56`, `3181–3186`). |

**Decision:** Do not treat a non-null result as proof that storage was available. The shown failure paths return a shared field-derived value rather than directly returning a missing flag. Its actual value, its initialization, and whether a valid block can produce the same value are **UNKNOWN**. In particular this is not proof of air. [BedrockTools Light Overlay](https://github.com/NgawiNgab/BedrockTools/blob/c03d3ae0c9022557de0acdf0d94e373e0b9e0579/src/modules/visual/lightoverlay.cpp#L291-L304) treats an out-of-range result as an air sentinel; the new report explains why it is a fallback candidate, not why the heuristic would be valid in the intended installation.

## Context and remaining gates

| Finding | What the report establishes, and what it does not |
| --- | --- |
| **CONFIRMED — sibling predicate sequence** | `0x10371db8` dispatches indirectly via the `+0x10` table entry before any test, then loads the result's `+0x68` field. `0x10922154` adds `0x20`; the predicate passes that address with `w1 = 5` to `0x111c9e24` (`zaphkiel_out-4.txt:4840–4849`, `4895–4899`). Unlike [XRAY-003](xray-003-evidence.md), the wrapper does not implement a stored-key equality test itself. The delegated test's semantics are **UNKNOWN**, and the wrapper is not an independent pre-read guard. |
| **CONFIRMED — numeric fallback path** | `0x10373a68` dispatches through a `+0x138` table entry. With a non-null result it packs the position and passes it to `0x10bad4b0`; otherwise it returns the halfword from the receiver's `+0x42` (`zaphkiel_out-4.txt:5432–5458`). The caller sign-extends the result and uses it in the middle coordinate (`zaphkiel_out-3.txt:59–69`, `116–124`). A height query is **INFERRED**, but the concrete lookup target and meaning of the returned number are not verified. |
| **CONFIRMED — cleanup-like receiver behavior** | `0x1036e8d8` writes `0x13d6e000 + 0x328 = 0x13d6e328` to the first word of its receiver, loops over an array-like pair, calls indirect/import-style helpers, and clears or resets fields (`zaphkiel_out-4.txt:6060–6123`, `6126–6131`). In XRAY-001 the caller passes the same stack address used earlier for acquisition (`zaphkiel_out.txt:125–135`, `201–202`). **INFERRED:** teardown of a scoped object. A table assigned during teardown does not establish the receiver's earlier dynamic type or any mod-safe lifetime. |
| **HYPOTHESIS — block/key resolver** | The caller of `0x10bab9e4` gets the static base via `0x10cc9f90` and compares a field of the read result to the corresponding field of a `0x10cd9424` result (`zaphkiel_out-4.txt:830–842`). Another call to `0x10cd9424` passes the same `0x141f3f58` address used as the comparison operand in XRAY-003 (`zaphkiel_out-4.txt:925–930`; `zaphkiel_out-3.txt:36–40`); a further site directly compares the two returned `x0` values (`zaphkiel_out-4.txt:1035–1044`). This is a better lead for a key-to-block resolver than repeating the generic getter. The key's spelling and semantics, and the meaning of the fields being compared, remain **UNKNOWN**. |

**Port boundary:** Use the evidence to guide host-only coordinate and missing-data fixtures if needed. Do not call the reader, decode a guessed native palette, classify by the compared values, or assume a loadedness predicate. The safe world/thread/lifetime contract, canonical ore identity, and camera acquisition remain open. The only validation here is static evidence review; no mod build, installation, or runtime test.

## Follow-up investigation — XRAY-005-BATCH

**Report received.** See [XRAY-005 evidence and decisions](xray-005-evidence.md). The original request is preserved below for provenance, not to rerun.

**Objective:** Resolve the delegated reader and the two relevant unshown branches, and collect the strongest identity/position follow-ups in the same run. There is no need to re-run the transform.

| Target in [batch `zInput.txt`](xray-005-batch/zInput.txt) | Grounding and decision criteria |
| --- | --- |
| `fn<0x10b69120>` | Success tail at `0x10baba84` (`zaphkiel_out-4.txt:57–66`). Trace input index to the returned value; storage/palette evidence must come from instructions, not the 12-bit index alone. |
| `range<0x10b70a94,0x10b70ad4>` | Selector-0 branch actually taken by the reader, from `0x10b70a7c` (`zaphkiel_out-4.txt:43–49`, `686–696`). Determine what controls the bit that leads to the fallback. A bit test on an unidentified object is not sufficient to name the condition. |
| `fn<0x10cd9424>` | Called at `0x907cf04` with the XRAY-003 static key, and at `0x907d0c8` before the result comparison (`zaphkiel_out-4.txt:925–930`, `1035–1044`). Look for how it interprets the input and finds the result. A registry/key lookup alone would not identify ore names or the fallback. |
| `range<0x10cdad68,0x10cdada8>` | Null branch at `0x10cdad60` (`zaphkiel_out-4.txt:2773–2776`). Look for initialization, a safe alternate value, or failure; that branch determines whether the displayed non-null formula can be generalized. |
| `fn<0x111c9e24>` | Tail target of the sibling predicate at `0x10371ddc` with an embedded-field address and selector `5` (`zaphkiel_out-4.txt:4840–4849`). Identify what it tests without naming the selector based on its number. |
| `fn<0x10bad4b0>` | Non-null callee of the numeric query at `0x10373abc` (`zaphkiel_out-4.txt:5436–5453`). Determine whether the result comes from height-like data, occupancy scanning, or another computation. |

**Command and limits (user-run only):** On the same unchanged LeviLauncher instance and game build, back up the current input and replace it with the six lines in the [batch `zInput.txt`](xray-005-batch/zInput.txt). Run Zaphkiel once as before at the configured path. [`fn<>` and `range<>`](https://github.com/NgawiNgab/Zaphkiel-RE/blob/ba566e6eea4b71c544edd21d9c79d988c5791508/README.md#L67-L74) request disassembly; neither invokes game functions. The two range starts are explicit branches from the report. Each end is `start + 0x40`, a small inspection window, not a claimed function boundary. Keep the [same focused config guidance](xray-004-batch.md#original-run-instructions) for path settings, per-target context, and disabled bulk dumps. No local hash or separate dumps are needed. Do not reuse the addresses after an instance or game change.

**Expected result:** A fresh `zaphkiel_out.txt` with `targets: 6` and the function or range sections. If a body is cut off, an address is rejected, or the tool fails, send the actual partial output and relevant `zaphkiel_log.txt` error. Do not assume a missing section means code does not exist. The actual output is analyzed in [XRAY-005](xray-005-evidence.md), not treated as a binding.
