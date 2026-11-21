# Apollon 1.26.66.1 -- offset recovery status

Target: `libminecraftpe.so` 342,231,392 B md5 `fa377b0e07e7558b16d3684984598cc2`
(confirmed byte-identical to the Drive `Nothing/Library` split archive, all 7 parts).

## Root cause of the launch crash (confirmed)

Shipped `jni/offsets.h` is the **1.21.x** table. Two distinct failure classes:

1. **30 `MemoryPatch::createWithHex(MineLib, Offsets64::X, hex)` sites where `X` was a
   stale 1.21.x address** -> wrote 4-12 bytes into 1.26.66.1 `.text` at the wrong place
   (63 of 77 entries land inside `.text`; 14 land outside it, e.g. `Font` sits in
   `.rodata`). Corrupting arbitrary instructions is what eventually trips
   `stack corruption detected (-fstack-protector)`.
2. **57 `A64HookFunction(getAbsoluteAddress(MineLib, X), ...)` sites were unguarded.**
   `getAbsoluteAddress(lib, 0)` returns the **library base**, not an error. With the new
   table putting unresolved entries at `0`, an unguarded hook would install a Dobby
   trampoline over the **ELF header** -> guaranteed SIGSEGV.

## Fix applied (safety, not recovery)

`main.cpp` gains `A64HookSafe(relOffset, replace, orig)`. All 27 `A64HookFunction`
call sites in `main.cpp` now route through it; it skips when `relOffset == 0` and
nulls the original-function pointer so the feature stays inert instead of crashing.

`MemoryPatch::createWithHex` already refuses `address == 0` (MemoryPatch.cpp:67),
so the 30 patch sites are inherently safe.

Verified: `main.cpp` compiles to a 13.3 MB object with the full libc++ flag set.

## Why the 52 class-method hooks are still 0

Every static symbol source was enumerated and is exhausted:

| source | result |
|---|---|
| `.symtab` | **absent** |
| `.dynsym` (90,670 defined) | libc++/cohtml/v8/webrtc/Xbox/JNI imports only. `Actor` 0, `ClientInstance` 0, `GameMode` 0, `Hud` 0, `Camera` 0, `Entitlement` 0, `AppPlatform` 0 |
| `.rodata` `Class::method` names | 14,128 strings, but leading classes are `entt`, `Runtime`, `Bedrock`, `v8`, `csl`, `Definition`, `mce`... no `Actor::`/`Player::`/`GameMode::` |
| RTTI (`_ZTV`, `_ZTI`) | absent |
| 1,383 bare function-signature strings | *logging only* -- xref to `ClientInstance::requestLeaveGame` lands on a log-call site (file string + line 0x3b1), and `Level::tick`/`Level::_subTick` have **0 code xrefs** |
| JNI exports (95) | `MainActivity_*`, `JellyBeanDeviceManager_*`, ... nothing class-level |

Conclusion: **no name->address source exists for Actor/Player/GameMode/Hud/ClientInstance.**
These are not string-recoverable; they need vtable-slot resolution, which requires an
independent anchor for each class's vtable. Fabricating addresses is prohibited.

## Structures discovered (usable for future work)

- **entt system registry**: 707 named system records via `R_AARCH64_RELATIVE`. Rich for
  movement/gravity/camera systems (`MobMovementGravity::tickAirGravity`,
  `MobMovementDrag::tickApplyLavaDrag`, `ItemUseSlowdownSystemImpl::*`,
  `ApplyJumpModifierSystem::_tickApplyJumpModifierSystem`). Adapter strings are real
  mangled names, so **JumpHack is recoverable from this registry**.
- **`{name, signature}` type registry** at 0x20 stride (e.g. 0x13a55fc8/0x13a55fd0):
  `name` -> short mangled typeid, `signature` -> `__FUNCSIG__` text. Used for diagnostics.
- **`Level` vtable-like run**: 11 consecutive `.data.rel.ro` slots at 0x13e0ba40 are
  `R_AARCH64_RELATIVE` addends inside `.text`. Candidate anchor for `Level`, but no slot
  is individually identified, so it cannot yet pin `getRuntimeActorList`.

## Recovery routes rejected as unsound

- **Constant-site attribution** (`const_scan.py`): 0.42 has 8 sites, but r2 disassembly
  shows the enclosing functions are constructors (allocations, vtable stores), not a jump
  modifier. The "`return 0.42`" concept is a hook, not literally a constant store.
- **Cross-version diffing**: needs a 1.21.x `libminecraftpe.so` as anchor. None on disk
  and it is not self-generatable.

---

## JumpHack investigation (this round)

`JumpHack` is *not* a void system tick. Its Apollon signature ignores all six args
and **returns a float**:

    float JumpHack(void*,void*,void*,void*,void*,void*) { return 0.42 (+ (jumpValue-1)/10); }

So the target must be a float-returning getter, recoverable only by exact instruction
shape. All three encodings clang uses for `return <const>;` were scanned
(`re/getter_scan.py`):

| encoding | result |
|---|---|
| `fmov s0,#imm8 ; ret` | 1 hit (mid-function) |
| `ldr s0,[pc,#off] ; ret` (literal pool) | 0 |
| `movz/movk wN ; fmov s0,wN ; ret` | the real ones, below |

**Two genuine `return 0.42f` bodies** exist:
`0xc3d75dc` — `movz w8,#0x0a3d; movk w8,#0x3ed7,lsl#16; fmov s0,w8; ret`
`0xcd28bb0` — same shape, inside a null-checking wrapper at `0xcd28ba0`.

**Neither can be verified as JumpHack statically.** Evidence:

- `bl` callers: **0** for both (and 0 for the wrapper).
- `R_AARCH64_RELATIVE` pointer slots holding them: **0** (not in any vtable).
- `adrp+add` code references: **0**.

They are only ever branched to from inside `0xcd28ba0`'s own tail
(`0xcd28ba4: b 0xc3d75c4`). A function with no caller, no vtable slot and no address
take is unreachable dead code as far as the shipped binary is concerned, so pinning
0.42 to `JumpHack` would be a guess. All other 0.42 sites are `fmov s1,` / `fmov s2,`
comparison constants, not returns.

**Conclusion:** `JumpHack` stays `0` pending the cross-version diff (below).

## Cross-version diff harness — built and validated

`re/diff_match.py` + `re/diff_selftest.py`.

- Candidate entries = `R_AARCH64_RELATIVE` addends inside `.text` **plus** every
  `stp x29,x30,[sp,#-N]!` and `sub sp,sp,#N` prologue.
- Probe = 14-instruction hash of `(mnemonic, normalised operands)`, where branch
  targets become opaque `TGT` and data immediates keep their value (so `cmp #1` never
  matches `cmp #7`). This makes the hash survive address shifts.
- Full-body signature similarity disambiguates ties (e.g. 777 candidates).
- `entry_of()` recovers the enclosing function, so interior hook offsets are handled by
  carrying `delta = old_offset - old_entry` across to `new_entry + delta`.

**Known-answer validation: 12/12.** Fed the shipped 1.21.x addresses, the harness
returns exactly the verified 1.26.66.1 addresses for all twelve entries including the
three with non-zero deltas (`NoCamDistortion`, `LavaSpeed`, `SpeedHack`) and the
frameless `SlowDownTriggers` (`sub sp,#0x90` instead of `stp`).

**Blocked on input:** a 1.21.x `libminecraftpe.so` to use as the far side of the diff.
