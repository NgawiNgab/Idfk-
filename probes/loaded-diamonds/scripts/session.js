import { LoadedDiamondScan } from "./scan.js";

export const EVENT_ID = "idfk:loaded_diamonds";
const OVERWORLD = "minecraft:overworld";
const HUD_TICKS = 10;
const TAG = "[Loaded Diamonds]";

function formatPos({ x, y, z }) { return `${x}, ${y}, ${z}`; }

function showBar(player, report) {
    const first = report.examples[0];
    const progress = `${report.sampled}/${report.total} slices`;
    player.onScreenDisplay.setActionBar(first
        ? `Diamond: ${formatPos(first)} | ${report.matchColumns} chunks`
        : `Diamonds: ${report.sweeps ? "none sampled" : "scanning"} | ${progress}`);
}

function sendReport(player, report) {
    player.sendMessage(`${TAG} ${report.sampled}/${report.total} slices sampled in ` +
        `${report.sampledColumns} loaded chunks; Y ${report.minY}..${report.maxY}.`);
    player.sendMessage(report.examples.length
        ? `${TAG} Samples in ${report.matchColumns} chunks: ${report.examples.map(formatPos).join(" | ")}`
        : `${TAG} No diamond sampled. Unavailable slices are not checked.`);
    player.sendMessage(`${TAG} ${report.sweeps} sweeps in this window; ${report.queries} queries since start, ` +
        `${report.failures} failed.${report.capped ? " Dense slice: example search capped." : ""}`);
}

export function createSessionControls(world, system, makeVolume) {
    let active = null;
    let lastScanTick = -1;

    function stop(player, reason = "Stopped.") {
        if (!active) return;
        const { playerId, interval } = active;
        active = null;
        if (interval !== undefined) system.clearRun(interval);
        try {
            if (player?.id !== playerId) return;
            player.onScreenDisplay.setActionBar("");
            player.sendMessage(`${TAG} ${reason}`);
        } catch {
            // Player or HUD can be gone; the scan has already been stopped.
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
            if (now >= session.nextHud) {
                session.nextHud = now + HUD_TICKS;
                showBar(player, session.scan.report(dimension, location));
            }
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
                    if (typeof dimension.isChunkLoaded !== "function" || typeof dimension.getBlocks !== "function") {
                        player.sendMessage(`${TAG} Loaded-volume Script API unavailable. Not started.`);
                        return;
                    }
                    const scan = new LoadedDiamondScan(makeVolume);
                    if (!scan.report(dimension, location)) {
                        player.sendMessage(`${TAG} Position outside the scan window. Not started.`);
                        return;
                    }
                    const session = { playerId: player.id, scan, nextHud: system.currentTick };
                    session.interval = system.runInterval(() => update(session), 1);
                    active = session;
                    player.sendMessage(`${TAG} Scanning already-loaded terrain. Stand still for a sweep.`);
                } else if (command === "status" && active?.playerId === player.id) {
                    const report = active.scan.report(dimension, location);
                    if (report) sendReport(player, report);
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
