## Miscellaneous Notes

**Rule of thumb:** If you can't find a C++ caller for a Q_INVOKABLE method, check the JavaScript. Many gameplay events (combat, movement, abilities, animations) originate in JavaScript and call back into C++ through Qt's meta-object system. 

For example, `Unit::reduceAmmo1()` is called in `ACTION_FIRE.js` function `battle()` like so: `attacker.reduceAmmo1(1)` 