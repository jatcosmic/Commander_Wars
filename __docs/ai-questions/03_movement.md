# Movement Questions and Answers for AI

| Question | Answer |
| ---- | --- |
| How is movement type determined? | `Unit::getMovementType()` |
| Where are movement tables? | `MovementTableManager` |
| How are movement costs calculated? | `Unit::getMovementCosts()`|
| What modifies movement cost? | `Player::getMovementcostModifier()`,`Terrain::getMovementcostModifier()`, `Player::getWeatherMovementCostModifier()` |
| How does pathfinding use this? | `PathFindingSystem::getAllNodePointsFast`, `PathFindingSystem::getTargetCosts` an example of use comes from `InfluenceFrontMap::addUnitInfluence()`