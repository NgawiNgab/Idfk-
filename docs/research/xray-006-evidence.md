# XRAY-006 — Nullable records, uniform storage, and name keys

**Decision:** Prefer the coordinate-lookup route over directly decoding palettes. XRAY-006 identifies a nullable record selector and one storage initialization path. A separate check of the verified Drive ELF identifies a uniform reader and ties several keys to names. It does not validate a live block reader, loadedness check, or ore classifier on the device.

## Scope

- **User-run report:** `zaphkiel_out-6.txt`, 36,290 bytes, 1,036 logical lines; SHA-256 `477a9dd45b33fa43a732879149841e687624488d10ab92b6a04cac5000716441` (**report only**). `targets: 4` and all four section headers match the [XRAY-006 input](xray-006-batch/zInput.txt). Three targets show code; the fourth ends at a tool heuristic, not a decoded body. The cache path, size, and segments match the previous reports (`zaphkiel_out-6.txt:3–18`).
- **Independent sample:** The existing [Drive ELF](intake.md#binary-intake) was rechecked against its recorded SHA-256 and inspected read-only; no new download or agent-run Zaphkiel. It matched 86 normalized instructions in the report's selector, bounds helper, initializer, and lookup windows (`zaphkiel_out-6.txt:37–58`, `74–86`, `220–246`, `984–1007`) and the four rejected bytes at `0x134ef010`. These are selected matches, not a binary identity check. Minecraft `1.26.62` is user-reported; the scanned cache file's byte identity and the loaded module's identity remain **UNVERIFIED**.

## What the report establishes

| Target | Finding and limit |
| --- | --- |
| `0x10bb9298` | **Nullable vertical record selector.** Stores the low halfword of its input, passes it to `0x10b3f254` using the object at `receiver + 0x30`, and returns null if the result bit is clear. The shown bounds path checks the section's base Y (`section × 16` as a signed halfword) against signed lower/upper bounds, inclusive/exclusive respectively. On success it indexes the `0x68`-byte records by the requested section minus the lower-bound section; see the expression below. It returns that record only if the sign-extended low byte from `0x10b70a44` equals the requested signed section. It does not independently check the record vector's end or the storage pointer in this body (`zaphkiel_out-6.txt:37–68`, `71–89`). |
| `0x10b70624` | **Storage installation, not a read.** With non-null `x1`, it obtains a `0x30`-byte object, writes the table address `0x13e0fde0` at offset zero, and on the shown setup branch stores the input at object `+0x8`. It installs the object at record `+0x20` and `+0x30`, clears `+0x28` and `+0x38`, and processes replaced fields. The caller reaches it with a key-resolved result while stepping through `0x68`-byte records (`zaphkiel_out-6.txt:220–278`, `414–444`). The table entries and other storage implementations are not decoded in this report. |
| `0x10cd9c5c` window | **Ordered key lookup, then string handling on a miss path.** Loads a root-like pointer at `receiver + 0x8`; compares a node key at `node + 0x20` to the input via `0x134eea9c`. The comparison's sign selects the child at `+0` or `+8` and updates the candidate. A reverse comparison gates the match branch. The visible unmatched path calls `0x134ef06c` and selects a pointer and length using a low-bit tag and an inline/indirect layout before passing them to `0x134ae788` (`zaphkiel_out-6.txt:984–1027`). This is not a simple numeric runtime-ID lookup. The range does not show the full miss handling. |
| `0x134ef010` | **No body in this report.** The direct target says `looks like string data (bytes: 08 20 40 39)` (`zaphkiel_out-6.txt:1033–1036`). The first word decodes as AArch64 `ldrb w8, [x0, #8]`, and [XRAY-005](xray-005-evidence.md) shows an actual call to this address (`zaphkiel_out-5.txt:1074–1084`). Use a bounded `range<>` rather than repeating the rejected `fn<>` query. |

Selector arithmetic, not an ABI: `candidate = load64(receiver + 0x180) + 0x68 × (s16(arg) − s12(lower_halfword >> 4))`. The enclosing XRAY-005 caller calculates `section` by arithmetic-shifting a signed vertical halfword before calling and checks the returned record for null (`zaphkiel_out-5.txt:169–181`). The additional XRAY-006 caller first checks a two-word acquisition result at `sp + 0x20`, then applies more tests before taking its alternate path. It also releases the second word on exit (`zaphkiel_out-6.txt:103–153`). **Neither a non-null record nor its vertical tag proves the storage is loaded and safe for the mod to read.**

## Independent corroboration — Drive sample only

### Uniform storage and a possible acquisition gate

- **Uniform reader:** In `.rela.dyn`, the `R_AARCH64_RELATIVE` entry for table address `0x13e0fde0 + 0x20 = 0x13e0fe00` has addend `0x10b89018`. The body is `ldr x0, [x0, #8]; ret`; it ignores the local index. The table's RTTI chain `0x13e0fdd8 → 0x13e0fec0`, name field `0x13e0fec8 → 0x34725d2`, demangles to `SubChunkStoragePaletted<Block, 0ul, (SubChunkStorageUnit::Type)0>`. This matches the initialized single-value representation. It explains **one** variant behind the [XRAY-005 dispatch](xray-005-evidence.md#reader-and-missing-data), not how mixed-block palettes or runtime IDs are encoded. Do not use the bare file table words as function addresses; the entries are relocated.
- **Acquisition lead, not a safety contract:** In the sample `0x10bece70–0x10becf34`, an indirect call via `+0x20` writes a two-word temporary through `x8`. This helper returns a null pair on a null object or a low-byte status below `0x0b`. Otherwise it returns the pair and, when the second word exists, increments the count at that object's `+0x8` before releasing its temporary. `0x10bbcf20–0x10bbcf28` acquire-loads the status byte at object `+0xb8`. The caller in the report already calls the wrapper at `0xa2daca4` and branches on the first result word. **Inferred:** an owned, status-gated acquisition path in this specific caller. The producer behind the indirect call, meaning of the status values, safe execution thread, and lifetime of dependent storage all remain open.

### Names, hashes, and the coordinate fallback

In the sample, `0x134ef010–0x134ef02c` tests for a zero length at key `+0x8`, using a tagged short/long representation. `0x134ef06c` returns `key + 0x8`. The comparator at `0x134eea9c–0x134eeae4` compares the first words and delegates a string comparison at `+0x8` on equality. The initializer at `0x134eef00–0x134eef38` writes the supplied 64-bit value at offset zero and passes the supplied string to setup at `+0x8`. The following observed initializer pairs match **64-bit FNV-1** of the listed name (not FNV-1a, and not a block-state ID).

| Name literal in the sample | Setup call site | Destination | First word |
| --- | --- | --- | --- |
| `minecraft:air` | `0x10a104d0` | `0x141f3f58` | `0xbd584baf00003448` |
| `minecraft:end_stone` | `0x10fc8bcc` | `0x142231b8` | `0x4d62f43730f6497d` |
| `minecraft:diamond_ore` | `0x10fc85f0` | `0x142230c8` | `0x64ac5906db894391` |
| `minecraft:deepslate_diamond_ore` | `0x10fc8240` | `0x14222fd8` | `0xf720faa144e50745` |

Both previous comparison-key locations are in `.bss`, not in file-backed string data. Their names come from tracing the initializers, not reading strings at those addresses. A separate copy path at `0x10765c70–0x10765cb4` loads a block-like object through `[x21 + 0x8]`, follows `+0x68`, adds `0xc8`, and passes the key to `0x134ef06c` before copying the string-like result. This supports the [XRAY-002](xray-002-evidence.md) name-key interpretation independently of the BedrockTools declarations. It does not establish the correct accessor for all blocks, state variants, or the user's loaded module.

**Coordinate fallback in the sample:** `0x10cdacfc–0x10cdad58` calls the resolver with `0x141f3f58` and zero selectors; it stores the result in a holder and writes the holder to registry `+0x130`. The startup caller obtains base `0x14208858` via `0x10cb7d38` at `0x9680df0` and passes it to the writer at `0x9681114`. The [earlier fallback chain](xray-004-evidence.md) reads this field, dereferences the holder, and reads result `+0x200`. Thus the sample initializes the coordinate fallback using the **air key**, not just the separate key-miss retry. If initialized normally, both paths can produce an air-derived result. The writer does not verify a successful resolve here; this is not a probe, and no runtime behavior was tested. **Do not use an air result to distinguish a loaded air block from an unavailable position.**

For reproduction on this exact ELF: preserve virtual-address to file-offset translation from `PT_LOAD`, use `.rela.dyn` for the table and RTTI, and decode only the cited AArch64 windows. The literal inputs above are at `0x28001d8`, `0x2728635`, `0x24d2b0e`, and `0x27c823b`, respectively, in file-backed `.rodata`. The ELF, scratch tools, and binary extracts are not part of this repo.

## Next investigation — XRAY-007-BATCH

**Status: report received.** See the [XRAY-007 results and terminology](xray-007-evidence.md). The input and instructions below are the original runbook, not a request to repeat it.

**Original objective:** Corroborate name copying, fallback initialization, and the acquisition gate on the user's scanned file in one run. Most windows came from the Drive sample; the prior reports supplied the acquisition call and the rejected small key query. These are read-only disassembly queries, not hooks, calls, or a native allowlist.

| Query in [batch `zInput.txt`](xray-007-batch/zInput.txt) | What to check |
| --- | --- |
| `range<0x10bece70,0x10becf38>` | Does the acquisition wrapper called at `0xa2daca4` write a nullable pair, check status, and retain/release a holder? |
| `range<0x10bbcf20,0x10bbcf2c>` | Does the wrapper's status helper perform the observed acquire byte load? This does not identify the enum or prove thread ownership. |
| `range<0x10a104a8,0x10a104d4>` | Does the initializer pass the air name/key pair to setup at `0x134eef00` for `0x141f3f58`? |
| `range<0x10cdacfc,0x10cdad5c>` | Does the writer populate the fallback field with the result of resolving that key? Do not execute it. |
| `range<0x10fc85cc,0x10fc85f4>` | Check the normal diamond ore key producer, comparing the literal and the initialized value. |
| `range<0x10fc821c,0x10fc8244>` | Check the deepslate diamond ore producer separately; the two names must stay distinct even if they share an outline category. |
| `range<0x10765c70,0x10765cb8>` | Check the path from a block-like input through the key to a copied name, rather than inferring it from a hash comparison. |
| `range<0x134ef010,0x134ef08c>` | Bypass the XRAY-006 `fn<>` rejection and inspect the length check and small string accessors. This window contains several separate entries, not one long function. |

**Run once on the same unchanged LeviLauncher instance and game build.** Back up and replace the configured `zInput.txt`. [`range<>`](https://github.com/NgawiNgab/Zaphkiel-RE/blob/ba566e6eea4b71c544edd21d9c79d988c5791508/README.md#L67-L74) requests bounded disassembly. Keep the [focused config](xray-004-batch.md#original-run-instructions), leave path settings alone, and keep `annotate_inline_strings = 1` if supported by the installed tool. No local SHA-256 or bulk dumps. Send the fresh `zaphkiel_out.txt` with `targets: 8`, even if string labels are absent or a section fails. Missing annotations must not be filled in as device evidence from the sample. If the instance/build changed, do not reuse this input.

**Port boundary:** Keep the game adapter disabled on unverified builds. An isolated fixture can classify copied names and distinguish `unavailable` from `minecraft:air`. Actual world acquisition, safe thread/lifetime, camera sampling, and controlled offline validation are still gates. No mod was built, installed, or executed.
