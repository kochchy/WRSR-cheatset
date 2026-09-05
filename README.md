# Cheatset Plugin

A plugin for TesmioLoader (Workers & Resources: Soviet Republic) that automatically activates the internal cheat menu (C+H+E) and your preferred cheat functions every time a map loads.

## Features
* **Direct Memory-Level Activation**: Configures the game's internal `world` structures directly upon map load, avoiding brittle UI macros or fake keystrokes.
* **Complete Granular Control**: Configure each of the 10 cheat/debug functions via `cheat_set.ini`:
  * **Speed up construction**
  * **Speed up research**
  * **Speed up vehicle production**
  * **Speed up growing trees**
  * **Speed up pollution**
  * **Speed up wear and tear**
  * **Landscape editor mode**
  * **Experimental traffic pathfinding**
  * **CO cooperate**
  * **Train route signal**
* **Vanilla Matching Defaults**: Default configuration mirrors standard game behavior (`co_cooperate=1`, `train_route_signal=1`, and speedups set to `0`), allowing you to enable only the exact cheats you desire.

## Installation
1. Ensure you have the TesmioLoader framework installed.
2. Download or compile the plugin, and copy `cheat_set.dll` and `cheat_set.ini` to your game directory:
   `Steam\steamapps\common\SovietRepublic\tesmioloader\build\plugins\`
3. Activate the plugin via the `tesmiolauncher.exe` interface.

## Configuration
Edit `cheat_set.ini` in your plugins folder to customize which features activate automatically on load.
