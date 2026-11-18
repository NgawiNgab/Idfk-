import { deepEqual, equal } from "node:assert/strict";
import { test } from "node:test";
import { CONTROL_POS, EVENT_ID, TEST_POS, handleProbeEvent, sampleControl, samplePosition } from
    "../probes/one-block/scripts/probe.js";

function fixture(nearBlockId) {
    const calls = [];
    const messages = [];
    const dimension = {
        id: "minecraft:overworld",
        isChunkLoaded(position) {
            calls.push(["gate", position]);
            return position.x === TEST_POS.x;
        },
        getBlock(position) {
            calls.push(["getBlock", position]);
            return { isValid: true, typeId: nearBlockId };
        },
    };
    const player = {
        typeId: "minecraft:player",
        dimension,
        location: { x: 2.5, y: 105, z: 2.5 },
        sendMessage(message) { messages.push(message); },
    };
    const event = { id: EVENT_ID, message: "sample", sourceType: "Entity", sourceEntity: player };
    const world = { getAllPlayers: () => [player] };
    return { calls, messages, dimension, player, event, world };
}

for (const blockId of ["minecraft:diamond_ore", "minecraft:stone", "minecraft:air"]) {
    test(`reads ${blockId} only after the local gate`, () => {
        const { calls, messages, event, world } = fixture(blockId);
        handleProbeEvent(event, world);
        deepEqual(calls, [["gate", TEST_POS], ["getBlock", TEST_POS], ["gate", CONTROL_POS]]);
        deepEqual(messages, [
            `[One Block] 0 100 0: ${blockId}; chunk=loaded; getBlock=1`,
            "[One Block] Control: chunk=unloaded; getBlock=0",
        ]);
    });
}

test("false, indeterminate, missing and throwing gates never call getBlock", () => {
    for (const [gate, state] of [
        [() => false, "unloaded"],
        [() => 1, "unknown"],
        [undefined, "unknown"],
        [() => { throw new Error("unavailable"); }, "unknown"],
    ]) {
        let reads = 0;
        const dimension = { isChunkLoaded: gate, getBlock() { ++reads; } };
        deepEqual(samplePosition(dimension), { chunk: state, calls: 0, typeId: null });
        equal(reads, 0);
    }
});

test("a disappearing block or failed read is unavailable, not air", () => {
    for (const read of [
        () => undefined,
        () => ({ isValid: false, typeId: "minecraft:air" }),
        () => ({ isValid: true, get typeId() { throw new Error("expired block"); } }),
        () => { throw new Error("invalid chunk"); },
    ]) {
        const dimension = { isChunkLoaded: () => true, getBlock: read };
        deepEqual(samplePosition(dimension), { chunk: "loaded", calls: 1, typeId: null });
    }
    deepEqual(samplePosition({ isChunkLoaded: () => true }), { chunk: "loaded", calls: 0, typeId: null });
});

test("control never calls getBlock even when unexpectedly loaded", () => {
    const f = fixture();
    f.dimension.isChunkLoaded = (p) => { f.calls.push(["gate", p]); return true; };
    deepEqual(sampleControl(f.dimension), { chunk: "loaded", calls: 0 });
    deepEqual(f.calls, [["gate", CONTROL_POS]]);
});

test("wrong command and automated event sources are ignored", () => {
    const f = fixture("minecraft:stone");
    for (const overrides of [{ id: "idfk:other" }, { message: "scan" }, { sourceType: "Block" },
        { sourceType: "Server" }, { sourceEntity: undefined }]) {
        handleProbeEvent({ ...f.event, ...overrides }, f.world);
    }
    deepEqual(f.calls, []);
    deepEqual(f.messages, []);
});

test("rejects other dimensions, nonlocal chunk and a second player before any probe", () => {
    const f = fixture("minecraft:stone");
    f.dimension.id = "minecraft:nether";
    handleProbeEvent(f.event, f.world);
    f.dimension.id = "minecraft:overworld";
    f.player.location.x = 16;
    handleProbeEvent(f.event, f.world);
    f.player.location.x = 2.5;
    f.world.getAllPlayers = () => [f.player, {}];
    handleProbeEvent(f.event, f.world);
    deepEqual(f.calls, []);
    equal(f.messages.length, 3);
});
