// modmenu.h -- LeviLaunchroid Mod Menu registration for Apollon.
//
// Replaces the ImGui surface, which could never appear because its only frame
// pump was a JNI entry point the game never called (Levi dlopen()s the mod and
// does not run JNI_OnLoad). See modmenu.cpp for the full explanation.
//
// Returns the number of modules registered; 0 means the menu API was
// unreachable (libpreloader.so missing or an older launcher).
#pragma once

int ApollonRegisterModMenu();
