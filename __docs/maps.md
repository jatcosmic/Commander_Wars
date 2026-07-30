### The Map

The `GameMap` class contains the map of the game. As shown in ai-control.md, the AI often uses this function as such:

`map.getPlayer(0).getBaseGameInput().setEnableNeutralTerrainAttack(true);`

So you can use the map to get the players among other things. Alternatively it can be retrieved through the `Player` as well with `getUnits` function.

| Question | Where does the answer come from? |
| --- | --- |
| What units do I own? | `GameMap::getUnits(player)` |
| What terrain is at (x, y)? | `GameMap::getTerrain(x,y)` |
