# Movement System

## Overview

Commander Wars separates **base movement costs** from **movement
modifiers**.

Final movement cost:

``` text
Base Movement Cost
+ CO Modifiers
+ Terrain Modifiers
+ Weather Modifiers
= Final Movement Cost
```

------------------------------------------------------------------------

## Call Flow

``` text
Unit::getMovementCosts()
    |
    +--> Unit::getBaseMovementCosts()
            |
            +--> MovementTableManager::getBaseMovementPoints(
                    movementType,
                    destinationTerrain,
                    currentTerrain,
                    unit)
                    |
                    +--> JavaScript movement table
                            |
                            +--> MOVE_FEET.getMovementpoints(...)
                                    |
                                    +--> movementpointsTable lookup
```

------------------------------------------------------------------------

## Unit Initialization

Each unit has a JavaScript definition.

Example (`infantry.js`):

``` javascript
this.getMovementType = function()
{
    return "MOVE_FEET";
};
```

During `Unit::initUnit()`:

``` cpp
m_MovementType =
    Interpreter::doFunction(
        m_UnitID,
        "getMovementType",
        args);
```

An Infantry therefore stores:

``` text
m_MovementType = "MOVE_FEET"
```

------------------------------------------------------------------------

## Movement Tables

Movement tables are defined separately from units.

Example:

``` javascript
MOVE_FEET.movementpointsTable = [
    ["PLAINS", 1],
    ["FOREST", 1],
    ["MOUNTAIN", 2],
    ...
];
```

This means movement behavior is determined by:

-   Movement type
-   Terrain

instead of terrain hardcoding behavior for every unit.

------------------------------------------------------------------------

## Modifiers

After the base cost is calculated, additional modifiers are applied:

-   CO powers
-   Terrain modifiers
-   Weather modifiers

Example:

``` cpp
costs += player->getMovementcostModifier(...);

costs += terrain->getMovementcostModifier(...);

costs += owner->getWeatherMovementCostModifier(...);
```

These modify the base value returned from the movement table.
