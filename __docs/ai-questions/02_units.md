| Question | Answer |
| --- | --- |
| How do I enumerate all units? | `GameMap::getUnits(player)` |
| How do I get my units? | `Player::getSpUnits()` |
| How do I get my enemies units? | `Player::getSpEnemyUnits()`, the function  `CoreAI::prepareEnemieData()` calls this and a few other useful functions like `Player::getSpEnemyBuildings()` to get all the data related to the enemy |
| How to I get my army value? | `Player::calcArmyValue()` |
| How do I enumerate friendly units? | ``` (foreach players as otherPlayer) { if(thisPlayer->isAlly(otherPlayer)) { friendlyUnits.push_back(otherPlayer->getSpUnits()) } }``` |
| How do I enumerate enemy units? | ``` (foreach players as otherPlayer) { if(thisPlayer->isEnemy(otherPlayer)) { enemyUnits.push_back(otherPlayer->getSpUnits()) } }``` |
| How do I determine whether a unit is alive when simulating damages (That is when I am simulating the hypothetical amount of damage a unit has been dealt?) | Check that `Unit::getVirtualHp() > 0` (1) |
| How do I determine if a unit is in fact alive (that is not hypothetically alive after combat simulations, but actually alive) | Check that `Unit::getHp > 0` (1) | 

## Unit Property Questions

For a unit, how do I obtain:

| Question | Answer |
| --- | --- |
| HP? | `Unit::getHp()` and `Unit::getVirtualHp()` see (1) for more details on when to use each |
| Max HP? | `Unit::MAX_UNIT_HP` | 
| Ammo | A unit has two different weapons. So they use two functions: `Unit::getAmmo1()` and `Unit::getAmmo2()` |
| MaxAmmo | A unit has two different weapons. So they use two functions: `UnitGetMaxAmmo1()` and `Unit::getMaxAmmo2()` |
| What weapon am I using? | `Unit::getWeapon1ID()` and `Unit::getWeapon2ID()`. See (2) for more information about how to get data about a weapon. | 
| Fuel | `Unit::getFuel()` |
| Owner | `Unit::getOwner()` |
| Position | `Unit::getX()`, `Unit::getY()`, `Unit::getPosition()`, or `Unit::getMapPosition()` any of these work--the last two functions are  identical | 
| Unit type | `Unit::getUnitID()` returns a string like `INFANTRY` which is then used by its correspondingly named `infantry.js` file to get information about the *infantry* unit |
| Vision with modifiers included? | For full vision with all bonuses included, call `Unit::getVision()` |
| Unit's base vision? | `Unit::getBaseVision()` |
| is the Unit visible? | `Unit::isStealthed()` |
| Min range | `Unit::getMinRange()`
| Max range | `Unit::getMaxRange()` |
| Base cost | `Unit::getBaseCosts()` |
| Total cost with modifiers involved? | `Unit::getCosts()` |
| can the unit capture? | `Unit::canCapture?` notice how it gets the list of actions the unit can perform and then checks that that list contains `CoreAI::Action_CAPTURE` | 
| Current state (loaded? capturing? waiting?) | Unknown but somewhat related to `Unit::updateStatusDurations()` and its use of `m_IconDurations`
| what units are in transports? | loop through transports held in field `m_TransportUnits` calling function `Unit::getLoadedUnit()` to return the unit in the transport |
| how many units are loaded inside of a transport unit? | `Unit::getLoadedUnitCount()` |

## Movement

| Questions | Answers |
|---|---|
| how is movement type determined? | `Unit::getMovementType()` returns a string like `MOVE_FEET` which can then be used by its correspondingly named `MOVE_FEET.js` file to get information about the way *feet* type movement can traverse terrain | 
| Where are movement points stored? | `Unit::getMovementPoints()` |
| How do I ask where a unit can move? | possibly `Unit::showRanges()` |
| Does the engine already generate reachable tiles? |
Does it use A*?

## Combat

| Questions | Answers |
| --- | --- |
| How do I determine whether one unit can attack another? | `Unit::isAttackableFromPosition()`, recall how no unit can attack a fogged unit. You must first have vision of the unit in order to attack. Similarly we have in `Unit::isAttackable()`, it ensuring that the function `Player::getFieldVisibileType()` returns `GameEnums::VisionType::VisionType_Clear` before it continues checking if the unit can be attacked |
| How is damage calculated? | `CoreAI::calcUnitDamage` for actual damage and `CoreAI::calcVirtuelUnitDamage` for damage checks in simulated attacks. There's a similar function in `DamageCalculator::calculateDamage()` |
| How are counterattacks handled? | `CoreAI::calculateCounterDamage` `Unit::canCounterAttack()` | 
| Luck? | `Unit::getBonusLuck()` |
| CO modifiers? | Depends. Can call `Unit::getBonusOffensive()`, `Unit::getBonusMovementPoints()`, `Unit::getBonusDeffensive()`, `Unit::getBonusMisfortune()`, `Unit::getBonusLuck()`, `Unit::getRepairBonus()`, etc.|



### References:

(1) A unit has two kinds of HP: `m_hp` and `m_virtualHp` accessed by functions `Unit::getHp()` and `Unit::getVirtualHp()` respectively. 

These functions are used differently. The AI uses `Unit::getVirtualHp()` as a scratch space when simulating possible actions. Suppose it were considering firing on unit0. In this case, it would calculate the damage it'd hypothetically deal to unit0 setting its `m_virtualHp` to the remaining hp after the unit was hypothetically hit without ever needing to roll back the true hp regardless what unit it actually ends up striking. 

**However `m_vitualHp` must be reset back to 0 when finished computing values for the unit. Uf this is not done, the AI will have an incorrect idea of how much health the unit has remaining and can therefore make incorrect decisions.**

As for `Unit::getHp()`, this function and its corresponding field `m_hp` is used to determine the true health of the unit--not the simulated health of the unit.



(2) A unit calls `Unit::getWeapon1ID()` or `Unit::getWeapon2ID()` which returns the string ID of the weapon used. An example is **WEAPON_INFANTRY_MG**. This weapon ID has a corresponding js file, `WEAPON_INFANTRY_MG.js` which has all the functions and data related to the weapon. There's more detail on how this all accessed in `04_combat.md` note (1)