# Notes on Normal AI

These are notes on the `NormalAi` class. `NormalAi` is derived from `CoreAI` which itself is derived from `BaseGameInputIF`.

These notes describe how `NormalAi` behaves from start of game and
how it's rule-based thinking works. 

## Program Flow

### How Does the AI Build its First Infantry?

On game start, `GameMenue::startGame()` calls `pInput->onGameStart()`. This calls `CoreAI::onGameStart()` which calls `SimpleProductionSystem::initialize()` to kick off building units from the `normalai.js` function `initializeSimpleProductionSystem()` which calls `COREAI.initializeSimpleProductionSystem()` in `__coreai.js`. This results in infantry added to the queue by `system.addInitialProduction(["INFANTRY"], 6)` through the the C++ function, `SimpleProductionSystem::addInitialProduction()`.