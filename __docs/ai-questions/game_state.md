# Game State 

| Question | Answer |
| ---- | --- |
| How do I enumerate units I can build? | `StringList m_BuildList` which has a comment saying, **"m_BuildList contains all units we're allowed to build"**. We can see how the AI does it in function `SimpleProductionSystem::buildUnit(qint32 x, qint32 y, ...)` which it used during a live game to build units (1) | 
| How do I enumerate buildings? | Often the type `QmlVectorBuilding` is used as in `SimpleProductionSystem` it uses it to enumerate the buildings. |
| Terrain lookup | `GameMap::getTerrain(x, y)` A function, `CoreAI::moveSupport(...)` makes use of this function |   
| Weather | `GameMap::getGameRules()->getCurrentWeather()` used like this in `Building::updateBuildingSprites()` |
| Current player | `GameMap::getCurrentPlayer()`, it is likely preferrable to get the Id of the current player so like so, `GameMap::getCurrentPlayer->getPlayerID()` seeing as multiple functions like `AiProcessPipe::onNewAction()` calls it like so: `m_pMap->getCurrentPlayer()->getPlayerId()` also `InfluenceFrontMap::addBuildingInfluence` calls it like this as well when getting the owner of a building that can build units `pBuilding->getOwner()->getPlayerID()` |
| Income | `Player::CalcIncome()`, used in `InfluenceFrontMap::addBuildingInfluence()` 
| Funds | `Player::getFunds()`, used in `NormalAi::buildUnits()` probably when building units |
| Fog of war | `Player::getFieldDirectVisible`, which relies on the field, `std::vector<std::vector<VisionFieldInfo>> m_FogVisionFields` so this contains the info the player can see |


### References:

(1) For this I logged the output of an AI as it acted using `F1`and `setDeveloperMode(1)`. It ran an action, `ACTION_BUILD_UNITS` when it built an infantry--this action starts at the line beginning, "Building unit INFANTRY at x=28 y=13"--this line is output in the function, `SimpleProductionSystem::buildUnit(qint32 x, qint32 y, ...)`

```
Debug: 31.07.2026 02:45:59: Stop running with seed File:  Line: 0 Function: 
Debug: 31.07.2026 02:45:59: emitting sigActionPerformed() File:  Line: 0 Function: 
Debug: 31.07.2026 02:45:59: ActionPerformer::onTriggeringActionFinished File:  Line: 0 Function: 
Debug: 31.07.2026 02:45:59: CoreAI::nextAction for player 1 for ai NORMALAI File:  Line: 0 Function: 
Debug: 31.07.2026 02:45:59: Building unit INFANTRY at x=28 y=13 File:  Line: 0 Function: 
Debug: 31.07.2026 02:45:59: Start running action ACTION_BUILD_UNITS File:  Line: 0 Function: 
Debug: 31.07.2026 02:45:59: GameMap::getMapHash File:  Line: 0 Function: 
Debug: 31.07.2026 02:45:59: GameMap::serializeObject with unique id counter 5 at day 2 File:  Line: 0 Function: 
Debug: 31.07.2026 02:45:59: storing player with control type 0 and name Josh File:  Line: 0 Function: 
```