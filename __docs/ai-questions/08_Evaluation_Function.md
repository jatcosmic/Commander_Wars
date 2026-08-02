# 08_Evaluation_Function.md

# Evaluation Function Implementation Concepts:

[Click here to jump to final evaluation scoring function](#final-evaluation-summary) 


## One caution: avoid double-counting

One thing to watch for is evaluating the same benefit multiple times.

For example:

* Threat map reports: "Factory can be blocked."
* Influence map reports: "This area controls the factory."
* Evaluation rewards: "Factory is blocked."
* Evaluation also rewards: "Production denied."

Those might all describe the same strategic fact.

A good pattern is:

* **Threat map** → reports possibilities ("this factory can be blocked")
* **Influence map** → reports territorial control ("this factory is under enemy pressure")
* **Evaluation**  → assigns value ("blocking this factory is worth +X because it denies production")

That keeps each subsystem focused and helps prevent the AI from overvaluing a single idea simply because multiple systems mention it.

___

## Primary Consideration: 

### Evaluate opportunities, Not just the current board

> static value is often less important than future consequences. 

A move isn't good because it captures a city, blocks a factory, or damages a unit in isolation. It's good because it **changes what each player can do over the next several turns. We should evaluate what the current position enables over the next few turns, not just what exists right now.** 

For example:

* A Recon delaying a city capture isn't valuable because it's on a city; it's valuable because it delays future income and forces an awkward response.
* A Missile beside an airport isn't valuable because of its location; it's valuable because it constrains what the opponent can safely produce.
* An Infantry threatening the HQ isn't valuable because it's adjacent; it's valuable because it creates an immediate winning threat.

That's an important design principle for your evaluation function. Rather than scoring only static facts, it should score the strategic opportunities and constraints the position creates. That aligns well with Advance Wars, where many strong moves derive their value from shaping the next several turns rather than from immediate material gains.

For something like this, we don't even need a perfect economic forecast to capture that. Even simple heuristics like:

* Expected turns of income denied
* Expected turns of production blocked
* Expected repair costs imposed
* Expected turns a forced response occupies an enemy unit

can give the evaluation function a much stronger strategic sense than just counting units and properties. These heuristics are also relatively inexpensive to compute compared to deep search, which makes them a good fit for a first-generation AI.

___

## What Makes a Good Evaluation Function? 

Create a clean evaluation function.

> It shouldn't compute threat maps.

> It shouldn't compute influence.

It simply scores a position using those results.

___

## What Categories should the evaluation function consider?

## 1. Material (What do I own?)

### Positive
* Unit strength (cost × HP)
* Property ownership
* Property income
* Production capability (factories, airports, harbors)
* Tech capability (ability to build important units)

### Negative

* Damaged expensive units
* Lost production capability
* Low fuel / ammo (if strategically important)

___

## 2. Position (Where are my units?)

### Positive
* Terrain advantage
* Chokepoint control
* Attack coverage (mutual support)
* Defensive support / Screening
* Front line cohesion
* Good formation
* Vision control (especially Fog)

### Negative
* Isolated units
* Poor positioning
* Exposed expensive units
* Vulnerable artillery
* Broken formation

Notice that "front line quality" becomes part of formation rather than a separate concept.

___

## 3. Tactical Value (What Happens if Fighting Starts?)

This section should come almost entirely from your combat simulator.

### Positive
* Favorable trades
* Safe attacks
* Strong retaliation potential
* Kill opportunities
* Protecting high-value units

### Negative
* Expected retaliation
* Threatened units
* Unsafe attacks
* Losing trades
* Overextended units

___

### 4. Economic Value

This becomes much richer than simply "income."

### Positive
* Income
* Capture progress
* Future income
* Production availability
* Repair efficiency

### Negative
* Losing economy
* Repair costs
* Delayed production
* Floating unspendable cash

___


## 5. Strategic Pressure

### Positive
* Property denial
* Production denial
* Factory blocking
* Airport blocking
* Harbor blocking
* Forced enemy responses
* Tempo
* Initiative
* Strategic diversion
* Capture contesting
* Repair-cost pressure

### Negative
* Being denied production
* Being forced into passive responses
* Losing initiative
* Being strategically contained

___

## 6. Win Conditions

This deserves its own category because these should dwarf almost everything else.

### Positive
* Threatening HQ capture
* Imminent HQ capture
* Winning by elimination
* Securing victory

### Negative
* Enemy threatens HQ
* Enemy threatens elimination
* Critical production loss

For example:

> Enemy infantry captures HQ next turn

should probably outweigh nearly every other heuristic since this leads directly to defeat.

___ 

## Expanding out some individual concepts

## Property value

```
Income

Cities

Factories

Airports

Harbors

HQ
```

___

## Capture progress

Capturing

```
Factory

Airport

HQ

etc.
```
__

## Positional Value

### Terrain advantage

Units standing on strong terrain.

___

## Attack coverage

### Units should support one another.

Example: mutually supporting tanks **are stronger** than isolated tanks.

___

## Defensive support / Screening

Cheap units

> (Mechs, Infantry)

can protect
```
Medium Tanks

Neotanks

Artillery
```

The evaluation should reward good screening.

___

## Expected retaliation


Example:

> Tank can kill infantry.

But, the Tank then dies to

```
Medium Tank

Artillery
```

The evaluation should consider **net exchange**, not just immediate damage.

This likely comes from the combat simulator.

___

## Unit exposure

Punish Expensive units that are left inside enemy threat.

___

## Threatened units

If a unit is threatened by multiple attackers, penalize accordingly.

___

## Safe Aggression

Reward attacks that

* gain material
* remain protected afterwards

___

## Front line quality

Units advancing together. Not isolated.

___

## Chokepoint control

```
Bridges

Mountain passes

Narrow roads
```

Often strategically valuable.

___

## Vision control

Especially important in **Fog of War.**

___

## Economy

Reward

* Income
* Properties
* Production

___

## Punish

* Losing economy
* Blocked captures
* Threatened production

___

## Strategic Objectives

This is where ideas like capturing the HQ belong.

Not in the threat map.

The threat map simply reports

> Enemy infantry can capture HQ next turn.

The evaluation says

> That's effectively catastrophic.

Likewise

Capturing enemy HQ

should receive an overwhelming positive score.

___

## CO-specific evaluation (Long-term idea--dont do add this in the first version. This is for later versions so that each CO can try to maximize their own effectiveness)

Instead of changing the search, change the evaluation weights.

### Examples:

#### Sami 

Early game:

Increase weight on
```
infantry
captures
economy
```
Late game

Reduce those weights.

Increase weights on

```
survival
positioning
preserving expensive units
```

___

#### Grit

Increase
```
Indirect fire

Terrain

Vision
```
___

#### Max

Increase
```
Direct combat

Tank trades
```

___

#### Kanbei
```
Preservation becomes more valuable because units cost more.
```

___

#### Eagle
```
Air superiority becomes more valuable.
```
___


## Production capability

Not just current income.

> Can you actually spend that income?

Owning 20 cities **isn't as valuable if every factory is blocked or threatened.**

___

## Strategic Pressure

These are the ways our function can consider strategic pressure. There's a few kinds of strategic pressure:

Positive

* Income gained
* Income denied
* Production denied
* Forced responses
* Tempo
* Initiative
* Property contesting
* Factory blocking
* Airport denial
* Repair cost pressure
* Strategic diversion
* Chokepoint control

Let's consider parts of them: 

### Denial Value

The AI should recognize that preventing an opponent from using an asset is often nearly as valuable as destroying it.

Examples:

* Blocking a factory with a unit.
* Parking a Missile beside an airport so air units can't safely spawn.
* Blocking a harbor.
* Occupying a bridge to prevent movement.
* Preventing APC resupply.

The key question is:

> **How much value is denied while this unit survives?**

This type of question can be answered by considered the future funds lost by delaying the capture of a property turn by turn; the funds trapped by blocking production of a building; or the opportunity cost in positional advantage by forcing a response out of the enemy on a weaker side than what they wanted to. 

Let's list some of the ways we can capture denial value:

### 1. Factory Blocking

Instead of evaluating

> Recon on Factory

as merely

> Recon Value

consider

```
Factory Production Denied

×

Expected Turns Blocked
```

For example,
```
Airport produces
15,000-cost Bomber

Missiles prevent safe production

for 3 turns
```

This isn't just "good positioning."

It's a **significant reduction** in the opponent's ability to convert money into military strength.

### 2. Income Denial

This is slightly different.

You're not denying production.

You're delaying income.

Suppose
```
City

1000 income

Normally

Turn 1

Enemy captures

↓

Turn 2

Enemy earns 1000
```

If you delay capture 2 turns

```
Enemy earns

0

0

1000
```

You've effectively denied
```
2000 funds
```
without owning the city yourself.

That's economically valuable. 

A possible long term improvement to computing this value could be considering how many turns you could reasonably contest a property's capture. That is, how long it would take for the enemy to create an effective response to your delay, how expensive that response could be, and how much the delay costs you to pursue (could be economically or could be in positional advantage on the map).  

### 3. Opportunity Cost

Suppose you build
```
Recon

4000
```
to contest
```
Infantry

1000
```
A naïve evaluation might conclude

> Terrible trade.

But that's missing several effects.

The Recon also:
```
delays capture
denies income
forces an enemy response
delays their expansion
buys time elsewhere
```
The true value is more like:
```
Capture delayed

+

Income denied

+

Enemy response forced

+

Tempo gained

+

Strategic initiative
```

### 4. Forced Responses

I think this deserves its own section.

Humans constantly make moves whose primary purpose is

> "You have to answer this."

Examples
```
Recon sitting on factory

↓

Enemy

must

build tank

or

send infantry

or

lose production.
```
That forced response has value.

It consumes

* production
* movement
* attention
* future flexibility

### 5. Resource Drain

Damaged units.

Example
```
Medium Tank

2 HP

continues surviving
```
Opponent repairs
```
2000 every turn.
```

The unit isn't contributing much combat value,

yet it's draining economy.

That's another kind of denial.

The evaluation could reward positions where
```
Expected Repair Costs
```
are imposed on the opponent.

### 6. Production Efficiency

This is different from income.

Imagine

Player A
```
Income

24000

Factories

1
```

Player B
```
Income

18000

Factories

5
```

Player A cannot actually spend all of that income efficiently. Which leads to a smaller army.

So the evaluation shouldn't only ask

> Who has more money?

It should ask

> Who can convert money into military strength?

## 7. Tempo

Tempo is essentially

> **Who is dictating the game?**

Examples

Force opponent

to

* defend HQ
* defend airport
* defend factory
* respond to capture
* answer artillery

instead of executing their own plan.

Reward

* forcing enemy responses
* maintaining initiative
* making productive moves

Sometimes gaining tempo is worth more than small material gains.

Tempo is difficult to measure directly, but it often emerges from several smaller heuristics:

* forced responses
* initiative
* denial
* mobility
* pressure

### 8. Long-Term Property Value

A property isn't worth merely
```
1000
```
It's worth
```
1000

×

Expected Remaining Turns
```

Obviously we can't know the future exactly.

But the AI can estimate.

Example:

Early game

Turn 4

Capturing city

may generate
```
18 turns

×

1000

=

18000
```

Late game

Turn 20

Same city

might only produce
```
4 turns

×

1000

=

4000
```

That's a huge difference and that difference also factors into considerations like why delaying property capture is important because it reduces the income the opponent receives over time. 

### 9. Strategic Diversion

Suppose
```
Left Front

Recon delays capture (or causes problems in general)
```

Opponent builds

```
Tank

Left
```

Now the Right Front is weaker beacuse even though nothing happened on the right, your move created an advantage there by diverting a tank away from the right and to the left front.

That's a strategic effect rather than a tactical one.

___

# Final Evaluation Summary:

The Evaluation Function when viewed as categories starts to look something like this:


**Best Version**

```
Material
+ Unit strength
+ Property ownership
+ Income
+ Production capability

Position
+ Terrain advantage
+ Attack coverage
+ Defensive support / Screening
+ Chokepoint control
+ Formation cohesion
+ Vision control

Tactical
+ Favorable combat trades
+ Safe attacks
+ Protected high-value units
- Expected retaliation
- Expected material loss
- Unsafe exchanges

Economic
+ Capture progress
+ Future income
+ Efficient repairs
- Repair costs
- Losing economy

Strategic Pressure
+ Property denial
+ Production denial
+ Factory/Airport blocking
+ Forced enemy responses
+ Initiative
+ Capture contesting
+ Strategic diversion

Victory
+ HQ capture threats
+ Elimination threats
- HQ danger
- Critical production loss
```

___

## My earlier evaluation function:

This is kept for historical purposes. Also its simpler scoring approach allows me to reason about an evaluation score better than the categorial scoring does. It also captures more individual elements directly and this directness allows me to remember certain things that I may forget to implement in the categorical approach above.



```
+ Unit strength
+ Property income
+ Capture progress
+ Terrain advantage
+ Attack coverage
+ Defensive support / Screening
+ Expected retaliation
+ Vision control
+ Chokepoint control
+ Front line quality
+ Tempo
+ Production capability

- Exposed expensive units
- Threatened units
- Unsafe attacks
- Losing economy
- Poor positioning
- Isolated units
```