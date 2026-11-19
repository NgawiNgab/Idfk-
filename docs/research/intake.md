# Minecraft Bedrock block visualization — research intake

Recorded 2026-11-18. **Downloaded ELF verified; device-reported loaded Build ID matches. Full device file identity is not verified.**

## Project and target

- **CONFIRMED — project checkout:** At base `49135103efaf4eb25e2d613fe3dbaf608588ca21`, the repository contains only `README.md`. There is no project native code or build configuration to review.
- **CONFIRMED — Apollon reference:** The supplied archive was downloaded, hash-checked against Drive, and reviewed statically. See the [Apollon source trace](apollon.md) for provenance, source locations, and limitations.
- **CONFIRMED — requested scope:** Apollon-style ore outlines as a LeviLauncher native mod for the user-reported Minecraft `1.26.62`. See the [port plan](xray-port-1.26.62.md) and [XRAY-001 evidence](xray-001-evidence.md).
- **USER-REPORTED — loaded build fingerprint:** The user reports Minecraft `1.26.62` and returned the matching [loaded Build ID](#loaded-build-id). The Zaphkiel reports still describe a scanned launcher-cache file. Neither source establishes byte-for-byte equivalence to Drive or independently verifies the release label.

## Binary intake

**CONFIRMED — byte retrieval:** All six parts in the user-supplied Drive folder downloaded without a permission or transfer error. Local byte counts matched the listing, and `sha256sum -c` verified all six against the [checksum manifest](libminecraftpe-parts.sha256). The Drive link, binary parts, and user reports are not included in this public repository. The Apollon ZIP is a separate reference. The unrelated subfolder was not opened.

| Drive filename | Size (bytes) |
| --- | ---: |
| `libminecraftpe.zip.1` | 68,157,440 |
| `libminecraftpe.zip.2` | 68,157,440 |
| `libminecraftpe.zip.3` | 68,157,440 |
| `libminecraftpe.zip.4` | 68,157,440 |
| `libminecraftpe.zip.5` | 68,157,440 |
| `libminecraftpe.zip.6` | 1,444,360 |
| **Total** | **342,231,560** |

**CONFIRMED — reassembly for these exact parts:** Part 1 starts with a ZIP local header. Part 6 contains a single-disk end-of-central-directory record whose directory offset and length agree with the combined byte count. After verifying the individual hashes, joining copies in `.1` through `.6` order produced a ZIP that passed `unzip -t`. Its only member was `libminecraftpe.so` (CRC32 `f270b755`). This verifies the archive and order for these copies without assuming the split tool. The originals were not modified. Copies, combined ZIP, and extracted binary were kept outside Git.

**CONFIRMED — extracted ELF identity** (`readelf -h -n`, `wc -c`, and `sha256sum`):

| Property | Verified value |
| --- | --- |
| File | `libminecraftpe.so` |
| Size | 342,231,392 bytes |
| Format | ELF64, little-endian, AArch64 shared object with an Android note |
| GNU build ID | `c60fa32908019eecae39ebfe7bac5755dc35cde2` |
| SHA-256 | `2a915cf07bd165c5f939b02b36ee756878a63c4489f4971fb14e6e7bdf39216b` |

**UNKNOWN — game version of the extracted ELF:** The ZIP contains no APK/version manifest. The ELF headers identify the downloaded binary, not the Minecraft release label or the installed game. A [Drive-only pattern scan](xray-001-evidence.md#independent-comparison--downloaded-sample-only) agrees with the user report on one candidate location and segment metadata; this is not an identity match. Do not transfer bindings between those scopes.

## Loaded Build ID

The user returned `c60fa32908019eecae39ebfe7bac5755dc35cde2` after the [v0.1.1 read-only check](../../README.md#read-only-loaded-build-id-check) from [PR #1's ARM64 build](https://github.com/NgawiNgab/Idfk-/actions/runs/44419259362). All 40 hex digits match the GNU Build ID of the separate Drive ELF above. This is the user-reported value, not an agent-run device test. The check reads the GNU note from an already-loaded `libminecraftpe.so` mapping, not the launcher-cache file that Zaphkiel scanned.

This resolves the fingerprint comparison for the reported session. It is not a hash of the installed file or of the running code, and it does not establish any callable ABI, safe block availability, thread/lifetime contract, or main-pass camera convention. No additional hash or Zaphkiel batch is needed for this comparison. The [separate passive terrain probe](passive-terrain.md) also checks exact loaded code and dispatch before it can attach; no native result has been reported yet.

## BedrockTools reference

Reviewed [`NgawiNgab/BedrockTools` at `c03d3ae0c9022557de0acdf0d94e373e0b9e0579`](https://github.com/NgawiNgab/BedrockTools/tree/c03d3ae0c9022557de0acdf0d94e373e0b9e0579). **CONFIRMED — reference declarations:** The [README](https://github.com/NgawiNgab/BedrockTools/blob/c03d3ae0c9022557de0acdf0d94e373e0b9e0579/README.md#L28-L59) documents Android `arm64-v8a` and LeviLauncher and notes that signatures and offsets are game-build-specific. [`levimod.json`](https://github.com/NgawiNgab/BedrockTools/blob/c03d3ae0c9022557de0acdf0d94e373e0b9e0579/levimod.json#L1-L7) declares mod version `1.5.12` and `minecraft_versions: ["1.26.6X.X"]`. Neither is evidence of the project's target build.

| Question | Source finding |
| --- | --- |
| How does a block overlay acquire and filter data? | **CONFIRMED (reference source):** Light Overlay's `s_lightOverlayTickCallback` [captures the player and position](https://github.com/NgawiNgab/BedrockTools/blob/c03d3ae0c9022557de0acdf0d94e373e0b9e0579/src/modules/visual/lightoverlay.cpp#L123-L133). [`_renderLevel_hook`](https://github.com/NgawiNgab/BedrockTools/blob/c03d3ae0c9022557de0acdf0d94e373e0b9e0579/src/modules/visual/lightoverlay.cpp#L247-L304) reads the player's dimension and its `BlockSource`, floors player coordinates, and scans a configured box. It filters by a solidity predicate or compares the returned block with a cached pointer from an out-of-range lookup. The latter is the module's air heuristic, not verified engine behavior. |
| How does that data become visible? | **CONFIRMED (reference source):** Light Overlay checks neighboring positions, rounds the sampled brightness times 15, chooses the configured safe/danger color, and submits camera-relative digit geometry before forwarding to the original render function ([`_renderLevel_hook`](https://github.com/NgawiNgab/BedrockTools/blob/c03d3ae0c9022557de0acdf0d94e373e0b9e0579/src/modules/visual/lightoverlay.cpp#L314-L365)). Radii, face/solid filters, threshold, and colors are [persisted by the module](https://github.com/NgawiNgab/BedrockTools/blob/c03d3ae0c9022557de0acdf0d94e373e0b9e0579/src/modules/visual/lightoverlay.cpp#L445-L479). Chunk Border also submits camera-relative lines, but after its original render call; its bounds come from coordinates rather than chunk block data ([`chunkborder.cpp`](https://github.com/NgawiNgab/BedrockTools/blob/c03d3ae0c9022557de0acdf0d94e373e0b9e0579/src/modules/visual/chunkborder.cpp#L143-L210)). |
| How does block identity affect terrain rendering? | **CONFIRMED (reference source):** Connected Glass reads a name via the offset-based [`Block::fullName()` wrapper](https://github.com/NgawiNgab/BedrockTools/blob/c03d3ae0c9022557de0acdf0d94e373e0b9e0579/include/bedrocktools/sdk/render/Block.hpp#L9-L18). [`classifyGlassName`, `classifyGlass`, and `compatibleGlass`](https://github.com/NgawiNgab/BedrockTools/blob/c03d3ae0c9022557de0acdf0d94e373e0b9e0579/src/modules/visual/connectedglass.cpp#L124-L183) recognize glass blocks and panes and apply shape/color rules. [`renderFace`](https://github.com/NgawiNgab/BedrockTools/blob/c03d3ae0c9022557de0acdf0d94e373e0b9e0579/src/modules/visual/connectedglass.cpp#L351-L392) takes the region from the block tessellator, uses [neighbor checks](https://github.com/NgawiNgab/BedrockTools/blob/c03d3ae0c9022557de0acdf0d94e373e0b9e0579/src/modules/visual/connectedglass.cpp#L201-L224) to determine borders, and can pass cropped texture coordinates to the original function. This changes terrain tessellation rather than drawing a separate overlay. |
| Is there a bounded scanning/cache example? | **CONFIRMED (reference source):** Colored Lights' [`scanSourceCell` and `appendCellSources`](https://github.com/NgawiNgab/BedrockTools/blob/c03d3ae0c9022557de0acdf0d94e373e0b9e0579/src/modules/visual/coloredlights/ColoredLightsFeature.cpp#L283-L333) sample a `16 × 16 × 16` coordinate cell and cache selected position/color/emission values by region and cell coordinates. [`brightnessChangedDetour`](https://github.com/NgawiNgab/BedrockTools/blob/c03d3ae0c9022557de0acdf0d94e373e0b9e0579/src/modules/visual/coloredlights/ColoredLightsFeature.cpp#L623-L635) invalidates nearby cells. This is coordinate sampling, not native subchunk or palette decoding. |

**Compatibility limits and risks:**

- **CONFIRMED (reference source):** Light Overlay [resolves `BlockSourceGetBlock`](https://github.com/NgawiNgab/BedrockTools/blob/c03d3ae0c9022557de0acdf0d94e373e0b9e0579/src/modules/visual/lightoverlay.cpp#L417-L426) and calls it with a region and position. Connected Glass [resolves `BlockSourceGetBlockForTessellation`](https://github.com/NgawiNgab/BedrockTools/blob/c03d3ae0c9022557de0acdf0d94e373e0b9e0579/src/modules/visual/connectedglass.cpp#L560-L565) and calls its wrapper with an additional `0`. The differently named bindings and call sites do not establish the native ABI or the meaning of that argument in the target. The public [`BlockSource` wrapper](https://github.com/NgawiNgab/BedrockTools/blob/c03d3ae0c9022557de0acdf0d94e373e0b9e0579/include/bedrocktools/sdk/world/BlockSource.hpp#L8-L13) exposes only `dimensionId()`, not a portable block reader.
- **CONFIRMED (reference source):** Light Overlay's [default radii](https://github.com/NgawiNgab/BedrockTools/blob/c03d3ae0c9022557de0acdf0d94e373e0b9e0579/src/modules/visual/lightoverlay.hpp#L19-L27) imply `33 × 17 × 33 = 18,513` candidate positions each time the render hook reaches its scan. The entry guard does not check `s_getBlock` before the scan can call it. Neither timing performance nor the air sentinel behavior was tested.
- **UNKNOWN (target):** Block state/runtime IDs, direct chunk/palette layout, unloaded-position semantics, object lifetimes and thread ownership, and a correct world-render pass. The audited paths do not establish these. For any needed native interface, use call-site data flow and controlled observations on the identified build rather than a matching name, signature, or RTTI entry alone.

## Next evidence to collect

1. Continue [launcher, HUD, and host-only design research](xray-port-1.26.62.md#launcher-and-hud-compatibility) without requiring a local library hash. Keep the downloaded ELF, the user-run report, and the loaded game module separately scoped.
2. [XRAY-001](xray-001-evidence.md) identifies a coordinate-lookup candidate, [XRAY-002](xray-002-evidence.md) decodes the related word read, and [XRAY-003](xray-003-evidence.md) rules out the key comparison as a pre-read availability check. [XRAY-004](xray-004-evidence.md) shows packed coordinates, indexed dispatch, and common fallback behavior. [XRAY-005](xray-005-evidence.md) establishes the storage-pointer check, the indirect read handoff, and the separate fixed-key retry.
3. [XRAY-006](xray-006-evidence.md) establishes nullable record selection and storage installation in the report. Independent Drive evidence identifies the uniform reader, block-name key producers, an air-key-derived fallback, and a status-gated acquisition lead. None proves device identity or safe native use. [XRAY-007](xray-007-evidence.md) corroborates the air fallback setup, name handling, and status check in the scanned file. This is static disassembly, not evidence that a block query failed in the live game or that Zaphkiel started too early.
4. [XRAY-008 is complete](xray-008-evidence.md). The eight requested windows and all 340 decoded instructions agree with the separate Drive ELF after formatting normalization. The report corroborates an update-to-region path, chunk-cache mutation with branch-specific status gates, and camera position and matrix-copy paths. It does not identify the full cache file or the loaded module, or establish safe sampling and a viewport mapping. No further static batch is pending.
5. The [host-only outline core](../../README.md#host-only-outline-core) exercises filters, the value cache, and projection with fixtures. The user [confirmed the synthetic HUD showed both boxes, filtered out diamond, and cleared on toggle-off](bedrocktools-integration.md#what-the-ci-result-means). The [loaded Build ID](#loaded-build-id) matches. Live world integration still needs an independently safe block reader and main-pass camera sampling.
6. The [one-block probe passed on device as reported by the user](one-block-world-probe.md#device-result): ore → stone → air at one loaded position, with an unloaded guard-only control. This establishes the reference outcomes through the supported Script API. It does not call or certify a native reader.
7. The [native chunk scheduler](chunk-scanner.md) discovers neighboring ores in host fixtures. Its render-ready feed and existing-only native read remain unbound; the HUD LeviPack does not invoke it.
8. The [passive terrain probe](passive-terrain.md) instead forwards the terrain builder's existing virtual read before layer selection. Its separate artifact starts off; the next gate is the covered and exposed device trial.
