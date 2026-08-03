# CoreAI helper functions

This is a list of helper CoreAI functions which do not contribute to the functionality of the AI. So these are functions which I will use--and not reimplement.

* `CoreAI::useBuilding()`. This function is used to determine if the building can be used by the AI. It does not allow the AI to build units nor does it check if the AI can build units from that building. For all other actions that may exist on a building, it checks if it can be used and if it can and there's no script for how it should be used, it just does a random, but legal, action.