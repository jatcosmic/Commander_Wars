# Combat Questions and Answers for AI

| Question | Answer |
| ---- | --- |
| What Units do I have? | `Player::getSpUnits()` |
| How is damage calculated? | `Unit::getBaseDamage(Unit*)` |
| Where is everything that describes how a unit behaves in combat? | `Unit` |
| What units can this unit damage? | `Unit::isAttackable(Unit* pDefender, ...)`, see `CoreAI::appendAttackTargets` for how it creates a list of attackable targets |
| What terrain can this unit damage? | `Unit::isEnvironmentAttackable(QString terrainID)` |
| Does the Unit consume ammo? | `hasAction("ACTION_FIRE")` But `CoreAI::getBestAttacksFromField(Unit* pUnit)` uses `pAction->canBePerformed` so this may be better |
| Can the Unit fire after moving? | `Unit::canMoveAndFire(Qpoint)` | 
| Is the Unit a direct fire unit? | `Unit::getBaseMaxRange() < 1`, see `CoreAI::useCOPower(...)` |
| Is this Unit an indirect fire unit? | `Unit::getBaseMaxRange() > 1`, see `CoreAI::useCOPower(...)` |
| What is the Unit's maximum range? | `Unit::getMaxRange(Qpoint)` |
| What is the Unit's minimum range? | `Unit::getMinRange(QPoint)` |
| Can it target air units? | `Unit::isAttackable(Unit* pDefender, ...)`, see `CoreAI::appendAttackTargets` for how it creates a list of attackable targets |
| Can it target naval units? | `Unit::isAttackable(Unit* pDefender, ...)`, see  `CoreAI::appendAttackTargets` for how it creates attackable targets |