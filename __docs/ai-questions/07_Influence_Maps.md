# 07_Influence_Maps.md

## Influence Map Implementation Concepts

[Click here to jump to Questions and answers](#questions-and-answers)


Influence is different than the threat map.

Threat is

> What can happen immediately?

Influence is

> Which side controls this area?

___

## Friendly Influence

Where your army projects power.

___

## Enemy Influence

Where your enemy's army projects power.

___

## Net Influence

```
Friendly

minus

Enemy
```

Interpretation: 
 
> Positive values = Safe

> **Negative values = Enemy-controlled**
___

## Economic Influence

```
Cities

Factories

HQ

Airport

Harbor
```

___

## Reinforcement Influence

How quickly each side can reinforce an area.

This becomes **very important** on large maps.

___

## Front line detection

Where friendly and enemy influence meet.

Useful for

* artillery
* tanks
* pushing
* retreating

___

### Strategic terrain
```
Bridges

Mountain passes

Narrow roads
```
These often deserve extra influence weight.

___

## Vision influence (Fog)

```
Recons

Mountains

Forests

Hidden units
```

Vision is **extremely valuable** in fog.

___

## CO-specific Influence (Long Term idea)

```
Sami

↓

Infantry influence matters more.
```
```
Grit

↓

Indirect influence matters more.
```
```
Max

↓

Direct combat influence matters more.
```

So the influence map itself can remain generic, while the weights change depending on the CO.

___

## Questions and Answers:

| Questions | Answers |
| --- | --- |
| What is influence? | Which side controls this area |
| How should influence decay? | Decay movement by movement cost, not tile distance. A mountain range blocks more influence than 3 empty plain tiles. Can probably use something like this `enemyData.pUnitPfs->getAllNodePointsFast()` from `NormalAI::calculateCounterDamage()` to get the pathfinder object per enemy unit |
| Linear or exponential? | Start linear not exponential. The decay curve itself is a tuning knob that can be revisited when watching the AI play and see influence maps mis-predicting front lines. |

___

| Question | Answer |
| --- | --- |
| Should influence depend on HP? Cost? CO? | Weight influence by `HP/cost`. A 2-HP tank shouldn't project the same influence as a full-HP one — something like `cost × (hp/maxhp)` as the "strength" a unit radiates outward is a reasonable starting point. |

___

Multiple maps?

Friendly influence

Enemy influence

City influence

Production influence

___

| Question | Answer |
| --- | --- |
| How often should influence be recomputed? | Recompute once per turn, not per unit — this also matches how the shipped AI caches its enemy data once per turn rather than recomputing per candidate move. |
