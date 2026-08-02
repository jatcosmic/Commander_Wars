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

## 05_Pathfinding.md

### Movement
How does the game determine legal moves?
Does it use A*?
Dijkstra?
BFS?

### Terrain
Where are movement costs looked up?
How are impassable tiles represented?
Bridges?
Rivers?
Teleport tiles?

### Obstacles
Friendly units
Enemy units
Hidden units
Buildings

How do these affect movement?

### Special Cases
Transport loading
Unloading
Airports
Naval movement
Pipe seams
Gates

### AI Questions

Can I ask:

```Give me every reachable tile.```

or do I have to compute it?


## 06_Threat_Maps.md

This is almost entirely AI.

Questions:

How do I compute:

Enemy attack range?
Friendly attack range?
Indirect fire?
Fog?

Should threat include:

Movement?

Example:

Tank

Moves 6

Attacks 1

Threat radius

7

How are indirect units handled?

Artillery

Move 5

Attack 2-3

Threat isn't a circle.

It's

Move

↓

Attack

Should threat account for

HP?
Ammo?
Fuel?


## 07_Influence_Maps.md

### Questions:

What is influence?

How should influence decay?

Distance?

Linear?

Exponential?

Should influence depend on

HP?
Cost?
CO?

Multiple maps?

Friendly influence

Enemy influence

City influence

Production influence

How often should influence be recomputed?

Every unit?

Every turn?

## 08_Evaluation_Function.md

Probably the most important file.

Questions:

How valuable is

Infantry?
Tank?
Md Tank?

Cities

How valuable?

Airport?

HQ?

Should value depend on

HP?

Example

1000-cost infantry

10 HP = 1000

5 HP = 500

Terrain?

Should a tank on a mountain be worth more?

Positional value?

Examples

Near HQ?

Blocking bridge?

Threatening artillery?

Capturing city?

Economic value?

Income?

Tech advantage?

Win conditions?

How do we score:

Destroy HQ

Destroy army

Capture cities