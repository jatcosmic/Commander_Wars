# Controlling the AI:

This document details how you can control the AI:


[AI-Movement](https://github.com/Robosturm/Commander_Wars/wiki/AI-Modification-for-a-Single-Maps)

Notably it describes how the AI can be accessed through the map object by getting the player. For example,

`map.getPlayer(0).getBaseGameInput().setEnableNeutralTerrainAttack(true);`

This can be used to enable or disable the ability of the AI to attack terrain.



----

## Setting the Unit AI Mode:

`Unit::setAiMode(const GameEnums::GameAi AiMode)`