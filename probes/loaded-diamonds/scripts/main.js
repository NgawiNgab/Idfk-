import { BlockVolume, system, world } from "@minecraft/server";
import { createSessionControls } from "./session.js";

const controls = createSessionControls(world, system, (from, to) => new BlockVolume(from, to));
system.afterEvents.scriptEventReceive.subscribe(controls.command, { namespaces: ["idfk"] });
world.afterEvents.playerLeave.subscribe(controls.playerLeave);
world.afterEvents.playerDimensionChange.subscribe(controls.dimensionChange);
