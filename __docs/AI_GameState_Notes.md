# Game State Notes

## Questions the AI Needs to Answer

  Question           Engine API
  ------------------ ------------------------------
  My units           `GameMap::getUnits(Player*)`
  Terrain at (x,y)   `GameMap::getTerrain(x,y)`
  Unit on tile       `Terrain::getSpUnit()`
  Unit HP            `Unit::getHp()`
  Unit owner         `Unit::getOwner()`
  Movement cost      `Unit::getMovementCosts()`

------------------------------------------------------------------------

## Engine vs AI

The engine stores facts.

``` text
GameMap
 ├── Terrain
 ├── Units
 └── Buildings
```

The AI should derive information such as:

-   Threat maps
-   Influence maps
-   Reachability
-   Capture opportunities
-   Combat evaluations

------------------------------------------------------------------------

## Useful Classes

### GameMap

Responsibilities:

-   Stores map tiles (`m_fields`)
-   Returns terrain
-   Returns player units

### Terrain

Responsibilities:

-   Stores terrain information
-   Knows occupying unit
-   Applies terrain-specific modifiers
-   Defense bonuses

### Unit

Responsibilities:

-   HP
-   Fuel
-   Ammo
-   Movement type
-   Computes final movement cost

------------------------------------------------------------------------

## JavaScript Dispatch

Many game rules are implemented in JavaScript.

Pattern:

``` cpp
Interpreter::doFunction(
    objectName,
    functionName,
    args);
```

Example:

``` cpp
doFunction("INFANTRY",
           "getMovementType",
           args);
```

Equivalent JavaScript:

``` javascript
INFANTRY.getMovementType();
```

Objects such as `INFANTRY`, `FOREST`, and `MOVE_FEET` exist as global
JavaScript objects.

------------------------------------------------------------------------

## AI Notes

Useful derived data structures to build later:

-   Friendly unit list
-   Enemy unit list
-   Reachable tiles
-   Attack map
-   Threat map
-   Influence map
-   Evaluation score
