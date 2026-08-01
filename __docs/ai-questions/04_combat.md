# Combat Questions and Answers for AI

| Question | Answer |
| ---- | --- |
| What Units do I have? | `Player::getSpUnits()`, see `CoreAi::GetOwnUnitCounts()` for how it computes the number of units it has |

## Damage

| Question | Answer |
| ---- | --- |
| How is damage calculated? | `Unit::getBaseDamage(Unit*)` (1) |
| Where is everything that describes how a unit behaves in combat? | `Unit` |
| What units can this unit damage? | `Unit::isAttackable(Unit* pDefender, ...)`, see `CoreAi::appendAttackTargets` for how it creates a list of attackable targets |
| What terrain can this unit damage? | `Unit::isEnvironmentAttackable(QString terrainID)` |
| Does the Unit consume ammo? | `hasAction("ACTION_FIRE")` But `CoreAi::getBestAttacksFromField(Unit* pUnit)` uses `pAction->canBePerformed` so this may be better |
| Can the Unit fire after moving? | `Unit::canMoveAndFire(Qpoint)` | 
| Is the Unit a direct fire unit? | `Unit::getBaseMaxRange() < 1`, see `CoreAi::useCOPower(...)` |
| Is this Unit an indirect fire unit? | `Unit::getBaseMaxRange() > 1`, see `CoreAi::useCOPower(...)` |
| What is the Unit's maximum range? | `Unit::getMaxRange(Qpoint)`, or `NormalAi::calculateCounterDamage` has m_EnemyUnits which it uses to retrieve a unit from it named `enemyData` then calls it to get the maximum fire range: `enemyData.maxFireRange`  |
| What is the Unit's minimum range? | `Unit::getMinRange(QPoint)` |
| Can it target air units? | `Unit::isAttackable(Unit* pDefender, ...)`, see `CoreAi::appendAttackTargets` for how it creates a list of attackable targets |
| Can it target naval units? | `Unit::isAttackable(Unit* pDefender, ...)`, see  `CoreAi::appendAttackTargets` for how it creates attackable targets |


## Luck and Misfortune

| Question | Answer |
| ---- | --- |
| How is luck handled? | `calcBattleDamage` on the `ACTION_FIRE` js object. See `CoreAi::calcUnitDamage()` for how it computes the damage and let the game manage the luck. We may be able to do better than this if we can instead return the range of damage. |

## Counterattacks

| Question | Answer |
| ---- | --- |
| How are counterattacks handled? | `NormalAi::calculateCounterDamage()` is used by the AI currently to calculate the damage a counter deals. |

## CO Modifiers

| Question | Answer |
| ---- | --- |
| Where are the CO Modifiers? | `CO::getAiCoUnitBonus`, `CO::getOffensiveBonus`, `CO::getDeffensiveBonus`, `CO::getFirerangeModifier` |

### References

(1) For `Unit::getBaseDamage(Unit*)`, consider the weapon `weapon_infantry_mg`. This is the machine gun used by infantry. It has a damage table in `weapon_infantry_mg.js` that contains the base damage the weapon deals to various types of enemies. Here is an excerpt:

```
this.damageTable = [["INFANTRY", 55],
                        ["MECH", 45],
                        ["MOTORBIKE", 45],
                        ["SNIPER", 55],
                        ...
                   ]
````

(1 cont.) So the infantry's machine gun deals 55 base damage to an infantry, 45 base damage to a mech, 45 to a motorbike, 55 to a sniper, etc. 

In the *Luck* section, the function in `ACTION_FIRE.js`, `this.calcDamage` uses this base damage and adds to it the *luck* and misfortune to compute the final damage dealt:

```
                    var luck = ACTION_FIRE.getDefaultLuck(attacker) + attacker.getBonusLuck(attackerPosition);
                    var misfortune = attacker.getBonusMisfortune(attackerPosition);
                    // only roll if we have valid luck misfortune pair
                    if (luck > -misfortune)
                    {
                        if (luckMode === GameEnums.LuckDamageMode_On)
                        {
                            var luckValue = 0;
                            // roll luck?
                            if (luck > 0)
                            {
                                // roll against zero or against negative misfortune?
                                if (misfortune < 0)
                                {
                                    luckValue = globals.randInt(-misfortune, luck);
                                }
                                else
                                {
                                    luckValue = globals.randInt(0, luck);
                                }
                            }
                            var misfortuneValue = 0;
                            // roll misfortune?
                            if (misfortune > 0)
                            {
                                // roll against zero or against luck?
                                if (luck < 0)
                                {
                                    misfortuneValue = globals.randInt(-misfortune, luck);
                                }
                                else
                                {
                                    misfortuneValue = globals.randInt(-misfortune, 0);
                                }
                            }
                            luckDamage += (luckValue + misfortuneValue);
                        }

```
