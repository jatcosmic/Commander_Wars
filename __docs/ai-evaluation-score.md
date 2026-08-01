# AI Evaluation Scoring:

A really good first Advance Wars AI could be entirely hand-built

Something like:

Evaluation:

+ unit strength
+ property income
+ capture progress
+ terrain advantage
+ attack coverage
+ defensive support
+ vision control

- exposed expensive units
- threatened units
- bad terrain
- losing economy

Then minimax searches.



Instead of merely

Can this unit attack?

consider

After attacking,

how vulnerable is this unit?

For example
```
Attack Value

=

Damage dealt

-

Expected retaliation
```

where

Expected retaliation

depends on

• enemy range
• enemy movement
• blocked paths
• friendly screens
• terrain