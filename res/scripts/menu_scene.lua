local settings = Settings.get()
local display = DisplaySettings.get()

local function play()
    Input.setUIActive(false)
    Scenes.load("Game")
end

local function quit()
    Application.quit()
end

local function syncUIWithSettings()
    -- Громкость
    local masterVol = math.floor(settings.masterVolume * 100)
    UI.setText("volLabel", "Громкость: " .. masterVol .. "%")

    -- FOV
    local fovVal = math.floor(settings.fov)
    UI.setText("fovLabel", "FOV: " .. fovVal .. "°")

    -- Если вызывать через двоеточие:
    UI.setChecked("bloomToggle", settings.enableBloom)
    UI.setChecked("vsyncToggle", display.vsync)
    UI.setChecked("debugToggle", settings.showFPS)
end

local function openSettings()
    syncUIWithSettings()
    UI.setVisible("mainPanel", false)
    UI.setVisible("settingsPanel", true)
end

local function closeSettings()
    settings:apply()
    display:apply()

    UI.setVisible("settingsPanel", false)
    UI.setVisible("mainPanel", true)
end

local function onVolumeChanged(value)
    settings.masterVolume = value / 100.0
    UI.setText("volLabel", "Громкость: " .. math.floor(value) .. "%")
end

local function onFovChanged(value)
    settings.fov = value
    UI.setText("fovLabel", "FOV: " .. math.floor(value) .. "°")
end

-- Обработчики Toggle
local function onBloomToggle(checked)
    settings.enableBloom = checked
end

local function onVSyncToggle(checked)
    display.vsync = checked
end

local function onDebugToggle(checked)
    settings.showFPS = checked
    settings.debugMode = checked
end

local function onShadowQualityChanged(index, option)
    if index == 0 then settings.shadowQuality = ShadowQuality.Off
    elseif index == 1 then settings.shadowQuality = ShadowQuality.Low
    elseif index == 2 then settings.shadowQuality = ShadowQuality.Medium
    elseif index == 3 then settings.shadowQuality = ShadowQuality.High
    elseif index == 4 then settings.shadowQuality = ShadowQuality.Ultra
    end
end

local function onTextureFilteringChanged(index, option)
    if index == 0 then settings.textureFiltering = TextureFiltering.Bilinear
    elseif index == 1 then settings.textureFiltering = TextureFiltering.Trilinear
    elseif index == 2 then settings.textureFiltering = TextureFiltering.Anisotropic2x
    elseif index == 3 then settings.textureFiltering = TextureFiltering.Anisotropic8x
    elseif index == 4 then settings.textureFiltering = TextureFiltering.Anisotropic16x
    end
end

local function onFpsChanged(index, option)
    if index == 0 then display.targetFPS = 0
    elseif index == 1 then display.targetFPS = 60
    elseif index == 2 then display.targetFPS = 120
    elseif index == 3 then display.targetFPS = 144
    end
end

local function resetSettings()
    settings:resetToDefaults()
    settings:apply()
    display:apply()
    syncUIWithSettings()
end

function OnCreate(scene)
    Input.setUIActive(true)

    local handlers = {
        play = play,
        openSettings = openSettings,
        closeSettings = closeSettings,
        quit = quit,
        
        onVolumeChanged = onVolumeChanged,
        onFovChanged = onFovChanged,
        onShadowQualityChanged = onShadowQualityChanged,
        onTextureFilteringChanged = onTextureFilteringChanged,
        onFpsChanged = onFpsChanged,
        
        -- Toggle handlers
        onBloomToggle = onBloomToggle,
        onVSyncToggle = onVSyncToggle,
        onDebugToggle = onDebugToggle,
        
        resetSettings = resetSettings
    }

    UI.loadXml("main_menu.xml", handlers)
    UI.addXml("settings_menu.xml", handlers)
end

function OnDestroy(scene)
    UI.clear()
end