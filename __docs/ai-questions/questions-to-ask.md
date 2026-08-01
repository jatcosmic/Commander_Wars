# Questions to Ask for AI

CommanderWars-AI-Notes/
│
├── 01_Game_State.md
│
├── 02_Units.md
│
├── 03_Movement.md
│
├── 04_Combat.md
│
├── 05_Pathfinding.md
│
├── 06_Threat_Maps.md
│
├── 07_Influence_Maps.md
│
├── 08_Evaluation_Function.md
│
├── 09_Search.md
│
└── AI_Architecture.md


## 01_Game_State.md

How do I enumerate units?
How do I enumerate buildings?
Terrain lookup
Weather
Current player
Income
Fog of war


## 02_Units.md

### Basic Information

How do I enumerate all units?
How do I enumerate friendly units?
How do I enumerate enemy units?
How do I determine whether a unit is alive?
Where are units stored?

#### Unit Properties

For a unit, how do I obtain:

```
HP
Max HP
Ammo
Fuel
Owner
Position
Unit type
Movement type
Vision
Min range
Max range
Base cost
Current state (loaded? capturing? waiting?)
Movement
How is movement type determined?
Where are movement points stored?
How do I ask where a unit can move?
Does the engine already generate reachable tiles?
Does it use A*?
Combat
How do I determine whether one unit can attack another?
How is damage calculated?
How are counterattacks handled?
Luck?
CO modifiers?
```

##### Things to think about for the AI:

1. Can I cache any of this?
2. Which values change every turn?


## 03_Movement.md

How is movement type determined?
Where are movement tables?
How are movement costs calculated?
What modifies movement cost?
How does pathfinding use this?

## 04_Combat.md

How is damage calculated?
Where are weapon definitions?
How is luck handled?
Counterattacks?
CO modifiers?
