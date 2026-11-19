import { isDiamondAt } from "./scan.js";

const SWITCH_MARGIN = 3;
const POINT_BUDGET = 2;
const MAX_DISTANCE = 64;
const FORWARD_MIN = 0.25;
const MARKER_COUNT = 12; // Includes the + at the actual block center.
const MARKER_LIFETIME = 1; // Seconds; also removed explicitly on stop/leave.
const RENEW_TICKS = 8;
const CYAN = { red: 0.24, green: 0.97, blue: 1, alpha: 1 };
const CLEAR = { red: 0, green: 0, blue: 0, alpha: 0 };

const finite = (p) => p && Number.isFinite(p.x) && Number.isFinite(p.y) && Number.isFinite(p.z);
const equal = (a, b) => a && b && a.x === b.x && a.y === b.y && a.z === b.z;
const center = ({ x, y, z }) => ({ x: x + 0.5, y: y + 0.5, z: z + 0.5 });
const delta = (a, b) => ({ x: a.x - b.x, y: a.y - b.y, z: a.z - b.z });
const length = ({ x, y, z }) => Math.hypot(x, y, z);
const dot = (a, b) => a.x * b.x + a.y * b.y + a.z * b.z;

export function chooseDiamond(scan, dimension, focus, previous) {
    const report = scan.report(dimension, focus);
    if (!report) return null;

    let remaining = POINT_BUDGET;
    const canUse = (pos) => {
        if (!remaining || !scan.contains(pos) || length(delta(center(pos), focus)) > MAX_DISTANCE) return false;
        --remaining;
        return isDiamondAt(dimension, pos);
    };
    const locked = previous && canUse(previous) ? previous : null;
    const lockedDistance = locked ? length(delta(center(locked), focus)) : Infinity;
    // Up to four copied examples in different columns. Don't recheck the old
    // target from this same list, even if it failed its point check above.
    const candidates = report.examples.map((pos) => ({ pos, distance: length(delta(center(pos), focus)) }));
    candidates.sort((a, b) => a.distance - b.distance);
    for (const { pos, distance } of candidates) {
        if (!remaining || distance + SWITCH_MARGIN >= lockedDistance) break;
        if (!equal(pos, previous) && canUse(pos)) return { ...pos };
    }
    return locked;
}

// Genuine world coordinates, not a guessed game-to-HUD pixel transform. In
// first person start just below the head's forward axis. Distribute dots by
// view depth so they don't all bunch up near the far endpoint in perspective.
export function tracerMarkers(eye, view, target) {
    if (!finite(eye) || !finite(view) || !finite(target)) return [];
    const end = center(target);
    const toOre = delta(end, eye);
    const magnitude = length(view);
    if (!(magnitude > 1e-6) || !Number.isFinite(magnitude) || length(toOre) > MAX_DISTANCE) return [];
    const dir = { x: view.x / magnitude, y: view.y / magnitude, z: view.z / magnitude };
    const endDepth = dot(toOre, dir);
    if (endDepth < FORWARD_MIN) return []; // Behind the view or too near its plane.

    const startDepth = Math.min(1.25, endDepth * 0.6);
    const horizontal = Math.hypot(dir.x, dir.z);
    const up = horizontal > 1e-6 ? {
        x: -dir.y * dir.x / horizontal,
        y: horizontal,
        z: -dir.y * dir.z / horizontal,
    } : { x: 0, y: 0, z: -Math.sign(dir.y) };
    const down = 0.18 * startDepth;
    const start = {
        x: eye.x + dir.x * startDepth - up.x * down,
        y: eye.y + dir.y * startDepth - up.y * down,
        z: eye.z + dir.z * startDepth - up.z * down,
    };
    const segment = delta(end, start);
    return Array.from({ length: MARKER_COUNT }, (_, i) => {
        const screenT = i / (MARKER_COUNT - 1);
        const t = (screenT * startDepth) / (endDepth * (1 - screenT) + screenT * startDepth);
        const depth = startDepth + t * (endDepth - startDepth);
        const endpoint = i === MARKER_COUNT - 1;
        return {
            position: endpoint ? end : { x: start.x + t * segment.x, y: start.y + t * segment.y, z: start.z + t * segment.z },
            scale: Math.max(0.06, Math.min(4, depth * (endpoint ? 0.12 : 0.09))),
            glyph: endpoint ? "+" : "\u2022",
        };
    });
}

export class DiamondTracer {
    #makeText;
    #getManager;
    #dots = [];
    #renewAt = 0;

    constructor(makeText, getManager) {
        this.#makeText = makeText;
        this.#getManager = getManager;
    }

    clear() {
        const dots = this.#dots;
        this.#dots = [];
        for (const { shape } of dots) {
            try {
                shape.remove();
            } catch {
                // Already gone, or the short lifetime will retire it.
            }
        }
    }

    update(player, dimension, target, tick) {
        if (!target) {
            this.clear();
            return;
        }
        try {
            const markers = tracerMarkers(player.getHeadLocation(), player.getViewDirection(), target);
            if (!markers.length) {
                this.clear();
                return;
            }
            if (!this.#dots.length) {
                const manager = this.#getManager();
                for (const { position, glyph, scale } of markers) {
                    const shape = this.#makeText({ ...position, dimension }, glyph);
                    shape.visibleTo = [player];
                    shape.depthTest = false;
                    shape.useRotation = false;
                    shape.color = { ...CYAN };
                    shape.backgroundColorOverride = { ...CLEAR };
                    shape.scale = scale;
                    shape.timeLeft = MARKER_LIFETIME;
                    this.#dots.push({ shape, position, scale });
                    manager.addText(shape, dimension);
                }
                this.#renewAt = tick + RENEW_TICKS;
                return;
            }
            // Move the same handles. No delete/recreate burst while walking or looking around.
            const renew = tick >= this.#renewAt;
            for (let i = 0; i < markers.length; ++i) {
                const { position, scale } = markers[i];
                const prior = this.#dots[i];
                if (!equal(prior.position, position)) {
                    prior.shape.setLocation({ ...position, dimension });
                    prior.position = position;
                }
                if (Math.abs(prior.scale - scale) >= 0.001) {
                    prior.shape.scale = scale;
                    prior.scale = scale;
                }
                if (renew) prior.shape.timeLeft = MARKER_LIFETIME;
            }
            if (renew) this.#renewAt = tick + RENEW_TICKS;
        } catch (error) {
            this.clear();
            throw error;
        }
    }
}
