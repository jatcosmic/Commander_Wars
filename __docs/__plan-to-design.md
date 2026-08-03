I'd probably structure it something like this
```
MyStrategicAI : public CoreAI
│
├── process()
│
├── GenerateActions()
│
├── EvaluateState()
│
├── Search()
│
├── InfluenceMap
│
├── ThreatMap
│
├── PathFinder
│
├── UnitEvaluator
│
├── EconomyEvaluator
│
├── FrontAnalyzer
│
├── CapturePlanner
│
└── OpeningBook (optional someday)
```

Notice that everything interesting lives below process().

I would not put everything directly inside process()

I'd make process() as small as possible.

Something like:
```
void MyAI::process()
{
    BuildInfluenceMaps();

    BuildThreatMaps();

    GenerateLegalActions();

    ScoreActions();

    auto best = Search();

    emit sigPerformAction(best);
}
```
Then each of those becomes its own subsystem.

For example:
```
ThreatMap
```
knows nothing about Qt.
```
InfluenceMap
```
knows nothing about signals.
```
Search
```
knows nothing about `ActionPerformer`.

That makes each component easier to test, reason about, and improve independently.

This also gives you a nice development path

You don't need to build everything before seeing results.

You could iterate like this:

Version 1
```
process()
    ↓
Generate legal actions
    ↓
Pick first legal action
```
Terrible AI—but it exercises your action generation.

___

Version 2
```
Generate actions

Evaluate captures

Choose best capture
```

___

Version 3
```
Add pathfinding
```

___

Version 4

```
Add influence maps
```

___

Version 5

```
Add threat maps
```

___

Version 6
```
Replace heuristic action selection
        ↓
Alpha-beta search
```

___


## One thing to try:

I'd go one step further than simply subclassing CoreAI.

Don't make your AI code depend heavily on CoreAI internals. Instead, think of process() as a translation layer:
```
void MyAI::process()
{
    GameState state = BuildGameState();

    Action best = m_search.FindBestAction(state);

    emit sigPerformAction(ConvertToGameAction(best));
}
```
Here, your search, evaluation, and map-analysis code operates on a representation of the game state that's as independent as possible from Qt and the engine. That has a few advantages:

* You can unit test your AI without launching the game.
* You can reuse algorithms in another project if you want.
* Your AI code stays focused on AI concepts rather than engine APIs.

