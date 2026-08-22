var OTTERAI =
{
    getName : function()
    {
        return qsTr("Otter");
    },

    groupDistribution : [1,      // infantry units
                        1,      // light units
                        1,      // medium units
                        1,],    // heavy units


    initializeSimpleProductionSystem : function(system, ai, map)
    {
        return COREAI.initializeSimpleProductionSystem(system, ai, map, OTTERAI.groupDistribution);
    },

    onNewBuildQueue : function(system, ai, buildings, units, enemyUnits, enemyBuildings, map)
    {
        return COREAI.onNewBuildQueue(system, ai, buildings, units, enemyUnits, enemyBuildings, map, OTTERAI.groupDistribution);
    },

    buildUnitSimpleProductionSystem : function(system, ai, buildings, units, enemyUnits, enemyBuildings, map)
    {
        return COREAI.buildUnitSimpleProductionSystem(system, ai, buildings, units, enemyUnits, enemyBuildings, map);
    },



    // for modding implement a function named after the action to modify or implement the behaviour for
    // it will given the following input on which you can return the score for the function
    // the score is capped at 1 and actions with a to low score won't be considered to be executed
    // example for capture
    // ACTION_CAPTURE : function(action)
    // {
    //     return 0.99;
    // },
};
