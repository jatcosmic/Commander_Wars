# Movement Questions and Answers for AI

| Question | Answer |
| ---- | --- |
| How is movement type determined? | `Unit::getMovementType()` |
| What are `movement points` and what are `movement costs`? |  See questions (1) and (2). The short summary is `movement cost` is how much it costs for a movement type to move through a terrain. `Movement points` is how far a unit can move--independent of cost. It can consider a unit's base movement, their co buffs/debuffs, enemy cos buffs/debuffs and weather buffs/debuffs. |
| Where are movement tables? | `MovementTableManager` |
| (1) How are movement costs calculated and what is movement cost? | `Unit::getMovementCosts()`. `Movement cost` is the cost it takes to move through a type of terrain. This depends on the movement type. As an example, consider `Infantry` with their movement type, `MOVE_FEET`. `MOVE_FEET.js` defines a movement table which returns the `movement cost` for this movement type over each kind of terrain. For example, `MOVE_FEET` has a **2 point movement cost** over mountains. Thus it's `movement cost` over a mountain terrain will return 2. Its used by `PathFindingSystem` while *exploring* to decide how far a unit can move. |
| (2) What are movement points and where are they? | Returned by `Unit::getMovementPoints()`. `Movement points` are *the **cost** for a unit to move--sometimes across types of terrain*. Their base movement points is defined in the unit's `init` function. Their total `movement points` considers various bonuses and modifiers. |
| What modifies movement cost? | `Player::getMovementcostModifier()`,`Terrain::getMovementcostModifier()`, `Player::getWeatherMovementCostModifier()` |
| How does pathfinding use this? | `PathFindingSystem::getAllNodePointsFast`, `PathFindingSystem::getTargetCosts` an example of use comes from `InfluenceFrontMap::addUnitInfluence()`
