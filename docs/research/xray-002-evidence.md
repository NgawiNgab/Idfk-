# XRAY-002 — What the block lookup caller compares

**Question:** Does the helper shown in [XRAY-001](xray-001-evidence.md) produce a block identifier, or just read a value whose meaning must be established elsewhere?

## Scope and provenance

- **CONFIRMED — report:** The user-run `zaphkiel_out-2.txt` is 94,099 bytes. Its SHA-256 is `86a590564c2eecdc6b56cbf5ddc759e2ac641f0256eb491e4f419e4c59713b00` (**report only**, not a library hash). Lines 18–24 show the single target from [XRAY-002 `zInput.txt`](xray-002/zInput.txt). The agent did not run Zaphkiel.
- **CONFIRMED — matching report headers:** `zaphkiel_out-2.txt:3–15` agrees with `zaphkiel_out.txt:3–15` on the reported launcher-cache path, byte count, and segment/range metadata. Both report AArch64 code. Neither header proves identical bytes, the binary loaded in the game process, or a match to the downloaded Drive ELF. Minecraft `1.26.62` remains user-reported; installed target binary identity remains **UNVERIFIED**. No new Drive binary analysis is used here.

## Result

**CONFIRMED (report disassembly):** `fn<0xe82ff94>` follows a branch stub to `0x134ef05c`. The resolved body reads one 64-bit value from the address passed in `x0`, returns it in `x0`, and does not compute a hash or read string bytes (`zaphkiel_out-2.txt:21–29`). The displayed `ret` contradicts the tool's `no ret found` heuristic; neither the `this` label nor the 64-bit load establishes a C++ type.

```asm
0x134ef05c  ldr x0, [x0]
0x134ef060  ret
```

**INFERRED — scope of the comparison:** Reading the two reports together, *if they scanned the same code*, the XRAY-001 branch tests equality of two stored 64-bit values:

| Operand | Evidence from `zaphkiel_out.txt:135–144` |
| --- | --- |
| Lookup result's key candidate | The caller loads the field at `lookup result + 0x68`, adds `0xc8` to that loaded address, and passes the sum to the helper. The helper reads at that address, not at the `+0x8` string position in BedrockTools. |
| Statically addressed comparison value | `adrp` gives `0x14223000` in the tool's ELF coordinates, and `add #0x1b8` forms `0x142231b8`. The second call reads the value there. The location is not the value itself, and the report doesn't show its initialization. |

**CONFIRMED (report scope):** XRAY-002 lists 2,878 caller entries for the resolved target and shows one excerpt, at `0xb6c0b18` (`zaphkiel_out-2.txt:30–69`). The shown caller mixes the loaded value and another byte using additions, shifts, XOR, and a mask. **INFERRED:** That use is consistent with hash/key mixing in that caller. It does not establish what the XRAY-001 caller compares. The caller count is tool output, not a count of verified C++ methods.

**HYPOTHESIS — cached name hash:** The field arithmetic fits the pinned [BedrockTools `Block::fullName()` wrapper](https://github.com/NgawiNgab/BedrockTools/blob/c03d3ae0c9022557de0acdf0d94e373e0b9e0579/include/bedrocktools/sdk/render/Block.hpp#L9-L18) up to its [split `0x88 + 0x40` position](https://github.com/NgawiNgab/BedrockTools/blob/c03d3ae0c9022557de0acdf0d94e373e0b9e0579/include/bedrocktools/sdk/offsets/Render.hpp#L86-L101). BedrockTools then reads a presumed `std::string` at another `+0x8`; XRAY-002 does not. The cached-hash interpretation needs a producer that ties the same field to known block names, or controlled observations that distinguish names, unrelated blocks, and state variants. An equality test is also compatible with other stored keys.

**Decision:** The immediate question of what the helper *does* is resolved in the report. What the value *means* is still **UNKNOWN**: neither `HashedString::getHash`, a canonical name, a runtime ID, nor a block-state layout is verified. Do not use this helper or these offsets as an ore classifier or native binding.

## Follow-up investigation — XRAY-003

**Report received.** The user ran `fn<0x10371d70>` from XRAY-001. [XRAY-003](xray-003-evidence.md) shows another comparison of stored 64-bit values after an indirect call, not an independent loaded-position check. It does not resolve the meaning of the key.
