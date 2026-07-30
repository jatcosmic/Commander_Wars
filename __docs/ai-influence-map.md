# Existing Influence Map

Location:
ai/influencefrontmap.cpp

Purpose:
Estimate military presence over the battlefield.

## Notable function:

- `void InfluenceFrontMap::addUnitInfluence(Unit* pUnit, UnitPathFindingSystem* pPfs, qint32 movePoints)`

Inputs:
- Unit combat value
- Reachable tiles
- Movement cost distance

Formula:

Influence =
    UnitValue / DistanceDivider

## Potential Limitations of Influence Map as a Whole:

- Does not appear to distinguish unit types
- Does not appear to include terrain defense
- Does not appear to include strategic objectives