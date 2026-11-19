import { system, world } from "@minecraft/server";
import { handleProbeEvent } from "./probe.js";

system.afterEvents.scriptEventReceive.subscribe((event) => handleProbeEvent(event, world), {
    namespaces: ["idfk"],
});
