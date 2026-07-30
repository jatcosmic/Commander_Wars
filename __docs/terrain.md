# Terrain

## Class

Terrain (C++)

## Purpose

These are the tiles of the game. A Terrain holds information about the terrain like movement cost modifier, vision, which unit is on the terrain, what type of terrain it is ("FOREST", "MOUNTAIN", ...) etc. 

## Key Members

- m_terrainID
  - String identifying the type of terrain (e.g. "FOREST")

## Notable Functions:

* `Terrain->getSpUnit(...)`
* `Terrain->getMovementcostModifier(...)`
* `Terrain->getDefense(...)`
* `Terrain->getVisionHide(...)`