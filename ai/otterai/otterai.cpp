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

    // TODO:
    // Rewrite Finite-State Machine to make use of States Pattern:
    // https://refactoring.guru/design-patterns/state
    

    // process() is re-entered once per completed action: performActionSteps emits a
    // single action and returns true, the engine performs it and calls us back. When
    // nothing is left to do the turn ends.
    if (!performActionSteps(spUnits, spEnemyUnits, spBuildings, spEnemyBuildings))
    {
        CoreAI::finishTurn();
    }
}

bool OtterAi::performActionSteps(spQmlVectorUnit & pUnits, spQmlVectorUnit & pEnemyUnits,
                                spQmlVectorBuilding & pBuildings, spQmlVectorBuilding & pEnemyBuildings)
{
    return buildUnits(pBuildings);
}

bool OtterAi::buildUnits(spQmlVectorBuilding & pBuildings)
{
    // The caller-side "m_aiStep <= buildUnits" guard belongs to the step ladder; a step
    // function only records that it has been reached. Once a second step exists, testing
    // "m_aiStep < buildUnits" here is the hook for per-turn build setup.
    m_aiStep = AISteps::buildUnits;

    auto legalBuilds = ProductionEngine::getLegalBuilds(*this, pBuildings.get());
    Q_ASSERT(legalBuilds.size() == static_cast<size_t>(pBuildings->size()));
    
    if(legalBuilds.empty())
    {
        return false;
    }


    for(int i = 0; i < pBuildings->size(); i++)
    {
        auto it = legalBuilds[i].find("INFANTRY");

        if (it == legalBuilds[i].end())
        {
            continue;
        }

        // buildUnit already returns false for anything that can't produce an INFANTRY,
        // so a building we can't use is skipped rather than ending the search.
        if (ProductionEngine::buildUnit(*this, it->second))
        {
            // Only one action may be in flight: ActionPerformer drops any further
            // emit while m_actionRunning is set. Return and let process() run again.
            return true;
        }
    }

    return false;
}
