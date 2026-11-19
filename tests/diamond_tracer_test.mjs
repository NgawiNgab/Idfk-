import assert from "node:assert/strict";
import { test } from "node:test";
import { isDiamondAt } from "../probes/loaded-diamonds/scripts/scan.js";
import { chooseDiamond, DiamondTracer, tracerMarkers } from "../probes/loaded-diamonds/scripts/tracer.js";

const a = { x: -4, y: -45, z: 96 };
const b = { x: 3, y: -45, z: 96 };
const ore = "minecraft:deepslate_diamond_ore";
const air = "minecraft:air";
const key = ({ x, y, z }) => `${x},${y},${z}`;

function targets() {
    const examples = [a, b];
    const denied = new Set();
    const outside = new Set();
    const data = new Map(examples.map((p) => [key(p), ore]));
    const reads = [];
    const gates = [];
    return {
        examples, denied, outside, data, reads, gates,
        scan: { report: () => ({ examples }), contains: (p) => !outside.has(key(p)) },
        dimension: {
            isChunkLoaded(p) {
                gates.push(key(p));
                return !denied.has(key(p));
            },
            getBlock(p) {
                reads.push(key(p));
                return { isValid: true, typeId: data.get(key(p)) ?? air };
            },
        },
    };
}

test("a valid lock survives near ties and a sample change; a clearly closer diamond can take over", () => {
    const f = targets();
    const at = (x) => ({ x, y: -44.5, z: 96.5 });
    const first = chooseDiamond(f.scan, f.dimension, at(-0.5), null);
    assert.deepEqual(first, a);
    assert.notEqual(first, a); // Copy, not the report's own object.
    f.examples.shift(); // The old target isn't a nearest example anymore.
    for (const x of [0.1, -0.1, 0.8, 1.2, 1.5]) {
        assert.equal(chooseDiamond(f.scan, f.dimension, at(x), first), first);
    }
    assert.deepEqual(chooseDiamond(f.scan, f.dimension, at(2), first), b);
});

test("old target recheck rejects stone/air; unloaded or out-of-window targets are not read", () => {
    const p = { x: -3, y: -45, z: 98 };
    for (const makeInvalid of [
        (f) => f.data.set(key(a), "minecraft:stone"),
        (f) => f.data.set(key(a), air),
        (f) => f.denied.add(key(a)),
        (f) => f.outside.add(key(a)),
    ]) {
        const f = targets();
        makeInvalid(f);
        // Cached report still includes the stale first coordinate.
        assert.deepEqual(chooseDiamond(f.scan, f.dimension, p, a), b);
        const expected = f.denied.size || f.outside.size ? [key(b)] : [key(a), key(b)];
        assert.deepEqual(f.reads, expected);
    }
});

test("point rechecks use a strict loaded guard, tolerate disappearing blocks, and enforce two attempts", () => {
    for (const gate of [() => false, () => 1, undefined, () => { throw new Error("unavailable"); }]) {
        let reads = 0;
        assert.equal(isDiamondAt({ isChunkLoaded: gate, getBlock() { ++reads; } }, a), false);
        assert.equal(reads, 0);
    }
    for (const getBlock of [() => undefined, () => ({ isValid: false, typeId: ore }), () => { throw new Error("expired"); }]) {
        assert.equal(isDiamondAt({ isChunkLoaded: () => true, getBlock }, a), false);
    }
    const f = targets();
    f.examples.push({ ...a, x: -5 }, { ...a, x: 10 });
    f.data.clear();
    assert.equal(chooseDiamond(f.scan, f.dimension, a, a), null);
    assert.equal(f.reads.length, 2);
    assert.deepEqual(f.gates, f.reads);
});

test("points are world-aligned with the block center and are spaced evenly in perspective", () => {
    const eye = { x: -4, y: -44, z: 90 };
    const view = { x: 0, y: 0, z: 2 };
    const points = tracerMarkers(eye, view, a);
    assert.equal(points.length, 12);
    assert.deepEqual(points.at(-1).position, { x: -3.5, y: -44.5, z: 96.5 });
    assert.equal(points.at(-1).glyph, "+");
    assert.equal(points[0].glyph, "\u2022");
    assert.ok(points[0].position.y < eye.y);
    const projected = points.map(({ position: p, scale }) => {
        assert.ok(Number.isFinite(scale) && scale > 0 && scale <= 4);
        assert.ok(p.z > eye.z);
        return [(p.x - eye.x) / (p.z - eye.z), (p.y - eye.y) / (p.z - eye.z)];
    });
    for (let i = 1; i < projected.length; i++) {
        for (let axis = 0; axis < 2; axis++) {
            const step = projected[i][axis] - projected[i - 1][axis];
            assert.ok(Math.abs(step - (projected[1][axis] - projected[0][axis])) < 1e-12);
        }
    }
});

test("tracer handles vertical view, clips behind the eye and rejects invalid or distant input", () => {
    const eye = { x: -3.5, y: -43, z: 96.5 };
    const down = { x: 0, y: -1, z: 0 };
    assert.equal(tracerMarkers(eye, down, a).length, 12);
    for (const invalid of [
        { eye, view: { ...down, y: 1 }, target: a },
        { eye, view: { ...down, y: 0 }, target: a },
        { eye: { ...eye, x: NaN }, view: down, target: a },
        { eye, view: down, target: { ...a, y: -500 } },
    ]) assert.deepEqual(tracerMarkers(invalid.eye, invalid.view, invalid.target), []);
});

function display({ failAt = Infinity } = {}) {
    const shapes = new Set();
    const foreign = { external: true };
    shapes.add(foreign);
    let added = 0;
    const dimension = { id: "minecraft:overworld" };
    const trace = new DiamondTracer((location, glyph) => ({
        location, glyph,
        setLocation(next) { this.location = next; },
        remove() { shapes.delete(this); },
    }), () => ({
        addText(shape, toDimension) {
            assert.equal(dimension, toDimension);
            shapes.add(shape);
            if (++added === failAt) throw new Error("failed to add text primitive");
        },
    }));
    let eye = { x: -4, y: -44, z: 90 };
    let view = { x: 0, y: 0, z: 1 };
    const player = { getHeadLocation: () => eye, getViewDirection: () => view };
    return {
        shapes, trace, player, dimension, foreign,
        look(newEye, newView) { eye = newEye; view = newView; },
        get added() { return added; },
    };
}

test("draws only scoped no-depth markers, moves the same handles, renews and clears only its own", () => {
    const f = display();
    f.trace.update(f.player, f.dimension, a, 1);
    assert.equal(f.added, 12);
    const owned = [...f.shapes].slice(1);
    for (const marker of owned) {
        assert.deepEqual(marker.visibleTo, [f.player]);
        assert.equal(marker.location.dimension, f.dimension);
        assert.equal(marker.depthTest, false);
        assert.equal(marker.useRotation, false);
        assert.equal(marker.backgroundColorOverride.alpha, 0);
        assert.equal(marker.timeLeft, 1);
        marker.timeLeft = 0.5; // Stand-in for elapsed engine time.
    }
    const start = owned[0].location;
    f.look({ x: -5, y: -44, z: 90 }, { x: 0, y: 0, z: 1 });
    f.trace.update(f.player, f.dimension, a, 9);
    assert.equal(f.added, 12);
    assert.notEqual(owned[0].location, start);
    assert.ok(owned.every((marker) => marker.timeLeft === 1));
    assert.ok(owned.every((marker) => f.shapes.has(marker)));
    f.look({ x: -5, y: -44, z: 90 }, { x: 0, y: 0, z: -1 });
    f.trace.update(f.player, f.dimension, a, 11);
    assert.deepEqual([...f.shapes], [f.foreign]);
    f.trace.clear();
    assert.deepEqual([...f.shapes], [f.foreign]);
});

test("rolls back an incomplete tracer if the engine rejects a shape", () => {
    const f = display({ failAt: 4 });
    assert.throws(() => f.trace.update(f.player, f.dimension, a, 1), /failed to add/);
    assert.deepEqual([...f.shapes], [f.foreign]);
});
