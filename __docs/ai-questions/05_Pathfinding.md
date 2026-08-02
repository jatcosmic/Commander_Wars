# Pathfinding Questions and Answers for AI

| Questions | Answers |
| --- | --- |
| How does the game determine legal moves? |  |

### Movement
How does the game determine legal moves?
Where are movement costs looked up
Does it use A*?
Dijkstra?
BFS?

### Terrain
| Questions | Answers |
| --- | --- |
| Where are terrain movement costs looked up? | `UnitPathFindingSystem::getCosts()` can be used to get the cost to move to a specific tile given your current tile. If you want the modifier the terrain applies, call `Terrain::getMovementcostModifier()`, for the base movement cost, each movement type like *MOVE_FEET* has a movement table that tells the base movement cost to move over a specific terrain.  |
| How are impassable tiles represented? | `UnitPathFindingSystem::isCrossable()` |
| Bridges? |
| Rivers? |
| Teleport tiles? |

### Obstacles
| Questions | Answers |
| --- | --- |
Friendly units 
| Enemy units | `UnitPathFindingSystem::blockedByEnemy()` |
Hidden units
Buildings

How do these affect movement?

### Special Cases
| Questions | Answers |
| --- | --- |
| Transport loading | `Unit::loadUnit` |
| Unloading | `Unit::unloadUnit()` |
| Airports | 
Naval movement
Pipe seams
Gates

### AI Questions

Can I ask:

```Give me every reachable tile.```

or do I have to compute it?