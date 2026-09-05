# Cheatset Plugin

A plugin for TesmioLoader (Workers & Resources: Soviet Republic) that automatically activates the internal cheat menu (C+H+E) and your preferred cheat functions every time a map loads.

## Features
* **Memory-Level Auto-Activation**: Bypasses the need for manual keystrokes or UI clicks by directly writing to the game's internal memory offsets.
* **Granular Control**: Use the INI file to selectively enable or disable individual cheat features:
  * Speed up construction
  * Speed up research
  * Speed up vehicle production
  * Speed up growing trees
  * Speed up pollution
  * Speed up wear and tear
  * Experimental traffic pathfinding
  * Train route signal
* **Vanilla Friendly**: Leaves unmapped cheat settings (like CO cooperate or Landscape editor mode) completely untouched in their native state.

## Installation
1. Ensure you have the TesmioLoader framework installed.
2. Download or compile the plugin, and copy `cheat_set.dll` and `cheat_set.ini` to your game directory:
   `Steam\steamapps\common\SovietRepublic\tesmioloader\build\plugins\`
3. Activate the plugin via the `tesmiolauncher.exe` interface.

## Configuration
Edit `cheat_set.ini` in your plugins folder to adjust the settings. The file contains detailed comments explaining how each toggle works.
