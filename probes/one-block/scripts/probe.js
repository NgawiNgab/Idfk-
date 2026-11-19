export const EVENT_ID = "idfk:one_block_probe";
export const TEST_POS = Object.freeze({ x: 0, y: 100, z: 0 });
export const CONTROL_POS = Object.freeze({ x: 524288, y: 100, z: 524288 });

function chunkState(dimension, position) {
    try {
        const loaded = dimension.isChunkLoaded({ ...position });
        if (loaded === true) return "loaded";
        if (loaded === false) return "unloaded";
    } catch {
        // A missing API or context is not permission to fall back to getBlock.
    }
    return "unknown";
}

export function samplePosition(dimension) {
    const sample = { chunk: chunkState(dimension, TEST_POS), calls: 0, typeId: null };
    if (sample.chunk !== "loaded") return sample;

    try {
        if (typeof dimension.getBlock !== "function") return sample;
        sample.calls = 1;
        const block = dimension.getBlock({ ...TEST_POS });
        if (block?.isValid !== true) return sample;
        const typeId = block.typeId;
        if (typeof typeId === "string" && typeId.length > 0) sample.typeId = typeId;
    } catch {
        // The context may have become invalid. Leave typeId null, not air.
    }
    return sample;
}

export function sampleControl(dimension) {
    // Guard only. Do not read this position even if already unexpectedly loaded.
    return { chunk: chunkState(dimension, CONTROL_POS), calls: 0 };
}

export function handleProbeEvent(event, world) {
    if (event.id !== EVENT_ID || event.sourceType !== "Entity" || event.message.trim() !== "sample") return;
    const player = event.sourceEntity;

    try {
        if (player?.typeId !== "minecraft:player") return;
        const { dimension, location } = player;
        if (world.getAllPlayers().length !== 1 || dimension.id !== "minecraft:overworld" ||
            !(0 <= location.x && location.x < 16 && 0 <= location.z && location.z < 16)) {
            player.sendMessage("[One Block] Use one player in an offline Overworld at X/Z 0 through 15.");
            return;
        }

        // Same synchronous callback: check, read at most once, copy a string.
        // No Block or Dimension object is retained or passed to the HUD mod.
        const near = samplePosition(dimension);
        const control = sampleControl(dimension);
        player.sendMessage(`[One Block] 0 100 0: ${near.typeId ?? "UNAVAILABLE"}; ` +
                           `chunk=${near.chunk}; getBlock=${near.calls}`);
        player.sendMessage(`[One Block] Control: chunk=${control.chunk}; getBlock=${control.calls}`);
    } catch {
        // On world exit most player operations may fail, including sendMessage.
        try {
            player.sendMessage("[One Block] UNAVAILABLE: player or world context expired.");
        } catch {
            // No valid recipient remains.
        }
    }
}
