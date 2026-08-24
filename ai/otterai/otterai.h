#pragma once 

#include <QObject>

#include "ai/coreai.h"
#include "game/gamerules/productionengine.h"

class QmlVectorUnit;
class QmlVectorBuilding;
class QmlVectorPoint;

class OtterAi;
using spOtterAi = std::shared_ptr<OtterAi>;

class OtterAi final : public CoreAI
{
    Q_OBJECT

    public:
        explicit OtterAi(GameMap* pMap, QString type, GameEnums::AiTypes aiType);
        virtual ~OtterAi() = default;
        virtual void onGameStart() override;
        bool buildUnits(spQmlVectorBuilding & pBuildings);

    protected:
            bool performActionSteps(spQmlVectorUnit & pUnits, spQmlVectorUnit & pEnemyUnits,
                                    spQmlVectorBuilding & pBuildings, spQmlVectorBuilding & pEnemyBuildings);

    public slots:
        virtual void process() override;
};

Q_DECLARE_INTERFACE(OtterAi, "OtterAi");