# Threat Map Implementation Concepts

[Click here to jump to Questions and Answers](#threat-map-questions-and-answers)

The threat map answers one question:

> **"What can the enemy do?"**

It should not decide whether something is good or bad—that's the evaluation's job.

## Threat Map should include

### Direct attack threat

Which tiles can be attacked next turn?

Examples:

* Tanks
* Infantry
* Recons

___

### Indirect attack threat

Artillery

Rocket

Battleship

Missiles

These need movement + firing range.

___

### Capture threats

Instead of only asking

> Can this infantry attack me?

also ask

> Can this infantry capture an important property?

Examples

* HQ
* Airport
* Factory
* City

The evaluation function can later decide **how valuable** those captures are.

___

### Production threats

Enemy threatens

* Factory
* Airport
* Harbor

These are often more valuable than ordinary terrain.

___ 

### Property denial

Enemy threatens cities you're capturing.

Enemy threatens your income.

___

### Counterattack opportunities

For every threatened tile, can the attacker itself be destroyed?

This becomes useful later during evaluation.

___

### Multiple attackers

Instead of simply
```
Tank threatened
```

track
```
Tank threatened by

Artillery
Tank
Mech
```

because multiple threats matter much more than one.

___

### Threat severity

Threat should probably store more than a boolean.

Example
```
Tile

Attackers

Expected Damage

Expected Cost Lost
```

# Threat Map Questions and Answers

## How do I compute:

| Question | Answer |
| --- | --- |
Enemy attack range?
Friendly attack range? | `CoreAI::getAttackTargets()`, `CoreAi::hasTargets()` |
| Indirect fire? |
Fog?

___

Should threat include:

Movement?

Example:

Tank

```
Moves 6

Attacks 1
```

Threat radius

```7```

___

How are indirect units handled?

Artillery

```
Move 5

Attack 2-3
```

Threat isn't a circle.

It's
```
Move

↓

Attack
```
___

Should threat account for

HP?
Ammo?
Fuel?