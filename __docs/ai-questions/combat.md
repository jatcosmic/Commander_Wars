# Combat Questions and Answers for AI

| Question | Answer |
| ---- | --- |
| How is damage calculated? | `Unit::getBaseDamage(Unit*)` |
| Where is everything that describes how a unit behaves in combat? | `Unit` |
| What units can this unit damage? | `Unit::isAttackable(Unit* pDefender, ...)` |
| What terrain can this unit damage? | `Unit::isEnvironmentAttackable(QString terrainID)` |
| Does the Unit consume ammo? | `hasAction("ACTION_FIRE")` But `CoreAI::getBestAttacksFromField(Unit* pUnit)` uses `pAction->canBePerformed` so this may be better |