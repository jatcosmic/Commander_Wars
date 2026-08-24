#include "game/gamerules/productionengine.h"

#include "ai/coreai.h"
#include "coreengine/gameconsole.h"
#include "coreengine/memorymanagement.h"
#include "coreengine/qmlvector.h"
#include "game/building.h"
#include "game/gameaction.h"
#include "game/gamemap.h"
#include "game/terrain.h"
#include "gameinput/menudata.h"

namespace ProductionEngine
{

namespace
{
    /**
     * @brief A build action targeted at a building, paired with the menu data listing
     *        what that building can produce right now.
     *
     * pAction is null when the building can't produce anything. Both handles are filled
     * by a single pass of script calls, so a caller interested in several units from the
     * same building opens it once and reads the menu data repeatedly.
     */
    struct OpenBuild
    {
        spGameAction pAction;
        spMenuData pData;
    };

    /**
     * @brief Opens the build action of the building at position.
     * @return An OpenBuild whose pAction is null unless the building exists, is free and
     *         has something to offer.
     */
    OpenBuild openBuildAction(CoreAI & ai, const QPoint & position)
    {
        Terrain* pTerrain = ai.getMap()->getTerrain(position);
        if (pTerrain == nullptr ||
            pTerrain->getUnit() != nullptr)
        {
            return OpenBuild();
        }
        Building* pBuilding = pTerrain->getBuilding();
        if (pBuilding == nullptr ||
            !pBuilding->getActionList().contains(CoreAI::ACTION_BUILD_UNITS))
        {
            return OpenBuild();
        }
        spGameAction pAction = MemoryManagement::create<GameAction>(CoreAI::ACTION_BUILD_UNITS, ai.getMap());
        pAction->setTarget(position);
        // Covers the construction list, the unit limit and whether the player can afford
        // anything at all. Nothing in it depends on the menu selection, so it never has
        // to be repeated once a unit has been picked.
        if (!pAction->canBePerformed())
        {
            return OpenBuild();
        }
        spMenuData pData = pAction->getMenuStepData();
        if (!pData->validData())
        {
            return OpenBuild();
        }
        return OpenBuild{pAction, pData};
    }

    /**
     * @brief Selects unitId on an open build, turning it into a performable action.
     * @return false if the building doesn't offer unitId or the player can't afford it.
     */
    bool selectUnit(CoreAI & ai, OpenBuild & open, const QString & unitId)
    {
        auto indexOf = open.pData->getActionIDs().indexOf(unitId);
        if (indexOf < 0 ||
            !open.pData->getEnabledList()[indexOf])
        {
            return false;
        }
        // Writes the selection into the action and advances it past its only input step.
        // ACTION_BUILD_UNITS asks for nothing further, so the action is now final.
        ai.addMenuItemData(open.pAction, unitId, open.pData->getCostList()[indexOf]);
        return true;
    }
}

    bool buildUnit(CoreAI & ai, const QPoint & position, const QString & unitId)
    {
        OpenBuild open = openBuildAction(ai, position);
        if (open.pAction.get() == nullptr ||
            !selectUnit(ai, open, unitId))
        {
            return false;
        }
        CONSOLE_PRINT("Building unit " + unitId +
                    " at x=" + QString::number(position.x()) +
                    " y=" + QString::number(position.y()), GameConsole::eDEBUG);
        emit ai.sigPerformAction(open.pAction);
        return true;
    }

    bool buildUnit(CoreAI & ai, BuildOption& option) 
    {
        return buildUnit(ai, option.position, option.unitId);
    }

    std::vector<BuildOption> getLegalBuilds(CoreAI & ai, QmlVectorBuilding * pBuildings)
    {
        std::vector<BuildOption> options;
        if (pBuildings == nullptr)
        {
            return options;
        }
        for (auto & pBuilding : pBuildings->getVector())
        {
            // The script calls are paid once per building. The unit loop below only indexes
            // vectors, so the cost scales with buildings rather than with (building, unit)
            // pairs.
            OpenBuild open = openBuildAction(ai, pBuilding->getPosition());
            if (open.pAction.get() == nullptr)
            {
                continue;
            }
            const QStringList unitIds = open.pData->getActionIDs();
            const QVector<qint32> costs = open.pData->getCostList();
            const QVector<bool> enabled = open.pData->getEnabledList();
            for (qint32 i = 0; i < unitIds.size(); ++i)
            {
                if (enabled[i])
                {
                    options.push_back(BuildOption{
                        .position = pBuilding->getPosition(), 
                        .unitId = unitIds[i], 
                        .cost = costs[i]
                    });
                }
            }
        }
        return options;
    }

}
