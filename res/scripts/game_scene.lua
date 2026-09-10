local isPaused = false
local settings = Settings.get()
local display = DisplaySettings.get()

local function togglePause()
    isPaused = not isPaused

    -- Если снимаем с паузы, гарантированно закрываем обе панели
    if not isPaused then
        UI.setVisible("PausePanel", false)
        UI.setVisible("settingsPanel", false)
    else
        UI.setVisible("PausePanel", true)
        UI.setVisible("settingsPanel", false)
    end

    Time.setTimeScale(isPaused and 0 or 1)
    Input.setUIActive(isPaused)
end

local function syncUIWithSettings()
    local masterVol = math.floor(settings.masterVolume * 100)
    UI.setText("volLabel", "Громкость: " .. masterVol .. "%")

    local fovVal = math.floor(settings.fov)
    UI.setText("fovLabel", "FOV: " .. fovVal .. "°")

    UI.setChecked("bloomToggle", settings.enableBloom)
    UI.setChecked("vsyncToggle", display.vsync)
    UI.setChecked("debugToggle", settings.showFPS)
end

local function primitive(scene, name, model, position, scale, color, collider)
    local object = scene:create(name)
    object.transform.position = position
    object.transform.scale = scale
    object:addComponent("MeshRenderer")
    object:setModel(model)
    object:setMaterialColor(color)
    if collider then
        object:addComponent(collider)
        object:fitCollider()
    end
    return object
end

function OnCreate(scene)
    -- Обработчики взаимодействия с интерфейсом
    local uiHandlers = {
        onResume = function()
            togglePause()
        end,
        onOpenSettings = function()
            syncUIWithSettings()
            UI.setVisible("PausePanel", false)
            UI.setVisible("settingsPanel", true)
        end,
        closeSettings = function()
            settings:apply()
            display:apply()
            UI.setVisible("settingsPanel", false)
            UI.setVisible("PausePanel", true)
        end,
        onRestart = function()
            Time.setTimeScale(1)
            Input.setUIActive(false)
            Scenes.load(scene.name)
        end,
        onQuit = function()
            Time.setTimeScale(1)
            Scenes.load("MainMenu")
        end,

        -- Обработчики из settings_menu.xml
        onVolumeChanged = function(value)
            settings.masterVolume = value / 100.0
            UI.setText("volLabel", "Громкость: " .. math.floor(value) .. "%")
        end,
        onFovChanged = function(value)
            settings.fov = value
            UI.setText("fovLabel", "FOV: " .. math.floor(value) .. "°")
        end,
        onBloomToggle = function(checked)
            settings.enableBloom = checked
        end,
        onVSyncToggle = function(checked)
            display.vsync = checked
        end,
        onDebugToggle = function(checked)
            settings.showFPS = checked
            settings.debugMode = checked
        end,
        onShadowQualityChanged = function(index)
            if index == 0 then settings.shadowQuality = ShadowQuality.Off
            elseif index == 1 then settings.shadowQuality = ShadowQuality.Low
            elseif index == 2 then settings.shadowQuality = ShadowQuality.Medium
            elseif index == 3 then settings.shadowQuality = ShadowQuality.High
            elseif index == 4 then settings.shadowQuality = ShadowQuality.Ultra
            end
        end,
        onTextureFilteringChanged = function(index)
            if index == 0 then settings.textureFiltering = TextureFiltering.Bilinear
            elseif index == 1 then settings.textureFiltering = TextureFiltering.Trilinear
            elseif index == 2 then settings.textureFiltering = TextureFiltering.Anisotropic2x
            elseif index == 3 then settings.textureFiltering = TextureFiltering.Anisotropic8x
            elseif index == 4 then settings.textureFiltering = TextureFiltering.Anisotropic16x
            end
        end,
        onFpsChanged = function(index)
            if index == 0 then display.targetFPS = 0
            elseif index == 1 then display.targetFPS = 60
            elseif index == 2 then display.targetFPS = 120
            elseif index == 3 then display.targetFPS = 144
            end
        end,
        resetSettings = function()
            settings:resetToDefaults()
            settings:apply()
            display:apply()
            syncUIWithSettings()
        end
    }

    -- Загрузка интерфейсов
    UI.addXml("pause_menu.xml", uiHandlers)
    UI.addXml("settings_menu.xml", uiHandlers)

    Renderer.setSkybox("skybox/qwantani_dusk_2_puresky_4k")

    local player = scene:create("Player")
    player.transform.position = Vector3(0, 2, -5)
    player:addComponent("Player")

    primitive(scene, "Ground", "plane.obj", Vector3(0, 0, 0), Vector3(25, 1, 25), Vector3(0.3, 0.3, 0.35), "MeshCollider")

    for index = 1, 4 do
        primitive(
            scene,
            "Stair_Step_" .. index,
            "cube.obj",
            Vector3(0, index * 0.5, index * 3),
            Vector3(3, index * 0.5, 2),
            Vector3(0.2, 0.6 + index * 0.1, 0.4),
            "BoxCollider"
        )
    end

    primitive(scene, "Rotating_Platform", "cube.obj", Vector3(8, 1.5, 5), Vector3(4, 0.3, 4), Vector3(0.8, 0.4, 0.1), "BoxCollider")
    primitive(scene, "Spinning_Blade", "cube.obj", Vector3(8, 2.5, 12), Vector3(5, 0.4, 0.8), Vector3(0.9, 0.1, 0.1), "BoxCollider")
    primitive(scene, "Sphere_Prop", "sphere.obj", Vector3(-6, 1, 5), Vector3(1, 1, 1), Vector3(0.9, 0.2, 0.2), "SphereCollider")
    primitive(scene, "Capsule_Prop", "capsule.obj", Vector3(-6, 1, 8), Vector3(1, 1, 1), Vector3(0.2, 0.8, 0.2), "CapsuleCollider")
    primitive(scene, "Ramp_Platform", "plane.obj", Vector3(-6, 1, 11), Vector3(2, 1, 2), Vector3(0.9, 0.8, 0.2), "MeshCollider")

    local sun = scene:create("Sun")
    sun.transform.position = Vector3(0, 25, 10)
    sun.transform.rotation = Vector3(0, -90, 0)
    sun:addComponent("DirectionalLight")
end

function Update(scene, deltaTime)
    if Input.consumeEscape() or Input.getKeyDown("escape") or Input.actionDown("pause") then
        togglePause()
    end

    local platform = scene:find("Rotating_Platform")
    if platform then platform.transform.rotation.y = platform.transform.rotation.y + 30 * deltaTime end

    local blade = scene:find("Spinning_Blade")
    if blade then blade.transform.rotation.z = blade.transform.rotation.z + 90 * deltaTime end
end