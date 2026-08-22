#pragma once 

#include <QObject>

#include "ai/coreai.h"

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
        bool buildUnits(spQmlVectorBuilding & pBuildings, spQmlVectorUnit & pUnits,
                        spQmlVectorUnit & pEnemyUnits, spQmlVectorBuilding & pEnemyBuildings);

    protected:
            bool performActionSteps(spQmlVectorUnit & pUnits, spQmlVectorUnit & pEnemyUnits,
                                    spQmlVectorBuilding & pBuildings, spQmlVectorBuilding & pEnemyBuildings);

    public slots:
        virtual void process() override;
};

Q_DECLARE_INTERFACE(OtterAi, "OtterAi");