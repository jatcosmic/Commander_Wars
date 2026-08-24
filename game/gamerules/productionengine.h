#pragma once

#include <vector>

#include <QPoint>
#include <QString>
#include <QtGlobal>

class CoreAI;
class QmlVectorBuilding;

using UnitId = QString;

/**
 * @brief Legality layer for unit production.
 *
 * Answers which units a player is allowed to produce and issues the order. It holds no
 * opinion on which of them are worth producing; that judgement belongs to the evaluator.
 */
namespace ProductionEngine
{
    /**
     * @brief A single legal production: unitId can be built at position for cost.
     */
    struct BuildOption
    {
        QPoint position;
        QString unitId;
        qint32 cost;
    };

    /**
     * @brief Orders unitId at position.
     * @return false if the build wasn't legal, in which case no action was emitted.
     */
    bool buildUnit(CoreAI & ai, const QPoint & position, const QString & unitId);

    
    /**
     * @brief Orders BuildOption.
     * @return false if the build wasn't legal, in which case no action was emitted.
     */
    bool buildUnit(CoreAI & ai, BuildOption& option);

    /**
     * @brief Every legal (building, unit) production currently available to ai's player.
     *
     * Affordability and the unit limit are part of legality, so every successful build
     * invalidates the result and it has to be recomputed rather than cached.
     * 
     * The returned vector has exactly one entry for every building in pBuildings.
     * The index of each entry corresponds to the index of the building in pBuildings.
     * An empty map means that building has no legal build options.
     */
    // std::vector<BuildOption> getLegalBuilds(CoreAI & ai, QmlVectorBuilding * pBuildings);
    std::vector<std::unordered_map<UnitId, BuildOption>> getLegalBuilds(CoreAI & ai, const QmlVectorBuilding * pBuildings);

}
