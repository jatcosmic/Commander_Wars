# Game State 

| Question | Answer |
| ---- | --- |
| How do I enumerate units I can build? | `SimpleProductionSystem::buildUnit(qint32 x, qint32 y, ...)` (1)  
| How do I enumerate buildings? | Often the type `QmlVectorBuilding` is used as in `SimpleProductionSystem` it uses it to enumerate the buildings. |
| Terrain lookup
| Weather
| Current player
| Income
| Fog of war


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