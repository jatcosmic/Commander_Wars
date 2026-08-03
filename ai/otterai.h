#ifndef OTTERAI_H
#define OTTERAI_H

#include <QObject>

#include "ai/coreai.h"
#include "ai/capturebuildingselector.h"
#include "ai/transporterselector.h"

#include "ai/influencefrontmap.h"

class QmlVectorUnit;
class QmlVectorBuilding;
class QmlVectorPoint;
class Building;

class OtterAi;
using spOtterAi = std::shared_ptr<OtterAi>;

class OtterAi final : public CoreAI 
{
    Q_OBJECT

    public slots:
        virtual void process() override;
};

Q_DECLARE_INTERFACE(OtterAi, "OtterAi");

#endif // OTTERAI_H