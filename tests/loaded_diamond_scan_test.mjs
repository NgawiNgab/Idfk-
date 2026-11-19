import assert from "node:assert/strict";
import { test } from "node:test";
import { LoadedDiamondScan } from "../probes/loaded-diamonds/scripts/scan.js";
import { createSessionControls, EVENT_ID } from "../probes/loaded-diamonds/scripts/session.js";

const makeVolume = (from, to) => ({ from, to });
const focus = { x: -0.6, y: -32.4, z: 8.5 };
const west = { x: -1, y: -33, z: 8 };
const east = { x: 0, y: -33, z: 9 };
const diamond = "minecraft:diamond_ore";
const deepslateDiamond = "minecraft:deepslate_diamond_ore";
const key = ({ x, y, z }) => `${x},${y},${z}`;
const column = ({ x, z }) => `${Math.floor(x / 16)},${Math.floor(z / 16)}`;
const inVolume = (p, { from, to }) => ["x", "y", "z"].every((axis) => p[axis] >= from[axis] && p[axis] <= to[axis]);

function terrain() {
    const blocks = new Map();
    const denied = new Set();
    const queried = [];
    const pointReads = [];
    const gates = [];
    const dimension = {
        id: "minecraft:overworld",
        isChunkLoaded(position) {
            const loaded = !denied.has(column(position));
            gates.push({ position, loaded });
            return loaded;
        },
        getBlocks(volume, filter, allowUnloaded) {
            queried.push({ volume, filter, allowUnloaded, precedingGate: gates.at(-1) });
            const hits = [];
            for (const { position, type } of blocks.values()) {
                if (inVolume(position, volume) && filter.includeTypes.includes(type)) hits.push(position);
            }
            return { getBlockLocationIterator: () => hits };
        },
        getBlock(position) {
            pointReads.push({ position, precedingGate: gates.at(-1) });
            return { isValid: true, typeId: blocks.get(key(position))?.type ?? "minecraft:air" };
        },
    };
    return {
        dimension, queried, pointReads, gates, denied,
        put(position, type) { blocks.set(key(position), { position, type }); },
    };
}

function ticks(scan, dimension, position = focus, count = 72) {
    for (let i = 0; i < count; i++) assert.equal(scan.step(dimension, position), true);
    return scan.report(dimension, position);
}

test("finds both kinds of diamonds in settled adjacent chunks without supplied coordinates", () => {
    const f = terrain();
    f.put(west, diamond);
    f.put(east, deepslateDiamond);
    f.put({ ...east, x: 1 }, "minecraft:gold_ore");
    f.put({ ...west, x: -17 }, diamond);
    f.put({ ...east, x: 16 }, diamond);
    f.denied.add("-2,0");
    const scan = new LoadedDiamondScan(makeVolume);
    const report = ticks(scan, f.dimension);
    assert.deepEqual(report, {
        sampled: 64, total: 72, minY: -52, maxY: -21, sweeps: 1, capped: false,
        sampledColumns: 8, matchColumns: 2, examples: [west, east], queries: 64, failures: 0,
    });
    assert.equal(f.queried.length, 64);
    const unique = new Set();
    for (const { volume: { from, to }, filter, allowUnloaded, precedingGate } of f.queried) {
        assert.deepEqual(filter.includeTypes, [diamond, deepslateDiamond]);
        assert.equal(allowUnloaded, false);
        assert.deepEqual(precedingGate, { position: from, loaded: true });
        assert.equal(column(from), column(to));
        assert.deepEqual([to.x - from.x, to.y - from.y, to.z - from.z], [15, 3, 15]);
        assert.notEqual(column(from), "-2,0");
        assert.ok([-32, -16, 0].includes(from.x));
        assert.ok([-16, 0, 16].includes(from.z));
        assert.ok(from.y >= report.minY && to.y <= report.maxY);
        unique.add(key(from));
    }
    assert.equal(unique.size, 64);
    // One report per represented column, and no new volume query for status.
    assert.equal(f.gates.length, 72 + 8);
});

test("stationary sweeps replace mined/stone hits and discover an ore that was not previously cached", () => {
    const f = terrain();
    const original = { ...west };
    f.put(original, diamond);
    f.put(east, deepslateDiamond);
    const scan = new LoadedDiamondScan(makeVolume);
    const first = ticks(scan, f.dimension);
    assert.deepEqual(first.examples, [west, east]);
    // Changing the query's returned object or the result copy can't mutate the cache.
    original.x = -10;
    first.examples[0].x = -9;
    assert.deepEqual(scan.report(f.dimension, focus).examples, [west, east]);
    f.put(west, "minecraft:stone");
    f.put(east, "minecraft:air");
    const newOre = { x: 3, y: -41, z: 5 };
    f.put(newOre, deepslateDiamond);
    const second = ticks(scan, f.dimension);
    assert.deepEqual(second.examples, [newOre]);
    assert.equal(second.matchColumns, 1);
    assert.equal(second.sweeps, 2);
    assert.equal(second.queries, 144);
});

test("unloaded and indeterminate columns are never queried, and their old results cannot reappear", () => {
    const f = terrain();
    f.put(west, diamond);
    const scan = new LoadedDiamondScan(makeVolume);
    assert.deepEqual(ticks(scan, f.dimension).examples, [west]);
    f.denied.add(column(west));
    const afterUnload = scan.report(f.dimension, focus);
    assert.equal(afterUnload.sampled, 64);
    assert.deepEqual(afterUnload.examples, []);
    f.denied.clear();
    assert.deepEqual(scan.report(f.dimension, focus).examples, []);
    for (const gate of [undefined, () => 1, () => { throw new Error("expired"); }]) {
        f.dimension.isChunkLoaded = gate;
        const report = ticks(scan, f.dimension);
        assert.equal(report.sampled, 0);
        assert.deepEqual(report.examples, []);
        assert.equal(f.queried.length, 72);
    }
});

test("a failed re-read is unavailable, not an empty or stale slab", () => {
    for (const fail of [
        () => { throw new Error("chunk disappeared after the gate"); },
        () => ({ getBlockLocationIterator: () => [{ ...west, y: 9999 }] }),
        () => ({ *getBlockLocationIterator() { yield west; throw new Error("iterator expired"); } }),
    ]) {
        const f = terrain();
        f.put(west, diamond);
        const scan = new LoadedDiamondScan(makeVolume);
        assert.deepEqual(ticks(scan, f.dimension).examples, [west]);
        const query = f.dimension.getBlocks;
        f.dimension.getBlocks = function (volume, ...args) {
            if (inVolume(west, volume)) return fail();
            return query.call(this, volume, ...args);
        };
        const report = ticks(scan, f.dimension);
        assert.equal(report.sampled, 71);
        assert.equal(report.failures, 1);
        assert.deepEqual(report.examples, []);
    }
});

test("window movement drops out-of-range slices, and old overlap expires without a revisit", () => {
    const f = terrain();
    const distant = { x: 31, y: -45, z: 31 };
    const nearOrigin = { x: 2, y: -32, z: 2 };
    f.put(distant, diamond);
    f.put(nearOrigin, deepslateDiamond);
    const scan = new LoadedDiamondScan(makeVolume);
    const center = { x: 2.5, y: -32, z: 2.5 };
    assert.deepEqual(ticks(scan, f.dimension, center).examples, [nearOrigin, distant]);
    const shifted = { ...center, x: 48 };
    const report = scan.report(f.dimension, shifted);
    assert.equal(report.sweeps, 0);
    assert.equal(report.sampled, 0);
    assert.deepEqual(report.examples, []);
    assert.deepEqual(ticks(scan, f.dimension, center).examples, [nearOrigin, distant]);
    // Repeated changes to the four-block Y anchor keep the distant slice in
    // range but don't let the cursor reach it. Don't show it indefinitely.
    for (let i = 0; i < 72; i++) scan.step(f.dimension, { ...center, y: i % 2 ? -31.9 : -32.1 });
    assert.deepEqual(scan.report(f.dimension, center).examples, [nearOrigin]);
    assert.equal(scan.step(f.dimension, { ...center, x: NaN }), false);
    assert.equal(scan.report(f.dimension, center).sampled, 0);
});

test("clamps the window at both build heights and the horizontal limit", () => {
    for (const [y, min, max, total] of [[-90, -64, -49, 16], [360, 300, 319, 20]]) {
        const f = terrain();
        const p = { x: -30_000_000, y, z: 29_999_999.9 };
        const report = ticks(new LoadedDiamondScan(makeVolume), f.dimension, p, total);
        assert.deepEqual([report.minY, report.maxY, report.sampled], [min, max, total]);
        for (const { volume: { from, to } } of f.queried) {
            assert.ok(from.y >= min && to.y <= max);
            assert.ok(from.x >= -30_000_000 && to.z < 30_000_000);
            assert.equal(column(from), column(to));
        }
    }
});

test("retains one nearest sampled example per column and caps inspection of dense slices", () => {
    const f = terrain();
    for (let x = -32; x < 16; x += 16) {
        for (let z = -16; z < 32; z += 16) {
            f.put({ x, y: -32, z }, diamond);
            f.put({ x, y: -35, z }, deepslateDiamond);
        }
    }
    const report = ticks(new LoadedDiamondScan(makeVolume), f.dimension);
    assert.equal(report.matchColumns, 9);
    assert.equal(report.examples.length, 4);
    assert.equal(new Set(report.examples.map(column)).size, 4);
    assert.deepEqual(report.examples[0], { x: 0, y: -32, z: 16 });

    let yielded = 0;
    f.dimension.getBlocks = ({ from }) => ({
        *getBlockLocationIterator() {
            for (let i = 0; i < 1024; i++) {
                ++yielded;
                yield { x: from.x + i % 16, y: from.y + Math.floor(i / 256), z: from.z + (i >> 4) % 16 };
            }
        },
    });
    const dense = ticks(new LoadedDiamondScan(makeVolume), f.dimension, focus, 1);
    assert.equal(yielded, 33); // One lookahead beyond the 32 inspected matches.
    assert.equal(dense.capped, true);
    assert.equal(dense.matchColumns, 1);
});

function sessionFixture({ hasRenderer = true } = {}) {
    const f = terrain();
    const hud = [];
    const chat = [];
    const shapes = new Set();
    const added = [];
    const player = {
        id: "offline-player", typeId: "minecraft:player", isValid: true,
        dimension: f.dimension, location: { ...focus },
        getHeadLocation() { return { ...this.location, y: this.location.y + 1.62 }; },
        getViewDirection() { return { x: 0, y: -1, z: 0 }; },
        onScreenDisplay: { setActionBar: (line) => hud.push(line) },
        sendMessage: (line) => chat.push(line),
    };
    const players = [player];
    const callbacks = new Map();
    let lastId = 0;
    const system = {
        currentTick: 0,
        runInterval(fn, interval) { assert.equal(interval, 1); callbacks.set(++lastId, fn); return lastId; },
        clearRun(id) { callbacks.delete(id); },
    };
    const world = {
        getAllPlayers: () => players,
        primitiveShapesManager: hasRenderer ? {
            addText(shape) { shapes.add(shape); added.push(shape); },
        } : undefined,
    };
    const makeText = (location, glyph) => ({
        location, glyph,
        setLocation(next) { this.location = next; },
        remove() { shapes.delete(this); },
    });
    const controls = createSessionControls(world, system, makeVolume, makeText);
    return {
        ...f, player, players, hud, chat, callbacks, controls, shapes, added,
        command(message, extra = {}) {
            controls.command({ id: EVENT_ID, message, sourceType: "Entity", sourceEntity: player, ...extra });
        },
        advance(count, doubleDelivery = false) {
            for (let i = 0; i < count; i++) {
                ++system.currentTick;
                for (const callback of callbacks.values()) {
                    callback();
                    if (doubleDelivery) callback();
                }
            }
        },
    };
}

test("explicit start/stop, live tracer without HUD text, and at most one volume query per tick", () => {
    const f = sessionFixture();
    f.put(west, diamond);
    f.put(east, deepslateDiamond);
    f.advance(80);
    assert.equal(f.queried.length, 0);
    f.command("start");
    f.command("start");
    f.command("status");
    assert.equal(f.callbacks.size, 1);
    assert.equal(f.queried.length, 0);
    const [late] = f.callbacks.values();
    f.advance(80, true);
    assert.equal(f.queried.length, 80);
    assert.equal(f.shapes.size, 12);
    assert.equal(f.added.length, 12);
    assert.deepEqual(f.hud, []);
    const pointCount = f.pointReads.length;
    assert.ok(pointCount > 0 && pointCount <= 2 * Math.ceil(80 / 6));
    for (const { position, precedingGate } of f.pointReads) assert.deepEqual(precedingGate, { position, loaded: true });
    f.command("status");
    assert.equal(f.queried.length, 80);
    assert.equal(f.pointReads.length, pointCount);
    assert.match(f.chat.at(-4), /72\/72 slices sampled in 9 loaded chunks/);
    assert.match(f.chat.at(-3), /Samples in 2 chunks: -1, -33, 8 \| 0, -33, 9/);
    assert.match(f.chat.at(-1), /Tracer: locked on -1, -33, 8/);
    f.command("stop");
    assert.equal(f.shapes.size, 0);
    assert.equal(f.callbacks.size, 0);
    f.command("start");
    late();
    const [newCallback] = f.callbacks.values();
    newCallback(); // Rearming during an already-scanned tick does not double the budget.
    assert.equal(f.queried.length, 80);
    assert.equal(f.shapes.size, 0);
    f.advance(1);
    assert.equal(f.queried.length, 81);
    assert.equal(f.shapes.size, 12); // A fresh sample may be available on the first new tick.
});

test("a mined target retargets before its slab is swept again and a far move clears immediately", () => {
    const f = sessionFixture();
    f.put(west, diamond);
    f.put(east, deepslateDiamond);
    f.command("start");
    f.advance(80);
    const marker = [...f.shapes].find((shape) => shape.glyph === "+");
    assert.equal(marker.location.x, west.x + 0.5);
    f.put(west, "minecraft:stone");
    f.advance(6);
    assert.equal(marker.location.x, east.x + 0.5);
    assert.equal(f.added.length, 12);
    f.player.location.x = 48;
    f.advance(1);
    assert.equal(f.shapes.size, 0);
    assert.equal(f.callbacks.size, 1);
});

test("disconnect and dimension changes remove an active tracer", () => {
    for (const leave of [
        (f) => f.controls.playerLeave({ playerId: f.player.id }),
        (f) => f.controls.dimensionChange({ player: f.player }),
    ]) {
        const f = sessionFixture();
        f.put(west, diamond);
        f.command("start");
        f.advance(72);
        assert.equal(f.shapes.size, 12);
        leave(f);
        assert.equal(f.shapes.size, 0);
        assert.equal(f.callbacks.size, 0);
    }
});

test("a drawing failure reports once and leaves the scan available via status", () => {
    const f = sessionFixture({ hasRenderer: false });
    f.put(west, diamond);
    f.command("start");
    f.advance(80);
    assert.equal(f.shapes.size, 0);
    assert.equal(f.chat.filter((line) => line.includes("Could not draw")).length, 1);
    f.command("status");
    assert.match(f.chat.at(-1), /Tracer: unavailable/);
    assert.match(f.chat.at(-4), /72\/72 slices sampled/);
});

test("non-player commands, another player, dimension change and leave do not run the scanner", () => {
    const f = sessionFixture();
    for (const overrides of [{ id: "idfk:other" }, { sourceType: "Block" },
        { sourceType: "Server" }, { sourceEntity: undefined }, { sourceEntity: { typeId: "minecraft:pig" } }]) {
        f.command("start", overrides);
    }
    f.dimension.id = "minecraft:nether";
    f.command("start");
    f.dimension.id = "minecraft:overworld";
    f.players.push({ id: "someone-else" });
    f.command("start");
    assert.equal(f.callbacks.size, 0);
    assert.equal(f.queried.length, 0);
    f.players.pop();

    f.command("start");
    f.players.push({ id: "someone-else" });
    f.advance(1);
    assert.equal(f.callbacks.size, 0);
    f.players.pop();
    f.command("start");
    f.controls.dimensionChange({ player: f.player });
    f.advance(1);
    assert.equal(f.callbacks.size, 0);
    f.command("start");
    f.controls.playerLeave({ playerId: f.player.id });
    f.advance(1);
    assert.equal(f.callbacks.size, 0);
    assert.equal(f.queried.length, 0);
});

test("missing volume API does not start or fall back to individual reads", () => {
    const f = sessionFixture();
    delete f.dimension.getBlocks;
    let fallback = 0;
    f.dimension.getBlock = () => { ++fallback; };
    f.command("start");
    f.advance(72);
    assert.match(f.chat.at(-1), /Script API unavailable/);
    assert.equal(f.callbacks.size, 0);
    assert.equal(fallback, 0);
});
