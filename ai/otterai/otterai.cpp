#include "ai/otterai/otterai.h"


OtterAi::OtterAi(GameMap* pMap, QString type, GameEnums::AiTypes aiType)
    : CoreAI(pMap, aiType, type)
{
#ifdef GRAPHICSUPPORT
    setObjectName("OtterAi");
#endif
    Interpreter::setCppOwnerShip(this);
    setupJsThis(this);

    CONSOLE_PRINT("Creating otter ai", GameConsole::eDEBUG);
}

void OtterAi::process() 
{
    spQmlVectorBuilding spBuildings = m_pPlayer->getSpBuildings();
    spQmlVectorUnit spUnits = m_pPlayer->getSpUnits();
    spQmlVectorUnit spEnemyUnits = m_pPlayer->getSpEnemyUnits();
    spQmlVectorBuilding spEnemyBuildings = m_pPlayer->getSpEnemyBuildings();

    if(performActionSteps(spUnits, spEnemyUnits, spBuildings, spEnemyBuildings)) {}
    else 
    {
        CoreAI::finishTurn();
    }
}

bool OtterAi::performActionSteps(spQmlVectorUnit & pUnits, spQmlVectorUnit & pEnemyUnits,
                                spQmlVectorBuilding & pBuildings, spQmlVectorBuilding & pEnemyBuildings)
{
    return buildUnits(pBuildings, pUnits, pEnemyUnits, pEnemyBuildings);
}


bool OtterAi::buildUnits(spQmlVectorBuilding & pBuildings, spQmlVectorUnit & pUnits,
                        spQmlVectorUnit & pEnemyUnits, spQmlVectorBuilding & pEnemyBuildings)
{
    if (m_aiStep < AISteps::buildUnits)
    {
        m_productionSystem.onNewBuildQueue(pBuildings.get(), pUnits.get(), pEnemyUnits, pEnemyBuildings.get());
    }
    m_aiStep = AISteps::buildUnits;
    bool executed = false;

    if (m_productionSystem.buildUnit(pBuildings.get(), pUnits.get(), pEnemyUnits.get(), pEnemyBuildings.get(), executed))
    {
        return executed;
    }

    return executed;
}
