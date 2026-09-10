-- Startup scene registry. Add new Lua scenes here without changing C++.
Scenes.register("MainMenu", "menu_scene.lua")
Scenes.register("Game", "game_scene.lua")
Scenes.load("MainMenu")
