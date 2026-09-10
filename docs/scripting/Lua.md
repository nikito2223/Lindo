# Lua scripting

Lindo exposes Unity-style lifecycle callbacks through `LuaScript`:

```lua
function Awake(self)
end

function Start(self)
end

function Update(self, deltaTime)
    local position = self.transform.position
    position.x = position.x + deltaTime
    self.transform.position = position
end

function OnDestroy(self)
end
```

Attach a script from C++:

```cpp
object->addComponent<Lindo::Scripting::LuaScript>("scripts/rotator.lua");
```

Available objects:

- `self` and `gameObject`: the owning `GameObject`.
- `self.name`, `self.tag`, `self.active`.
- `self.transform.position`, `rotation`, and `scale` as `Vector3`.
- `self:getWorldPosition()` and `self:setWorldPosition(Vector3(...))`.
- `Time.deltaTime`, `Time.unscaledDeltaTime`, `Time.time`, `Time.timeScale`.
- `Time.setTimeScale(value)`.

Lua errors are reported through the engine logger and do not terminate the scene.

## Scenes

At startup the engine executes `res/scripts/bootstrap.lua`. Register new scenes there without touching C++:

```lua
Scenes.register("Credits", "credits_scene.lua")
Scenes.load("Credits")
```

The default `Game` scene is also created by `res/scripts/game_scene.lua`. Lua `GameObject` supports `addComponent` for `Player`, `Camera`, `MeshRenderer`, `Material`, colliders, and `DirectionalLight`, plus `setModel`, `setMaterialColor`, and `fitCollider`.

Register a Lua scene during startup, then load it by name:

```lua
Scenes.register("Menu", "menu_scene.lua")
Scenes.load("Menu")
```

Scene scripts receive `scene` and use the same object hierarchy as C++ scenes:

```lua
function OnCreate(scene)
    local camera = scene:create("MenuCamera")
    camera.transform.position = Vector3(0, 0, 5)
end

function Update(scene, deltaTime)
end
```

## UI

Lua can create widgets directly or load an XML layout. `UI.loadXml` accepts a table of button handlers.

```lua
local root = UI.root()
local label = UI.createLabel("Welcome")
label:setPosition(40, 30)
label:setSize(300, 40)
root:addChild(label)

UI.loadXml("main_menu.xml", {
    start = function() Scenes.load("Game") end
})
```

XML supports `Panel`, `Label`, and `Button` elements with `x`, `y`, `width`, `height`, `fontSize`, `color`, and `onClick` attributes. Colors use `r,g,b,a` values from 0 to 1.

## Input and scripts

Use `Input.action`, `Input.actionDown`, `Input.actionUp`, and `Input.axis`. Component scripts continue to be attached from C++ with `LuaScript`.

`Application.quit()` closes the engine window. The sample `menu_scene.lua` uses it for the XML `Выход` button and loads `Game` from the `Играть` button.

Use `Input.setUIActive(true)` for menus and `Input.setUIActive(false)` when entering gameplay. The engine keeps the cursor visible while UI or the console is open and blocks gameplay input while the console is active.

Console commands include `scene_list` (registered scenes, with `*` marking the active one), `scene_current`, `scene_load <name>`, and `pause [0|1]`. Tab completion filters by the first command word, cycles only matching commands, and resets after editing, navigation, or entering arguments.
