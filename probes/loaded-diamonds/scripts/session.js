import { LoadedDiamondScan } from "./scan.js";
import { chooseDiamond, DiamondTracer } from "./tracer.js";

export const EVENT_ID = "idfk:loaded_diamonds";
const OVERWORLD = "minecraft:overworld";
const TARGET_TICKS = 6;
const DRAW_TICKS = 2;
const TAG = "[Loaded Diamonds]";

function formatPos({ x, y, z }) { return `${x}, ${y}, ${z}`; }

function sendReport(player, report, session) {
    player.sendMessage(`${TAG} ${report.sampled}/${report.total} slices sampled in ` +
        `${report.sampledColumns} loaded chunks; Y ${report.minY}..${report.maxY}.`);
    player.sendMessage(report.examples.length
        ? `${TAG} Samples in ${report.matchColumns} chunks: ${report.examples.map(formatPos).join(" | ")}`
        : `${TAG} No diamond sampled. Unavailable slices are not checked.`);
    player.sendMessage(`${TAG} ${report.sweeps} sweeps in this window; ${report.queries} volume queries since start, ` +
        `${report.failures} failed.${report.capped ? " Dense slice: example search capped." : ""}`);
    player.sendMessage(`${TAG} Tracer: ${session.tracerFailed ? "unavailable" : session.target
        ? `locked on ${formatPos(session.target)}` : "searching"}.`);
}

export function createSessionControls(world, system, makeVolume, makeText) {
    let active = null;
    let lastScanTick = -1;

    function stop(player, reason = "Stopped.") {
        if (!active) return;
        const { playerId, interval, tracer } = active;
        active = null;
        tracer.clear();
        if (interval !== undefined) system.clearRun(interval);
        try {
            if (player?.id === playerId) player.sendMessage(`${TAG} ${reason}`);
        } catch {
            // Player can be gone; the scan has already been stopped.
        }
    }

    function updateTracer(session, player, dimension, location, now) {
        if (session.tracerFailed) return;
        if (session.target && !session.scan.contains(session.target)) {
            session.target = null;
            session.tracer.clear();
        }
        if (now >= session.nextTarget) {
            session.nextTarget = now + TARGET_TICKS;
            const target = chooseDiamond(session.scan, dimension, location, session.target);
            if (target !== session.target) {
                session.target = target;
                session.nextDraw = now;
            }
        }
        if (now >= session.nextDraw) {
            session.nextDraw = now + DRAW_TICKS;
            try {
                session.tracer.update(player, dimension, session.target, now);
            } catch {
                session.tracerFailed = true;
                session.target = null;
                player.sendMessage(`${TAG} Could not draw the tracer. Scan status is still available in chat.`);
            }
        }
    }

    function update(session) {
        if (active !== session) return;
        let player;
        try {
            const players = world.getAllPlayers();
            player = players.find((p) => p.id === session.playerId);
            if (players.length !== 1 || !player?.isValid) {
                stop(player, "Stopped: one player required.");
                return;
            }
            const { dimension, location } = player;
            if (dimension.id !== OVERWORLD) {
                stop(player, "Stopped: left the Overworld.");
                return;
            }
            const now = system.currentTick;
            if (now === lastScanTick) return;
            lastScanTick = now;
            if (!session.scan.step(dimension, location)) {
                stop(player, "Stopped: position outside the scan window.");
                return;
            }
            updateTracer(session, player, dimension, location, now);
        } catch {
            stop(player, "Stopped: the script context became unavailable.");
        }
    }

    return {
        command(event) {
            if (event.id !== EVENT_ID || event.sourceType !== "Entity") return;
            const player = event.sourceEntity;
            try {
                if (player?.typeId !== "minecraft:player" || !player.isValid) return;
                const command = event.message.trim();
                if (command === "stop" && active?.playerId === player.id) {
                    stop(player);
                    return;
                }
                const players = world.getAllPlayers();
                const { dimension, location } = player;
                if (players.length !== 1 || players[0].id !== player.id || dimension.id !== OVERWORLD) {
                    if (active?.playerId === player.id) stop(player, "Stopped: use a local, one-player Overworld.");
                    else player.sendMessage(`${TAG} Use a local, one-player Overworld.`);
                    return;
                }
                if (command === "start") {
                    if (active) {
                        player.sendMessage(`${TAG} Already scanning. Use /scriptevent ${EVENT_ID} status.`);
                        return;
                    }
                    if (typeof dimension.isChunkLoaded !== "function" || typeof dimension.getBlocks !== "function" ||
                        typeof dimension.getBlock !== "function") {
                        player.sendMessage(`${TAG} Loaded-world Script API unavailable. Not started.`);
                        return;
                    }
                    const scan = new LoadedDiamondScan(makeVolume);
                    if (!scan.report(dimension, location)) {
                        player.sendMessage(`${TAG} Position outside the scan window. Not started.`);
                        return;
                    }
                    const tracer = new DiamondTracer(makeText, () => world.primitiveShapesManager);
                    const session = {
                        playerId: player.id, scan, tracer, target: null, tracerFailed: false,
                        nextTarget: system.currentTick, nextDraw: system.currentTick,
                    };
                    session.interval = system.runInterval(() => update(session), 1);
                    active = session;
                    player.sendMessage(`${TAG} Scanning already-loaded terrain. Look around in first person for the cyan tracer.`);
                } else if (command === "status" && active?.playerId === player.id) {
                    const report = active.scan.report(dimension, location);
                    if (report) sendReport(player, report, active);
                    else stop(player, "Stopped: position outside the scan window.");
                } else {
                    player.sendMessage(`${TAG} Use /scriptevent ${EVENT_ID} start, status, or stop.`);
                }
            } catch {
                if (active?.playerId === player?.id) stop(player, "Stopped: the script context became unavailable.");
            }
        },
        playerLeave({ playerId }) {
            if (active?.playerId === playerId) stop();
        },
        dimensionChange({ player }) {
            if (active?.playerId === player.id) stop(player, "Stopped: changed dimension.");
        },
    };
}
