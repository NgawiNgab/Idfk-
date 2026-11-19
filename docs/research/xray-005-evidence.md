# XRAY-005 — Storage dispatch, registry fallback, and column values

**Question:** What is actually checked before the inner read, what happens when fallback state is absent, and how does the related key resolver select its result?

## Scope

- **CONFIRMED (report):** User-run `zaphkiel_out-5.txt`, 143,189 bytes; SHA-256 `9baa8fb6f964e7a2038aa237bfcc524b181530b2aa85eaa985a92bf5a22333f3` (**report only**). All six sections match the [XRAY-005 batch input](xray-005-batch/zInput.txt). The agent did not run Zaphkiel or use new Drive binary evidence.
- **CONFIRMED (reported metadata):** The launcher-cache path, size, and segments agree with XRAY-001 through XRAY-004 (`zaphkiel_out-5.txt:3–18`). These are observations of scanned AArch64 code. Minecraft `1.26.62` is user-reported. The installed target binary identity is still **UNVERIFIED**; agreement between tool reports does not prove the loaded library or a match to Drive.

## Reader and missing data

| Classification | Evidence and limit |
| --- | --- |
| **CONFIRMED — selector-zero check** | The branch at `0x10b70a94` returns equality with zero for the 64-bit entry at `record + 0x30 + 8 × (x1 & 0xff)` (`zaphkiel_out-5.txt:794–802`). [XRAY-004](xray-004-evidence.md) supplies selector `0` and calls the reader when this bit is clear (`zaphkiel_out-4.txt:43–49`, `57–66`). Thus the check on this path is whether the first storage pointer is null. It is not a loaded-chunk or thread-safety check. The next prologue at `0x10b70aac` is after the branch's `ret`. |
| **CONFIRMED — final observed dispatch** | `0x10b69120` loads that first pointer at `record + 0x30`, reads its table pointer, then tail-dispatches through the entry at byte offset `0x20` (`zaphkiel_out-5.txt:21–29`). The local index stays in `x1`. The stub overwrites `x2` with the destination, so the tool's `x2=arg2` guess is not evidence of a third input. The following prologue at `0x10b69130` is not on the entry's observed path. **UNKNOWN:** Which concrete implementation handles the index and how it represents block states or palettes. |
| **CONFIRMED — absent fallback field is not a safe default** | The previously missing branch at `0x10cdad68` performs a store through a zero address, rather than supplying an alternate object (`zaphkiel_out-4.txt:2770–2776`; `zaphkiel_out-5.txt:2960–2967`). **INFERRED:** a deliberate fail-fast path under normal process memory protections. The shared fallback from XRAY-004 depends on that field already being populated. Do not invoke the helper or try to trigger the null path to test it. No runtime fault was tested or observed by the agent. |

**Decision:** Use the local pointer check only as evidence for the reader's control flow, not as a callable availability implementation. The outer lookup can route to a field-derived fallback even when no local storage is available. Nothing in this report identifies that value as air or proves retention and access safety in a live world.

## Block/key resolver

**CONFIRMED — selection and retry.** `0x10cd9424` forwards the caller's `x1` with `w2`, `w3`, and `w4` cleared to `0x10cd9bdc`. If the returned `x0` is zero, it retries with `0x141f3000 + 0xf58 = 0x141f3f58` as the key address and loops on a zero result. On a nonzero result it tail-calls `0x1091a034`, whose reported body reads `+0x200` (`zaphkiel_out-5.txt:819–839`; `zaphkiel_out-4.txt:3181–3186`). This wrapper does not return a simple “not found” status and is not a safe lookup for an uninitialized mod context. Do not attribute the procedure starting at `0x10cd9478` or its diagnostic text to this wrapper.

**CONFIRMED — comparison keys recur as resolver arguments.** The caller at `0xa9d59a0` obtains the static base via `0x10cc9f90`, passes `0x141f3f58` (the XRAY-003 comparison operand) to `0x10cd9424`, and later compares its returned `x0` with the read result (`zaphkiel_out-5.txt:203–210`, `281–287`; `zaphkiel_out-3.txt:36–40`). Another caller passes `0x142231b8`, the XRAY-001 comparison operand, to the same resolver (`zaphkiel_out-5.txt:1135–1136`, `1148–1153`; `zaphkiel_out.txt:139–144`).

**INFERRED — key-to-block/default-state selection.** The common base, reused keys, returned field, and caller comparisons strengthen the resolver candidate without establishing symbol names or what the key contains. The registry-derived fallback from the failed coordinate lookup and the fixed-key retry are distinct observed mechanisms; they are not proven to return the same block. Neither static key has a verified spelling or runtime/state ID. In the shown entry to `0x10cd9bdc`, a call to `0x134ef010` tests its key argument, and one branch returns zero; the other branch at `0x10cd9c5c` is not shown (`zaphkiel_out-5.txt:1074–1096`). This is where further evidence could distinguish a stored hash, string, or another key representation.

## Other two batch results

| Target | Result |
| --- | --- |
| `0x111c9e24` | **CONFIRMED:** The low byte of its selector is compared to `0x19`; that value yields true. Otherwise it returns equality against the byte at the passed address (`zaphkiel_out-5.txt:2986–2991`; `ccmp` uses `NZCV=0x4` when the `ne` condition is false). For the XRAY-004 sibling's selector `5`, this is a byte-equals-5 test after lookup (`zaphkiel_out-4.txt:4840–4849`, `4895–4899`). The byte's enum or property is **UNKNOWN**. It is not evidence of availability. |
| `0x10bad4b0` | **CONFIRMED:** Uses `(byte0 + 16 × byte1) & 0xff` to index two-byte entries in a table based at `receiver + 0xc40`. It adds that halfword to the halfword returned by `0x10b3f230` from `[load64(receiver + 0x30) + 0xa8]`; the packed vertical halfword is not read here (`zaphkiel_out-5.txt:3597–3617`). **INFERRED:** a cached column height plus a base, consistent with its caller using the result as a vertical coordinate (`zaphkiel_out-3.txt:59–69`, `116–124`). Neither valid world bounds nor concrete storage ownership are proved. |

**Port boundary:** Packed-coordinate and missing-data fixtures can be designed without a game binding. Use `unavailable` as a distinct state in that design, not an assumed air value or an absence of a pointer. A mod still needs corroborated block identity, safe world/thread/lifetime access, and camera acquisition. There is no verified native binding, build, installation, or runtime validation here.

## Next investigation — XRAY-006-BATCH

**Status: report received.** See the [XRAY-006 evidence](xray-006-evidence.md) and its grouped follow-up. The input and instructions below are preserved for provenance, not a request to rerun XRAY-006.

**Original objective:** Identify the source of the nullable record and the concrete storage dispatch, and resolve the most useful missing key path in one batch of four report-grounded targets.

| Target in [batch `zInput.txt`](xray-006-batch/zInput.txt) | Current evidence and decision criteria |
| --- | --- |
| `fn<0x10bb9298>` | The caller gets a non-null object through the `+0x148` table dispatch, passes a shifted signed halfword to this call at `0xa9d5930`, checks for null, and later uses the saved result as the receiver of the observed read dispatcher (`zaphkiel_out-5.txt:169–181`, `281–285`). Look for record selection and what makes it return null. A record's existence alone does not prove safe availability. |
| `fn<0x10b70624>` | An XRAY-004 caller walks the `0x68`-stride records and passes the key resolver’s result with selectors zero at `0x907ced8` (`zaphkiel_out-4.txt:891–921`). This is an **initialization hypothesis** to trace storage-pointer/table assignment, not a candidate for the mod to execute or use to change world data. Only an identified assignment and dispatch target would narrow the concrete reader. |
| `range<0x10cd9c5c,0x10cd9d1c>` | This is the explicit branch destination when the entry's key screening bit is clear (`zaphkiel_out-5.txt:1074–1096`). Inspect the loads, comparisons, and delegated calls that follow. A numeric comparison without a known producer does not verify a canonical block name. The end is `start + 0xc0`, a bounded inspection window, not a claimed function end. |
| `fn<0x134ef010>` | Called with the key as `x0` at `0x10cd9c1c`; the caller branches on its result bit (`zaphkiel_out-5.txt:1074–1084`). Determine what the early check reads. Do not infer a key layout or name from a heuristic label. |

**Command (user-run only):** On the same unchanged LeviLauncher instance and game build, back up the current input and replace it with [this four-line `zInput.txt`](xray-006-batch/zInput.txt) at the configured path. Run Zaphkiel once as before. [`fn<>` and `range<>`](https://github.com/NgawiNgab/Zaphkiel-RE/blob/ba566e6eea4b71c544edd21d9c79d988c5791508/README.md#L67-L74) request disassembly, not function calls. Keep the [focused config guidance](xray-004-batch.md#original-run-instructions), unchanged path settings, and bulk dumps off. No local SHA-256 is needed. If the game or instance changed, do not reuse these targets.

**Expected output:** Fresh `zaphkiel_out.txt` with `targets: 4` and one section per target. Send the raw output, including stubs or truncated sections, and include the relevant `zaphkiel_log.txt` error if a target fails. Do not infer an answer from a missing or unresolved section; wait for the actual report before proceeding.
