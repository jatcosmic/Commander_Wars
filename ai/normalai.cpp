#include <QElapsedTimer>
#include <QCoreApplication>

#include "coreengine/qmlvector.h"
#include "coreengine/globalutils.h"
#include "coreengine/gameconsole.h"
#include "coreengine/interpreter.h"

#include "game/player.h"
#include "game/unit.h"
#include "game/co.h"
#include "game/gameaction.h"
#include "game/gamemap.h"
#include "game/building.h"
#include "game/unitpathfindingsystem.h"

#include "ai/normalai.h"
#include "ai/targetedunitpathfindingsystem.h"

#include "resource_management/weaponmanager.h"
#include "resource_management/movementtablemanager.h"

NormalAi::NormalAi(GameMap *pMap, const QString &configurationFile, GameEnums::AiTypes aiType, const QString &jsName)
    : CoreAI(pMap, aiType, jsName),
    m_InfluenceFrontMap(pMap, m_IslandMaps),
    m_captureBuildingSelector(*this),
    m_transporterSelector(*this)
{
#ifdef GRAPHICSUPPORT
    setObjectName("NormalAi");
#endif
    CONSOLE_PRINT("Creating normal ai", GameConsole::eDEBUG);
    Interpreter::setCppOwnerShip(this);
    setupJsThis(this);
    m_timer.setSingleShot(false);
    connect(&m_timer, &QTimer::timeout, this, &NormalAi::process, Qt::QueuedConnection);
    // [AI: REPLACE - AI POLICY]
    // The entire tuning table below is Normal AI's hand-tuned weight set: threshold
    // constants, bonus/malus multipliers and probability percentages, loaded from
    // normal/*.ini. Nearly every heuristic flagged elsewhere in this file reads one
    // of these values, so this table is the single root of the tuned behaviour.
    // Search AI must not inherit it. Its evaluator needs weights expressed in one
    // common currency (funds, per AI_Design_Notes_Consolidated.md section 4), not
    // an unstructured pile of unitless magic numbers spanning damage percentages,
    // funds amounts, HP counts, tile distances and 0-100 dice chances.
    m_iniData = {
        // General
        {"MinMovementDamage", "General", &m_minMovementDamage, 0.3f, -1.0f, 1.0f},
        {"MinAttackFunds", "General", &m_minAttackFunds, 0.0f, -1.0f, 1.0f},
        {"MinSuicideDamage", "General", &m_minSuicideDamage, 0.75f, -1.0f, 1.0f},
        {"SpamingFunds", "General", &m_spamingFunds, 7500.0f, 5000.0f, 10000.0f},
        {"OwnUnitValue", "General", &m_ownUnitValue, 2.0f, -10.0f, 10.0f},
        {"BuildingValue", "General", &m_buildingValue, 1.0f, 1.0f, 1.0f},
        {"NotAttackableDamage", "General", &m_notAttackableDamage, 25.0f, 0.0f, 40.0f},
        {"MidDamage", "General", &m_midDamage, 55.0f, 40.0f, 60.0f},
        {"HighDamage", "General", &m_highDamage, 75.0f, 60.0f, 100.0f},
        {"DirectIndirectRatio", "General", &m_directIndirectRatio, 5.0f, 0.2f, 10.0f},
        {"MinSiloDamage", "General", &m_minSiloDamage, 7000.0f, 7000.0f, 7000.0f},
        {"MinSameIslandDistance", "General", &m_minSameIslandDistance, 3.0f, 3.0f, 3.0f},
        {"SlowUnitSpeed", "General", &m_slowUnitSpeed, 2.0f, 2.0f, 2.0f},
        {"EnemyPruneRange", "General", &m_enemyPruneRange, 3.0f, 1.0f, 5.0f},
        // CO Unit
        {"CoUnitValue", "CoUnit", &m_coUnitValue, 6000.0f, 5000.0f, 20000.0f},
        {"MinCoUnitScore", "CoUnit", &m_minCoUnitScore, 5000.0f, 3000.0f, 20000.0f},
        {"CoUnitRankReduction", "CoUnit", &m_coUnitRankReduction, 1000.0f, 0.0f, 5000.0f},
        {"CoUnitScoreMultiplier", "CoUnit", &m_coUnitScoreMultiplier, 1.1f, 1.0f, 3.0f},
        {"MinCoUnitCount", "CoUnit", &m_minCoUnitCount, 5.0f, 1.0f, 10.0f},
        // Repairing
        {"MinUnitHealth", "Repairing", &m_minUnitHealth, 3.0f, 0.0f, 10.0f},
        {"MaxUnitHealth", "Repairing", &m_maxUnitHealth, 7.0f, 0.0f, 10.0f},
        {"FuelResupply", "Repairing", &m_fuelResupply, 0.33f, 0.0f, 0.7f},
        {"AmmoResupply", "Repairing", &m_ammoResupply, 0.25f, 0.0f, 0.7f},
        // Moving
        {"LockedUnitHp", "Moving", &m_lockedUnitHp, 4.0f, 1.0f, 10.0f},
        {"NoMoveAttackHp", "Moving", &m_noMoveAttackHp, 3.5f, 1.0f, 10.0f},
        {"MinTerrainDamage", "Moving", &m_minTerrainDamage, 20.0f, 20.0f, 20.0f},
        {"InfluenceUnitRange", "Moving", &m_influenceUnitRange, 1.75f, 1.0f, 5.0f},

        // Attacking
        {"OwnIndirectAttackValue", "Attacking", &m_ownIndirectAttackValue, 2.0f, 0.1f, 10.0f},
        {"EnemyKillBonus", "Attacking", &m_enemyKillBonus, 2.0f, 0.1f, 10.0f},
        {"EnemyIndirectBonus", "Attacking", &m_enemyIndirectBonus, 3.0f, 0.1f, 10.0f},
        {"AntiCaptureHqBonus", "Attacking", &m_antiCaptureHqBonus, 50.0f, 0.0f, 10000.0f},
        {"AntiCaptureBonus", "Attacking", &m_antiCaptureBonus, 21.0f, 0.0f, 100.0f},
        {"AntiCaptureBonusScoreReduction", "Attacking", &m_antiCaptureBonusScoreReduction, 6.0f, 0.0f, 200.0f},
        {"AntiCaptureBonusScoreDivider", "Attacking", &m_antiCaptureBonusScoreDivider, 2.0f, 0.1f, 10.0f},
        {"EnemyCounterDamageMultiplier", "Attacking", &m_enemyCounterDamageMultiplier, 10.0f, 0.1f, 40.0f},
        {"WatermineDamage", "Attacking", &m_watermineDamage, 4.0f, 4.0f, 4.0f},
        {"EnemyUnitCountDamageReductionMultiplier", "Attacking", &m_enemyUnitCountDamageReductionMultiplier, 0.5f, 0.01f, 10.0f},
        {"OwnProdctionMalus", "Attacking", &m_ownProdctionMalus, 5000.0f, 5000.0f, 50000.0f},
        {"SupportDamageBonus", "Attacking", &m_supportDamageBonus, 1.0f, 0.1f, 10.0f},
        {"InfluenceIgnoreValue", "Attacking", &m_influenceIgnoreValue, 0.2f, 0.01f, 1.0f},
        {"InfluenceMultiplier", "Attacking", &m_influenceMultiplier, 2.0f, 0.1f, 10.0f},
        {"MinHpDamage", "Attacking", &m_minHpDamage, -2.0f, -10.0f, 0.0f},

        // Production
        {"FundsPerBuildingFactorA", "Production", &m_fundsPerBuildingFactorA, 1.85f, 0.5f, 10.0f},
        {"FundsPerBuildingFactorB", "Production", &m_fundsPerBuildingFactorB, 2.0f, 0.5f, 10.0f},
        {"FundsPerBuildingFactorC", "Production", &m_fundsPerBuildingFactorC, 3.0f, 1.0f, 10.0f},
        {"FundsPerBuildingFactorD", "Production", &m_fundsPerBuildingFactorD, 3.0f, 1.0f, 10.0f},
        {"OwnUnitEnemyUnitRatioAverager", "Production", &m_ownUnitEnemyUnitRatioAverager, 10.0f, 5.0f, 30.0f},
        {"MaxDayScoreVariancer", "Production", &m_maxDayScoreVariancer, 10.0f, 5.0f, 30.0f},
        {"StartDayScoreVariancer", "Production", &m_startDayScoreVariancer, 5.0f, 0.0f, 15.0f},
        {"DirectIndirectUnitBonusFactor", "Production", &m_directIndirectUnitBonusFactor, 1.75f, 0.1f, 10.0f},
        {"ScoringCutOffDamageHigh", "Production", &m_scoringCutOffDamageHigh, 100.0f, 100.0f, 100.0f},
        {"ScoringCutOffDamageLow", "Production", &m_scoringCutOffDamageLow, 7.5f, 0.1f, 40.0f},
        {"SmoothingValue", "Production", &m_smoothingValue, 3.0f, 1.0f, 20.0f},
        {"MaxDistanceMultiplier", "Production", &m_maxDistanceMultiplier, 1.5f, 1.0f, 10.0f},
        {"SameIslandBonusInRangeDays", "Production", &m_sameIslandBonusInRangeDays, 2.0f, 1.0f, 10.0f},
        {"SameIslandOutOfDayMalusFactor", "Production", &m_sameIslandOutOfDayMalusFactor, 0.2f, 0.01f, 1.0f},
        {"HighDamageBonus", "Production", &m_highDamageBonus, 2.0f, 0.0f, 10.0f},
        {"MidDamageBonus", "Production", &m_midDamageBonus, 1.5f, 0.0f, 10.0f},
        {"LowDamageBonus", "Production", &m_lowDamageBonus, 1.0f, 0.0f, 10.0f},
        {"VeryLowDamageBonus", "Production", &m_veryLowDamageBonus, 0.5f, 0.0f, 10.0f},
        {"TransportBonus", "Production", &m_transportBonus, 0.125f, 0.0f, 10.0f},
        {"CurrentlyNotAttackableBonus", "Production", &m_currentlyNotAttackableBonus, 0.5f, 0.0f, 10.0f},
        {"DifferentIslandBonusInRangeDays", "Production", &m_differentIslandBonusInRangeDays, 1.0f, 1.0f, 1.0f},
        {"DifferentIslandOutOfDayMalusFactor", "Production", &m_differentIslandOutOfDayMalusFactor, 0.33f, 0.33f, 0.33f},
        {"NoTransporterBonus", "Production", &m_noTransporterBonus, 70.0f, 70.0f, 70.0f},
        {"TransporterToRequiredPlaceFactor", "Production", &m_transporterToRequiredPlaceFactor, 6.0f, 6.0f, 6.0f},
        {"MinFlyingTransportScoreForBonus", "Production", &m_minFlyingTransportScoreForBonus, 15.0f, 15.0f, 15.0f},
        {"FlyingTransporterBonus", "Production", &m_flyingTransporterBonus, 15.0f, 15.0f, 15.0f},
        {"SmallTransporterBonus", "Production", &m_smallTransporterBonus, 30.0f, 30.0f, 30.0f},
        {"UnitToSmallTransporterRatio", "Production", &m_unitToSmallTransporterRatio, 5.0f, 5.0f, 5.0f},
        {"AdditionalLoadingUnitBonus", "Production", &m_additionalLoadingUnitBonus, 10.0f, 10.0f, 10.0f},
        {"IndirectUnitAttackCountMalus", "Production", &m_indirectUnitAttackCountMalus, 4.0f, 0.0f, 20.0f},
        {"MinAttackCountBonus", "Production", &m_minAttackCountBonus, 5.0f, 0.0f, 20.0f},
        {"LowIndirectUnitBonus", "Production", &m_lowIndirectUnitBonus, 30.0f, 0.0f, 100.0f},
        {"LowIndirectMalus", "Production", &m_lowIndirectMalus, 30.0f, 0.0f, 100.0f},
        {"HighIndirectMalus", "Production", &m_highIndirectMalus, 40.0f, 0.0f, 100.0f},
        // [AI: INVESTIGATE]
        // Existing bug, not a heuristic: "LowDirectUnitBonus" is bound to
        // &m_highIndirectMalus rather than &m_lowDirectUnitBonus. So HighIndirectMalus
        // is written twice (the ini's LowDirectUnitBonus value wins) and
        // m_lowDirectUnitBonus is never loaded -- it keeps the 0.35 header default
        // while every neighbouring bonus is on a 0-100 scale. That makes the direct
        // unit bonus in calcBuildScore effectively dead. Worth knowing before using
        // any observed Normal AI production behaviour as a reference baseline.
        {"LowDirectUnitBonus", "Production", &m_highIndirectMalus, 35.0f, 0.0f, 100.0f},
        {"LowDirectMalus", "Production", &m_lowDirectMalus, 20.0f, 0.0f, 100.0f},
        {"HighDirectMalus", "Production", &m_highDirectMalus, 40.0f, 0.0f, 100.0f},
        {"MinUnitCountForDamageBonus", "Production", &m_minUnitCountForDamageBonus, 3.0f, 0.0f, 20.0f},
        {"CurrentlyNotAttackableScoreBonus", "Production", &m_currentlyNotAttackableScoreBonus, 30.0f, 0.0f, 100.0f},
        {"CoUnitBuffBonus", "Production", &m_coUnitBuffBonus, 17.0f, 5.0f, 100.0f},
        {"NearEnemyBonus", "Production", &m_nearEnemyBonus, 10.0f, 0.0f, 100.0f},
        {"MovementpointBonus", "Production", &m_movementpointBonus, 6.0f, 0.0f, 30.0f},
        {"DamageToUnitCostRatioBonus", "Production", &m_damageToUnitCostRatioBonus, 20.0f, 0.0f, 100.0f},
        {"SuperiorityRatio", "Production", &m_superiorityRatio, 1.8f, 1.0f, 10.0f},
        {"CheapUnitRatio", "Production", &m_cheapUnitRatio, 1.8f, 0.5f, 2.0f},
        {"NormalUnitRatio", "Production", &m_normalUnitRatio, 1.0f, 0.8f, 1.2f},
        {"CheapUnitValue", "Production", &m_cheapUnitValue, 3000.0f, 1500.0f, 5000.0f},
        {"CheapUnitBonusMultiplier", "Production", &m_cheapUnitBonusMultiplier, 45.0f, 0.0f, 100.0f},
        {"NormalUnitBonusMultiplier", "Production", &m_normalUnitBonusMultiplier, 45.0f, 0.0f, 100.0f},
        {"ExpensiveUnitBonusMultiplier", "Production", &m_expensiveUnitBonusMultiplier, 20.0f, 0.0f, 100.0f},
        {"LowOwnBuildingEnemyBuildingRatio", "Production", &m_lowOwnBuildingEnemyBuildingRatio, 1.25f, 0.0f, 10.0f},
        {"LowInfantryRatio", "Production", &m_lowInfantryRatio, 0.4f, 0.0f, 1.0f},
        {"LowIncomeInfantryBonusMultiplier", "Production", &m_lowIncomeInfantryBonusMultiplier, 50.0f, 0.0f, 100.0f},
        {"ProducingTransportSearchrange", "Production", &m_ProducingTransportSearchrange, 6.0f, 6.0f, 6.0f},
        {"ProducingTransportSizeBonus", "Production", &m_ProducingTransportSizeBonus, 15.0f, 15.0f, 15.0f},
        {"ProducingTransportRatioBonus", "Production", &m_ProducingTransportRatioBonus, 10.0f, 10.0f, 10.0f},
        {"ProducingTransportLoadingBonus", "Production", &m_ProducingTransportLoadingBonus, 15.0f, 15.0f, 15.0f},
        {"ProducingTransportMinLoadingTransportRatio", "Production", &m_ProducingTransportMinLoadingTransportRatio, 4.5f, 2.0f, 7.0f},
        {"BuildingBonusMultiplier", "Production", &m_buildingBonusMultiplier, 6.0f, 0.0f, 30.0f},
        {"MinInfantryCount", "Production", &m_minInfantryCount, 2.0f, 0.0f, 10.0f},
        {"CanSupplyBonus", "Production", &m_canSupplyBonus, 10.0f, 5.0f, 40.0f},
        {"MaxSupplyUnitRatio", "Production", &m_maxSupplyUnitRatio, 0.05f, 0.01f, 0.05f},
        {"AverageSupplySupport", "Production", &m_averageSupplySupport, 8.0f, 0.0f, 20.0f},
        {"CappingFunds", "Production", &m_cappingFunds, 6500.0f, 2000.0f, 8000.0f},
        {"CappedFunds", "Production", &m_cappedFunds, 1500.0f, 500.0f, 4000.0f},
        {"TargetPriceDifference", "Production", &m_targetPriceDifference, 0.35f, 0.0f, 1.0f},
        {"MaxCloseDistanceDamageBonus", "Production", &m_maxCloseDistanceDamageBonus, 1.0f, 1.1f, 10.0f},
        {"MinCloseDistanceDamageBonus", "Production", &m_minCloseDistanceDamageBonus, 1.0f, 1.1f, 10.0f},
        {"SameFundsMatchUpBonus", "Production", &m_sameFundsMatchUpBonus, 16.0f, 10.0f, 60.0f},
        {"SameFundsMatchUpMovementMalus", "Production", &m_sameFundsMatchUpMovementMalus, 0.3f, 0.1f, 1.0f},
        {"AttackCountBonus", "Production", &m_attackCountBonus, 25.0f, 5.0f, 60.0f},
        {"MaxOverkillBonus", "Production", &m_maxOverkillBonus, 2.0f, 1.5f, 10.0f},
        {"CounterDamageBonus", "Production", &m_counterDamageBonus, 25.0f, 1.0f, 100.0f},
        {"EarlyGame", "Production", &m_earlyGame, 5.0f, 2.f, 10.0f},
        {"MaxProductionBuildings", "Production", &m_maxProductionBuildings, 5.0f, 2.f, 10.0f},
        {"LowThreadDamage", "Production", &m_lowThreadDamage, 10.0f, 1.f, 20.0f},
        {"MaxProductionBuildingsForB", "Production", &m_maxProductionBuildingsForB, 1.0f, 1.f, 5.0f},
        {"TurnOneDmageMalus", "Production", &m_turnOneDmageMalus, 10.0f, 1.f, 20.0f},
        {"CounterUnitRatio", "Production", &m_counterUnitRatio, 2.0f, 1.f, 5.0f},
        {"SpamInfantryChance", "Production", &m_spamInfantryChance, 50.0f, 100.0f, 50.0f},
        {"SameFundsMatchUpNoMatchUpValue", "Production", &m_sameFundsMatchUpNoMatchUpValue, 0.5f, 0.0f, 1.0f},
        {"SpamLightUnitChance", "Production", &m_spamLightUnitChance, 30.0f, 0.0f, 100.0f},
        {"SpamMediumUnitChance", "Production", &m_spamMediumUnitChance, 30.0f, 0.0f, 100.0f},
        {"OwnBuildingPruneRange", "Production", &m_ownBuildingPruneRange, 10.0f, 7.0f, 10.0f},
    };

    if (m_pMap != nullptr &&
        !m_pMap->getSavegame())
    {
        loadIni("normal/" + configurationFile);
    }
    // [AI: REPLACE - AI POLICY]
    // Hard-coded per-unit production weight ("build Mech 10% more often").
    // A blanket unit-ID multiplier is a designer preference, not a property of the
    // board. Search AI should reach the same conclusion, when it is true, from the
    // enemy composition and terrain instead of a fixed constant.
    m_BuildingChanceModifier.insert("MECH", 1.1f);
}

void NormalAi::process()
{
    AI_CONSOLE_PRINT("NormalAi::process()", GameConsole::eDEBUG);
    if (m_pause)
    {
        m_timer.start(1000);
        return;
    }
    else
    {
        m_timer.stop();
    }
    spQmlVectorBuilding pBuildings = m_pPlayer->getSpBuildings();
    // [AI: REPLACE - AI POLICY]
    // Existing AI shuffles its own building list so that ties later in buildUnits()
    // resolve to a different building each turn. Search AI must be deterministic:
    // the order buildings are considered in cannot be allowed to change the outcome.
    // If two production sites really are equivalent the evaluator should say so and
    // the tie should be broken by a stable rule, not by a shuffle.
    pBuildings->randomize();
    spQmlVectorUnit pUnits = m_pPlayer->getSpUnits();
    spQmlVectorUnit pEnemyUnits;
    spQmlVectorBuilding pEnemyBuildings;
    qint32 cost = 0;
    // [AI: REPLACE - AI POLICY]
    // Existing AI reduces silo usage to one boolean by testing the best silo target's
    // funds damage against a fixed threshold (MinSiloDamage, pinned at 7000).
    // Firing a silo is an action with a cost and a board consequence; Search AI should
    // generate it as a candidate and let the evaluator rank it against every other
    // action, rather than pre-approving it with a constant.
    // The (2, 3) blast/search arguments are themselves unexplained magic numbers.
    m_pPlayer->getSiloRockettarget(2, 3, cost);
    m_missileTarget = (cost >= m_minSiloDamage);
    if (useBuilding(pBuildings, pUnits))
    {
    }
    else
    {
        AI_CONSOLE_PRINT("NormalAi::creating unit arrays()", GameConsole::eDEBUG);
        prepareEnemieData(pUnits, pBuildings, pEnemyUnits, pEnemyBuildings);
        updateAllUnitData(pUnits, pBuildings);
        if (useCOPower(pUnits, pEnemyUnits))
        {
            clearUnitData();
        }
        else
        {
            m_turnMode = GameEnums::AiTurnMode_DuringDay;
            if (performActionSteps(pUnits, pEnemyUnits, pBuildings, pEnemyBuildings))
            {
            }
            else
            {
                if (m_aiStep == AISteps::buildUnits)
                {
                    m_aiStep = AISteps::moveUnits;
                }
                if (performActionSteps(pUnits, pEnemyUnits, pBuildings, pEnemyBuildings))
                {
                }
                else
                {
                    if (m_aiStep == AISteps::buildUnits)
                    {
                        m_aiStep = AISteps::moveUnits;
                        m_aiFunctionStep = 0;
                    }
                    clearUnitData();
                    m_IslandMaps.clear();
                    m_turnMode = GameEnums::AiTurnMode_EndOfDay;
                    if (useCOPower(pUnits, pEnemyUnits))
                    {
                        m_usedTransportSystem = false;
                        m_usedPredefinedAi = false;
                        m_turnMode = GameEnums::AiTurnMode_DuringDay;
                    }
                    else
                    {
                        m_turnMode = GameEnums::AiTurnMode_StartOfDay;
                        finishTurn();
                    }
                }
            }
        }
    }
}

void NormalAi::toggleAiPause()
{
    m_pause = !m_pause;
}

void NormalAi::showIslandMap(QString unitId)
{
    Unit unit(unitId, m_pPlayer, false, m_pMap);
    qint32 unitIslandIdx = getIslandIndex(&unit);
    if (unitIslandIdx >= 0 && unitIslandIdx < m_IslandMaps.size())
    {
        m_IslandMaps[unitIslandIdx]->show();
    }
}

void NormalAi::hideIslandMap(QString unitId)
{
    Unit unit(unitId, m_pPlayer, false, m_pMap);
    qint32 unitIslandIdx = getIslandIndex(&unit);
    if (unitIslandIdx >= 0 && unitIslandIdx < m_IslandMaps.size())
    {
        m_IslandMaps[unitIslandIdx]->hide();
    }
}

void NormalAi::showFrontMap()
{
    m_InfluenceFrontMap.show();
}

void NormalAi::showFrontLines()
{
    m_InfluenceFrontMap.showFrontlines();
}

void NormalAi::hideFrontMap()
{
    m_InfluenceFrontMap.hide();
}

void NormalAi::resetToTurnStart()
{
    clearUnitData();
    m_productionData.clear();
    m_captureBuildingSelector.resetUsedFarAwayBuildings();
    m_secondMoveRound = false;
    CoreAI::resetToTurnStart();
}

void NormalAi::finishTurn()
{
    AI_CONSOLE_PRINT("NormalAi::finishTurn()", GameConsole::eDEBUG);
    m_productionData.clear();
    m_secondMoveRound = false;
    CoreAI::finishTurn();
}

bool NormalAi::performActionSteps(spQmlVectorUnit &pUnits, spQmlVectorUnit &pEnemyUnits,
                                  spQmlVectorBuilding &pBuildings, spQmlVectorBuilding &pEnemyBuildings)
{
    AI_CONSOLE_PRINT("NormalAi::performActionSteps()", GameConsole::eDEBUG);
    // [AI: REPLACE - AI POLICY]
    // This whole if/else-if ladder is Normal AI's turn structure, and it is the single
    // largest policy decision in the file. The ordering itself is the strategy:
    // capture before shooting, indirects before directs, repair before refill, build
    // units last. The first branch that succeeds performs one action and returns,
    // so a lower branch never gets to argue that its action was worth more.
    //
    // This is action-shaped control flow (AI_Design_Notes_Consolidated.md section 6.1):
    // it decides *what kind of thing to do* before anything has evaluated a board state.
    // A capture is taken because capture ranks above attack in this list, not because
    // the resulting board is better.
    //
    // Search AI should not reproduce this ladder. It should enumerate every legal
    // action for every unit as candidates in one pool, score the resulting board, and
    // take the max -- with the sequencing concern handled by move ordering
    // (section 6.3: resolve expensive units first), not by a fixed behaviour priority.
    // Note the ladder also carries the resume/step state machine (m_aiStep,
    // m_aiFunctionStep, nextAiStep) that lets the AI be re-entered after each action;
    // that mechanism is worth keeping even though the priority ordering is not.
    if (m_aiStep <= AISteps::moveUnits && buildCOUnit(pUnits))
    {
    }
    else if (m_aiStep <= AISteps::moveUnits && CoreAI::moveFlares(pUnits))
    {
    }
    else if (m_aiStep <= AISteps::moveUnits && CoreAI::moveOoziums(pUnits, pEnemyUnits))
    {
    }
    else if (m_aiStep <= AISteps::moveUnits && CoreAI::moveBlackBombs(pUnits, pEnemyUnits))
    {
    }
    else if (m_aiStep <= AISteps::moveUnits && captureBuildings(pUnits, pBuildings, pEnemyBuildings))
    {
    }
    else if (m_aiStep <= AISteps::moveUnits && joinCaptureBuildings(pUnits))
    {
    }
    else if (m_aiStep <= AISteps::moveUnits && moveSupport(AISteps::moveUnits, pUnits, false))
    {
    }
    // indirect units
    else if (m_aiStep <= AISteps::moveUnits && fireWithUnits(pUnits, 1, 2, std::numeric_limits<qint32>::max(), pBuildings, pEnemyBuildings))
    {
    }
    // direct units
    else if (m_aiStep <= AISteps::moveUnits && fireWithUnits(pUnits, 1, 1, 1, pBuildings, pEnemyBuildings))
    {
    }
    else if (m_aiStep <= AISteps::moveUnits && repairUnits(pUnits, pBuildings, pEnemyBuildings))
    {
    }
    else if (m_aiStep <= AISteps::moveToTargets && refillUnits(pUnits, pBuildings, pEnemyBuildings))
    {
    }
    else if (m_aiStep <= AISteps::moveToTargets && moveUnits(pUnits, pBuildings, pEnemyUnits, pEnemyBuildings, 1, 1))
    {
    }
    else if (m_aiStep <= AISteps::moveIndirectsToTargets && moveUnits(pUnits, pBuildings, pEnemyUnits, pEnemyBuildings, 2, std::numeric_limits<qint32>::max()))
    {
    }
    else if (m_aiStep <= AISteps::loadUnits && !m_usedTransportSystem && loadUnits(pUnits, pBuildings, pEnemyBuildings))
    {
    }
    else if (m_aiStep <= AISteps::moveTransporters && !m_usedTransportSystem && moveTransporters(pUnits, pEnemyUnits, pBuildings, pEnemyBuildings))
    {
    }
    else
    {
        if (!m_usedTransportSystem)
        {
            m_usedTransportSystem = true;
            m_aiStep = AISteps::moveUnits;
            m_aiFunctionStep = 0;
            prepareEnemieData(pUnits, pBuildings, pEnemyUnits, pEnemyBuildings);
            for (auto &unit : m_OwnUnits)
            {
                if (!unit.pUnit->getHasMoved())
                {
                    unit.nextAiStep = m_aiFunctionStep;
                }
            }
            return performActionSteps(pUnits, pEnemyUnits, pBuildings, pEnemyBuildings);
        }
        else if (m_aiStep <= AISteps::loadUnits && loadUnits(pUnits, pBuildings, pEnemyBuildings))
        {
        }
        else if (m_aiStep <= AISteps::moveSupportUnits && moveSupport(AISteps::moveSupportUnits, pUnits, true))
        {
        }
        else if (m_aiStep <= AISteps::moveSupportUnits && moveUnits(pUnits, pBuildings, pEnemyUnits, pEnemyBuildings, 1, std::numeric_limits<qint32>::max(), true))
        {
        }
        else if (m_aiStep <= AISteps::moveAway && moveAwayFromProduction(pUnits))
        {
        }
        else if (m_aiStep <= AISteps::buildUnits && buildUnits(pBuildings, pUnits, pEnemyUnits, pEnemyBuildings))
        {
        }
        else
        {
            return false;
        }
    }
    return true;
}

// [AI: REPLACE - AI POLICY]
// Existing AI uses two fixed HP thresholds (MinUnitHealth 3, MaxUnitHealth 7) to
// declare a unit unusable this turn, which removes it from consideration entirely
// before any scoring happens.
// Search AI should not gate units out of the candidate pool. "This 3 HP tank should
// sit on the base and repair" is a conclusion the evaluator must reach by comparing
// the board after repairing against the board after attacking -- sometimes a 2 HP
// unit is the only thing that can block a chokepoint or finish a kill, and a hard
// gate makes that move unreachable.
bool NormalAi::isUsingUnit(Unit *pUnit)
{
    if (needsRefuel(pUnit))
    {
        return false;
    }

    if (m_pMap->onMap(pUnit->Unit::getX(), pUnit->Unit::getY()))
    {
        Building *pBuilding = m_pMap->getTerrain(pUnit->Unit::getX(), pUnit->Unit::getY())->getBuilding();
        if (pBuilding == nullptr && pUnit->getHpRounded() <= m_minUnitHealth)
        {
            // Unit isn't on a building and its health is too low. Don't use it.
            return false;
        }
        else if (pBuilding != nullptr && pBuilding->getOwner() == m_pPlayer &&
                 pUnit->getHpRounded() <= m_maxUnitHealth)
        {
            // Unit is on a building receiving repairs, but its current health is less 
            // than the health that it needs to be considered effective.
            return false;
        }
    }
    if (pUnit->getHasMoved())
    {
        // Can't use a unit that has already moved.
        return false;
    }
    return true;
}

bool NormalAi::captureBuildings(spQmlVectorUnit &pUnits, spQmlVectorBuilding &pBuildings, spQmlVectorBuilding &pEnemyBuildings)
{
    AI_CONSOLE_PRINT("NormalAi::captureBuildings()", GameConsole::eDEBUG);
    MoveUnitData* targetUnitData = nullptr;
    spGameAction pAction = m_captureBuildingSelector.getNextCaptureBuilding(m_OwnUnits, &targetUnitData);
    if (pAction.get() != nullptr)
    {
        ++targetUnitData->nextAiStep;
        m_updatePoints.push_back(pAction->getTargetUnit()->getPosition());
        m_updatePoints.push_back(pAction->getActionTarget());
        if (pAction->getActionID() == ACTION_WAIT)
        {
            std::vector<QVector3D> targets;
            auto actionTarget = pAction->getActionTarget();
            targets.push_back(QVector3D(actionTarget.x(), actionTarget.y(), 1));
            std::vector<QVector3D> transporterTargets;
            if (moveUnit(pAction, targetUnitData, pUnits, targetUnitData->actions, targets, transporterTargets, true, pBuildings, pEnemyBuildings))
            {
                m_captureBuildingSelector.addUsedFarAwayBuildings(actionTarget);
                return true;
            }
        }
        else
        {
            emit sigPerformAction(pAction);
            return true;
        }
    }
    ++m_aiFunctionStep;
    return false;
}

bool NormalAi::joinCaptureBuildings(spQmlVectorUnit &pUnits)
{
    AI_CONSOLE_PRINT("NormalAi::joinCaptureBuildings()", GameConsole::eDEBUG);
    Interpreter *pInterpreter = Interpreter::getInstance();
    for (auto &unitData : m_OwnUnits)
    {
        if (unitData.nextAiStep <= m_aiFunctionStep)
        {
            pInterpreter->threadProcessEvents();
            Unit *pUnit = unitData.pUnit.get();
            ++unitData.nextAiStep;
            if (!pUnit->getHasMoved() &&
                unitData.actions.contains(ACTION_CAPTURE) &&
                unitData.actions.contains(ACTION_JOIN) &&
                pUnit->getAiMode() == GameEnums::GameAi_Normal)
            {
                spGameAction pAction = MemoryManagement::create<GameAction>(ACTION_JOIN, m_pMap);
                pAction->setTarget(QPoint(pUnit->Unit::getX(), pUnit->Unit::getY()));
                auto targets = unitData.pUnitPfs->getAllNodePointsFast(unitData.movementPoints + 1);
                for (auto &target : targets)
                {
                    pAction->setMovepath(QVector<QPoint>(1, target), 0);
                    if (pAction->canBePerformed())
                    {
                        Unit *pCaptureUnit = m_pMap->getTerrain(target.x(), target.y())->getUnit();
                        if (pCaptureUnit->getCapturePoints() > 0)
                        {
                            auto path = unitData.pUnitPfs->getPathFast(static_cast<qint32>(target.x()), static_cast<qint32>(target.y()));
                            pAction->setMovepath(path, unitData.pUnitPfs->getCosts(path));
                            m_updatePoints.push_back(pUnit->getPosition());
                            m_updatePoints.push_back(pAction->getActionTarget());
                            emit sigPerformAction(pAction);
                            return true;
                        }
                    }
                }
            }
        }
    }
    ++m_aiFunctionStep;
    return false;
}

bool NormalAi::fireWithUnits(spQmlVectorUnit &pUnits, qint32 minfireRange, qint32 minMaxfireRange, qint32 maxfireRange,
                             spQmlVectorBuilding &pBuildings, spQmlVectorBuilding &pEnemyBuildings)
{
    AI_CONSOLE_PRINT("NormalAi::fireWithUnits()", GameConsole::eDEBUG);
    Interpreter *pInterpreter = Interpreter::getInstance();
    for (auto &unitData : m_OwnUnits)
    {
        if (unitData.nextAiStep <= m_aiFunctionStep)
        {
            pInterpreter->threadProcessEvents();
            Unit *pUnit = unitData.pUnit.get();
            ++unitData.nextAiStep;
            if (!pUnit->getHasMoved() &&
                (!needsRefuel(pUnit) || pUnit->getFuel() == 0) &&
                unitData.minFireRange >= minfireRange &&
                unitData.maxFireRange >= minMaxfireRange &&
                unitData.maxFireRange <= maxfireRange &&
                (pUnit->getAmmo1() != 0 || pUnit->getAmmo2() != 0) &&
                unitData.actions.contains(CoreAI::ACTION_FIRE) &&
                pUnit->getAiMode() == GameEnums::GameAi_Normal)
            {
                spGameAction pAction = MemoryManagement::create<GameAction>(ACTION_FIRE, m_pMap);
                pAction->setTarget(QPoint(pUnit->Unit::getX(), pUnit->Unit::getY()));
                std::vector<CoreAI::DamageData> ret;
                std::vector<QVector3D> moveTargetFields;
                CoreAI::getAttackTargets(pUnit, pAction, unitData.pUnitPfs.get(), ret, moveTargetFields, unitData.movementPoints + 1);
                qint32 targetIdx = getBestAttackTarget(unitData, ret, moveTargetFields, pBuildings, pEnemyBuildings);
                if (targetIdx >= 0)
                {
                    CoreAI::DamageData target = ret[targetIdx];
                    auto path = unitData.pUnitPfs->getPathFast(static_cast<qint32>(moveTargetFields[targetIdx].x()),
                                                               static_cast<qint32>(moveTargetFields[targetIdx].y()));
                    pAction->setMovepath(path, unitData.pUnitPfs->getCosts(path));
                    CoreAI::addSelectedFieldData(pAction, QPoint(static_cast<qint32>(target.x), static_cast<qint32>(target.y)));
                    if (m_pMap->getTerrain(static_cast<qint32>(target.x), static_cast<qint32>(target.y))->getUnit() == nullptr)
                    {
                        m_IslandMaps.clear();
                    }
                    if (pAction->isFinalStep() && pAction->canBePerformed())
                    {
                        m_updatePoints.push_back(pUnit->getPosition());
                        m_updatePoints.push_back(pAction->getActionTarget());
                        m_updatePoints.push_back(QPoint(static_cast<qint32>(target.x), static_cast<qint32>(target.y)));
                        emit sigPerformAction(pAction);
                        return true;
                    }
                }
            }
        }
    }
    ++m_aiFunctionStep;
    return false;
}

bool NormalAi::refillUnits(spQmlVectorUnit &pUnits, spQmlVectorBuilding &pBuildings, spQmlVectorBuilding &pEnemyBuildings)
{
    AI_CONSOLE_PRINT("NormalAi::refillUnits()", GameConsole::eDEBUG);
    if (m_aiStep < AISteps::moveToTargets)
    {
        createMovementMap(pBuildings, pEnemyBuildings);
    }
    m_aiStep = AISteps::moveToTargets;
    Interpreter *pInterpreter = Interpreter::getInstance();
    for (auto &unitData : m_OwnUnits)
    {
        if (unitData.nextAiStep <= m_aiFunctionStep)
        {
            pInterpreter->threadProcessEvents();
            Unit *pUnit = unitData.pUnit.get();
            // can we use the unit?
            if (isUsingUnit(pUnit) &&
                pUnit->getLoadedUnitCount() == 0 &&
                pUnit->getAiMode() == GameEnums::GameAi_Normal)
            {
                QStringList &actions = unitData.actions;
                if (isRefuelUnit(actions))
                {
                    spGameAction pAction = MemoryManagement::create<GameAction>(ACTION_WAIT, m_pMap);
                    pAction->setTarget(QPoint(pUnit->Unit::getX(), pUnit->Unit::getY()));
                    bool found = false;
                    QPoint moveTarget;
                    if (actions.contains(ACTION_SUPPORTALL_RATION))
                    {
                        pAction->setActionID(ACTION_SUPPORTALL_RATION);
                        QPoint refillTarget;
                        found = getBestRefillTarget(*unitData.pUnitPfs.get(), 4, moveTarget, refillTarget, unitData.movementPoints);
                    }
                    else if (actions.contains(ACTION_SUPPORTALL_RATION))
                    {
                        pAction->setActionID(ACTION_SUPPORTALL_RATION_MONEY);
                        QPoint refillTarget;
                        found = getBestRefillTarget(*unitData.pUnitPfs.get(), 4, moveTarget, refillTarget, unitData.movementPoints);
                    }
                    else if (actions.contains(ACTION_SUPPORTSINGLE_REPAIR))
                    {
                        pAction->setActionID(ACTION_SUPPORTSINGLE_REPAIR);
                        QPoint refillTarget;
                        found = getBestRefillTarget(*unitData.pUnitPfs.get(), 1, moveTarget, refillTarget, unitData.movementPoints);
                        CoreAI::addSelectedFieldData(pAction, refillTarget);
                    }
                    else if (actions.contains(ACTION_SUPPORTSINGLE_FREEREPAIR))
                    {
                        pAction->setActionID(ACTION_SUPPORTSINGLE_FREEREPAIR);
                        QPoint refillTarget;
                        found = getBestRefillTarget(*unitData.pUnitPfs.get(), 1, moveTarget, refillTarget, unitData.movementPoints);
                        CoreAI::addSelectedFieldData(pAction, refillTarget);
                    }
                    else if (actions.contains(ACTION_SUPPORTSINGLE_SUPPLY))
                    {
                        pAction->setActionID(ACTION_SUPPORTSINGLE_SUPPLY);
                        QPoint refillTarget;
                        found = getBestRefillTarget(*unitData.pUnitPfs.get(), 1, moveTarget, refillTarget, unitData.movementPoints);
                        CoreAI::addSelectedFieldData(pAction, refillTarget);
                    }
                    if (found)
                    {
                        auto path = unitData.pUnitPfs->getPathFast(moveTarget.x(), moveTarget.y());
                        pAction->setMovepath(path, unitData.pUnitPfs->getCosts(path));
                        if (pAction->canBePerformed())
                        {
                            m_updatePoints.push_back(pUnit->getPosition());
                            m_updatePoints.push_back(pAction->getActionTarget());
                            emit sigPerformAction(pAction);
                            return true;
                        }
                    }
                    else if (m_usedTransportSystem)
                    {
                        AI_CONSOLE_PRINT("move to supply needed units", GameConsole::eDEBUG);
                        std::vector<QVector3D> targets;
                        std::vector<QVector3D> transporterTargets;
                        pAction->setActionID(ACTION_WAIT);
                        appendRefillTargets(actions, pUnit, pUnits, targets);
                        if (moveUnit(pAction, &unitData, pUnits, actions, targets, transporterTargets, true, pBuildings, pEnemyBuildings))
                        {
                            return true;
                        }
                    }
                }
            }
        }
    }
    ++m_aiFunctionStep;
    return false;
}

// [AI: INVESTIGATE]
// Picks the reachable tile adjacent to the most units that need refuelling, capped by
// maxRefillCount (called with hard-coded 4 and 1 from refillUnits). The count is a
// proxy for value: it treats "supplies 3 units" as strictly better than "supplies 2"
// regardless of which units they are or whether they are anywhere useful.
// The reachable-tile enumeration is reusable; the "most adjacent needy units wins"
// tie-break is the part Search AI should replace with a board score.
bool NormalAi::getBestRefillTarget(UnitPathFindingSystem &pfs, qint32 maxRefillCount, QPoint &moveTarget, QPoint &refillTarget, qint32 movepoints) const
{
    bool ret = false;
    const auto points = pfs.getAllNodePointsFast(movepoints + 1);

    spQmlVectorPoint circle = GlobalUtils::getSpCircle(1, 1);
    qint32 highestCount = 0;
    for (const auto &point : points)
    {
        if (m_pMap->getTerrain(point.x(), point.y())->getUnit() == nullptr)
        {
            qint32 count = 0;
            for (const auto circlePos : circle->getVector())
            {
                qint32 x = point.x() + circlePos.x();
                qint32 y = point.y() + circlePos.y();
                if (m_pMap->onMap(x, y))
                {
                    Unit *pSupplyUnit = m_pMap->getTerrain(x, y)->getUnit();
                    if (pSupplyUnit != nullptr &&
                        pSupplyUnit->getOwner() == m_pPlayer &&
                        needsRefuel(pSupplyUnit))
                    {
                        ++count;
                        ret = true;
                        if (count == maxRefillCount)
                        {
                            moveTarget = point;
                            refillTarget = QPoint(x, y);
                            break;
                        }
                    }
                }
            }
            if (count == maxRefillCount)
            {
                break;
            }
            else if (count > highestCount)
            {
                moveTarget = point;
                highestCount = count;
            }
        }
    }
    return ret;
}

void NormalAi::appendRefillTargets(const QStringList &actions, Unit *pUnit, spQmlVectorUnit &pUnits, std::vector<QVector3D> &targets)
{
    if (isRefuelUnit(actions))
    {
        spQmlVectorPoint circle = GlobalUtils::getSpCircle(1, 1);

        qint32 islandIdx = getIslandIndex(pUnit);
        qint32 curX = pUnit->Unit::getX();
        qint32 curY = pUnit->Unit::getY();
        for (auto &pSupplyUnit : pUnits->getVector())
        {
            if (needsRefuel(pSupplyUnit.get()))
            {
                qint32 unitX = pSupplyUnit->Unit::getX();
                qint32 unitY = pSupplyUnit->Unit::getY();
                for (auto &point : circle->getVector())
                {
                    qint32 x = unitX + point.x();
                    qint32 y = unitY + point.y();
                    if (m_pMap->onMap(x, y))
                    {
                        if (onSameIsland(islandIdx, curX, curY, x, y))
                        {
                            if (!GlobalUtils::contains(targets, QVector3D(x, y, 1)))
                            {
                                targets.push_back(QVector3D(x, y, 1));
                            }
                        }
                    }
                }
            }
        }
    }
}

bool NormalAi::moveUnits(spQmlVectorUnit &pUnits, spQmlVectorBuilding &pBuildings,
                         spQmlVectorUnit &pEnemyUnits, spQmlVectorBuilding &pEnemyBuildings,
                         qint32 minfireRange, qint32 maxfireRange, bool supportUnits)
{
    AI_CONSOLE_PRINT("NormalAi::moveUnits()", GameConsole::eDEBUG);
    Interpreter *pInterpreter = Interpreter::getInstance();
    for (auto &unitData : m_OwnUnits)
    {
        if (unitData.nextAiStep <= m_aiFunctionStep)
        {
            pInterpreter->threadProcessEvents();
            Unit *pUnit = unitData.pUnit.get();
            ++unitData.nextAiStep;
            // [AI: REPLACE - AI POLICY]
            // Guessed constant standing in for "how far could a transporter carry this
            // unit", used below by hasTargets() to decide whether the unit has anything
            // worth doing at all. It is blind to which transports actually exist, where
            // they are, and whether the map even has water.
            // Search AI should derive reachability from real transports via the
            // transport-aware eta() described in AI_Design_Notes_Consolidated.md
            // section 6.4, not from an average.
            constexpr qint32 AVERAGE_TRANSPORTER_MOVEMENT = 7;
            bool canCapture = unitData.actions.contains(ACTION_CAPTURE);
            qint32 loadingIslandIdx = getIslandIndex(pUnit);
            qint32 loadingIsland = getIsland(pUnit);

            // can we use the unit?
            if (!pUnit->getHasMoved() &&
                pUnit->getBaseMaxRange() >= minfireRange &&
                pUnit->getBaseMaxRange() <= maxfireRange &&
                pUnit->hasWeapons() && pUnit->getLoadedUnitCount() == 0 &&
                (m_usedTransportSystem || (isUsingUnit(pUnit) && hasTargets(AVERAGE_TRANSPORTER_MOVEMENT, pUnit, canCapture, pEnemyUnits.get(), pEnemyBuildings.get(),
                                                                            loadingIslandIdx, loadingIsland, false))))
            {
                std::vector<QVector3D> targets;
                std::vector<QVector3D> transporterTargets;
                spGameAction pAction = MemoryManagement::create<GameAction>(ACTION_WAIT, m_pMap);
                QStringList &actions = unitData.actions;
                // [AI: REPLACE - AI POLICY]
                // distanceModifier is Normal AI's target-priority weight. It is set to 1,
                // then bumped to 4 and 5 purely because capture targets were found, and
                // is passed into every append*Targets call below where it inflates the
                // effective distance of attack/support targets so the pathfinder prefers
                // the capture. It encodes "capturing outranks attacking" as a distance
                // fudge factor rather than as a value comparison.
                // Search AI should put capture and attack candidates in the same pool and
                // let the evaluator compare the resulting boards -- capture value comes
                // from projected income (section 6.4), not from a hand-picked multiplier.
                qint32 distanceModifier = 1;
                // find possible targets for this unit
                pAction->setTarget(QPoint(pUnit->Unit::getX(), pUnit->Unit::getY()));
                if (pUnit->getAiMode() == GameEnums::GameAi_Normal)
                {
                    // find some cool targets
                    appendCaptureTargets(actions, pUnit, pEnemyBuildings, targets);
                }
                if (targets.size() > 0)
                {
                    distanceModifier = 4;
                    appendCaptureTransporterTargets(pUnit, pUnits, pEnemyBuildings, transporterTargets, distanceModifier);
                    distanceModifier = 5;
                    targets.insert(targets.cbegin(), transporterTargets.cbegin(), transporterTargets.cend());
                }
                if (pUnit->getAiMode() == GameEnums::GameAi_Normal)
                {
                    appendAttackTargets(pUnit, pEnemyUnits, targets, distanceModifier);
                    appendAttackTargetsIgnoreOwnUnits(pUnit, pEnemyUnits, targets, distanceModifier);
                    appendTerrainBuildingAttackTargets(pUnit, pEnemyBuildings, targets, distanceModifier);
                    // [AI: REPLACE - AI POLICY]
                    // Repair is a last-resort fallback: only considered when no attack or
                    // capture target exists anywhere. A damaged unit with any target in
                    // sight can therefore never choose to go repair first.
                    // Search AI should treat repairing as an ordinary candidate action
                    // competing on board value, not as an empty-list fallback.
                    if (targets.size() == 0)
                    {
                        appendRepairTargets(pUnit, pBuildings, targets);
                    }
                    if (supportUnits)
                    {
                        appendSupportTargets(actions, pUnit, pUnits, pEnemyUnits, targets, distanceModifier);
                    }
                }
                if (targets.size() > 0 || transporterTargets.size() > 0)
                {
                    if (moveUnit(pAction, &unitData, pUnits, actions, targets, transporterTargets, true, pBuildings, pEnemyBuildings))
                    {
                        return true;
                    }
                }
            }
        }
    }
    if (!m_secondMoveRound)
    {
        createUnitInfluenceMap();
        m_secondMoveRound = true;
        m_aiStep = AISteps::moveUnits;
    }
    else
    {
        m_aiStep = AISteps::moveIndirectsToTargets;
    }
    ++m_aiFunctionStep;
    return false;
}

bool NormalAi::loadUnits(spQmlVectorUnit &pUnits, spQmlVectorBuilding &pBuildings, spQmlVectorBuilding &pEnemyBuildings)
{
    AI_CONSOLE_PRINT("NormalAi::loadUnits()", GameConsole::eDEBUG);
    m_aiStep = AISteps::loadUnits;
    Interpreter *pInterpreter = Interpreter::getInstance();
    for (auto &unitData : m_OwnUnits)
    {
        if (unitData.nextAiStep <= m_aiFunctionStep)
        {
            pInterpreter->threadProcessEvents();
            Unit *pUnit = unitData.pUnit.get();
            ++unitData.nextAiStep;
            // can we use the unit?
            if (!pUnit->getHasMoved() &&
                (pUnit->getLoadingPlace() == 0 || (pUnit->getLoadedUnitCount() > 0 && m_usedTransportSystem)))
            {
                std::vector<QVector3D> targets;
                std::vector<QVector3D> transporterTargets;
                spGameAction pAction = MemoryManagement::create<GameAction>(ACTION_LOAD, m_pMap);
                QStringList &actions = unitData.actions;
                // find possible targets for this unit
                pAction->setTarget(QPoint(pUnit->Unit::getX(), pUnit->Unit::getY()));

                // find some cool targets
                appendTransporterTargets(pUnit, pUnits, transporterTargets);
                targets.insert(targets.cbegin(), transporterTargets.cbegin(), transporterTargets.cend());
                // till now the selected targets are a little bit lame cause we only search for reachable transporters
                // but not for reachable loading places.
                if (moveUnit(pAction, &unitData, pUnits, actions, targets, transporterTargets, false, pBuildings, pEnemyBuildings))
                {
                    return true;
                }
            }
        }
    }
    ++m_aiFunctionStep;
    return false;
}

bool NormalAi::moveTransporters(spQmlVectorUnit &pUnits, spQmlVectorUnit &pEnemyUnits, spQmlVectorBuilding &pBuildings, spQmlVectorBuilding &pEnemyBuildings)
{
    AI_CONSOLE_PRINT("NormalAi::moveTransporters()", GameConsole::eDEBUG);
    m_aiStep = AISteps::moveTransporters;
    Interpreter *pInterpreter = Interpreter::getInstance();
    for (auto &unitData : m_OwnUnits)
    {
        if (unitData.nextAiStep <= m_aiFunctionStep)
        {
            pInterpreter->threadProcessEvents();
            Unit *pUnit = unitData.pUnit.get();
            ++unitData.nextAiStep;
            // can we use the unit?
            if (!pUnit->getHasMoved() &&
                pUnit->getLoadingPlace() > 0 &&
                pUnit->getAiMode() == GameEnums::GameAi_Normal)
            {
                // wooohooo it's a transporter
                if (pUnit->getLoadedUnitCount() > 0)
                {
                    spGameAction pAction = MemoryManagement::create<GameAction>(ACTION_WAIT, m_pMap);
                    QStringList &actions = unitData.actions;
                    pAction->setTarget(QPoint(pUnit->Unit::getX(), pUnit->Unit::getY()));
                    // find possible targets for this unit
                    std::vector<QVector3D> targets;
                    // can one of our units can capture buildings?
                    bool captureFound = false;
                    bool attackFound = false;
                    for (auto &pLoaded : pUnit->getLoadedUnits())
                    {
                        QStringList actions = pLoaded->getActionList();
                        if (!captureFound && actions.contains(ACTION_CAPTURE))
                        {
                            appendUnloadTargetsForCapturing(pUnit, pUnits, pEnemyBuildings, targets);
                            captureFound = true;
                        }
                    }
                    if (!captureFound)
                    {
                        for (auto &pLoaded : pUnit->getLoadedUnits())
                        {
                            QStringList actions = pLoaded->getActionList();
                            if (!attackFound && actions.contains(ACTION_FIRE))
                            {
                                appendUnloadTargetsForAttacking(pUnit, pEnemyUnits, targets, 1);
                                attackFound = true;
                            }
                            if (attackFound)
                            {
                                break;
                            }
                        }
                    }
                    // if not find closest unloading field
                    if (targets.size() == 0)
                    {
                        appendUnloadTargetsForAttacking(pUnit, pEnemyUnits, targets, 3);
                    }
                    if (targets.size() == 0)
                    {
                        appendNearestUnloadTargets(pUnit, pEnemyUnits, pEnemyBuildings, targets);
                    }
                    if (moveToUnloadArea(pAction, &unitData, pUnits, actions, targets, pBuildings, pEnemyBuildings, pEnemyUnits))
                    {
                        return true;
                    }
                }
                else
                {
                    spGameAction pAction = MemoryManagement::create<GameAction>(ACTION_WAIT, m_pMap);
                    QStringList &actions = unitData.actions;
                    // find possible targets for this unit
                    pAction->setTarget(QPoint(pUnit->Unit::getX(), pUnit->Unit::getY()));
                    // we need to move to a loading place
                    std::vector<QVector3D> targets;
                    std::vector<QVector3D> transporterTargets;
                    appendCaptureTargets(actions, pUnit, pEnemyBuildings, targets);
                    appendLoadingTargets(pUnit, pUnits, pEnemyUnits, pEnemyBuildings, false, false, targets, false, 5);
                    if (targets.size() == 0)
                    {
                        appendLoadingTargets(pUnit, pUnits, pEnemyUnits, pEnemyBuildings, true, false, targets, false, 5);
                    }
                    if (moveUnit(pAction, &unitData, pUnits, actions, targets, transporterTargets, false, pBuildings, pEnemyBuildings))
                    {
                        return true;
                    }
                }
            }
        }
    }
    ++m_aiFunctionStep;
    return false;
}

bool NormalAi::moveToUnloadArea(spGameAction &pAction, MoveUnitData *pUnitData, spQmlVectorUnit &pUnits, QStringList &actions,
                                std::vector<QVector3D> &targets,
                                spQmlVectorBuilding &pBuildings, spQmlVectorBuilding &pEnemyBuildings,
                                spQmlVectorUnit &pEnemyUnits)
{
    AI_CONSOLE_PRINT("NormalAi::moveToUnloadArea()", GameConsole::eDEBUG);
    Unit *pUnit = pUnitData->pUnit.get();
    TargetedUnitPathFindingSystem pfs(m_pMap, pUnit, targets, &m_MoveCostMap);
    pfs.explore();
    qint32 movepoints = pUnit->getMovementpoints(QPoint(pUnit->Unit::getX(), pUnit->Unit::getY()));
    QPoint targetFields = pfs.getReachableTargetField(movepoints);
    if (targetFields.x() >= 0)
    {
        auto path = pUnitData->pUnitPfs->getPathFast(targetFields.x(), targetFields.y());
        pAction->setMovepath(path, pUnitData->pUnitPfs->getCosts(path));
        pAction->setActionID(ACTION_UNLOAD);
        if (pAction->canBePerformed() && targetFields == pfs.getTarget())
        {
            return unloadUnits(pAction, pUnit, pEnemyUnits);
        }
        else
        {
            return moveUnit(pAction, pUnitData, pUnits, actions, targets, targets, true, pBuildings, pEnemyBuildings);
        }
    }
    return false;
}

bool NormalAi::unloadUnits(spGameAction &pAction, Unit *pUnit, spQmlVectorUnit &pEnemyUnits)
{
    m_transporterSelector.prepareUnloadInformation(pAction, pUnit, pEnemyUnits);
    m_updatePoints.push_back(pUnit->getPosition());
    m_updatePoints.push_back(pAction->getActionTarget());
    if (pAction->canBePerformed())
    {
        emit sigPerformAction(pAction);
        return true;
    }
    return false;
}

bool NormalAi::repairUnits(spQmlVectorUnit &pUnits, spQmlVectorBuilding &pBuildings, spQmlVectorBuilding &pEnemyBuildings)
{
    AI_CONSOLE_PRINT("NormalAi::repairUnits()", GameConsole::eDEBUG);
    m_aiStep = AISteps::moveUnits;
    Interpreter *pInterpreter = Interpreter::getInstance();
    for (auto &unitData : m_OwnUnits)
    {
        if (unitData.nextAiStep <= m_aiFunctionStep)
        {
            pInterpreter->threadProcessEvents();
            Unit *pUnit = unitData.pUnit.get();
            ++unitData.nextAiStep;
            // can we use the unit?
            if (!isUsingUnit(pUnit) &&
                !pUnit->getHasMoved() &&
                pUnit->getAiMode() == GameEnums::GameAi_Normal)
            {
                std::vector<QVector3D> targets;
                std::vector<QVector3D> transporterTargets;
                spGameAction pAction = MemoryManagement::create<GameAction>(ACTION_WAIT, m_pMap);
                QStringList &actions = unitData.actions;
                // find possible targets for this unit
                pAction->setTarget(QPoint(pUnit->Unit::getX(), pUnit->Unit::getY()));
                appendRepairTargets(pUnit, pBuildings, targets);
                if (needsRefuel(pUnit))
                {
                    appendTransporterTargets(pUnit, pUnits, transporterTargets);
                    targets.insert(targets.cend(), transporterTargets.cbegin(), transporterTargets.cend());
                }
                if (moveUnit(pAction, &unitData, pUnits, actions, targets, transporterTargets, false, pBuildings, pEnemyBuildings))
                {
                    return true;
                }
                else
                {
                    pAction = MemoryManagement::create<GameAction>(ACTION_WAIT, m_pMap);
                    pAction->setTarget(QPoint(pUnit->Unit::getX(), pUnit->Unit::getY()));
                    if (suicide(pAction, pUnit, *unitData.pUnitPfs.get(), unitData.movementPoints))
                    {
                        return true;
                    }
                }
            }
        }
    }
    ++m_aiFunctionStep;
    return false;
}

bool NormalAi::moveUnit(spGameAction &pAction, MoveUnitData *pUnitData, spQmlVectorUnit &pUnits, QStringList &actions,
                        std::vector<QVector3D> &targets, std::vector<QVector3D> &transporterTargets,
                        bool shortenPathForTarget,
                        spQmlVectorBuilding &pBuildings, spQmlVectorBuilding &pEnemyBuildings)
{
    if (targets.size() == 0)
    {
        return false;
    }
    AI_CONSOLE_PRINT("NormalAi::moveUnit()", GameConsole::eDEBUG);
    Unit *pUnit = pUnitData->pUnit.get();
    TargetedUnitPathFindingSystem pfs(m_pMap, pUnit, targets, &m_MoveCostMap);
    pfs.explore();
    qint32 movepoints = pUnitData->movementPoints;
    QPoint targetFields = pfs.getReachableTargetField(movepoints);
    if (targetFields.x() >= 0)
    {
        Unit *pTargetUnit = m_pMap->getTerrain(targetFields.x(), targetFields.y())->getUnit();
        UnitPathFindingSystem &turnPfs = *(pUnitData->pUnitPfs.get());
        if (CoreAI::contains(transporterTargets, targetFields) &&
            pTargetUnit != nullptr &&
            (pTargetUnit->getHasMoved() == false || m_usedTransportSystem) &&
            pUnit->getAiMode() == GameEnums::GameAi_Normal)
        {
            auto path = turnPfs.getPathFast(targetFields.x(), targetFields.y());
            pAction->setMovepath(path, turnPfs.getCosts(path));
            pAction->setActionID(ACTION_LOAD);
            if (pAction->canBePerformed())
            {
                m_updatePoints.push_back(pUnit->getPosition());
                m_updatePoints.push_back(pAction->getActionTarget());
                emit sigPerformAction(pAction);
                return true;
            }
        }
        else if (!shortenPathForTarget && CoreAI::contains(targets, targetFields))
        {
            auto movePath = turnPfs.getClosestReachableMovePath(targetFields, pUnitData->movementPoints);
            pAction->setMovepath(movePath, turnPfs.getCosts(movePath));
            if (actions.contains(ACTION_CAPTURE))
            {
                pAction->setActionID(ACTION_CAPTURE);
                if (pAction->canBePerformed())
                {
                    m_updatePoints.push_back(pUnit->getPosition());
                    m_updatePoints.push_back(pAction->getActionTarget());
                    emit sigPerformAction(pAction);
                    return true;
                }
            }
            pAction->setActionID(ACTION_WAIT);
            if (pAction->canBePerformed())
            {
                m_updatePoints.push_back(pUnit->getPosition());
                m_updatePoints.push_back(pAction->getActionTarget());
                emit sigPerformAction(pAction);
                return true;
            }
        }
        else
        {
            auto movePath = turnPfs.getClosestReachableMovePath(targetFields, pUnitData->movementPoints);
            if (movePath.size() == 0)
            {
                movePath.push_back(QPoint(pUnit->Unit::getX(), pUnit->Unit::getY()));
            }
            qint32 idx = getMoveTargetField(*pUnitData, turnPfs, movePath, pBuildings, pEnemyBuildings, pUnitData->movementPoints);
            if (idx < 0 || idx == movePath.size() - 1)
            {
                std::tuple<QPoint, float, bool> target = moveToSafety(*pUnitData, turnPfs, movePath[0], pBuildings, pEnemyBuildings, pUnitData->movementPoints);
                QPoint ret = std::get<0>(target);
                float minDamage = std::get<1>(target);
                bool allEqual = std::get<2>(target);
                // [AI: REPLACE - AI POLICY]
                // "If the safest reachable tile still costs me more than half my own
                // value, or every tile is equally bad, throw the unit away in a suicide
                // attack." The 50%-of-unit-value cut-off is an arbitrary despair
                // threshold, and the branch is reached on an ordering accident: it fires
                // whenever getMoveTargetField() found no acceptable tile.
                // Search AI should express this as ordinary comparison -- a trade is
                // worth making when the resulting board scores higher than retreating,
                // including the denial value of a unit that dies but costs the enemy more
                // (AI_Design_Notes_Consolidated.md section 4, Strategic Pressure).
                // There should be no separate "suicide" concept at all.
                if (((ret.x() == pUnit->Unit::getX() && ret.y() == pUnit->Unit::getY()) ||
                     minDamage > pUnit->getCoUnitValue() / 2 ||
                     allEqual) &&
                    minDamage > 0.0f)
                {
                    if (suicide(pAction, pUnit, turnPfs, pUnitData->movementPoints))
                    {
                        return true;
                    }
                    else
                    {
                        auto movePath = turnPfs.getPathFast(ret.x(), ret.y());
                        pAction->setMovepath(movePath, turnPfs.getCosts(movePath));
                    }
                }
                else
                {
                    auto movePath = turnPfs.getPathFast(ret.x(), ret.y());
                    pAction->setMovepath(movePath, turnPfs.getCosts(movePath));
                }
            }
            else
            {
                auto path = turnPfs.getPathFast(movePath[idx].x(), movePath[idx].y());
                pAction->setMovepath(path, turnPfs.getCosts(path));
            }
            // [AI: REPLACE - AI POLICY]
            // Two HP thresholds (LockedUnitHp 4, NoMoveAttackHp 3.5) decide whether a
            // stationary unit is even allowed to look for an attack. A 3 HP unit that
            // could move is silently forbidden from firing; a 3 HP unit that is boxed in
            // is allowed to. Search AI should let the evaluator decide whether attacking
            // at low HP is worth the retaliation, and never suppress the candidate.
            bool lockedUnit = (pAction->getMovePath().size() == 1) &&
                              (pUnit->getHp() < m_lockedUnitHp);
            // when we don't move try to attack if possible
            if ((pUnit->getHp() > m_noMoveAttackHp) ||
                lockedUnit)
            {
                pAction->setActionID(ACTION_FIRE);
                std::vector<QVector3D> ret;
                std::vector<QVector3D> moveTargetFields;
                getBestAttacksFromField(pUnit, pAction, ret, moveTargetFields);
                // [AI: REPLACE - AI POLICY]
                // Accept-threshold plus random pick, and the worst offender of the two is
                // the random pick. getBestAttacksFromField() returns targets already
                // sorted by score, then the AI throws that ordering away and takes a
                // uniformly random element of the whole list -- not a tie-break among
                // equals, an outright dice roll between a good attack and a mediocre one.
                // MinSuicideDamage (0.75) then gates the list on "am I willing to lose 75%
                // of my own value for this".
                // Search AI must select deterministically by evaluated board score.
                // Same pattern repeats twice more in this function and once in suicide().
                if (ret.size() > 0 &&
                    (ret[0].z() >= -pUnit->getCoUnitValue() * m_minSuicideDamage ||
                     lockedUnit))
                {
                    qint32 selection = GlobalUtils::randIntBase(0, ret.size() - 1);
                    QVector3D target = ret[selection];
                    CoreAI::addSelectedFieldData(pAction, QPoint(static_cast<qint32>(target.x()),
                                                                 static_cast<qint32>(target.y())));
                    if (pAction->isFinalStep() && pAction->canBePerformed())
                    {
                        m_updatePoints.push_back(pUnit->getPosition());
                        m_updatePoints.push_back(pAction->getActionTarget());
                        m_updatePoints.push_back(QPoint(static_cast<qint32>(target.x()),
                                                        static_cast<qint32>(target.y())));
                        emit sigPerformAction(pAction);
                        return true;
                    }
                }
            }
            if (pAction->getMovePath().size() > 0)
            {
                m_updatePoints.push_back(pUnit->getPosition());
                m_updatePoints.push_back(pAction->getActionTarget());
                // [AI: REPLACE - AI POLICY]
                // Having chosen a destination tile, the AI now picks what to do there by
                // walking a fixed preference order and taking the first thing that is
                // legal: SUPPORTALL -> BUILD -> STEALTH -> UNSTEALTH -> PLACE -> FIRE ->
                // CAPTURE -> WAIT. Nothing is scored. A unit standing on an enemy city
                // with a good attack available captures or fires depending only on where
                // those actions sit in this list.
                // This is the per-tile twin of the performActionSteps() ladder and needs
                // the same treatment: every legal action at the tile is a candidate, and
                // the evaluator ranks them. Note also that destination and action are
                // chosen in two separate stages here, so a tile is never selected
                // *because* of the action it enables -- Search AI should score the
                // (tile, action) pair as one candidate.
                for (const auto &action : actions)
                {
                    if (action.startsWith(ACTION_SUPPORTALL))
                    {
                        pAction->setActionID(action);
                        if (pAction->canBePerformed() &&
                            pAction->isFinalStep())
                        {
                            emit sigPerformAction(pAction);
                            return true;
                        }
                    }
                    else if (action.startsWith(ACTION_BUILD))
                    {
                        pAction->setActionID(action);
                        if (pAction->canBePerformed() &&
                            pAction->isFinalStep())
                        {
                            emit sigPerformAction(pAction);
                            return true;
                        }
                    }
                }
                if (actions.contains(ACTION_STEALTH))
                {
                    pAction->setActionID(ACTION_STEALTH);
                    if (pAction->canBePerformed())
                    {
                        emit sigPerformAction(pAction);
                        return true;
                    }
                }
                if (actions.contains(ACTION_UNSTEALTH))
                {
                    pAction->setActionID(ACTION_UNSTEALTH);
                    if (pAction->canBePerformed())
                    {
                        float counterDamage = calculateCounterDamage(*pUnitData, pAction->getActionTarget(), nullptr, 0, pBuildings, pEnemyBuildings, true);
                        if (counterDamage <= 0)
                        {
                            emit sigPerformAction(pAction);
                            return true;
                        }
                    }
                }
                for (const auto &action : actions)
                {
                    if (action.startsWith(ACTION_PLACE))
                    {
                        pAction->setActionID(action);
                        if (pAction->canBePerformed())
                        {
                            spMarkedFieldData pData = pAction->getMarkedFieldStepData();
                            // [AI: REPLACE - AI POLICY]
                            // Placement target (mines, etc.) chosen uniformly at random
                            // from every legal tile -- no scoring whatsoever. Where a mine
                            // goes is a real positional decision; Search AI should
                            // evaluate each legal placement tile.
                            QPoint point = pData->getPoints()->at(GlobalUtils::randIntBase(0, pData->getPoints()->size() - 1));
                            CoreAI::addSelectedFieldData(pAction, point);
                            emit sigPerformAction(pAction);
                            return true;
                        }
                    }
                }
                if (pUnit->canMoveAndFire(pAction->getActionTarget()) ||
                    pUnit->getPosition() == pAction->getActionTarget())
                {
                    pAction->setActionID(ACTION_FIRE);
                    // if we run away and still find a target we should attack it
                    std::vector<QVector3D> moveTargets(1, QVector3D(pAction->getActionTarget().x(),
                                                                    pAction->getActionTarget().y(), 1));
                    std::vector<QVector3D> ret;
                    getBestAttacksFromField(pUnit, pAction, ret, moveTargets);
                    // [AI: REPLACE - AI POLICY]
                    // Second instance of the accept-threshold + uniform random target
                    // pick described above. Same replacement applies.
                    if (ret.size() > 0 && ret[0].z() >= -pUnit->getCoUnitValue() * m_minSuicideDamage)
                    {
                        qint32 selection = GlobalUtils::randIntBase(0, ret.size() - 1);
                        QVector3D target = ret[selection];
                        CoreAI::addSelectedFieldData(pAction, QPoint(static_cast<qint32>(target.x()),
                                                                     static_cast<qint32>(target.y())));
                        if (pAction->isFinalStep() && pAction->canBePerformed())
                        {
                            m_updatePoints.push_back(pUnit->getPosition());
                            m_updatePoints.push_back(pAction->getActionTarget());
                            m_updatePoints.push_back(QPoint(static_cast<qint32>(target.x()),
                                                            static_cast<qint32>(target.y())));
                            emit sigPerformAction(pAction);
                            return true;
                        }
                    }
                }
                if (actions.contains(ACTION_CAPTURE))
                {
                    pAction->setActionID(ACTION_CAPTURE);
                    if (pAction->canBePerformed())
                    {
                        m_updatePoints.push_back(pUnit->getPosition());
                        m_updatePoints.push_back(pAction->getActionTarget());
                        emit sigPerformAction(pAction);
                        return true;
                    }
                }
                pAction->setActionID(ACTION_WAIT);
                if (pAction->canBePerformed())
                {
                    emit sigPerformAction(pAction);
                    return true;
                }
                else
                {
                    oxygine::handleErrorPolicy(oxygine::ep_show_error, "NormalAi::moveUnit invalid action calculated");
                }
            }
        }
    }
    return false;
}

// [AI: REPLACE - AI POLICY]
// The whole function is an escape hatch: "no good option was found, so attack
// something." Search AI should not need it. If throwing the unit at a target really is
// the best available line, that candidate wins on evaluated board score like any other;
// if it is not, the unit should retreat or wait. A dedicated all-in path only exists
// because the caller's ordering can leave a unit with no scored options at all.
bool NormalAi::suicide(spGameAction &pAction, Unit *pUnit, UnitPathFindingSystem &turnPfs, qint32 movepoints)
{
    AI_CONSOLE_PRINT("NormalAi::suicide", GameConsole::eDEBUG);
    // we don't have a good option do the best that we can attack with an all in attack :D
    pAction->setActionID(ACTION_FIRE);
    std::vector<QVector3D> ret;
    std::vector<QVector3D> moveTargetFields;
    CoreAI::getBestTarget(pUnit, pAction, &turnPfs, ret, moveTargetFields, movepoints + 1);
    // [AI: REPLACE - AI POLICY]
    // Third instance of accept-threshold + uniform random target pick.
    if (ret.size() > 0 && ret[0].z() >= -pUnit->getCoUnitValue() * m_minSuicideDamage)
    {
        qint32 selection = GlobalUtils::randIntBase(0, ret.size() - 1);
        QVector3D target = ret[selection];
        auto path = turnPfs.getPathFast(static_cast<qint32>(moveTargetFields[selection].x()),
                                        static_cast<qint32>(moveTargetFields[selection].y()));
        pAction->setMovepath(path, turnPfs.getCosts(path));
        CoreAI::addSelectedFieldData(pAction, QPoint(static_cast<qint32>(target.x()),
                                                     static_cast<qint32>(target.y())));
        if (pAction->isFinalStep() && pAction->canBePerformed())
        {
            m_updatePoints.push_back(pUnit->getPosition());
            m_updatePoints.push_back(pAction->getActionTarget());
            m_updatePoints.push_back(QPoint(static_cast<qint32>(target.x()),
                                            static_cast<qint32>(target.y())));
            emit sigPerformAction(pAction);
            return true;
        }
    }
    return false;
}

// [AI: REPLACE - AI POLICY]
// Single-objective retreat: minimise counter damage to *this one unit*, tie-break on
// raw distance to the original target. It is the clearest example of action-shaped
// scoring in the movement code (AI_Design_Notes_Consolidated.md section 6.1) -- nothing
// about the rest of the board enters the comparison, so a tile that is 1 point safer
// for this unit always beats a tile that shields an expensive ally, holds a chokepoint,
// or keeps a capture in reach.
// Under a board-shaped evaluator this function disappears: "retreat" is just the
// candidate set of reachable tiles, scored like every other candidate, and the
// defensive term is the collapsed sum from section 6.2 rather than one unit's damage.
// The reachable-tile enumeration and the counter-damage call are still reusable.
// Note the integer casts in the comparisons quantise scores to whole funds, which is
// what makes the allFieldsEqual signal (used to trigger suicide) fire as often as it does.
std::tuple<QPoint, float, bool> NormalAi::moveToSafety(MoveUnitData &unitData, UnitPathFindingSystem &turnPfs, QPoint target,
                                                       spQmlVectorBuilding &pBuildings, spQmlVectorBuilding &pEnemyBuildings,
                                                       qint32 movePoints)
{
    AI_CONSOLE_PRINT("NormalAi::moveToSafety", GameConsole::eDEBUG);
    Unit *pUnit = unitData.pUnit.get();
    auto targets = turnPfs.getAllNodePointsFast(movePoints + 1);
    QPoint ret(pUnit->Unit::getX(), pUnit->Unit::getY());
    float leastDamageField = std::numeric_limits<float>::max();
    qint32 shortestDistance = std::numeric_limits<qint32>::max();
    bool allFieldsEqual = true;
    for (auto &moveTarget : targets)
    {
        qint32 x = moveTarget.x();
        qint32 y = moveTarget.y();
        if (m_pMap->getTerrain(x, y)->getUnit() == nullptr &&
            turnPfs.getCosts(turnPfs.getIndex(x, y), x, y, x, y, 0) > 0)
        {
            float currentDamage = calculateCounterDamage(unitData, moveTarget, nullptr, 0.0f, pBuildings, pEnemyBuildings, true);
            if (currentDamage < 0)
            {
                currentDamage = 0;
            }
            if (leastDamageField < std::numeric_limits<float>::max() &&
                static_cast<qint32>(leastDamageField) != static_cast<qint32>(currentDamage))
            {
                allFieldsEqual = false;
            }
            qint32 distance = GlobalUtils::getDistance(target, moveTarget);
            if (currentDamage < leastDamageField)
            {
                ret = moveTarget;
                leastDamageField = currentDamage;
                shortestDistance = distance;
            }
            else if (static_cast<qint32>(currentDamage) == static_cast<qint32>(leastDamageField) &&
                     distance < shortestDistance &&
                     distance > 0)
            {
                ret = moveTarget;
                leastDamageField = currentDamage;
                shortestDistance = distance;
            }
        }
    }
    return std::tuple<QPoint, float, bool>(ret, leastDamageField, allFieldsEqual);
}

// [AI: REPLACE - AI POLICY]
// Walks the path toward the target and stops at the furthest tile whose counter damage
// is under a risk budget of MinMovementDamage (0.3) x the unit's own value. Two
// heuristics in one: the 30%-of-my-value risk tolerance, and the rule that only tiles
// *on the already-chosen path* are considered -- so the unit can advance or stop, but
// never step aside.
// Search AI should score all reachable tiles as candidates, and let acceptable risk fall
// out of the board comparison instead of a fixed fraction of unit cost.
qint32 NormalAi::getMoveTargetField(MoveUnitData &unitData, UnitPathFindingSystem &turnPfs,
                                    std::vector<QPoint> &movePath, spQmlVectorBuilding &pBuildings, spQmlVectorBuilding &pEnemyBuildings,
                                    qint32 movePoints)
{
    const float minDamage = unitData.unitCosts * m_minMovementDamage;
    float bestMinDamge = minDamage;
    qint32 bestIdx = -1;
    qint32 movePathSize = movePath.size() - 1;
    for (qint32 i = 0; i <= movePathSize; i++)
    {
        // empty or own field
        qint32 x = movePath[i].x();
        qint32 y = movePath[i].y();
        Terrain *pTerrain = m_pMap->getTerrain(x, y);
        Building *pBuilding = pTerrain->getBuilding();
        qint32 costs = turnPfs.getCosts(turnPfs.getIndex(x, y), x, y, x, y, 0);
        if ((pTerrain->getUnit() == nullptr ||
             pTerrain->getUnit() == unitData.pUnit.get()) &&
            costs >= 0 &&
            costs <= movePoints)
        {
            if (isMoveableTile(pBuilding, turnPfs))
            {
                float counterDamage = calculateCounterDamage(unitData, movePath[i], nullptr, 0.0f, pBuildings, pEnemyBuildings, true);
                if (counterDamage <= bestMinDamge &&
                    (i != movePathSize || counterDamage <= 0))
                {
                    bestIdx = i;
                    if (counterDamage <= 0)
                    {
                        return bestIdx;
                    }
                    bestMinDamge = counterDamage;
                }
            }
        }
    }
    return bestIdx;
}

qint32 NormalAi::getBestAttackTarget(MoveUnitData &unitData, std::vector<CoreAI::DamageData> &ret,
                                     std::vector<QVector3D> &moveTargetFields,
                                     spQmlVectorBuilding &pBuildings, spQmlVectorBuilding &pEnemyBuildings)
{
    qint32 target = -1;
    qint32 currentDamage = std::numeric_limits<qint32>::min();
    qint32 deffense = 0;
    Unit *pUnit = unitData.pUnit.get();

    float minFundsDamage = -unitData.unitCosts * m_minAttackFunds;

    for (qint32 i = 0; i < ret.size(); i++)
    {
        QPoint moveTarget(static_cast<qint32>(moveTargetFields[i].x()), static_cast<qint32>(moveTargetFields[i].y()));
        Unit *pEnemy = m_pMap->getTerrain(static_cast<qint32>(ret[i].x), static_cast<qint32>(ret[i].y))->getUnit();

        qint32 minfireRange = pUnit->getMinRange(moveTarget);
        qint32 fundsDamage = 0;
        float bonusDamage = 0.0f;
        // [AI: REPLACE - AI POLICY]
        // The multiplier stack below is Normal AI's attack evaluation. Starting from a
        // real quantity (fundsDamage) it applies four tuned factors in sequence:
        //   OwnIndirectAttackValue (x2)  - I am an indirect, so value this more
        //   EnemyKillBonus         (x2)  - this attack kills, so value it more
        //   EnemyIndirectBonus     (x3)  - the victim is an indirect, so value it more
        //   OwnProdctionMalus  (-5000)   - do not park on my own factory
        // These compound multiplicatively: killing an enemy artillery from an indirect
        // scores 12x its funds value. The intent behind each is defensible, but as
        // multipliers on a funds figure they leave the score in no meaningful unit, so it
        // cannot be compared against capture value, income, or positional value.
        // Search AI should express each of these as a funds-denominated term in the
        // evaluator (kills remove future damage output; indirects are worth more because
        // of what they deny) and sum them, per AI_Design_Notes_Consolidated.md section 4
        // suggestion 1 -- one common unit of measure.
        if (pEnemy != nullptr)
        {
            float currentHp = pEnemy->getHp();
            float newHp = currentHp - static_cast<float>(ret[i].hpDamage);
            auto captureBonus = calculateCaptureBonus(pEnemy, newHp);
            fundsDamage = static_cast<qint32>(ret[i].fundsDamage * captureBonus);
            if (fundsDamage > minFundsDamage && newHp > 0)
            {
                pEnemy->setVirtualHpValue(newHp);
                fundsDamage += getOwnSupportDamage(pUnit, moveTarget, pEnemy, bonusDamage);
                pEnemy->setVirtualHpValue(0.0f);
            }
            if (minfireRange > 1)
            {
                fundsDamage *= m_ownIndirectAttackValue;
            }
            if (newHp <= 0)
            {
                fundsDamage *= m_enemyKillBonus;
            }
            if (pEnemy->getMinRange(pEnemy->getPosition()) > 1)
            {
                fundsDamage *= m_enemyIndirectBonus;
            }
            if (!isMoveableTile(m_pMap->getTerrain(moveTarget.x(), moveTarget.y())->getBuilding(), *unitData.pUnitPfs))
            {
                fundsDamage -= m_ownProdctionMalus;
            }
        }
        else
        {
            fundsDamage = static_cast<qint32>(ret[i].fundsDamage);
        }
        float counterDamage = calculateCounterDamage(unitData, moveTarget, pEnemy, ret[i].hpDamage + bonusDamage, pBuildings, pEnemyBuildings, true);
        if (counterDamage < 0 ||
            !unitData.pUnitPfs->hasPoints(pUnit->getX(), pUnit->getY()))
        {
            counterDamage = 0;
        }
        fundsDamage -= counterDamage;
        Terrain *pTerrain = m_pMap->getTerrain(static_cast<qint32>(ret[i].x), static_cast<qint32>(ret[i].y));
        qint32 targetDefense = pTerrain->getDefense(pUnit);
        // [AI: REPLACE - AI POLICY]
        // Two admission filters before a target may be considered at all:
        // MinAttackFunds scales a floor by the attacker's own cost, and MinHpDamage
        // (-2.0) rejects any exchange where the AI loses more than 2 HP more than it
        // deals. The second is a flat refusal to take unfavourable HP trades regardless
        // of what the trade accomplishes -- it blocks exactly the deliberate bad-looking
        // trades that AI_Design_Notes_Consolidated.md section 4 (opportunity cost /
        // denial value) argues are sometimes correct.
        // Search AI should let unfavourable trades be scored and rejected on their merits,
        // not filtered out before scoring.
        if (fundsDamage >= minFundsDamage &&
            ret[i].hpDamageDifference >= m_minHpDamage)
        {
            if (fundsDamage > currentDamage)
            {
                currentDamage = fundsDamage;
                target = i;
                deffense = targetDefense;
            }
            else if (fundsDamage == currentDamage && targetDefense > deffense)
            {
                currentDamage = fundsDamage;
                target = i;
                deffense = targetDefense;
            }
        }
    }
    return target;
}

// [AI: INVESTIGATE]
// Estimates "if I attack this enemy, how much extra damage can my other unmoved units
// pile onto the same target." The *idea* is sound and is the focus-fire behaviour
// AI_Design_Notes_Consolidated.md section 6.1 says should emerge for free from
// sequential board-shaped evaluation -- so Search AI likely gets this without a
// dedicated function.
// The implementation carries three heuristics that should not be inherited:
//   - the CheapUnitValue (3000) cut-off deciding which other enemies "count" as
//     competing for the supporting unit's attention,
//   - dividing the support damage by (competing targets + 1), the ad-hoc "split
//     attention" discount criticised in section 2 -- the well-formed version is target
//     reservation during the sequential pass (section 6.3),
//   - the SupportDamageBonus multiplier on top.
// It also assumes the supporting unit will actually choose this target, which nothing
// guarantees.
float NormalAi::getOwnSupportDamage(Unit *pUnit, QPoint moveTarget, Unit *pEnemy, float &hpDamage) const
{
    float supportDamage = 0;
    hpDamage = 0;
    for (auto &pUnitData : m_OwnUnits)
    {
        if (pUnitData.pUnit.get() != pUnit &&
            !pUnitData.pUnit->getHasMoved() &&
            pUnitData.pUnit->hasWeapons())
        {
            auto position = pUnitData.pUnit->getPosition();
            qint32 movepoints = pUnitData.movementPoints;
            float minFundsDamage = -pUnitData.pUnit->getCoUnitValue() * m_minAttackFunds;
            if (GlobalUtils::getDistance(moveTarget, position) <= movepoints)
            {
                std::vector<CoreAI::DamageData> ret;
                spQmlVectorPoint firerange;
                firerange = GlobalUtils::getSpCircle(pUnitData.minFireRange, pUnitData.maxFireRange);
                CoreAI::getAttackTargetsFast(pUnitData.pUnit.get(), *firerange.get(), pUnitData.pUnitPfs.get(), ret);

                std::vector<Unit *> pUsedUnits;
                float newFundsDamage = std::numeric_limits<float>::lowest();
                float newHpDamage = std::numeric_limits<float>::lowest();
                for (auto &data : ret)
                {
                    QPoint newMoveTarget(static_cast<qint32>(data.x), static_cast<qint32>(data.y));
                    Unit *pNewEnemy = m_pMap->getTerrain(static_cast<qint32>(data.x), static_cast<qint32>(data.y))->getUnit();
                    if (pNewEnemy == pEnemy &&
                        moveTarget != newMoveTarget &&
                        pNewEnemy != nullptr)
                    {
                        float newHp = pEnemy->getHp() - static_cast<float>(data.hpDamage);
                        qint32 fundsDamage = static_cast<qint32>(data.fundsDamage * calculateCaptureBonus(pEnemy, newHp));
                        if (fundsDamage > minFundsDamage && fundsDamage > newFundsDamage)
                        {
                            newFundsDamage = fundsDamage;
                            newHpDamage = data.hpDamage;
                        }
                    }
                    else if (pNewEnemy != nullptr &&
                             pNewEnemy != pEnemy &&
                             !GlobalUtils::contains(pUsedUnits, pNewEnemy))
                    {
                        if (pNewEnemy->getCoUnitValue() >= m_cheapUnitValue)
                        {
                            pUsedUnits.push_back(pNewEnemy);
                        }
                    }
                }
                if (newFundsDamage > minFundsDamage)
                {
                    supportDamage += static_cast<float>(newFundsDamage) / static_cast<float>(pUsedUnits.size() + 1) * m_supportDamageBonus;
                    hpDamage += static_cast<float>(newHpDamage) / static_cast<float>(pUsedUnits.size() + 1) * m_supportDamageBonus;
                }
            }
        }
    }
    return supportDamage;
}

// [AI: REPLACE - AI POLICY]
// Multiplier applied to attack value when the target is mid-capture. It computes
// something genuinely useful -- how many turns the capture is delayed by the damage --
// but then converts that into a bare multiplier through a stack of tuned constants:
// AntiCaptureBonus (21x) for killing a capturer about to finish, AntiCaptureHqBonus
// (50x) if the building is our HQ, plus literal 0.8f/1.0f cases and a
// Reduction/Divider pair that rescales the result when it grows too large.
// The 20-capture-point figure is also hard-coded rather than read from the building.
// Search AI has the right quantity in the wrong form. Delay-a-capture should be priced
// as denied income (income x turns delayed, AI_Design_Notes_Consolidated.md section 4
// point 2), and losing the HQ belongs in the clamped victory term (section 4
// suggestion 2) -- a 50x multiplier can still be outvoted by accumulated small terms,
// a clamp cannot.
float NormalAi::calculateCaptureBonus(Unit *pUnit, float newLife) const
{
    float ret = 1.0f;
    qint32 capturePoints = pUnit->getCapturePoints();
    Building *pBuilding = pUnit->getTerrain()->getBuilding();
    if (capturePoints > 0)
    {
        qint32 restCapture = 20 - capturePoints;
        qint32 currentHp = pUnit->getHpRounded();
        qint32 newHp = GlobalUtils::roundUp(newLife);
        qint32 remainingDays = GlobalUtils::roundUp(static_cast<float>(restCapture) / static_cast<float>(currentHp));
        if (remainingDays <= 1)
        {
            if (newHp <= 0)
            {
                ret = m_antiCaptureBonus;
            }
            else
            {
                qint32 newRemainingDays = GlobalUtils::roundUp(static_cast<float>(restCapture) / static_cast<float>(newHp));
                if (remainingDays > newRemainingDays)
                {
                    ret = 0.8f;
                }
                else if (remainingDays == newRemainingDays && remainingDays < 2)
                {
                    ret = 1.0f;
                }
                else if (remainingDays == 0)
                {
                    ret = 1.0f;
                }
                else
                {
                    ret = 1 + (newRemainingDays - remainingDays) / remainingDays;
                }
                if (ret > m_antiCaptureBonusScoreReduction)
                {
                    ret = ret / m_antiCaptureBonusScoreDivider + m_antiCaptureBonusScoreReduction / m_antiCaptureBonusScoreDivider;
                }
            }
        }
    }
    if (pBuilding != nullptr &&
        pBuilding->getOwner() == m_pPlayer &&
        pBuilding->getBuildingID() == CoreAI::BUILDING_HQ &&
        pUnit->hasAction(ACTION_CAPTURE))
    {
        ret *= m_antiCaptureHqBonus;
    }
    return ret;
}

// [AI: REPLACE - AI POLICY]
// This is Normal AI's threat map, and it is the function
// AI_Design_Notes_Consolidated.md section 2 calls out by name for violating the
// "threat describes capability, evaluation decides how much I care" boundary. It is
// called from moveToSafety, getMoveTargetField and getBestAttackTarget, so the value
// judgments baked in here contaminate every movement and attack decision.
// Specific problems, each marked individually below:
//   - EnemyCounterDamageMultiplier inflating predicted enemy damage,
//   - the NotAttackableDamage threshold gating whether a threat is counted at all,
//   - the 0.5x "this enemy has other targets so it probably won't shoot me" discount,
//   - influence and building damage summed into the same return value, so callers
//     cannot tell threat from positional preference (double-counting, section 4).
// Search AI needs this split in two: a threat map that reports, per tile, which enemies
// can reach it and for how much expected funds damage (a real struct, section 2 -- note
// this code repurposes QRectF as a float box via .x()/.moveLeft(), which the notes say
// not to copy), and an evaluator that decides what that threat is worth.
// The underlying reachability + calcVirtuelUnitDamage machinery is reusable as-is.
float NormalAi::calculateCounterDamage(MoveUnitData &curUnitData, QPoint newPosition,
                                       Unit *pEnemyUnit, float enemyTakenDamage,
                                       spQmlVectorBuilding &pBuildings, spQmlVectorBuilding &pEnemyBuildings,
                                       bool ignoreOutOfVisionRange)
{
    AI_CONSOLE_PRINT("NormalAi calculateCounterDamage", GameConsole::eDEBUG);
    Interpreter *pInterpreter = Interpreter::getInstance();
    pInterpreter->threadProcessEvents();
    Unit *pUnit = curUnitData.pUnit.get();
    std::map<QString, qint32> unitDamageData;
    float counterDamage = 0;
    for (auto &enemyData : m_EnemyUnits)
    {
        spUnit pNextEnemy = enemyData.pUnit;
        if (pNextEnemy->getHp() > 0 && pNextEnemy->getTerrain() != nullptr)
        {
            QPoint enemyPos = QPoint(pNextEnemy->Unit::getX(), pNextEnemy->Unit::getY());
            qint32 distance = GlobalUtils::getDistance(newPosition, enemyPos);
            qint32 maxFireRange = enemyData.maxFireRange;
            bool hasDamage = unitDamageData.contains(pNextEnemy->getUnitID());
            float unitDamage = -1;
            if (hasDamage)
            {
                unitDamage = unitDamageData[pNextEnemy->getUnitID()];
            }
            qint32 moveRange = 0;
            bool canMoveAndFire = false;
            if (distance <= enemyData.movementPoints + maxFireRange)
            {
                canMoveAndFire = pNextEnemy->canMoveAndFire(enemyPos);
                if (canMoveAndFire)
                {
                    moveRange = enemyData.movementPoints;
                }
            }
            if (distance <= moveRange + maxFireRange &&
                (unitDamage >= 0 ||
                 (!hasDamage && pNextEnemy->isAttackable(pUnit, true))))
            {
                qint32 minFireRange = enemyData.minFireRange;
                float enemyDamage = enemyData.virtualDamageData;
                if (pNextEnemy.get() == pEnemyUnit)
                {
                    enemyDamage += enemyTakenDamage;
                }
                // [AI: REPLACE - AI POLICY]
                // Scales the damage this AI expects to have already dealt to the enemy
                // (virtualDamageData, itself a discounted estimate from calcVirtualDamage)
                // by a tuned 10x, then uses the result to decide the enemy will be dead
                // and cannot retaliate. A pessimism/optimism dial dressed as a prediction.
                // Search AI should track expected damage already committed against each
                // target honestly (target reservation, section 6.3) rather than
                // multiplying an estimate by a constant.
                enemyDamage *= m_enemyCounterDamageMultiplier;
                if (enemyDamage < pNextEnemy->getHp() * Unit::MAX_UNIT_HP)
                {
                    QRectF damageData;
                    if (distance >= minFireRange && distance <= maxFireRange)
                    {
                        // indirect attack
                        if (hasDamage)
                        {
                            damageData.setX(unitDamage * pNextEnemy->getHp() / Unit::MAX_UNIT_HP);
                        }
                        else
                        {
                            damageData = CoreAI::calcVirtuelUnitDamage(m_pMap, pNextEnemy.get(), enemyDamage, enemyPos, GameEnums::LuckDamageMode_Average,
                                                                       pUnit, 0, newPosition, GameEnums::LuckDamageMode_Average,
                                                                       ignoreOutOfVisionRange);
                            if (damageData.x() >= 0)
                            {
                                unitDamageData.insert_or_assign(pNextEnemy->getUnitID(), damageData.x() * Unit::MAX_UNIT_HP / pNextEnemy->getHp());
                            }
                        }
                        // [AI: REPLACE - AI POLICY]
                        // Threat below NotAttackableDamage (25%) is treated as no threat.
                        // Above it, the loop below discounts the threat by up to 50% per
                        // *other* friendly unit the enemy could also shoot -- the "split
                        // attention between multiple targets" guess. Both are value
                        // judgments living inside threat computation, and the discount is
                        // cumulative across own units, so a unit standing in a crowd can
                        // have its predicted incoming damage driven arbitrarily close to
                        // zero and will happily walk into fire.
                        // Search AI: threat reports what each enemy can do to this tile.
                        // Which target the enemy will actually pick is a prediction that
                        // belongs in the opponent-reply model (section 5 point 2), and
                        // over-commitment is handled by target reservation (section 6.3).
                        if (damageData.x() >= m_notAttackableDamage)
                        {
                            for (auto &unitData : m_OwnUnits)
                            {
                                distance = GlobalUtils::getDistance(QPoint(unitData.pUnit->Unit::getX(), unitData.pUnit->Unit::getY()), enemyPos);
                                if (distance >= minFireRange && distance <= maxFireRange &&
                                    pNextEnemy->isAttackable(unitData.pUnit.get(), true))
                                {
                                    if (unitData.unitCosts > 0 && curUnitData.unitCosts > 0)
                                    {
                                        if (curUnitData.unitCosts > unitData.unitCosts)
                                        {
                                            // reduce damage the more units it can attack
                                            damageData.moveLeft(damageData.x() - damageData.x() * 0.5f * unitData.unitCosts / curUnitData.unitCosts);
                                        }
                                        else
                                        {
                                            damageData.moveLeft(damageData.x() - damageData.x() * 0.5f * curUnitData.unitCosts / unitData.unitCosts);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (canMoveAndFire)
                    {
                        auto targets = enemyData.pUnitPfs->getAllNodePointsFast(enemyData.movementPoints + 1);
                        bool found = false;
                        for (auto &target : targets)
                        {
                            distance = GlobalUtils::getDistance(newPosition, target);
                            Unit *pTerrainUnit = m_pMap->getTerrain(target.x(), target.y())->getUnit();
                            if (distance >= minFireRange && distance <= maxFireRange &&
                                (pTerrainUnit == nullptr ||
                                 pTerrainUnit == pNextEnemy.get()))
                            {
                                if (hasDamage)
                                {
                                    damageData.setX(unitDamage * pNextEnemy->getHp() / Unit::MAX_UNIT_HP);
                                }
                                else
                                {
                                    damageData = CoreAI::calcVirtuelUnitDamage(m_pMap, pNextEnemy.get(), enemyDamage, target, GameEnums::LuckDamageMode_Average,
                                                                               pUnit, 0, newPosition, GameEnums::LuckDamageMode_Average,
                                                                               ignoreOutOfVisionRange);
                                    if (damageData.x() >= 0)
                                    {
                                        unitDamageData.insert_or_assign(pNextEnemy->getUnitID(), damageData.x() * Unit::MAX_UNIT_HP / pNextEnemy->getHp());
                                    }
                                }
                                found = true;
                                break;
                            }
                        }
                        qint32 enemyIslandIdx = getIslandIndex(pNextEnemy.get());
                        qint32 enemyIsland = getIsland(pNextEnemy.get());
                        // [AI: REPLACE - AI POLICY]
                        // Same NotAttackableDamage gate and same cumulative 50%
                        // split-attention discount as the indirect branch above, repeated
                        // for move-and-fire attackers. Replace together.
                        if (found &&
                            damageData.x() >= m_notAttackableDamage)
                        {
                            for (auto &unitData : m_OwnUnits)
                            {
                                for (auto &target : targets)
                                {
                                    Unit *pNextUnit = unitData.pUnit.get();
                                    distance = GlobalUtils::getDistance(QPoint(pNextUnit->Unit::getX(), pNextUnit->Unit::getY()), target);
                                    Unit *pTargetUnit = m_pMap->getTerrain(target.x(), target.y())->getUnit();
                                    if (distance >= minFireRange && distance <= maxFireRange &&
                                        (pTargetUnit == nullptr ||
                                         pTargetUnit->getOwner()->isAlly(m_pPlayer)) &&
                                        enemyIsland == m_IslandMaps[enemyIslandIdx]->getIsland(target.x(), target.y()))
                                    {
                                        if (unitData.unitCosts > 0 && curUnitData.unitCosts > 0)
                                        {
                                            if (curUnitData.unitCosts > unitData.unitCosts)
                                            {
                                                // reduce damage the more units it can attack
                                                damageData.moveLeft(damageData.x() - damageData.x() * 0.5f * unitData.unitCosts / curUnitData.unitCosts);
                                            }
                                            else
                                            {
                                                damageData.moveLeft(damageData.x() - damageData.x() * 0.5f * curUnitData.unitCosts / unitData.unitCosts);
                                            }
                                        }
                                        break;
                                    }
                                }
                            }
                        }                        
                    }
                    if (damageData.x() < 0)
                    {
                        damageData.moveLeft(0);
                    }
                    if (damageData.x() > 0)
                    {
                        auto fundsDamageData = calcFundsDamage(damageData, pNextEnemy.get(), pUnit);
                        counterDamage += static_cast<qint32>(fundsDamageData.fundsDamage);
                    }
                }
            }
        }
    }
    // [AI: REPLACE - AI POLICY]
    // Three different quantities are summed into one number and returned as "counter
    // damage": actual predicted enemy retaliation, damage from enemy buildings/mines,
    // and a synthetic penalty derived from the influence map. Callers
    // (moveToSafety, getMoveTargetField, getBestAttackTarget) then compare that total
    // against funds thresholds as if it were all retaliation.
    // Folding influence into threat is the double-counting failure of
    // AI_Design_Notes_Consolidated.md section 4: influence already describes territorial
    // control, and mixing it in here means territory is priced once inside the threat
    // figure and again wherever positional value is scored. Search AI must keep threat,
    // influence and evaluation as three separate outputs and combine them only in the
    // evaluator, with per-category subtotals retained for debugging (section 4
    // suggestion 3 -- the AI_CONSOLE_PRINT below is the right instinct, keep that part).
    float buildingCounterDamage = calculateCounteBuildingDamage(pUnit, newPosition, pBuildings, pEnemyBuildings);
    float influenceCounterDamage = getMapInfluenceModifier(pUnit, newPosition.x(), newPosition.y());
    float totalCounterDamage = counterDamage + influenceCounterDamage + buildingCounterDamage;
    AI_CONSOLE_PRINT("NormalAi counter damage at x=" + QString::number(newPosition.x()) + " y=" + QString::number(newPosition.y()) +
                         " total score=" + QString::number(totalCounterDamage) +
                         " counter damage=" + QString::number(counterDamage) +
                         " influence damage=" + QString::number(influenceCounterDamage) +
                         " building damage=" + QString::number(buildingCounterDamage),
                     GameConsole::eDEBUG);
    return totalCounterDamage;
}

float NormalAi::calculateCounteBuildingDamage(Unit *pUnit, QPoint newPosition, spQmlVectorBuilding &pBuildings, spQmlVectorBuilding &pEnemyBuildings) const
{
    float counterDamage = 0.0f;
    // [AI: INVESTIGATE]
    // Existing bug, not a heuristic: the same loop over pEnemyBuildings is written
    // twice, so every enemy building's damage is counted double, and the pBuildings
    // parameter is never used. The second loop was presumably meant to iterate
    // pBuildings. Do not port the doubling; check what the intended second loop was
    // (own buildings shouldn't threaten us, so it may simply be dead code) before
    // treating any Normal AI positioning behaviour as a reference.
    for (auto &pBuilding : pEnemyBuildings->getVector())
    {
        counterDamage += calcBuildingDamage(pUnit, newPosition, pBuilding.get());
    }
    for (auto &pBuilding : pEnemyBuildings->getVector())
    {
        counterDamage += calcBuildingDamage(pUnit, newPosition, pBuilding.get());
    }
    spQmlVectorPoint pCircle = GlobalUtils::getSpCircle(1, 2);

    for (auto &circlePos : pCircle->getVector())
    {
        QPoint pos = newPosition + circlePos;
        if (m_pMap->onMap(pos.x(), pos.y()))
        {
            Unit *pMine = m_pMap->getTerrain(pos.x(), pos.y())->getUnit();
            // [AI: INVESTIGATE]
            // Hard-coded unit ID and a flat WatermineDamage (4.0) penalty for being
            // within 2 tiles of one, added into the same total as real predicted
            // retaliation. Search AI should get mine danger from the threat map like any
            // other source of expected damage, keyed off unit properties rather than a
            // literal "WATERMINE" string.
            if (pMine != nullptr &&
                !pMine->isStealthed(m_pPlayer) &&
                pMine->getUnitID() == "WATERMINE")
            {
                counterDamage += m_watermineDamage;
            }
        }
    }
    return counterDamage;
}

void NormalAi::updateAllUnitData(spQmlVectorUnit &pUnits, spQmlVectorBuilding &pBuildings)
{
    AI_CONSOLE_PRINT("NormalAi::updateAllUnitData()", GameConsole::eDEBUG);
    bool initial = m_EnemyUnits.size() == 0;
    spQmlVectorUnit enemyUnits = m_pPlayer->getSpEnemyUnits();
    // [AI: INVESTIGATE]
    // Discards enemy units further than EnemyPruneRange (3) from any own unit and
    // OwnBuildingPruneRange (10) from any own building, before any analysis runs. This is
    // a performance measure, but it silently deletes board state: a pruned enemy cannot
    // appear in the threat map, cannot influence production, and cannot be counted by
    // anything downstream, so the AI is blind to a force massing just outside the radius.
    // Search AI will need some bound on cost too, but pruning must not change what the
    // evaluator can see. Decide deliberately: prune candidate *moves* (section 5,
    // candidate pruning per unit) rather than pruning the world model.
    enemyUnits->pruneEnemies(pUnits.get(), pBuildings.get(), m_ownBuildingPruneRange, m_enemyPruneRange);
    
    // Will create an Island Map of all movable tiles on map for all units
    rebuildIsland(pUnits);
    rebuildIsland(enemyUnits);

    updateUnitData(pUnits, m_OwnUnits, false, m_EnemyUnits);
    updateUnitData(enemyUnits, m_EnemyUnits, true, m_OwnUnits);
    // [AI: REPLACE - AI POLICY]
    // Move ordering: units furthest from the enemy resolve first. Because every step in
    // performActionSteps() iterates m_OwnUnits in this order and returns on the first
    // success, this ordering materially changes the turn -- it is a policy, not a detail.
    // AI_Design_Notes_Consolidated.md section 6.3 identifies ordering as the one
    // coordination gap a board-shaped evaluator does not close on its own, and prescribes
    // the opposite rule: resolve units in decreasing order of cost/value so expensive
    // units commit first and cheap ones can react to them (screen, support, finish kills).
    // Keep the hook, replace the comparator; making the sequence itself searchable is the
    // v2 upgrade.
    sortUnitsFarFromEnemyFirst(m_OwnUnits, enemyUnits);
    if (initial)
    {
        calcVirtualDamage();
    }
    m_updatePoints.clear();

    if (initial)
    {
        createUnitInfluenceMap();
    }
}

void NormalAi::createUnitInfluenceMap()
{
    // create influence map at the start of the turn
    m_InfluenceFrontMap.clear();
    m_InfluenceFrontMap.setOwner(m_pPlayer);
    m_InfluenceFrontMap.addBuildingInfluence();
    Interpreter *pInterpreter = Interpreter::getInstance();
    for (auto &unit : m_OwnUnits)
    {
        pInterpreter->threadProcessEvents();
        m_InfluenceFrontMap.addUnitInfluence(unit.pUnit.get(), unit.pUnitPfs.get(), unit.movementPoints);
    }
    for (auto &unit : m_EnemyUnits)
    {
        pInterpreter->threadProcessEvents();
        m_InfluenceFrontMap.addUnitInfluence(unit.pUnit.get(), unit.pUnitPfs.get(), unit.movementPoints);
    }
    m_InfluenceFrontMap.updateOwners();
    m_InfluenceFrontMap.calculateGlobalData();
}

void NormalAi::updateUnitData(spQmlVectorUnit &pUnits, std::vector<MoveUnitData> &pUnitData, bool enemy, std::vector<MoveUnitData> &otherUnitData)
{
    AI_CONSOLE_PRINT("NormalAi::updateEnemyData", GameConsole::eDEBUG);
    if (pUnitData.size() == 0)
    {
        Interpreter *pInterpreter = Interpreter::getInstance();
        pUnitData.reserve(pUnits->size());
        for (auto &pUnit : pUnits->getVector())
        {
            pInterpreter->threadProcessEvents();
            MoveUnitData data;
            createUnitData(pUnit, data, enemy, m_influenceUnitRange, otherUnitData, !enemy);
            pUnitData.push_back(data);
        }
    }
    else
    {
        qint32 i = 0;
        while (i < pUnitData.size())
        {
            if (pUnitData[i].pUnit->getHp() <= 0 ||
                pUnitData[i].pUnit->getTerrain() == nullptr)
            {
                pUnitData.erase(pUnitData.cbegin() + i);
            }
            else
            {
                i++;
            }
        }
    }
    if (!enemy && m_aiStep >= AISteps::moveTransporters)
    {
        Interpreter *pInterpreter = Interpreter::getInstance();
        pUnitData.reserve(pUnits->size());
        for (auto &pUnit : pUnits->getVector())
        {
            pInterpreter->threadProcessEvents();
            bool found = false;
            for (auto &unitData : pUnitData)
            {
                if (unitData.pUnit == pUnit)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                MoveUnitData data;
                createUnitData(pUnit, data, enemy, m_influenceUnitRange, otherUnitData, !enemy);
                pUnitData.push_back(data);
            }
        }
    }
    Interpreter *pInterpreter = Interpreter::getInstance();
    std::vector<qint32> updated;
    for (auto point : m_updatePoints)
    {
        for (qint32 i2 = 0; i2 < pUnitData.size(); i2++)
        {
            if (!GlobalUtils::contains(updated, i2))
            {
                pInterpreter->threadProcessEvents();
                auto &unitData = pUnitData[i2];
                spUnit pUnit = unitData.pUnit;
                if (pUnit != nullptr &&
                    pUnit->getHp() > 0 &&
                    pUnit->getTerrain() != nullptr)
                {
                    if (qAbs(point.x() - pUnit->Unit::getX()) +
                            qAbs(point.y() - pUnit->Unit::getY()) <=
                        unitData.movementPoints * m_influenceUnitRange + 2)
                    {
                        createUnitData(pUnit, unitData, enemy, m_influenceUnitRange + 1, otherUnitData, true);
                    }
                    updated.push_back(i2);
                }
            }
        }
    }
}

void NormalAi::createUnitData(spUnit pUnit, MoveUnitData &data, bool enemy, double moveMultiplier, std::vector<MoveUnitData> &otherUnitData, bool always)
{
    QPoint pos = pUnit->Unit::getPosition();
    data.pUnitPfs = MemoryManagement::create<UnitPathFindingSystem>(m_pMap, pUnit.get());
    data.movementPoints = pUnit->getMovementpoints(pos);
    data.maxFireRange = pUnit->getMaxRange(pos);
    data.pUnit = pUnit;
    data.minFireRange = pUnit->getMinRange(pos);
    data.unitCosts = pUnit->getCoUnitValue();
    data.nextAiStep = m_aiFunctionStep;
    bool valid = false;
    if (!always)
    {
        auto range = data.movementPoints + data.maxFireRange;
        for (auto &otherUnit : otherUnitData)
        {
            if (GlobalUtils::getDistance(pos, otherUnit.pUnit->getPosition()) <= range + otherUnit.movementPoints + otherUnit.maxFireRange + 1)
            {
                valid = true;
                break;
            }
        }
    }
    if (valid || always)
    {
        if (enemy)
        {
            data.pUnitPfs->setIgnoreEnemies(UnitPathFindingSystem::CollisionIgnore::OnlyNotMovedEnemies);
        }
        else
        {
            data.actions = pUnit->getActionList();
        }
        // [AI: REPLACE - AI POLICY]
        // Each unit's pathfinder is explored with its movement inflated by
        // InfluenceUnitRange (1.75x), minus 1 if the unit already moved. The multiplier
        // is a stand-in for "roughly how far this unit projects power over the next
        // turn or two", and the -1 is an unexplained nudge.
        // Search AI needs the same idea expressed honestly: threat and influence should
        // be computed over real reachability per turn (section 3 -- decay by movement
        // cost, weight by cost x hp fraction), not by scaling one turn's move points by
        // a tuned factor. Note this inflated pfs is also what feeds the influence map
        // and calculateCounterDamage, so the fudge propagates widely.
        if (pUnit->getHasMoved())
        {
            // After the unit has moved, it should have slightly less influence--hence the "-1"
            data.pUnitPfs->setMovepoints(moveMultiplier * data.movementPoints - 1);
        }
        else
        {
            // Before the unit has moved, it should have more influence
            data.pUnitPfs->setMovepoints(moveMultiplier * data.movementPoints);
        }
        data.pUnitPfs->explore();
    }
}

// [AI: REPLACE - AI POLICY]
// Pre-computes, once per turn, how much damage each enemy unit "will probably" take
// from all own units, and stores it in enemyData.virtualDamageData -- which
// calculateCounterDamage then uses to decide the enemy may already be dead and cannot
// retaliate. Three heuristics stack up inside:
//   - maxDistance = 2, a guess that units matter within two turns of movement,
//   - damage / (movementPoints / maxDistance + 1), an ad-hoc distance discount,
//   - EnemyUnitCountDamageReductionMultiplier (0.5) x damage / attacks.size(),
//     spreading each unit's output evenly over everything it could hit.
// The result is a fiction: it assumes every unit attacks, spread thinly over all
// reachable targets, and no unit's actual choice is consulted.
// Search AI should get this from committed-damage bookkeeping during the sequential
// pass (target reservation, AI_Design_Notes_Consolidated.md section 6.3) -- record what
// has actually been decided, not an averaged prediction of what might happen.
void NormalAi::calcVirtualDamage()
{
    for (auto &ownUnit : m_OwnUnits)
    {
        Unit *pUnit = ownUnit.pUnit.get();
        struct DamagePosInfo
        {
            QPoint pos;
            float damage{0};
        };

        std::vector<DamagePosInfo> attacks;
        if (isUsingUnit(pUnit))
        {
            static constexpr float maxDistance = 2;
            spGameAction action = MemoryManagement::create<GameAction>(ACTION_FIRE, m_pMap);
            action->setTarget(QPoint(pUnit->Unit::getX(), pUnit->Unit::getY()));
            std::vector<CoreAI::DamageData> ret;
            std::vector<QVector3D> moveTargetFields;
            
            // We want to get all targets pUnit can attack over the next maxDistance turns.
            // This is what the value:
            //
            //      ownUnit.movementPoints * maxDistance + 1
            //
            // Is responsible for. The +1 is an off-by-one fix so that tiles at the maximum range of a
            // unit's movement are considered. 
            CoreAI::getAttackTargets(pUnit, action, ownUnit.pUnitPfs.get(), ret, moveTargetFields, ownUnit.movementPoints * maxDistance + 1);
            QPoint ownPos = pUnit->Unit::getPosition();
            for (auto &damageData : ret)
            {
                QPoint pos(static_cast<qint32>(damageData.x), static_cast<qint32>(damageData.y));
                bool containsPos = false;
                for (auto &attack : attacks)
                {
                    if (attack.pos.x() == pos.x() &&
                        attack.pos.y() == pos.y())
                    {
                        containsPos = true;
                        break;
                    }
                }

                if (!containsPos)
                {
                    float damage = 0;

                    if (GlobalUtils::getDistance(ownPos, pos) > ownUnit.movementPoints)
                    {
                        qint32 dis = ownUnit.movementPoints / maxDistance + 1;
                        damage = (damageData.hpDamage / dis);
                    }
                    else
                    {
                        damage = (damageData.hpDamage);
                    }
                    attacks.push_back(DamagePosInfo{pos, damage});
                }
            }
        }
        float attacksSize = attacks.size();
        for (auto &attack : attacks)
        {
            for (auto &enemyData : m_EnemyUnits)
            {
                if (enemyData.pUnit->Unit::getX() == attack.pos.x() &&
                    enemyData.pUnit->Unit::getY() == attack.pos.y())
                {
                    enemyData.virtualDamageData += (m_enemyUnitCountDamageReductionMultiplier * attack.damage  / attacksSize);
                    break;
                }
            }
        }
    }
}

// [AI: REPLACE - AI POLICY]
// Converts the influence map into a pseudo-damage figure so it can be added to counter
// damage: normalises own vs enemy influence into a -1..1 ratio, ignores it entirely
// below InfluenceIgnoreValue (0.2), then multiplies by the unit's value and
// InfluenceMultiplier (2.0) to produce "funds".
// This is where influence stops describing territorial control and starts asserting
// value, which AI_Design_Notes_Consolidated.md section 3 and 4 keep separate. The
// dead-zone threshold and the x2 are pure tuning, and the output is only in funds by
// construction, not by measurement.
// Search AI should read influence as a feature and weight it in the evaluator, where
// its contribution is visible and comparable, rather than laundering it into the threat
// figure. The InfluenceFrontMap itself is reusable -- it is this conversion that is not.
float NormalAi::getMapInfluenceModifier(Unit *pUnit, qint32 x, qint32 y) const
{
    const auto *info = m_InfluenceFrontMap.getInfluenceInfo(x, y);
    float enemyInfluence = info->getEnemyInfluence();
    float ownInfluence = info->getOwnInfluence();
    float influence = 0.0f;
    float influenceDamage = 0.0f;
    if (enemyInfluence > ownInfluence && ownInfluence > 0)
    {
        influence = 1.0f - ownInfluence / enemyInfluence;
    }
    else if (enemyInfluence > 0)
    {
        influence = - (1.0f - enemyInfluence / ownInfluence);
    }
    if (qAbs(influence) > m_influenceIgnoreValue)
    {
        influenceDamage = influence * pUnit->getCoUnitValue() * m_influenceMultiplier;
    }
    return influenceDamage;
}

void NormalAi::clearUnitData()
{
    m_EnemyUnits.clear();
    m_OwnUnits.clear();
    m_usedPredefinedAi = false;
}

bool NormalAi::buildUnits(spQmlVectorBuilding &pBuildings, spQmlVectorUnit &pUnits,
                          spQmlVectorUnit &pEnemyUnits, spQmlVectorBuilding &pEnemyBuildings)
{
    AI_CONSOLE_PRINT("NormalAi::buildUnits()", GameConsole::eDEBUG);
    pEnemyUnits->pruneEnemies(pUnits.get(), pBuildings.get(), m_ownBuildingPruneRange, m_enemyPruneRange);
    pBuildings->sortClosestToEnemy(pEnemyUnits);
    if (m_aiStep < AISteps::buildUnits)
    {
        m_productionData.clear();
        m_productionSystem.onNewBuildQueue(pBuildings.get(), pUnits.get(), pEnemyUnits, pEnemyBuildings.get());
    }
    m_aiStep = AISteps::buildUnits;
    bool executed = false;
    if (m_productionSystem.buildUnit(pBuildings.get(), pUnits.get(), pEnemyUnits.get(), pEnemyBuildings.get(), executed))
    {
        return executed;
    }

    // Everything below this comment may be dead code if buildUnit always returns true
    qint32 enemeyCount = 0;
    for (qint32 i = 0; i < m_pMap->getPlayerCount(); i++)
    {
        if (m_pPlayer->isEnemy(m_pMap->getPlayer(i)) && !m_pMap->getPlayer(i)->getIsDefeated())
        {
            enemeyCount++;
        }
    }
    float funds = m_pPlayer->getFunds();
    std::vector<float> data(BuildItems::Max, 0);
    float productionBuildings = 0;
    for (auto &pBuilding : pBuildings->getVector())
    {
        if (pBuilding->isProductionBuilding() &&
            pBuilding->getTerrain()->getUnit() == nullptr)
        {
            auto buildList = pBuilding->getConstructionList();
            for (auto &unitId : buildList)
            {
                Unit dummy(unitId, m_pPlayer, false, m_pMap);
                if (m_pPlayer->getCosts(unitId, pBuilding->getPosition()) < funds && dummy.hasWeapons())
                {
                    productionBuildings++;
                    break;
                }
            }
        }
    }
    if (productionBuildings <= 0)
    {
        return false;
    }
    UnitCountData countData;
    GetOwnUnitCounts(m_OwnUnits, pUnits, pEnemyUnits, pEnemyBuildings, countData);
    std::vector<NotAttackableData> attackCount(pEnemyUnits->size(), NotAttackableData());
    getEnemyDamageCounts(pUnits, pEnemyUnits, attackCount);
    data[InfantryUnitRatio] = static_cast<float>(countData.infantryUnits) / static_cast<float>(pUnits->size() + 1);
    data[SupplyRatio] = static_cast<float>(countData.supplyUnits) / static_cast<float>(pUnits->size() + 1);
    data[RequiredSupplyRatio] = static_cast<float>(countData.supplyNeededUnits) / static_cast<float>(pUnits->size() + 1);
    data[InfantryCount] = countData.infantryUnits;
    m_currentDirectIndirectRatio = m_directIndirectRatio * getAiCoBuildRatioModifier();
    if (productionBuildings > m_maxProductionBuildings)
    {
        productionBuildings = m_maxProductionBuildings;
    }
    // [AI: REPLACE - AI POLICY]
    // Everything from here to the end of the fundsPerFactory ladder is a price-bracket
    // policy: it decides how much to spend per factory this turn, which then drives
    // calcCostScore() and effectively picks the *tier* of unit before any unit has been
    // evaluated. A 0-100 dice roll selects between the brackets via SpamLightUnitChance
    // (30), SpamMediumUnitChance (30) and SpamInfantryChance (50), so with identical
    // board state the AI builds a light unit, an expensive unit, or infantry depending on
    // the roll. This is the same class of problem as the chance table in
    // SimpleProductionSystem::addItemToBuildDistribution, one layer up.
    // The bracket boundaries themselves (SpamingFunds 7500 x FundsPerBuildingFactorA/B/C,
    // CappingFunds, CappedFunds) are tuned constants unrelated to the map.
    // Search AI should not pre-select a price bracket at all. Score each buildable unit
    // by the board it produces (including projected capture income for transports,
    // AI_Design_Notes_Consolidated.md section 6.4) and let cost enter as one funds-
    // denominated term among others. Saving up for an expensive unit is a conclusion,
    // not a mode.
    // calc average costs if we would build same cost units on every building
    float fundsPerFactory = funds - m_cappedFunds * (productionBuildings - 1) * m_fundsPerBuildingFactorD;
    AI_CONSOLE_PRINT("NormalAI: Funds: " + QString::number(funds) + " funds for the next factory: " + QString::number(fundsPerFactory), GameConsole::eDEBUG);
    auto chance = GlobalUtils::randInt(0, 100);
    if (funds >= m_spamingFunds && productionBuildings > 1 && chance <= m_spamLightUnitChance)
    {
        fundsPerFactory = m_spamingFunds;
    }
    else if (funds >= m_spamingFunds * m_fundsPerBuildingFactorA && productionBuildings > 1 && chance < m_spamLightUnitChance + m_spamMediumUnitChance)
    {
        AI_CONSOLE_PRINT("NormalAI: Building expensive units", GameConsole::eDEBUG);
        fundsPerFactory = m_spamingFunds * m_fundsPerBuildingFactorA;
        data[UseHighTechUnits] = FundsMode::Expensive;
    }
    else if (fundsPerFactory <= m_cappingFunds)
    {
        if (funds >= m_spamingFunds && chance > m_spamInfantryChance)
        {
            fundsPerFactory = m_spamingFunds;
        }
        else
        {
            data[LowFunds] = 1.0;
            if (fundsPerFactory > m_cappedFunds * m_fundsPerBuildingFactorB && productionBuildings <= m_maxProductionBuildingsForB)
            {
                fundsPerFactory = m_cappedFunds * m_fundsPerBuildingFactorB;
            }
            else
            {
                fundsPerFactory = m_cappedFunds;
            }
        }
    }
    else if (fundsPerFactory < m_spamingFunds * m_fundsPerBuildingFactorA)
    {
        fundsPerFactory = m_spamingFunds;
    }
    else if (fundsPerFactory >= m_spamingFunds * m_fundsPerBuildingFactorC)
    {
        AI_CONSOLE_PRINT("NormalAI: Building very expensive units", GameConsole::eDEBUG);
        fundsPerFactory = m_spamingFunds * m_fundsPerBuildingFactorC;
        data[UseHighTechUnits] = FundsMode::VeryExpensive;
    }
    else if (fundsPerFactory >= m_spamingFunds * m_fundsPerBuildingFactorA)
    {
        if (fundsPerFactory >= m_spamingFunds * m_fundsPerBuildingFactorA + m_spamingFunds)
        {
            AI_CONSOLE_PRINT("NormalAI: Building expensive units with no spend malus", GameConsole::eDEBUG);
            data[UseHighTechUnits] = NoSpendMalus;
        }
        else
        {
            AI_CONSOLE_PRINT("NormalAI: Building expensive units", GameConsole::eDEBUG);
            data[UseHighTechUnits] = FundsMode::Expensive;
        }
        fundsPerFactory = m_spamingFunds * m_fundsPerBuildingFactorA;
    }
    AI_CONSOLE_PRINT("NormalAI: fundsPerFactory=" + QString::number(fundsPerFactory), GameConsole::eDEBUG);
    data[DirectUnitRatio] = static_cast<float>(countData.directUnits) / static_cast<float>(countData.indirectUnits + 1);

    data[UnitEnemyRatio] = (static_cast<float>(pUnits->size()) + m_ownUnitEnemyUnitRatioAverager) / (static_cast<float>(pEnemyUnits->size()) + m_ownUnitEnemyUnitRatioAverager);
    if (enemeyCount > 1)
    {
        data[UnitEnemyRatio] *= static_cast<float>(enemeyCount - 1);
    }
    data[UnitCount] = pUnits->size();
    data[EnemyUnitCount] = pEnemyUnits->size();
    spGameAction pAction = MemoryManagement::create<GameAction>(ACTION_BUILD_UNITS, m_pMap);
    float bestScore = NO_BUILD_SCORE + 1;
    std::vector<qint32> buildingIdx;
    std::vector<qint32> unitIDx;
    std::vector<float> scores;
    std::vector<bool> transporters;
    // [AI: REPLACE - AI POLICY]
    // "variance" is a deliberate unpredictability band: every candidate scoring within
    // `variance` of the best is kept in a shortlist, and one is then chosen at random
    // (see the randIntBase call after this loop). The band widens with the day number
    // from StartDayScoreVariancer (5) up to MaxDayScoreVariancer (10).
    // This is unpredictability for its own sake -- good for a human opponent, directly
    // opposed to a search AI, which must build the unit its evaluator ranks highest and
    // must produce the same decision from the same board every time (otherwise the
    // evaluator cannot be debugged, tuned, or searched over).
    float variance = m_pMap->getCurrentDay() - 1 + m_startDayScoreVariancer;
    if (variance > m_maxDayScoreVariancer)
    {
        variance = m_maxDayScoreVariancer;
    }
    spQmlVectorPoint pFields = GlobalUtils::getSpCircle(1, 1);
    Interpreter *pInterpreter = Interpreter::getInstance();
    for (qint32 i = 0; i < pBuildings->size(); i++)
    {
        pInterpreter->threadProcessEvents();
        Building *pBuilding = pBuildings->at(i);
        if (pBuilding->isProductionBuilding() &&
            pBuilding->getTerrain()->getUnit() == nullptr)
        {
            pAction->setTarget(QPoint(pBuilding->Building::getX(), pBuilding->Building::getY()));
            if (pAction->canBePerformed())
            {
                // we're allowed to build units here
                spMenuData pData = pAction->getMenuStepData();
                if (pData->validData())
                {
                    auto enableList = pData->getEnabledList();
                    auto actionIds = pData->getActionIDs();
                    qint32 index = getIndexInProductionData(pBuilding);
                    auto &buildingData = m_productionData[index];
                    for (qint32 i2 = 0; i2 < pData->getActionIDs().size(); i2++)
                    {
                        pInterpreter->threadProcessEvents();
                        if (enableList[i2])
                        {
                            float score = 0.0f;
                            qint32 unitIdx = getUnitProductionIdx(index, actionIds[i2],
                                                                  pUnits, countData.transportTargets,
                                                                  pEnemyUnits, pEnemyBuildings,
                                                                  attackCount, data);
                            // [AI: INVESTIGATE]
                            // Hard-coded building ID: anything produced at a harbour is
                            // pre-labelled a transporter, before createUnitBuildData() has
                            // decided whether the unit actually has no weapons. Search AI
                            // should classify from unit properties, not the building's
                            // string ID (which also breaks for modded buildings).
                            bool isTransporter = pBuilding->getBuildingID() == "HARBOUR";
                            if (unitIdx >= 0)
                            {
                                auto &unitData = buildingData.m_buildData[unitIdx];
                                data[UnitCost] = unitData.cost;
                                if (unitData.canMove)
                                {
                                    data[Movementpoints] = unitData.movePoints;
                                    data[ReachDistance] = unitData.closestTarget;
                                    data[COBonus] = unitData.coBonus;
                                    data[NotAttackableCount] = unitData.notAttackableCount;
                                    data[DamageData] = unitData.damage;
                                    data[AttackCount] = unitData.attackCount;
                                    data[FundsFactoryRatio] = unitData.cost / fundsPerFactory;
                                    data[SameFundsMatchUpScore] = unitData.sameFundsMatchUpScore;
                                    data[CounterDamage] = unitData.counterDamage;
                                    data[TurnOneDamageMalus] = unitData.turnOneDamage;
                                    if (pEnemyBuildings->size() > 0 && enemeyCount > 0)
                                    {
                                        data[BuildingEnemyRatio] = static_cast<float>(pBuildings->size()) / (static_cast<float>(pEnemyBuildings->size()) / static_cast<float>(enemeyCount));
                                    }
                                    else
                                    {
                                        data[BuildingEnemyRatio] = 0.0;
                                    }
                                    if (unitData.infantryUnit)
                                    {
                                        data[InfantryUnit] = 1.0;
                                    }
                                    else
                                    {
                                        data[InfantryUnit] = 0.0;
                                    }
                                    if (unitData.indirectUnit)
                                    {
                                        data[IndirectUnit] = 1.0;
                                        data[DirectUnit] = 0.0;
                                    }
                                    else
                                    {
                                        data[IndirectUnit] = 0.0;
                                        data[DirectUnit] = 1.0;
                                    }
                                    if (!unitData.isTransporter)
                                    {
                                        score = calcBuildScore(data, unitData);
                                    }
                                    else
                                    {
                                        score = calcTransporterScore(unitData, pUnits, data);
                                        isTransporter = true;
                                    }
                                }
                                else
                                {
                                    score = NO_BUILD_SCORE;
                                }
                            }
                            if (score > bestScore)
                            {
                                bestScore = score;
                                buildingIdx.push_back(i);
                                unitIDx.push_back(i2);
                                scores.push_back(score);
                                transporters.push_back(isTransporter);
                                qint32 index = 0;
                                while (index < scores.size())
                                {
                                    if (scores[index] < bestScore - variance)
                                    {
                                        buildingIdx.erase(buildingIdx.cbegin() + index);
                                        unitIDx.erase(unitIDx.cbegin() + index);
                                        scores.erase(scores.cbegin() + index);
                                        transporters.erase(transporters.cbegin() + index);
                                    }
                                    else
                                    {
                                        index++;
                                    }
                                }
                            }
                            else if (score >= bestScore - variance)
                            {
                                buildingIdx.push_back(i);
                                unitIDx.push_back(i2);
                                scores.push_back(score);
                                transporters.push_back(isTransporter);
                            }
                        }
                    }
                }
            }
        }
    }

    if (buildingIdx.size() > 0)
    {
        // [AI: REPLACE - AI POLICY]
        // The payoff of the `variance` band above: the final production choice is a
        // uniform random pick from every (building, unit) pair that scored within the
        // band of the best. The evaluator's ranking is computed and then discarded.
        // This is the decision the user's note on
        // SimpleProductionSystem::addItemToBuildDistribution describes from the other
        // end -- weights are accumulated there, and the dice are rolled here.
        // Search AI: take the argmax, deterministically.
        qint32 item = GlobalUtils::randIntBase(0, buildingIdx.size() - 1);
        Building *pBuilding = pBuildings->at(buildingIdx[item]);
        pAction->setTarget(QPoint(pBuilding->Building::getX(), pBuilding->Building::getY()));
        spMenuData pData = pAction->getMenuStepData();
        QString unitId = pData->getActionIDs()[unitIDx[item]];
        AI_CONSOLE_PRINT("NormalAi::buildUnits() producing " + unitId + " with score " + QString::number(scores[item]), GameConsole::eDEBUG);
        if (transporters[item])
        {
            for (auto &building : m_productionData)
            {
                for (auto &unit : building.m_buildData)
                {
                    if (unit.isTransporter)
                    {
                        unit.noTransporterBonus = 0;
                        unit.transportCount += 1;
                    }
                }
            }
        }
        if (pData->validData())
        {
            CoreAI::addMenuItemData(pAction, unitId, pData->getCostList()[unitIDx[item]]);
        }
        // produce the unit
        if (pAction->isFinalStep())
        {
            if (pAction->canBePerformed())
            {
                m_updatePoints.push_back(pAction->getActionTarget());
                emit sigPerformAction(pAction);
                return true;
            }
        }
    }
    return false;
}

qint32 NormalAi::getIndexInProductionData(Building *pBuilding)
{
    AI_CONSOLE_PRINT("NormalAi::getIndexInProductionData()", GameConsole::eDEBUG);
    qint32 x = pBuilding->Building::getX();
    qint32 y = pBuilding->Building::getY();
    qint32 ret = -1;
    for (qint32 i = 0; i < m_productionData.size(); ++i)
    {
        if (m_productionData[i].m_x == x &&
            m_productionData[i].m_y == y)
        {
            ret = i;
            break;
        }
    }
    if (ret < 0)
    {
        m_productionData.push_back(ProductionData());
        ret = m_productionData.size() - 1;
        m_productionData[ret].m_x = x;
        m_productionData[ret].m_y = y;
        m_productionData[ret].buildList = pBuilding->getConstructionList();
    }
    return ret;
}

qint32 NormalAi::getUnitProductionIdx(qint32 index, const QString &unitId,
                                      spQmlVectorUnit &pUnits, std::vector<std::tuple<Unit *, Unit *>> &transportTargets,
                                      spQmlVectorUnit &pEnemyUnits, spQmlVectorBuilding &pEnemyBuildings,
                                      std::vector<NotAttackableData> &attackCount, std::vector<float> &buildData)
{
    AI_CONSOLE_PRINT("NormalAi::getUnitProductionIdx()", GameConsole::eDEBUG);
    qint32 ret = -1;
    auto &data = m_productionData[index];
    for (qint32 i = 0; i < data.m_buildData.length(); ++i)
    {
        auto &unitData = data.m_buildData[i];
        if (unitData.unitId == unitId)
        {
            if (unitData.notAttackableCount > 0)
            {
                Unit dummy(unitId, m_pPlayer, false, m_pMap);
                dummy.setVirtuellX(data.m_x);
                dummy.setVirtuellY(data.m_y);
                float bonusFactor = 1.0f;
                if ((buildData[DirectUnitRatio] > m_directIndirectRatio && unitData.baseRange > 1) ||
                    (buildData[DirectUnitRatio] < m_directIndirectRatio && unitData.baseRange == 1))
                {
                    bonusFactor = m_directIndirectUnitBonusFactor;
                }
                auto damageData = calcExpectedFundsDamage(data.m_x, data.m_y, dummy, pEnemyUnits, attackCount, bonusFactor, unitData.movePoints);
                unitData.notAttackableCount = damageData.notAttackableCount;
                unitData.damage = damageData.damage;
                unitData.attackCount = damageData.attackCount;
                unitData.counterDamage = damageData.counterDamage;
                unitData.turnOneDamage = damageData.turnOneDamage;
            }
            ret = i;
            break;
        }
    }
    if (ret < 0)
    {
        data.m_buildData.append(UnitBuildData());
        ret = data.m_buildData.length() - 1;
        auto &unitBuildData = data.m_buildData[ret];
        unitBuildData.unitId = unitId;
        createUnitBuildData(data.m_x, data.m_y, unitBuildData,
                            pUnits, transportTargets,
                            pEnemyUnits, pEnemyBuildings,
                            attackCount, buildData, data.buildList);
    }
    return ret;
}

void NormalAi::createUnitBuildData(qint32 x, qint32 y, UnitBuildData &unitBuildData,
                                   spQmlVectorUnit &pUnits, std::vector<std::tuple<Unit *, Unit *>> &transportTargets,
                                   spQmlVectorUnit &pEnemyUnits, spQmlVectorBuilding &pEnemyBuildings,
                                   std::vector<NotAttackableData> &attackCount, std::vector<float> &buildData, const QStringList &buildList)
{
    Interpreter *pInterpreter = Interpreter::getInstance();
    pInterpreter->threadProcessEvents();
    AI_CONSOLE_PRINT("NormalAi::createUnitBuildData()", GameConsole::eDEBUG);
    MovementTableManager *pMovementTableManager = MovementTableManager::getInstance();
    Unit dummy(unitBuildData.unitId, m_pPlayer, false, m_pMap);
    dummy.setVirtuellX(x);
    dummy.setVirtuellY(y);
    spTerrain pDummyTerrain = Terrain::createTerrain(GameMap::PLAINS, -1, -1, "", m_pMap);
    qint32 baseMovementCost = pMovementTableManager->getBaseMovementPoints(dummy.getMovementType(), pDummyTerrain.get(), pDummyTerrain.get(), &dummy);
    if (baseMovementCost < 0)
    {
        baseMovementCost = 1;
    }
    createIslandMap(dummy.getMovementType(), dummy.getUnitID());
    UnitPathFindingSystem pfs(m_pMap, &dummy, m_pPlayer);
    pfs.explore();
    unitBuildData.movePoints = dummy.getMovementpoints(QPoint(x, y)) / baseMovementCost;
    auto points = pfs.getAllNodePointsFast();
    if (points.size() >= unitBuildData.movePoints * 1.5f)
    {
        QStringList actionList = dummy.getActionList();
        unitBuildData.canMove = true;
        unitBuildData.isTransporter = (dummy.getWeapon1ID().isEmpty() &&
                                       dummy.getWeapon2ID().isEmpty());
        unitBuildData.isSupplyUnit = isRefuelUnit(actionList);
        if (unitBuildData.isTransporter)
        {
            getTransporterData(unitBuildData, dummy, pUnits, pEnemyUnits, pEnemyBuildings, transportTargets);
        }
        else
        {
            unitBuildData.closestTarget = getClosestTargetDistance(x, y, dummy, pEnemyUnits, pEnemyBuildings);
            unitBuildData.coBonus += getAiCoUnitMultiplier(m_pPlayer->getCO(0), &dummy);
            unitBuildData.coBonus += getAiCoUnitMultiplier(m_pPlayer->getCO(1), &dummy);
            unitBuildData.baseRange = dummy.getBaseMaxRange();
            float bonusFactor = 1.0f;
            if ((buildData[DirectUnitRatio] > m_directIndirectRatio && unitBuildData.baseRange > 1) ||
                (buildData[DirectUnitRatio] < m_directIndirectRatio && unitBuildData.baseRange == 1))
            {
                bonusFactor = m_directIndirectUnitBonusFactor;
            }
            auto damageData = calcExpectedFundsDamage(x, y, dummy, pEnemyUnits, attackCount, bonusFactor, unitBuildData.movePoints);
            unitBuildData.notAttackableCount = damageData.notAttackableCount;
            unitBuildData.damage = damageData.damage;
            unitBuildData.attackCount = damageData.attackCount;
            unitBuildData.counterDamage = damageData.counterDamage;
            unitBuildData.turnOneDamage = damageData.turnOneDamage;
            unitBuildData.cost = dummy.getUnitCosts();
            unitBuildData.infantryUnit = (actionList.contains(ACTION_CAPTURE) &&
                                          dummy.getLoadingPlace() == 0);
            unitBuildData.indirectUnit = dummy.getBaseMaxRange() > 1;
            unitBuildData.sameFundsMatchUpScore = calcSameFundsMatchUpScore(dummy, buildList);
        }
    }
}

// [AI: INVESTIGATE]
// Counts, per enemy unit, how many of our units can hit it for high / mid / low damage,
// using base weapon damage only (no terrain, no CO, no luck). The counts are genuinely
// useful board information -- "we have no answer to their bombers" -- and Search AI
// wants something like it as a matchup-coverage feature.
// What should not be inherited is the bucketing: HighDamage (75), MidDamage (55) and
// NotAttackableDamage (25) collapse a continuous quantity into three tuned bins, and
// everything downstream in calcExpectedFundsDamage reasons about bin counts rather than
// actual damage. A 74% matchup and a 26% matchup land in the same bucket.
void NormalAi::getEnemyDamageCounts(spQmlVectorUnit &pUnits, spQmlVectorUnit &pEnemyUnits, std::vector<NotAttackableData> &attackCount)
{
    WeaponManager *pWeaponManager = WeaponManager::getInstance();
    for (qint32 i2 = 0; i2 < pEnemyUnits->size(); i2++)
    {
        for (auto &pUnit : pUnits->getVector())
        {
            float dmg1 = 0.0f;
            float hpValue = static_cast<float>(pUnit->getHpRounded()) / Unit::MAX_UNIT_HP;
            Unit *pEnemyUnit = pEnemyUnits->at(i2);
            // get weapon 1 damage
            if (!pUnit->getWeapon1ID().isEmpty())
            {
                dmg1 = pWeaponManager->getBaseDamage(pUnit->getWeapon1ID(), pEnemyUnit) * hpValue;
            }
            // get weapon 2 damage
            float dmg2 = 0.0f;
            if (!pUnit->getWeapon2ID().isEmpty())
            {
                dmg2 = pWeaponManager->getBaseDamage(pUnit->getWeapon2ID(), pEnemyUnit) * hpValue;
            }

            if (dmg1 > m_highDamage || dmg2 > m_highDamage)
            {
                attackCount[i2].highDamageCount += 1;
            }
            else if (dmg1 > m_midDamage || dmg2 > m_midDamage)
            {
                attackCount[i2].midDamageCount += 1;
            }
            else if ((dmg1 > m_notAttackableDamage || dmg2 > m_notAttackableDamage))
            {
                if (onSameIsland(pUnit.get(), pEnemyUnits->at(i2)))
                {
                    attackCount[i2].sameIslandNotAttackableCount += 1;
                }
                attackCount[i2].notAttackableCount += 1;
            }
        }
    }
}

qint32 NormalAi::getClosestTargetDistance(qint32 posX, qint32 posY, Unit &dummy, spQmlVectorUnit &pEnemyUnits, spQmlVectorBuilding &pEnemyBuildings)
{
    qint32 minDistance = std::numeric_limits<qint32>::max();
    QPoint pos(posX, posY);
    qint32 islandIdx = CoreAI::getIslandIndex(&dummy);
    for (auto &pEnemyUnit : pEnemyUnits->getVector())
    {
        if (onSameIsland(islandIdx, posX, posY, pEnemyUnit->Unit::getX(), pEnemyUnit->Unit::getY()))
        {
            if (dummy.isAttackable(pEnemyUnit.get(), true))
            {
                qint32 distance = GlobalUtils::getDistance(pos, pEnemyUnit->getPosition());
                if (minDistance > distance)
                {
                    minDistance = distance;
                }
            }
        }
    }
    if (dummy.getActionList().contains(ACTION_CAPTURE))
    {
        bool missileTarget = hasMissileTarget();
        for (auto &pBuilding : pEnemyBuildings->getVector())
        {
            if (dummy.canMoveOver(pBuilding->Building::getX(), pBuilding->Building::getY()))
            {
                if (pBuilding->isCaptureOrMissileBuilding(missileTarget) &&
                    pBuilding->getTerrain()->getUnit() == nullptr)
                {
                    qint32 distance = GlobalUtils::getDistance(pos, pBuilding->getPosition());
                    if (minDistance > distance)
                    {
                        minDistance = distance;
                    }
                }
            }
        }
    }
    return minDistance;
}

// [AI: REPLACE - AI POLICY]
// The densest heuristic in the file, and the core of Normal AI's production decision:
// "if I built this unit here, how much enemy value could it expect to threaten?"
// It walks every enemy unit and accumulates a score through roughly a dozen tuned
// adjustments, among them:
//   - ownRange/enemyRange approximated as movement + firerange, or their average x0.5
//     when the unit cannot move-and-fire,
//   - MaxOverkillBonus rescaling damage that exceeds the target's remaining value,
//   - a (range + SmoothingValue) / (range + SmoothingValue) ratio capped at
//     MaxDistanceMultiplier, standing in for "who shoots first",
//   - DirectIndirectUnitBonusFactor applied when the unit is of the type we currently
//     have too few of,
//   - SameIsland / DifferentIsland distance bonuses with two more caps on top,
//   - High/Mid/Low/VeryLowDamageBonus bin weights, then
//     CurrentlyNotAttackableBonus applied once or squared depending on which bins
//     are empty,
//   - IndirectUnitAttackCountMalus, a loop that scales the whole result down if the
//     unit can attack too small a fraction of the enemy army.
// Every one of these is a proxy standing in for a simulation the AI never runs, and
// they compound, so the output is not in any interpretable unit.
// Search AI should answer the same question directly: hypothesise the unit on the board
// and evaluate the resulting state with the same evaluator used for movement, including
// projected capture income (section 6.4). The one piece worth keeping is the framing --
// production is scored by expected board improvement per funds spent.
NormalAi::ExpectedFundsData NormalAi::calcExpectedFundsDamage(qint32 posX, qint32 posY, Unit &dummy, spQmlVectorUnit &pEnemyUnits, const std::vector<NotAttackableData> &attackCount, float bonusFactor, float myMovepoints)
{
    ExpectedFundsData ret;
    float notAttackableCount = 0;
    float damageCount = 0;
    float attacksCount = 0;
    qint32 baseAttacksCount = 0;
    float extraMalusCount = 0;

    float counterDamageCount = 0;
    float counterAttacksCount = 0;

    float turnOneDamage = 0;
    float turnOneDamageCount = 0;

    if (myMovepoints == 0)
    {
        myMovepoints = 1;
    }

    float myFirerange = dummy.getBaseMaxRange();
    float ownRange = myMovepoints;
    if (dummy.canMoveAndFire(QPoint(posX, posY)))
    {
        ownRange += myFirerange;
    }
    else
    {
        ownRange = (myMovepoints + myFirerange) * 0.5f;
    }

    QPoint position = dummy.getPosition();
    auto ownValue = dummy.getCoUnitValue();

    qint32 lowUnitAttackCount = 0;
    qint32 lowUnitDamageCount = 0;

    auto enemyUnitCount = pEnemyUnits->size();
    for (qint32 i3 = 0; i3 < enemyUnitCount; i3++)
    {
        Unit *pEnemyUnit = pEnemyUnits->at(i3);
        QPoint enemyPosition = pEnemyUnit->getPosition();
        float enemyFirerange = pEnemyUnit->getBaseMaxRange();
        float enemyMovepoints = pEnemyUnit->getBaseMovementPoints();
        float enemyRange = enemyMovepoints;
        float turnOneRange = enemyFirerange;
        if (pEnemyUnit->canMoveAndFire(enemyPosition))
        {
            turnOneRange += enemyMovepoints;
            enemyRange += enemyFirerange;
        }
        else
        {
            enemyRange = (enemyMovepoints + enemyFirerange) * 0.5f;
        }
        float distance = GlobalUtils::getDistance(position, enemyPosition);
        float dmg = getBaseDamage(&dummy, pEnemyUnit).x();
        float counterDmg = 0.0f;
        float baseCounterDmg = getBaseDamage(pEnemyUnit, &dummy).x();
        if ((baseCounterDmg < m_lowThreadDamage && dmg >= m_midDamage) ||
            dmg > pEnemyUnit->getHp() * Unit::MAX_UNIT_HP)
        {
            dmg = pEnemyUnit->getHp() * Unit::MAX_UNIT_HP;
        }
        counterDmg = baseCounterDmg * pEnemyUnit->getHp() / Unit::MAX_UNIT_HP;

        if (counterDmg > m_lowThreadDamage &&
            distance <= turnOneRange)
        {
            turnOneDamage += counterDmg;
            turnOneDamageCount += 1;
        }

        if (dmg > 0.0f)
        {
            bool firstStrikes = ownRange >= enemyRange || counterDmg <= m_notAttackableDamage;
            float resDamage = 0;

            auto enemyValue = pEnemyUnit->getCoUnitValue();
            resDamage = (dmg / (pEnemyUnit->getHp() * Unit::MAX_UNIT_HP)) * enemyValue;

            if (resDamage > enemyValue && resDamage > 0)
            {
                resDamage = enemyValue * (m_maxOverkillBonus - enemyValue / resDamage);
            }
            float mult = (ownRange + m_smoothingValue) / (enemyRange + m_smoothingValue);
            if (mult > m_maxDistanceMultiplier || counterDmg <= m_notAttackableDamage)
            {
                mult = m_maxDistanceMultiplier;
            }
            resDamage *= mult;
            if (firstStrikes)
            {
                resDamage *= bonusFactor;
            }
            float factor = 0.0f;
            if (dmg > m_highDamage)
            {
                factor += 1.5f - (static_cast<float>(attackCount[i3].highDamageCount) + m_smoothingValue) / (static_cast<float>(attackCount[i3].notAttackableCount) + m_smoothingValue);
            }
            else if (dmg > m_midDamage)
            {
                factor += 1.0f - (static_cast<float>(attackCount[i3].midDamageCount) + m_smoothingValue) / (static_cast<float>(attackCount[i3].notAttackableCount) + m_smoothingValue);
            }
            else if (dmg < m_notAttackableDamage)
            {
                ++extraMalusCount;
            }
            ++baseAttacksCount;
            bool sameIsland = onSameIsland(dummy.getMovementType(), posX, posY, pEnemyUnit->Unit::getX(), pEnemyUnit->Unit::getY());
            float distanceBonus = 0;
            if (sameIsland)
            {
                distanceBonus = (m_sameIslandBonusInRangeDays - (distance / myMovepoints) * m_sameIslandOutOfDayMalusFactor);
                if (distanceBonus > m_maxCloseDistanceDamageBonus)
                {
                    distanceBonus = m_maxCloseDistanceDamageBonus;
                }
                else if (distanceBonus > m_minCloseDistanceDamageBonus)
                {
                    distanceBonus = m_minCloseDistanceDamageBonus;
                }
            }
            else
            {
                distanceBonus = (m_differentIslandBonusInRangeDays - (distance / myMovepoints) * m_differentIslandOutOfDayMalusFactor);
                if (distanceBonus > m_maxCloseDistanceDamageBonus)
                {
                    distanceBonus = m_maxCloseDistanceDamageBonus;
                }
                else if (distanceBonus > m_minCloseDistanceDamageBonus)
                {
                    distanceBonus = m_minCloseDistanceDamageBonus;
                }
            }
            factor += distanceBonus;
            if (pEnemyUnit->hasWeapons())
            {
                float notAttackableValue = 0.0f;
                if (dmg >= m_highDamage)
                {
                    notAttackableValue = m_highDamageBonus;
                    if (sameIsland && distanceBonus >= 1)
                    {
                        if (attackCount[i3].midDamageCount == 0 &&
                            attackCount[i3].highDamageCount > 0)
                        {
                            lowUnitAttackCount += attackCount[i3].highDamageCount;
                            lowUnitDamageCount++;
                        }
                    }
                }
                else if (dmg >= m_midDamage)
                {
                    notAttackableValue = m_midDamageBonus;
                }
                else if (dmg > m_notAttackableDamage)
                {
                    notAttackableValue = m_lowDamageBonus;
                }
                else
                {
                    factor *= m_veryLowDamageBonus;
                }
                if (!sameIsland)
                {
                    notAttackableValue *= 0.5f;
                }

                if (attackCount[i3].sameIslandNotAttackableCount == 0 &&
                    attackCount[i3].notAttackableCount == 0 &&
                    attackCount[i3].midDamageCount == 0 &&
                    attackCount[i3].highDamageCount == 0)
                {
                    notAttackableCount += notAttackableValue * m_currentlyNotAttackableBonus;
                }
                else if (attackCount[i3].notAttackableCount == 0 &&
                         attackCount[i3].midDamageCount == 0 &&
                         attackCount[i3].highDamageCount == 0)
                {
                    notAttackableCount += notAttackableValue * m_currentlyNotAttackableBonus * m_currentlyNotAttackableBonus;
                }
            }
            else
            {
                factor += m_transportBonus;
            }
            if (factor < 0)
            {
                factor = 0;
            }

            damageCount += resDamage * factor;
            if (dmg >= m_midDamage)
            {
                attacksCount += factor;
            }
            else
            {
                extraMalusCount += factor;
            }
        }
        if (counterDmg >= 0.0f)
        {
            if ((counterDmg >= m_notAttackableDamage) ||
                (counterDmg <= m_notAttackableDamage && dmg <= m_notAttackableDamage))
            {
                bool firstStrikes = enemyRange >= ownRange;
                float resDamage = 0;
                resDamage = (counterDmg / (Unit::MAX_UNIT_HP * Unit::MAX_UNIT_HP)) * ownValue;
                if (firstStrikes)
                {
                    resDamage *= bonusFactor;
                }
                float mult = (enemyRange + m_smoothingValue) / (ownRange + m_smoothingValue);
                if (mult > m_maxDistanceMultiplier)
                {
                    mult = m_maxDistanceMultiplier;
                }
                resDamage *= mult;
                if (resDamage > ownValue)
                {
                    resDamage = ownValue;
                }
                counterDamageCount += resDamage;
            }
            ++counterAttacksCount;
        }
    }
    // low on counter units
    if (lowUnitAttackCount < lowUnitDamageCount * 2.0f &&
        lowUnitAttackCount > 0)
    {
        notAttackableCount += m_highDamageBonus * m_currentlyNotAttackableBonus;
    }

    float damage = 0.0f;
    if (attacksCount > 0.0f)
    {
        damage = damageCount / (attacksCount + extraMalusCount);
    }
    if (damage > 0)
    {
        float value = static_cast<float>(baseAttacksCount) / static_cast<float>(enemyUnitCount);
        if (baseAttacksCount > m_minAttackCountBonus)
        {
            damage *= static_cast<float>(baseAttacksCount + m_minAttackCountBonus) / static_cast<float>(enemyUnitCount);
        }
        else
        {
            damage *= value;
        }
        // reduce effectiveness of units who can't attack a lot of units
        if (dummy.getMinRange(position) > 1)
        {
            for (qint32 i = m_indirectUnitAttackCountMalus; i > 1; --i)
            {
                float factor = 1.0f / static_cast<float>(i);
                if (value < factor)
                {
                    notAttackableCount *= factor;
                    damage *= factor;
                    break;
                }
            }
        }
    }
    ret.damage = damage;
    ret.notAttackableCount = notAttackableCount;
    ret.attackCount = attacksCount;
    if (counterAttacksCount > 0)
    {
        ret.counterDamage = counterDamageCount / counterAttacksCount;
    }
    if (turnOneDamageCount > 0)
    {
        ret.turnOneDamage = turnOneDamage / (Unit::MAX_UNIT_HP * turnOneDamageCount);
    }
    return ret;
}

// [AI: INVESTIGATE]
// Asks "against the similarly-priced units this factory could build, how does this unit
// match up on raw damage" -- a rock-paper-scissors check against the *build list*, not
// against anything the enemy actually owns. The underlying question (are we buying a
// unit that loses to what the opponent can field) is worth keeping, but Search AI should
// ask it about the observed enemy army and their production capability, not about a
// hypothetical price bracket.
// Heuristics not to inherit: TargetPriceDifference (0.35) defining "similar cost",
// SameFundsMatchUpMovementMalus (0.3) penalising slower units, NotAttackableDamage as a
// zeroing floor, normalisation by DAMAGE_100/2, and SameFundsMatchUpNoMatchUpValue (0.5)
// returned as a neutral guess when nothing comparable exists.
float NormalAi::calcSameFundsMatchUpScore(Unit &dummy, const QStringList &buildList)
{
    auto dummyValue = dummy.getUnitValue();
    float resultScore = 0;
    auto movepoints = dummy.getMovementpoints(dummy.getPosition());
    float count = 0;
    for (const auto &unitId : std::as_const(buildList))
    {
        Unit dummyMatchUp(unitId, m_pPlayer, false, m_pMap);
        auto matchUpMovepoints = dummyMatchUp.getMovementpoints(dummy.getPosition());
        auto matchUpValue = dummyMatchUp.getUnitValue();

        if (matchUpValue <= dummyValue + dummyValue * m_targetPriceDifference &&
            matchUpValue >= dummyValue - dummyValue * m_targetPriceDifference)
        {
            float dmg = dummy.getBaseDamage(&dummyMatchUp);
            float counterDmg = dummyMatchUp.getBaseDamage(&dummy);
            if (dmg >= 0.0f &&
                counterDmg >= 0.0f)
            {
                if (dmg <= m_notAttackableDamage)
                {
                    dmg = 0.0f;
                }
                else if (matchUpMovepoints > movepoints)
                {
                    dmg *= m_sameFundsMatchUpMovementMalus;
                }
                resultScore += dmg;
                ++count;
            }
        }
    }
    if (count > 0)
    {
        return resultScore / (Unit::DAMAGE_100 / 2) / count;
    }
    else
    {
        return m_sameFundsMatchUpNoMatchUpValue;
    }
}

void NormalAi::getTransporterData(UnitBuildData &unitBuildData, Unit &dummy, spQmlVectorUnit &pUnits,
                                  spQmlVectorUnit &pEnemyUnits, spQmlVectorBuilding &pEnemyBuildings,
                                  std::vector<std::tuple<Unit *, Unit *>> &transportTargets)
{
    std::vector<QVector3D> targets;
    spQmlVectorUnit relevantUnits = MemoryManagement::create<QmlVectorUnit>();
    QPoint position = dummy.getPosition();
    float movement = dummy.getBaseMovementPoints();
    if (movement == 0)
    {
        movement = 1;
    }

    qint32 loadingPlace = dummy.getLoadingPlace();
    qint32 smallTransporterCount = 0;
    qint32 transporterCount = 0;
    // [AI: REPLACE - AI POLICY]
    // maxDayDistance (6) is a hard cut-off: units more than six turns of travel away are
    // not considered as passengers at all. Search AI should let distance discount a
    // transport's value continuously via eta (AI_Design_Notes_Consolidated.md section
    // 6.4), not exclude candidates past a fixed horizon.
    static constexpr float maxDayDistance = 6.0f;
    for (auto &pUnit : pUnits->getVector())
    {
        float distance = GlobalUtils::getDistance(position, pUnit->getPosition());
        if (distance / movement <= maxDayDistance)
        {
            relevantUnits->append(pUnit.get());
        }
        qint32 place = pUnit->getLoadingPlace();
        // [AI: INVESTIGATE]
        // Existing bug, not a heuristic: `place == 1` is nested inside `place > 1`, so it
        // can never be true and smallTransporterCount is always 0. That makes the
        // small-transporter branch in calcTransporterScore() fire on
        // `pUnits->size() / 1 > UnitToSmallTransporterRatio`, i.e. for any army above 5
        // units regardless of how many small transports already exist -- and
        // SmallTransporterBonus is then awarded unconditionally.
        // Do not port this; it means Normal AI's observed transport behaviour is not
        // evidence of what the intended rule would do.
        if (place > 1)
        {
            if (place == 1)
            {
                smallTransporterCount++;
            }
            ++transporterCount;
        }
    }
    bool onlyTrueIslands = loadingPlace > 1;
    std::vector<Unit *> loadingUnits = appendLoadingTargets(&dummy, relevantUnits, pEnemyUnits, pEnemyBuildings, false, true, targets, false, 1, onlyTrueIslands);
    std::vector<Unit *> transporterUnits;
    for (auto target : transportTargets)
    {
        if (!GlobalUtils::contains(transporterUnits, std::get<0>(target)))
        {
            transporterUnits.push_back(std::get<0>(target));
        }
    }
    qint32 i = 0;
    Interpreter *pInterpreter = Interpreter::getInstance();
    while (i < loadingUnits.size())
    {
        pInterpreter->threadProcessEvents();
        if (canTransportToEnemy(&dummy, loadingUnits[i], pEnemyUnits, pEnemyBuildings))
        {
            qint32 transporter = 0;
            for (auto &target : transportTargets)
            {
                if (std::get<1>(target)->getPosition() == loadingUnits[i]->getPosition())
                {
                    transporter++;
                    break;
                }
            }
            if (transporter == 0)
            {
                unitBuildData.noTransporterBonus += m_noTransporterBonus;
            }
            i++;
        }
        else
        {
            loadingUnits.erase(loadingUnits.cbegin() + i);
        }
    }
    unitBuildData.smallTransporterCount = smallTransporterCount;
    unitBuildData.loadingPlace = loadingPlace;
    unitBuildData.transportCount = transporterUnits.size();
    unitBuildData.loadingCount = loadingUnits.size();
    unitBuildData.transporterCount = transporterCount;
    auto actions = dummy.getActionList();
    unitBuildData.utilityTransporter = !dummy.useTerrainDefense() ||
                                       actions.contains(CoreAI::ACTION_SUPPORTALL_RATION);
}

// [AI: REPLACE - AI POLICY]
// Decides whether to build a transport from ratios and flat bonuses:
// UnitToSmallTransporterRatio (5) units per small transport, SmallTransporterBonus (30),
// FlyingTransporterBonus (15) awarded to anything whose score already exceeded
// MinFlyingTransportScoreForBonus, ProducingTransportRatioBonus, AdditionalLoadingUnitBonus
// per loading place, NoTransporterBonus (70) per unloaded passenger, and
// ProducingTransportMinLoadingTransportRatio (4.5) below which the unit is banned outright
// via NO_BUILD_SCORE.
// AI_Design_Notes_Consolidated.md section 6.4 singles out the ratio test at the top of
// this function as the sharpest contrast with the intended design: it is blind to map
// geometry, producing the same transport mix on a cramped four-city map as on a sprawling
// archipelago. The replacement is projectedCaptureIncome -- score a hypothetical transport
// by the delta in capture eta it creates, which reads the map for free and needs no
// per-map tuning.
// (Minor: `score == 0.0f` on the first line is always true, score having just been
// initialised to 0; and smallTransporterCount is always 0 owing to the getTransporterData
// bug marked above, so that branch is effectively unconditional for armies over 5 units.)
float NormalAi::calcTransporterScore(UnitBuildData &unitBuildData, spQmlVectorUnit &pUnits, std::vector<float> &data)
{
    float score = 0.0f;
    if (score == 0.0f && pUnits->size() / (unitBuildData.smallTransporterCount + 1) > m_unitToSmallTransporterRatio && unitBuildData.loadingPlace == 1)
    {

        if (unitBuildData.smallTransporterCount > 0)
        {
            score += qMin(m_smallTransporterBonus, static_cast<float>(pUnits->size()) / static_cast<float>(unitBuildData.smallTransporterCount + 1.0f) * 10.0f);
        }
        else
        {
            score += m_smallTransporterBonus;
        }
        // give a bonus to t-heli's or similar units cause they are mostlikly much faster
        if (score > m_minFlyingTransportScoreForBonus)
        {
            score += m_flyingTransporterBonus;
        }
    }
    if (unitBuildData.transportCount > 0 && unitBuildData.loadingCount > 0)
    {
        score += (static_cast<float>(unitBuildData.loadingCount) / static_cast<float>(unitBuildData.transportCount)) * m_ProducingTransportRatioBonus;
    }
    else
    {
        score += unitBuildData.loadingCount * m_ProducingTransportLoadingBonus;
    }
    float supplyScore = calcSupplyScore(data, unitBuildData);
    score += supplyScore;
    if (unitBuildData.loadingCount > 0)
    {
        if (unitBuildData.transportCount <= 0 ||
            static_cast<float>(unitBuildData.loadingCount) / static_cast<float>(unitBuildData.transportCount + 1) >= m_ProducingTransportMinLoadingTransportRatio)
        {
            if (data[FundsFactoryRatio] <= m_cheapUnitRatio + m_targetPriceDifference)
            {
                score += (1 + m_cheapUnitRatio + m_targetPriceDifference - data[FundsFactoryRatio]) * m_normalUnitBonusMultiplier;
            }
            else
            {
                score -= (1 + m_cheapUnitRatio - m_targetPriceDifference - data[FundsFactoryRatio]) * m_cheapUnitBonusMultiplier;
            }
            score += unitBuildData.loadingPlace * m_additionalLoadingUnitBonus;
            score += unitBuildData.loadingCount * m_additionalLoadingUnitBonus;
            score += unitBuildData.noTransporterBonus;
            score -= unitBuildData.transporterCount * m_additionalLoadingUnitBonus;
            AI_CONSOLE_PRINT("NormalAi::calcTransporterScore for " + unitBuildData.unitId +
                                 " score=" + QString::number(score) +
                                 " loadingCount=" + QString::number(unitBuildData.loadingCount) +
                                 " transportCount=" + QString::number(unitBuildData.transportCount) +
                                 " transportCounter=" + QString::number(unitBuildData.transporterCount),
                             GameConsole::eDEBUG);
        }
        else if (supplyScore <= 0.0f)
        {
            score = NO_BUILD_SCORE;
        }
    }
    else if (supplyScore <= 0.0f)
    {
        score = NO_BUILD_SCORE;
    }
    return score;
}

// [AI: REPLACE - AI POLICY]
// Normal AI's production evaluation function: a weighted sum of ~10 heuristic terms,
// most of them army-composition ratios rather than statements about the board.
//   - direct/indirect balance pushed toward DirectIndirectRatio (5) with four
//     asymmetric bonus/malus constants,
//   - an infantry bonus driven by MinInfantryCount, LowInfantryRatio and
//     LowOwnBuildingEnemyBuildingRatio,
//   - MovementpointBonus (6) per move point, flat,
//   - CurrentlyNotAttackableScoreBonus, DamageToUnitCostRatioBonus, CounterDamageBonus,
//     AttackCountBonus, SameFundsMatchUpBonus, CoUnitBuffBonus, TurnOneDmageMalus,
//     NearEnemyBonus / distance,
//   - a hard NO_BUILD_SCORE veto for any unit with no attack targets, and
//   - a final multiply by BaseGameInputIF::getUnitBuildValue (the per-map/per-mod
//     designer weight -- worth honouring as a constraint, but it is a preference,
//     not board state).
// The structure is right (weighted linear sum, section 4) and the AI_CONSOLE_PRINT
// breakdown is exactly the debugging affordance section 4 suggestion 3 asks for.
// What has to change is the terms: they are unitless ratios tuned against each other,
// so they cannot be compared with, or traded off against, the funds-denominated terms
// the movement evaluator will produce. Search AI should re-derive these as
// funds-equivalent contributions to a hypothesised board.
float NormalAi::calcBuildScore(std::vector<float> &data, UnitBuildData &unitBuildData)
{
    float score = 0;
    // used index 0, 1, 2, 3, 4, 5, 6, 7, 9, 10, 11
    if (data[InfantryUnit] == 0.0f)
    {
        if (data[IndirectUnit] == 1.0f)
        {
            // indirect unit
            if (data[DirectUnitRatio] > m_currentDirectIndirectRatio)
            {
                score += m_lowIndirectUnitBonus * (data[DirectUnitRatio] - m_currentDirectIndirectRatio);
            }
            else if (data[DirectUnitRatio] < m_currentDirectIndirectRatio / 2)
            {
                score -= m_highIndirectMalus * (m_currentDirectIndirectRatio - data[DirectUnitRatio]);
            }
            else if (data[DirectUnitRatio] < m_directIndirectRatio)
            {
                score -= m_lowIndirectMalus * (m_currentDirectIndirectRatio - data[DirectUnitRatio]);
            }
        }
        else if (data[DirectUnit] == 1.0f)
        {
            // direct unit
            if (data[DirectUnitRatio] < m_currentDirectIndirectRatio)
            {
                score += m_lowDirectUnitBonus * (m_currentDirectIndirectRatio - data[DirectUnitRatio]);
            }
            else if (data[DirectUnitRatio] > m_currentDirectIndirectRatio * 2)
            {
                score -= m_highDirectMalus * (data[DirectUnitRatio] - m_currentDirectIndirectRatio);
            }
            else if (data[DirectUnitRatio] > m_currentDirectIndirectRatio)
            {
                score -= m_lowDirectMalus * (data[DirectUnitRatio] - m_currentDirectIndirectRatio);
            }
        }
    }
    // infantry bonus
    if (data[InfantryUnit] == 1.0f)
    {
        float infScore = 0.0f;
        if (data[InfantryCount] <= m_minInfantryCount && data[BuildingEnemyRatio] < m_lowOwnBuildingEnemyBuildingRatio)
        {
            infScore += (m_minInfantryCount - data[InfantryCount]) * m_minInfantryCount + (m_lowOwnBuildingEnemyBuildingRatio - data[BuildingEnemyRatio]) * m_lowIncomeInfantryBonusMultiplier;
        }
        else if (data[InfantryUnitRatio] < m_lowInfantryRatio)
        {
            infScore += (m_lowOwnBuildingEnemyBuildingRatio - data[BuildingEnemyRatio]) * m_buildingBonusMultiplier;
        }
        else
        {
            infScore += (m_lowOwnBuildingEnemyBuildingRatio - data[BuildingEnemyRatio]) * m_buildingBonusMultiplier;
        }
        if (infScore > 0.0f)
        {
            score += infScore;
        }
        else if (unitBuildData.unitId != UNIT_INFANTRY)
        {
            score += infScore;
        }
    }
    score += calcCostScore(data, unitBuildData);
    // apply movement bonus
    score += data[Movementpoints] * m_movementpointBonus;
    if (data[UnitCount] > m_minUnitCountForDamageBonus)
    {
        // apply not attackable unit bonus
        score += data[NotAttackableCount] * m_currentlyNotAttackableScoreBonus;
    }
    if (data[UnitCost] > 0 &&
        (data[UnitCount] > m_minUnitCountForDamageBonus ||
         m_pMap->getCurrentDay() > m_earlyGame))
    {
        float attackScore = 0.0f;
        float attackCountScore = 0.0f;
        attackScore += data[DamageData] / data[UnitCost] * m_damageToUnitCostRatioBonus;
        attackScore += (1.0f - data[CounterDamage] / data[UnitCost]) * m_counterDamageBonus;
        if (data[EnemyUnitCount] > 0)
        {
            attackCountScore += data[AttackCount] / data[EnemyUnitCount] * m_attackCountBonus;
        }
        float sameFoundsScore = data[SameFundsMatchUpScore] * m_sameFundsMatchUpBonus;
        score += attackScore + attackCountScore + sameFoundsScore;
        AI_CONSOLE_PRINT("NormalAi::calcBuildScore damage=" + QString::number(data[DamageData]) +
                             " and counter damage " + QString::number(data[CounterDamage]) +
                             " attack score=" + QString::number(attackScore) +
                             " attack count score=" + QString::number(attackCountScore) +
                             " same founds score=" + QString::number(sameFoundsScore),
                         GameConsole::eDEBUG);
    }
    // apply co buff bonus
    score += data[COBonus] * m_coUnitBuffBonus;
    score += calcSupplyScore(data, unitBuildData);
    score -= data[TurnOneDamageMalus] * m_turnOneDmageMalus;
    if (data[ReachDistance] > 0 && data[Movementpoints] > 0)
    {
        score += m_nearEnemyBonus * data[Movementpoints] / GlobalUtils::roundUp(data[ReachDistance]);
    }
    if (data[UnitCount] > m_minUnitCountForDamageBonus &&
        data[AttackCount] <= 0 && data[Movementpoints] > 0 &&
        !unitBuildData.isTransporter)
    {
        score = NO_BUILD_SCORE;
    }
    score *= BaseGameInputIF::getUnitBuildValue(unitBuildData.unitId);
    AI_CONSOLE_PRINT("NormalAi::calcBuildScore final score=" + QString::number(score) + " for " + unitBuildData.unitId, GameConsole::eDEBUG);
    return score;
}

// [AI: INVESTIGATE]
// Supply-unit production from two ratios: build one if supply units are under
// MaxSupplyUnitRatio (5%) of the army, valued at CanSupplyBonus (10) per unit needing
// supply beyond what existing supply units cover, where each is assumed to serve
// AverageSupplySupport (8) units.
// The "8 units per APC" figure is a guess standing in for a reachability question --
// whether a supply unit can actually get to the units that need it, given the map.
// Search AI should answer that with the pathfinder, as it does for capture eta.
float NormalAi::calcSupplyScore(std::vector<float> &data, UnitBuildData &unitBuildData)
{
    float score = 0.0f;
    if (unitBuildData.isSupplyUnit && data[SupplyRatio] <= m_maxSupplyUnitRatio)
    {
        qint32 supplyUnits = data[SupplyRatio] * data[UnitCount];
        qint32 supplyNeeds = data[RequiredSupplyRatio] * data[UnitCount] - supplyUnits * m_averageSupplySupport;
        score += m_canSupplyBonus * supplyNeeds;
    }
    return score;
}

// [AI: REPLACE - AI POLICY]
// Scores a unit purely on how close its cost sits to the price bracket chosen by the
// dice roll in buildUnits() -- a five-branch ladder over CheapUnitRatio (1.8),
// NormalUnitRatio (1.0), SuperiorityRatio (1.8) and TargetPriceDifference (0.35), with
// separate multipliers per bracket and two bare literals (outScore 0.25, inScore 0.5).
// Cost is a real input to production, but this asks "does this unit cost about what I
// decided to spend", never "is this unit worth its price on this board". A unit that
// wins the game is penalised for being 40% off the target bracket.
// Search AI should treat cost as a funds term in the same currency as everything else:
// value produced minus funds spent, with the opportunity cost of not banking the money
// for next turn -- no brackets.
float NormalAi::calcCostScore(std::vector<float> &data, UnitBuildData &unitBuildData)
{
    float score = 0;
    // funds bonus
    double normalDifference = data[FundsFactoryRatio] - m_normalUnitRatio;
    double cheapDifference = data[FundsFactoryRatio] - m_cheapUnitRatio;
    const double outScore = 0.25f;
    const double inScore = 0.5f;

    if (data[UseHighTechUnits] == static_cast<float>(NoSpendMalus) &&
        data[FundsFactoryRatio] > m_normalUnitRatio + m_targetPriceDifference)
    {
        // spend what we can mode
        score = 0;
    }
    else if (data[UseHighTechUnits] > static_cast<float>(FundsMode::Normal) &&
             normalDifference > m_targetPriceDifference)
    {
        // expensive malus
        score = m_normalUnitBonusMultiplier * outScore - m_normalUnitBonusMultiplier * (qAbs(normalDifference) - m_targetPriceDifference);
    }
    else if (data[FundsFactoryRatio] > m_superiorityRatio)
    {
        score = m_normalUnitBonusMultiplier * outScore - m_expensiveUnitBonusMultiplier * (qAbs(normalDifference) - m_targetPriceDifference);
    }
    else if (qAbs(normalDifference) <= m_targetPriceDifference)
    {
        score = m_normalUnitBonusMultiplier * (1.0 - qAbs(normalDifference) / m_targetPriceDifference * inScore);
    }
    else if (qAbs(cheapDifference) <= m_targetPriceDifference &&
             data[UseHighTechUnits] <= static_cast<float>(FundsMode::Expensive))
    {
        if (data[LowFunds] > 0)
        {
            score = m_cheapUnitBonusMultiplier * (1.0 - qAbs(cheapDifference) / m_targetPriceDifference * inScore);
        }
        else
        {
            score = m_normalUnitBonusMultiplier * outScore - m_cheapUnitBonusMultiplier * (qAbs(normalDifference) - m_targetPriceDifference);
        }
    }
    else
    {
        score = m_normalUnitBonusMultiplier * outScore - m_normalUnitBonusMultiplier * (qAbs(normalDifference) - m_targetPriceDifference);
    }
    AI_CONSOLE_PRINT("NormalAi::calcCostScore score=" + QString::number(score) +
                         " funds ratio=" + QString::number(data[FundsFactoryRatio]),
                     GameConsole::eDEBUG);
    return score;
}
