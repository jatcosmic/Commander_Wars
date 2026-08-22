#ifndef TARGETEDUNITPATHFINDINGSYSTEM_H
#define TARGETEDUNITPATHFINDINGSYSTEM_H

#include "game/unitpathfindingsystem.h"

class TargetedUnitPathFindingSystem;
using spTargetedUnitPathFindingSystem = std::shared_ptr<TargetedUnitPathFindingSystem>;

class TargetedUnitPathFindingSystem final : public UnitPathFindingSystem
{
    Q_OBJECT
    struct FinishNodeInfo
    {
        qint32 x{-1};
        qint32 y{-1};
        qint32 movementCost{-1};
        float multiplier{1.0f};
    };

public:
    /**
     * @brief TargetedUnitPathFindingSystem
     * 
     * [AI: Search]
     * 
     * This constructor only keeps the first 20 targets by default. It truncates the rest.
     * Pass in maxTargets value if more than 20 are needed.
     * 
     */
    explicit TargetedUnitPathFindingSystem(GameMap* pMap, Unit* pUnit, std::vector<QVector3D>& targets, std::vector<std::vector<std::tuple<qint32, bool>>>* pMoveCostMap, qint32 maxTargets = 20);
    virtual ~TargetedUnitPathFindingSystem() = default;
    /**
     * @brief getRemainingCost
     * 
     * [AI: Replace - AI Policy]
     * Heuristic used to estimate remaining cost is specific to the weights assigned to 
     * different targets. Targets found from CoreAI::appendCaptureTargets receive no penalty
     * to their heuristic value. Other targets appended other CoreAI::append*Targets do receive
     * a penalty
     * 
     * @param x
     * @param y
     * @return the remaining costs for this node to reach the target
     */
    virtual qint32 getRemainingCost(qint32 x, qint32 y, qint32) override;
    /**
     * @brief finished records (x, y) in m_FinishNodes when it matches one of m_Targets, and reports
     * whether exploration can stop.
     *
     * [AI: Search]
     *
     * The stopping rule is not sound and should not be carried over. It stops once a popped node
     * exceeds bestCost + remainingCost, an additive margin, but setFinishNode() then picks the winner
     * by movementCost * multiplier, a multiplicative score. The cutoff therefore does not bound the
     * quantity being minimised, and a better-scoring target can be cut off before it is ever popped -
     * a target never popped never enters m_FinishNodes and cannot be selected. Two related problems:
     * remainingCost is m_Targets[index].z() - m_Targets[0].z(), which goes negative whenever a high-z
     * target sorts first (the constructor sorts by distance * z, not by z), and getRemainingCost() is
     * inadmissible and inconsistent for z > 1, so pops are not in ascending cost order to begin with.
     *
     * A replacement should keep priority out of the search itself: explore with a plain admissible
     * distance heuristic, then rank the targets it reached by evaluated board value. If a cutoff is
     * needed for speed, it must bound the same quantity that decides the winner.
     *
     * @param x
     * @param y
     * @param movementCosts the popped node's totalCost (g + h), despite the parameter name
     * @return true when exploration should stop
     */
    virtual bool finished(qint32 x, qint32 y, qint32 movementCosts)  override;
    /**
     * @brief setFinishNode
     */
    virtual void setFinishNode(qint32, qint32) override;
    /**
     * @brief getTargetPath gets the target path shortened by the movepoints of this unit
     * @param movepoints
     * @return
     */
    QPoint getReachableTargetField(qint32 movepoints);
    /**
     * @brief getCosts
     * @param x
     * @param y
     * @return the exact costs needed to get onto the given field. -1 = unreachable
     */
    virtual qint32 getCosts(qint32 index, qint32 x, qint32 y, qint32 curX, qint32 curY, qint32 currentCost)  override;
    /**
     * @brief getAbortOnCostExceed
     * @return
     */
    bool getAbortOnCostExceed() const;
    /**
     * @brief setAbortOnCostExceed
     * @param abortOnCostExceed
     */
    void setAbortOnCostExceed(bool abortOnCostExceed);
    /**
     * @brief getTargets
     * @return
     */
    const std::vector<QVector3D> &getTargets() const;

private:
    bool m_abortOnCostExceed{true};
    std::vector<QVector3D> & m_Targets;
    std::vector<FinishNodeInfo> m_FinishNodes;
    std::vector<std::vector<std::tuple<qint32, bool>>>* m_pMoveCostMap;
    qint32 m_endCosts{-1};
    struct
    {
        qint32 bestCost{-1};
        qint32 target{-1};
        qint32 remainingCost{-1};
    } m_finishInfo;
};

#endif // TARGETEDUNITPATHFINDINGSYSTEM_H
