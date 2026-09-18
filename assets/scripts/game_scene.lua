local settingsMenu = require("ui.scripts.settings_menu")

local isPaused = false
local isInventoryOpen = false           -- НОВОЕ

local function refreshInputState()
    local anyUI = isPaused or isInventoryOpen
    Time.setTimeScale(anyUI and 0 or 1)
    Input.setUIActive(anyUI)
end

local function togglePause()
    isPaused = not isPaused

    if isPaused then
        isInventoryOpen = false
        UI.setVisible("InventoryPanel", false)
        UI.setVisible("PausePanel", true)
        UI.setVisible("settingsPanel", false)
    else
        UI.setVisible("PausePanel", false)
        UI.setVisible("settingsPanel", false)
    end

    refreshInputState()
end

-- НОВОЕ
local function toggleInventory()
    isInventoryOpen = not isInventoryOpen

    if isInventoryOpen then
        isPaused = false
        UI.setVisible("PausePanel", false)
        UI.setVisible("settingsPanel", false)
    end

    UI.setVisible("InventoryPanel", isInventoryOpen)
    refreshInputState()
end

-- Открытие настроек — тот же паттерн, что в main_menu.lua
local function openSettings()
    settingsMenu.syncUIWithSettings()
    UI.setVisible("PausePanel", false)
    UI.setVisible("settingsPanel", true)
end

-- Закрытие настроек — сохраняем через модуль и возвращаемся в паузу
local function closeSettings()
    settingsMenu.closeSaveSettings()
    UI.setVisible("settingsPanel", false)
    UI.setVisible("PausePanel", true)
end

-- texturePath is optional: when given, it's applied on top of `color`, which
-- then acts as a tint multiplier instead of a flat fill.
--
-- tiling / offset (optional, Unity-style):
--   tiling = {x, y} — сколько раз текстура повторяется по U и V
--   offset = {x, y} — сдвиг UV
-- Если не заданы — 1x1 без сдвига (как было раньше).
local function primitive(scene, name, model, position, scale, color, collider, texturePath, tiling, offset)
    local object = scene:create(name)
    object.transform.position = position
    object.transform.scale = scale
    object:addComponent("MeshRenderer")
    object:setModel(model)
    object:setMaterialColor(color)
    if texturePath then
        object:setMaterialTexture(texturePath)
        if tiling then
            object:setMaterialTiling(tiling[1], tiling[2])
        end
        if offset then
            object:setMaterialOffset(offset[1], offset[2])
        end
    end
    if collider then
        object:addComponent(collider)
        object:fitCollider()
    end
    return object
end

function OnCreate(scene)
    -- Обработчики взаимодействия с интерфейсом.
    -- Настройки делегируются в общий модуль settingsMenu —
    -- тот же, что использует главное меню.
    local uiHandlers = {
        -- pause_menu.xml
        onResume = function()
            togglePause()
        end,
        onOpenSettings = openSettings,
        closeSettings  = closeSettings,
        onRestart = function()
            Time.setTimeScale(1)
            Input.setUIActive(false)
            Scenes.load(scene.name)
        end,
        onQuit = function()
            Time.setTimeScale(1)
            Scenes.load("MainMenu")
        end,
        closeInventory = function() toggleInventory() end,


        -- settings_menu.xml — всё из общего модуля
        onVolumeChanged           = settingsMenu.onVolumeChanged,
        onFovChanged              = settingsMenu.onFovChanged,
        onShadowQualityChanged    = settingsMenu.onShadowQualityChanged,
        onTextureFilteringChanged = settingsMenu.onTextureFilteringChanged,
        onFpsChanged              = settingsMenu.onFpsChanged,
        onBloomToggle             = settingsMenu.onBloomToggle,
        onVSyncToggle             = settingsMenu.onVSyncToggle,
        onDebugToggle             = settingsMenu.onDebugToggle,
        resetSettings             = settingsMenu.resetSettings,
    }

    -- Загрузка интерфейсов
    UI.addXml("pause_menu.xml", uiHandlers)
    UI.addXml("settings_menu.xml", uiHandlers)
    UI.addXml("inventory_menu.xml", uiHandlers)

    Renderer.setSkybox("skybox/qwantani_dusk_2_puresky_4k")

    local player = scene:create("Player")
    player.transform.position = Vector3(0, 2, -5)
    player:addComponent("Player")

    -- Ground: текстура 25x25 раз — идеально под размер площадки.
    primitive(
        scene, "Ground", "plane.obj",
        Vector3(0, 0, 0), Vector3(25, 1, 25),
        Vector3(0.3, 0.3, 0.35),
        "MeshCollider",
        "textures/textures.png",
        {25, 25}
    )

    -- Лестница: у каждой ступеньки свой tiling по размеру (по X и Z).
    for index = 1, 4 do
        primitive(
            scene,
            "Stair_Step_" .. index,
            "cube.obj",
            Vector3(0, index * 0.5, index * 3),
            Vector3(3, index * 0.5, 2),
            Vector3(0.2, 0.6 + index * 0.1, 0.4),
            "BoxCollider",
            "textures/textures.png",
            {3, 2}
        )
    end

    -- Rotating_Platform: 4x4 плитки — площадка 4x4 unit'а.
    primitive(
        scene, "Rotating_Platform", "cube.obj",
        Vector3(8, 1.5, 5), Vector3(4, 0.3, 4),
        Vector3(0.8, 0.4, 0.1),
        "BoxCollider",
        "textures/textures.png",
        {4, 4}
    )

    -- Spinning_Blade: тонкая, но длинная — пусть плитки идут только по X.
    primitive(
        scene, "Spinning_Blade", "cube.obj",
        Vector3(8, 2.5, 12), Vector3(5, 0.4, 0.8),
        Vector3(0.9, 0.1, 0.1),
        "BoxCollider",
        "textures/textures.png",
        {5, 1}
    )

    -- Sphere / Capsule без текстуры (чистый color).
    primitive(scene, "Sphere_Prop", "sphere.obj",
        Vector3(-6, 1, 5), Vector3(1, 1, 1),
        Vector3(0.9, 0.2, 0.2), "SphereCollider")

    primitive(scene, "Capsule_Prop", "capsule.obj",
        Vector3(-6, 1, 8), Vector3(1, 1, 1),
        Vector3(0.2, 0.8, 0.2), "CapsuleCollider")

    -- Ramp_Platform: 2x2.
    primitive(
        scene, "Ramp_Platform", "plane.obj",
        Vector3(-6, 1, 11), Vector3(2, 1, 2),
        Vector3(0.9, 0.8, 0.2),
        "MeshCollider",
        "textures/textures.png",
        {2, 2}
    )

    -- Contener.fbx — уже готовый меш со своими UV; не трогаем.
    primitive(scene, "Ramp_", "test.fbx",
        Vector3(-2, 4, 0), Vector3(1, 1, 1),
        Vector3(0.9, 0.8, 0.2), "MeshCollider")

    -- box: динамический RigidBody с текстурой, тоже тайлим 1x1 (по размеру).
    local box = scene:create("box")
    box.transform.position = Vector3(0, 10, 0)
    box:addComponent("MeshRenderer")
    box:setModel("cube.obj")
    box:setMaterialTexture("textures/textures.png")
    box:setMaterialColor(Vector3(5, 5, 5))
    box:setMaterialTiling(5, 5)
    box:addComponent("BoxCollider")
    box:fitCollider()
    box:addComponent("RigidBody")

    local sun = scene:create("Sun")
    sun.transform.position = Vector3(0, 25, 10)
    sun.transform.rotation = Vector3(-45, 0, 0)
    sun:addComponent("DirectionalLight")

end

-- Храним накопители UV-offset, чтобы не зависеть от deltaTime на больших dt.
local uvScroll = {x = 0.0, y = 0.0}

function Update(scene, deltaTime)
    if Input.consumeEscape() or Input.getKeyDown("escape") then
        if isInventoryOpen then
            toggleInventory()
        else
            togglePause()
        end
    end

    if Input.actionDown("pause") then
        togglePause()
    end

    -- Tab открывает/закрывает инвентарь (только если не на паузе/в настройках)
    if Input.getKeyDown("I") and not isPaused then
        toggleInventory()
    end

    local platform = scene:find("Rotating_Platform")
    if platform then platform.transform.rotation.y = platform.transform.rotation.y + 30 * deltaTime end

    local blade = scene:find("Spinning_Blade")
    if blade then blade.transform.rotation.z = blade.transform.rotation.z + 90 * deltaTime end

    -- --- Пример: анимированный скролл UV на вращающейся платформе ---
    uvScroll.x = uvScroll.x + deltaTime * 0.15
    uvScroll.y = uvScroll.y + deltaTime * 0.05

    if platform then
        platform:setMaterialOffset(uvScroll.x, uvScroll.y)
    end
end

function OnDestroy(scene)
    UI.clear()
end