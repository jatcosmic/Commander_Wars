#Live Debugging of Scripts and Mods

Press `F1` to open the console. Once there, you can do things like `setDeveloperMode(1)` to enable debug logging. This can show information about the cursor, unit selected, etc to the console. It is of limited use at running functions without a script, however. 

For example, if you had a script with a function that set the power of the first player to 10, you could access it like so:

`game:game.getMap().getPlayer(0).getCO(0).setPowerFilled(10)`

This page describes another example of things that can be done in the console:

[Debugging and Testing](https://github.com/Robosturm/Commander_Wars/wiki/Debugging-and-Testing-mods-or-game-scripts)

The functions available can be output with `help()`

The code is in `gameconsole.cpp`