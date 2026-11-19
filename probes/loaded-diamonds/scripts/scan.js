// Only copied coordinates and counters leave a synchronous script tick.
const CHUNK = 16;
const SLAB = 4;
const HALF_HEIGHT = 16;
const MIN_Y = -64;
const MAX_Y = 319;
const XZ_LIMIT = 30_000_000;
const MAX_SLICES = 9 * (2 * HALF_HEIGHT / SLAB);
const MAX_MATCHES = 32;
const TYPES = ["minecraft:diamond_ore", "minecraft:deepslate_diamond_ore"];

function anchorFor(p) {
    if (!p || ![p.x, p.y, p.z].every(Number.isFinite) ||
        p.x < -XZ_LIMIT || p.x >= XZ_LIMIT || p.z < -XZ_LIMIT || p.z >= XZ_LIMIT) return null;
    return {
        x: Math.floor(p.x / CHUNK) * CHUNK,
        y: Math.floor(Math.max(MIN_Y, Math.min(MAX_Y, p.y)) / SLAB) * SLAB,
        z: Math.floor(p.z / CHUNK) * CHUNK,
    };
}

function distance(a, b) {
    return (a.x - b.x) ** 2 + (a.y - b.y) ** 2 + (a.z - b.z) ** 2;
}

function compareNear(a, b, focus) {
    return distance(a, focus) - distance(b, focus) || a.x - b.x || a.y - b.y || a.z - b.z;
}

function makeWindow(anchor, focus) {
    const minY = Math.max(MIN_Y, anchor.y - HALF_HEIGHT);
    const maxY = Math.min(MAX_Y, anchor.y + HALF_HEIGHT - 1);
    const slices = [];
    for (let x = anchor.x - CHUNK; x <= anchor.x + CHUNK; x += CHUNK) {
        for (let z = anchor.z - CHUNK; z <= anchor.z + CHUNK; z += CHUNK) {
            if (x < -XZ_LIMIT || x >= XZ_LIMIT || z < -XZ_LIMIT || z >= XZ_LIMIT) continue;
            const column = `${x},${z}`;
            for (let y = minY; y <= maxY; y += SLAB) {
                const from = { x, y, z };
                const to = { x: x + CHUNK - 1, y: y + SLAB - 1, z: z + CHUNK - 1 };
                const center = { x: x + CHUNK / 2, y: y + SLAB / 2, z: z + CHUNK / 2 };
                slices.push({ key: `${column},${y}`, column, from, to, priority: distance(center, focus) });
            }
        }
    }
    slices.sort((a, b) => a.priority - b.priority);
    return { slices, minY, maxY, cursor: 0, sweeps: 0 };
}

function loaded(dimension, from) {
    try {
        return dimension.isChunkLoaded({ ...from }) === true;
    } catch {
        return false;
    }
}

function sampleSlice(dimension, makeVolume, slice, focus) {
    const { from, to } = slice;
    const volume = makeVolume({ ...from }, { ...to });
    // false means an unloaded volume must fail, not silently return a partial result.
    const matches = dimension.getBlocks(volume, { includeTypes: [...TYPES] }, false);
    let nearest = null;
    let inspected = 0;
    for (const match of matches.getBlockLocationIterator()) {
        if (inspected++ === MAX_MATCHES) return { nearest, capped: true };
        const { x, y, z } = match;
        if (![x, y, z].every(Number.isInteger) || x < from.x || x > to.x ||
            y < from.y || y > to.y || z < from.z || z > to.z) {
            throw new Error("Volume yielded an invalid match");
        }
        const pos = { x, y, z };
        if (!nearest || compareNear(pos, nearest, focus) < 0) nearest = pos;
    }
    return { nearest, capped: false };
}

export class LoadedDiamondScan {
    #makeVolume;
    #anchorKey = "";
    #window;
    #samples = new Map();
    #step = 0;
    #queries = 0;
    #failures = 0;

    constructor(makeVolume) {
        this.#makeVolume = makeVolume;
    }

    #setWindow(focus) {
        const anchor = anchorFor(focus);
        if (!anchor) {
            this.#anchorKey = "";
            this.#window = undefined;
            this.#samples.clear();
            return false;
        }
        const key = `${anchor.x},${anchor.y},${anchor.z}`;
        if (key === this.#anchorKey) return true;
        this.#anchorKey = key;
        this.#window = makeWindow(anchor, focus);
        const keep = new Set(this.#window.slices.map((slice) => slice.key));
        for (const sample of this.#samples.keys()) {
            if (!keep.has(sample)) this.#samples.delete(sample);
        }
        return true;
    }

    // Called at most once per script tick. No retries or catch-up.
    step(dimension, focus) {
        if (!this.#setWindow(focus)) return false;
        const window = this.#window;
        const slice = window.slices[window.cursor++];
        if (window.cursor === window.slices.length) {
            window.cursor = 0;
            ++window.sweeps;
        }
        ++this.#step;
        if (!loaded(dimension, slice.from)) {
            for (const other of window.slices) {
                if (other.column === slice.column) this.#samples.delete(other.key);
            }
            return true;
        }
        ++this.#queries;
        try {
            const sample = sampleSlice(dimension, this.#makeVolume, slice, focus);
            this.#samples.set(slice.key, { ...sample, step: this.#step });
        } catch {
            ++this.#failures;
            this.#samples.delete(slice.key);
        }
        return true;
    }

    // A report does not run a volume query. Check each represented column
    // again before using its latest slice samples, which are not atomic.
    report(dimension, focus) {
        if (!this.#setWindow(focus)) return null;
        const columns = new Map();
        const best = new Map();
        let sampled = 0;
        let capped = false;
        for (const slice of this.#window.slices) {
            const sample = this.#samples.get(slice.key);
            if (!sample) continue;
            if (this.#step - sample.step >= MAX_SLICES) {
                this.#samples.delete(slice.key);
                continue;
            }
            if (!columns.has(slice.column)) columns.set(slice.column, loaded(dimension, slice.from));
            if (!columns.get(slice.column)) {
                this.#samples.delete(slice.key);
                continue;
            }
            ++sampled;
            capped ||= sample.capped;
            const { nearest } = sample;
            const previous = best.get(slice.column);
            if (nearest && (!previous || compareNear(nearest, previous, focus) < 0)) best.set(slice.column, nearest);
        }
        const { slices, minY, maxY, sweeps } = this.#window;
        return {
            sampled, total: slices.length, minY, maxY, sweeps, capped,
            sampledColumns: [...columns.values()].filter(Boolean).length,
            matchColumns: best.size,
            examples: [...best.values()].sort((a, b) => compareNear(a, b, focus)).slice(0, 4).map((p) => ({ ...p })),
            queries: this.#queries, failures: this.#failures,
        };
    }
}
