# XRAY-004-BATCH — Block reads, fallbacks, and caller context

**Objective:** Collect the useful unresolved direct call targets from [XRAY-001](xray-001-evidence.md) and [XRAY-003](xray-003-evidence.md) in one user-run Zaphkiel report. This supersedes the single-target XRAY-004 request and includes that target first. It is a disassembly request, not a native binding. **Status: report received.** See the [XRAY-004 evidence and decision](xray-004-evidence.md). This is the preserved runbook, not a request to rerun it.

**Scope:** The targets are tool-reported virtual addresses in the scanned AArch64 launcher-cache file. Minecraft `1.26.62` is user-reported; installed target binary identity is **UNVERIFIED**. No address is taken from the Drive ELF, and no local SHA-256 is needed. Use the addresses only if the LeviLauncher instance and game build are unchanged from the earlier reports.

## Targets and the questions they answer

The call targets and use sites below are **CONFIRMED in the supplied reports**. Descriptive labels are questions, not identified C++ symbols or callable contracts.

| Priority | Target | Report evidence | What to look for |
| --- | --- | --- | --- |
| Core | `0x10bab9e4` | Called at `0x103719b4` with the non-null indirect result and the transformed position buffer (`zaphkiel_out.txt:47–67`). | Does the result come from position-dependent block storage, another delegated call, or a fallback? |
| Core | `0x11f0e5e8` | Called at `0x103719a8` with destination `sp`, the original position-like input, and the halfword loaded from the receiver (`zaphkiel_out.txt:56–63`). | What fields does it write? Do coordinate arithmetic and the halfword explain the buffer passed to the inner reader? |
| Core | `0x10cc9f90` | First call on both the bounds-failure and null-result paths, at `0x103719cc` and `0x103719fc` (`zaphkiel_out.txt:69–84`). | What does it prepare or return before the second helper? Is it a shared context or something else? |
| Core | `0x10cdad5c` | Second call on both paths, at `0x103719d0` and `0x10371a00`; the wrapper then dereferences its result (`zaphkiel_out.txt:69–84`). | Where does that result come from? Does it select a default object, rather than return an explicit unavailable status? |
| Core | `0x1091a034` | Tail-called at `0x103719f8` on bounds failure and called at `0x10371a08` after a null indirect result (`zaphkiel_out.txt:69–93`). | What does the common final helper return, and could that value also appear for a valid block? |
| Context | `0x10371db8` | Called at `0x8b9f9f4` using the same receiver and tuple when the XRAY-003 check is false (`zaphkiel_out-3.txt:116–129`). | Is it another stored-key comparison, a property check, or a different predicate? Do not assume it is a loadedness guard. |
| Context | `0x10373a68` | Called at `0x8b9f8f8`; the caller sign-extends its low 16 bits, stores the result, then uses that field in the position-like tuple (`zaphkiel_out-3.txt:59–69`, `116–124`). | Is this a height-like query, and what provides the result? A coordinate used by one caller is not proof of the world's valid bounds. |
| Context | `0x1036e8d8` | Later call at `0x9004a90` receives `sp + 0x1a8`, the same stack address used as lookup receiver earlier in the shown caller (`zaphkiel_out.txt:119–135`, `197–205`). | Does this show receiver cleanup or ownership? A cleanup-time vtable assignment alone would not identify the runtime type used at lookup. |

**Excluded deliberately:** Already-read bodies (including `0x10371934`, `0x10371d70`, and `0x134ef05c`); omitted callers without a specific question; static data at `0x142231b8` and `0x141f3f58`, which are not `fn<>` targets; and indirect table entries whose concrete executable targets have not been identified. No grounded camera function or canonical ore-identifier producer is in these reports yet.

## Original run instructions

1. Back up your current `zInput.txt` and, if changing settings, `zaphkiel_config.txt`. If the game build or launcher instance changed, do not reuse this address list.
2. Replace the contents of the configured input file with [the batch `zInput.txt`](xray-004-batch/zInput.txt). The default path is `/storage/emulated/0/Android/media/org.levimc.launcher/kurumi/zInput.txt`. Zaphkiel documents [one target per line](https://github.com/NgawiNgab/Zaphkiel-RE/blob/ba566e6eea4b71c544edd21d9c79d988c5791508/README.md#L24-L33) and [`fn<>` for disassembly at a VA](https://github.com/NgawiNgab/Zaphkiel-RE/blob/ba566e6eea4b71c544edd21d9c79d988c5791508/README.md#L67-L73).
3. Keep path settings unchanged. To limit expanded context, use the documented [limits](https://github.com/NgawiNgab/Zaphkiel-RE/blob/ba566e6eea4b71c544edd21d9c79d988c5791508/zaphkiel_config.txt#L1-L29) and [disable bulk dumps](https://github.com/NgawiNgab/Zaphkiel-RE/blob/ba566e6eea4b71c544edd21d9c79d988c5791508/zaphkiel_config.txt#L76-L104) in your existing config, rather than replacing it:

```ini
max_fn_insns = 120
max_callers = 1
max_callees = 1
caller_depth = 1
callee_depth = 1
dump_rtti = false
dump_dynsym = false
dump_live_strings = false
dump_vtables = false
full_decompile = false
get_strings = false
```

4. Run Zaphkiel once as before. **Expected:** one fresh `zaphkiel_out.txt` with `targets: 8` and a section for each address. It may still list omitted callers. Send the report as-is, including stubs. If the run fails or only completes part of the batch, send any partial report and the relevant `zaphkiel_log.txt` error. Separate RTTI, symbol, vtable, or strings dumps are not needed.

## How the result will be used

- **Reader and position:** Trace the transformed fields into the inner call and trace its return. Indexing tied to both the coordinate and resolved storage would support a reader candidate. An unidentified delegate or truncated body leaves the question open.
- **Missing data:** Follow the two failing handoffs through the shared helpers. If the same returned value can represent a valid block and missing data, do not use it alone to infer loadedness. Even a visible default object does not identify it as air without independent evidence.
- **Predicates and lifetime:** Separate numeric bounds, lookup, key equality, and cleanup. Do not promote a compared value to a canonical ore identifier or a cleanup caller to a verified ownership/thread contract.

See the [actual report findings](xray-004-evidence.md) before drawing conclusions. The batch does not enable native code or verify runtime compatibility.
