local settings = Settings.get()
local display = DisplaySettings.get()


local function syncUIWithSettings()

    -- Громкость (слайдер + label)
    local masterVol = math.floor(settings.masterVolume * 100)
    UI.setSliderValue("volSlider", masterVol)
    UI.setText("volLabel", "Громкость: " .. masterVol .. "%")

    -- FOV
    local fovVal = math.floor(settings.fov)
    UI.setSliderValue("fovSlider", fovVal)
    UI.setText("fovLabel", "FOV: " .. fovVal .. "°")

    -- Качество теней
    local shadowIdx = 0
    if settings.shadowQuality == ShadowQuality.Low         then shadowIdx = 1
    elseif settings.shadowQuality == ShadowQuality.Medium  then shadowIdx = 2
    elseif settings.shadowQuality == ShadowQuality.High    then shadowIdx = 3
    elseif settings.shadowQuality == ShadowQuality.Ultra   then shadowIdx = 4
    end
    UI.setDropdownIndex("shadowSelect", shadowIdx)

    -- Фильтрация текстур
    local texIdx = 0
    if settings.textureFiltering == TextureFiltering.Trilinear      then texIdx = 1
    elseif settings.textureFiltering == TextureFiltering.Anisotropic2x  then texIdx = 2
    elseif settings.textureFiltering == TextureFiltering.Anisotropic8x  then texIdx = 3
    elseif settings.textureFiltering == TextureFiltering.Anisotropic16x then texIdx = 4
    end
    UI.setDropdownIndex("texSelect", texIdx)

    -- Лимит FPS
    local fpsIdx = 0
    if display.targetFPS == 60  then fpsIdx = 1
    elseif display.targetFPS == 120 then fpsIdx = 2
    elseif display.targetFPS == 144 then fpsIdx = 3
    end
    UI.setDropdownIndex("fpsSelect", fpsIdx)

    -- Toggle
    UI.setChecked("bloomToggle", settings.enableBloom)
    UI.setChecked("vsyncToggle", display.vsync)
    UI.setChecked("debugToggle", settings.showFPS)
end


local function onVolumeChanged(value)

    settings.masterVolume = value / 100.0

    UI.setText(
        "volLabel",
        "Громкость: " .. math.floor(value) .. "%"
    )
end


local function onFovChanged(value)

    settings.fov = value

    UI.setText(
        "fovLabel",
        "FOV: " .. math.floor(value) .. "°"
    )
end


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

    if index == 0 then
        settings.shadowQuality = ShadowQuality.Off

    elseif index == 1 then
        settings.shadowQuality = ShadowQuality.Low

    elseif index == 2 then
        settings.shadowQuality = ShadowQuality.Medium

    elseif index == 3 then
        settings.shadowQuality = ShadowQuality.High

    elseif index == 4 then
        settings.shadowQuality = ShadowQuality.Ultra
    end
end


local function onTextureFilteringChanged(index, option)

    if index == 0 then
        settings.textureFiltering = TextureFiltering.Bilinear

    elseif index == 1 then
        settings.textureFiltering = TextureFiltering.Trilinear

    elseif index == 2 then
        settings.textureFiltering = TextureFiltering.Anisotropic2x

    elseif index == 3 then
        settings.textureFiltering = TextureFiltering.Anisotropic8x

    elseif index == 4 then
        settings.textureFiltering = TextureFiltering.Anisotropic16x
    end
end


local function onFpsChanged(index, option)

    if index == 0 then
        display.targetFPS = 0

    elseif index == 1 then
        display.targetFPS = 60

    elseif index == 2 then
        display.targetFPS = 120

    elseif index == 3 then
        display.targetFPS = 144
    end
end

local function closeSaveSettings()
    settings:apply()
    display:apply()

    UI.setVisible("settingsPanel", false)
    UI.setVisible("mainPanel", true)
end

local function resetSettings()

    settings:resetToDefaults()

    settings:apply()
    display:apply()

    syncUIWithSettings()
end


return {
    syncUIWithSettings = syncUIWithSettings,

    onVolumeChanged = onVolumeChanged,
    onFovChanged = onFovChanged,

    onBloomToggle = onBloomToggle,
    onVSyncToggle = onVSyncToggle,
    onDebugToggle = onDebugToggle,

    onShadowQualityChanged = onShadowQualityChanged,
    onTextureFilteringChanged = onTextureFilteringChanged,
    onFpsChanged = onFpsChanged,

    resetSettings = resetSettings,
    closeSaveSettings = closeSaveSettings
}