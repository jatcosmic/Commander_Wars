#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <vector>
#include <map>
#include "coreengine/fileserializable.h"
#include "coreengine/scriptvariables.h"
#include "coreengine/qmlvector.h"
#include "coreengine/jsthis.h"
#include "game/unit.h"

class Building;
class CoreAI;

class SimpleProductionSystem final : public QObject, public FileSerializable, public JsThis
{
    Q_OBJECT
public:
    struct InitialProduction
    {
        QStringList unitIds;
        qint32 count{0};
    };
    struct ForcedProduction
    {
        qint32 x{-1};
        qint32 y{-1};
        QStringList unitIds;
        spQmlVectorUnit targets;
    };
    struct BuildDistribution
    {
        QStringList unitIds;
        QVector<qint32> chance;
        QVector<spUnit> units;
        qint32 totalChance;
        qreal distribution;
        qreal maxUnitDistribution;
        qint32 buildMode;
        QString guardCondition;
    };
    struct CurrentBuildDistribution
    {
        qreal currentValue;
        BuildDistribution distribution;
    };
    struct AverageBuildData
    {
        qreal averageValue{0};
        std::map<QString, qint32> islandSizes;
    };

    /**
     * @brief SimpleProductionSystem creates the production system of the given AI.
     */
    explicit SimpleProductionSystem(CoreAI * owner);
    virtual ~SimpleProductionSystem() = default;

    /**
     * @brief serialize stores the object
     * @param pStream
     */
    virtual void serializeObject(QDataStream& pStream) const override;
    /**
     * @brief deserialize restores the object
     * @param pStream
     */
    virtual void deserializeObject(QDataStream& pStream) override;
    /**
     * @brief getVersion version of the file
     * @return
     */
    virtual qint32 getVersion() const override
    {
        return 1;
    }
    /**
     * @brief initialize sets up the production rules by running the AI script's setup hook on the
     * first call. Later calls only restore the sample units the rules need, e.g. after loading a save.
     */
    void initialize();
    /**
     * @brief buildUnit hands the current build decision to the AI script.
     * @param executed [out] true if a unit was actually ordered
     * @return true if this system handled the decision, false if the caller has to fall back to its
     * own production logic
     */
    bool buildUnit(QmlVectorBuilding* pBuildings, QmlVectorUnit* pUnits, QmlVectorUnit * pEnemyUnits, QmlVectorBuilding * pEnemyBuildings, bool & executed);
    /**
     * @brief onNewBuildQueue starts a new production phase: notifies the AI script and refreshes the
     * cached enemy units, unit groups and per building island data. Call once before building units.
     */
    void onNewBuildQueue(QmlVectorBuilding* pBuildings, QmlVectorUnit* pUnits, spQmlVectorUnit &pEnemyUnits, QmlVectorBuilding * pEnemyBuildings);
    /**
     * @brief getInit
     * @return true once the AI script has set up the production rules
     */
    Q_INVOKABLE bool getInit() const;
    /**
     * @brief getEnabled
     * @return true while this system handles production for the AI
     */
    Q_INVOKABLE bool getEnabled() const;
    /**
     * @brief setEnabled turns the system on or off. While off buildUnit leaves production to the caller.
     */
    Q_INVOKABLE void setEnabled(bool newEnabled);
    /**
     * @brief getVariables
     * @return the script visible variables of this system, stored in the save game
     */
    Q_INVOKABLE inline ScriptVariables* getVariables()
    {
        return &m_Variables;
    }
    /**
     * @brief resetBuildDistribution drops all unit groups.
     */
    Q_INVOKABLE void resetBuildDistribution();
    /**
     * @brief resetForcedProduction drops all pending forced builds.
     */
    Q_INVOKABLE void resetForcedProduction();
    /**
     * @brief resetInitialProduction drops all pending initial builds.
     */
    Q_INVOKABLE void resetInitialProduction();
    /**
     * @brief buildNextUnit orders at most one unit, preferring pending initial production, then
     * forced production, then the unit group furthest below its target share of the army.
     * @param minBuildMode,maxBuildMode only groups within this build mode range are considered
     * @param minAverageIslandSize how much of a building's average reachable area a unit has to be
     * able to reach to be built there
     * @param minBaseCost,maxBaseCost cost range of eligible units, a negative maxBaseCost means the
     * player's current funds
     * @param alwaysBuild skips the reasonableBuildField safety check
     * @return true if a unit was ordered
     */
    Q_INVOKABLE bool buildNextUnit(QmlVectorBuilding* pBuildings, QmlVectorUnit* pUnits, qint32 minBuildMode, qint32 maxBuildMode,
                                   qreal minAverageIslandSize = 0.025, qint32 minBaseCost = 0, qint32 maxBaseCost = -1, bool alwaysBuild = false);
    /**
     * @brief addInitialProduction queues units to be built before anything else, e.g. an opening build.
     * @param unitIds candidates, the first one that can be built is used
     * @param count how often this entry is repeated before it is dropped
     */
    Q_INVOKABLE void addInitialProduction(const QStringList & unitIds, qint32 count);
    /**
     * @brief addForcedProduction queues a one time build that takes priority over the normal
     * distribution and is dropped once it succeeds.
     * @param unitId candidates, the first one that can be built is used
     * @param x,y optional building to build at, any suitable building is used if it isn't ours
     */
    Q_INVOKABLE void addForcedProduction(const QStringList & unitId, qint32 x = -1, qint32 y = -1);
    /**
     * @brief addForcedProductionCloseToTargets queues a one time build like addForcedProduction, but
     * places it at the production building closest to the given targets.
     */
    Q_INVOKABLE void addForcedProductionCloseToTargets(const QStringList & unitIds, QmlVectorUnit* targets);
    /**
     * @brief addItemToBuildDistribution declares a named unit group and the share of the army it
     * should make up. Calling it again for the same group merges the new units and weights into it.
     * @param unitIds the units of the group
     * @param chance their relative weights, has to have the same length as unitIds
     * @param distribution the share of the army this group aims for
     * @param buildMode gate deciding when buildNextUnit considers this group
     * @param guardCondition optional script function that has to return true for the group to be used
     * @param maxUnitDistribution share of the army above which the group is no longer built
     */
    Q_INVOKABLE void addItemToBuildDistribution(const QString & group, const QStringList & unitIds, const QVector<qint32> & chance, qreal distribution, qint32 buildMode, const QString & guardCondition = "", qreal maxUnitDistribution = 1.0);
    /**
     * @brief getDummyUnit creates a dummy unit to calculate values not only one dummy unit will be alive at all time.
     * @param unitId
     * @return
     */
    Q_INVOKABLE Unit* getDummyUnit(const QString & unitId);
    /**
     * @brief getProductionFromList picks which of the offered units fits the current distribution best.
     * @param enableList optional mask marking which entries of unitIds are currently selectable
     * @return index into unitIds, or -1 if none of them fits
     */
    Q_INVOKABLE qint32 getProductionFromList(const QStringList & unitIds, QmlVectorUnit* pUnits, QmlVectorBuilding* pBuildings, qint32 minBuildMode, qint32 maxBuildMode, const QVector<bool> & enableList = QVector<bool>());
    /**
     * @brief updateIslandSizeForBuildings recalculates for every production building how much of the
     * map each of its buildable units can reach. Has to be rerun when buildings change owner.
     */
    Q_INVOKABLE void updateIslandSizeForBuildings(QmlVectorBuilding* pBuildings);
    /**
     * @brief getCurrentTurnProducedUnitsCounter
     * @return how many units were built since the counter was last reset
     */
    Q_INVOKABLE qint32 getCurrentTurnProducedUnitsCounter() const;
    /**
     * @brief setCurrentTurnProducedUnitsCounter overrides the produced unit counter, usually to reset
     * it at the start of a turn.
     */
    Q_INVOKABLE void setCurrentTurnProducedUnitsCounter(qint32 newCurrentTurnProducedUnitsCounter);

    /**
     * @brief getMaxDamageCheckRange
     * @return how far around a build field reasonableBuildField looks for threatening enemies
     */
    Q_INVOKABLE qint32 getMaxDamageCheckRange() const;
    /**
     * @brief setMaxDamageCheckRange sets how far around a build field enemies are checked.
     */
    Q_INVOKABLE void setMaxDamageCheckRange(qint32 newMaxDamageCheckRange);

    /**
     * @brief getMaxSingleDamage
     * @return the base damage from which reasonableBuildField treats an enemy as a threat
     */
    Q_INVOKABLE qint32 getMaxSingleDamage() const;
    /**
     * @brief setMaxSingleDamage sets the base damage from which an enemy counts as a threat.
     */
    Q_INVOKABLE void setMaxSingleDamage(qint32 newMaxSingleDamage);
    /**
     * @brief reasonableBuildField Returns true if the AI can build the unit assuming it satisfies certain heuristics 
     * like not taking a certain amount of damage by nearby units, etc.
     * 
     * [AI: Heuristic/Evaluator function - Use with Care]
     * This function applies a mini-evaluation that would interfere with AI. 
     * Its return can influence whether or not a unit is built. 
     */
    Q_INVOKABLE bool reasonableBuildField(qint32 x, qint32 y, QString unitId, qint32 maxDamageCheckRange, qint32 maxSingleDamage);
private:
    bool buildUnit(QmlVectorBuilding* pBuildings, QString unitId, qreal minAverageIslandSize, bool alwaysBuild);
    bool buildUnitCloseTo(QmlVectorBuilding* pBuildings, QString unitId, qreal minAverageIslandSize, const spQmlVectorUnit & pUnits, bool alwaysBuild);
    bool buildUnit(qint32 x, qint32 y, QString unitId, bool alwaysBuild);
    void getBuildDistribution(std::vector<CurrentBuildDistribution> & buildDistribution, QmlVectorUnit* pUnits,
                              qint32 minBuildMode, qint32 maxBuildMode, qint32 minBaseCost, qint32 maxBaseCost);
    void updateActiveProductionSystem(QmlVectorBuilding* pBuildings);
private:
    CoreAI * m_owner{nullptr};
    bool m_init{false};
    bool m_enabled{true};
    spQmlVectorUnit m_pEnemyUnits;
    qint32 m_maxDamageCheckRange{10};
    qint32 m_maxSingleDamage{70};
    std::vector<InitialProduction> m_initialProduction;
    std::vector<ForcedProduction> m_forcedProduction;
    std::map<QString, BuildDistribution> m_buildDistribution;
    std::map<QString, BuildDistribution> m_activeBuildDistribution;
    std::map<Building*, AverageBuildData> m_averageMoverange;
    ScriptVariables m_Variables;
    spUnit m_dummy;
    qint32 m_currentTurnProducedUnitsCounter{0};
};

Q_DECLARE_INTERFACE(SimpleProductionSystem, "SimpleProductionSystem");
