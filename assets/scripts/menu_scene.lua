local settingsMenu = require("ui.scripts.settings_menu")

local function play()
    Input.setUIActive(false)
    Scenes.load("Game")
end

local function quit()
    Application.quit()
end

local function openSettings()

    settingsMenu.syncUIWithSettings()

    UI.setVisible("mainPanel", false)
    UI.setVisible("settingsPanel", true)
end

local function closeSettings()
end

function OnCreate(scene)
    Input.setUIActive(true)

    local handlers = {
        play = play,
        openSettings = openSettings,
        closeSettings = settingsMenu.closeSaveSettings,
        quit = quit,

        onVolumeChanged = settingsMenu.onVolumeChanged,
        onFovChanged = settingsMenu.onFovChanged,

        onShadowQualityChanged =
            settingsMenu.onShadowQualityChanged,

        onTextureFilteringChanged =
            settingsMenu.onTextureFilteringChanged,

        onFpsChanged =
            settingsMenu.onFpsChanged,

        onBloomToggle =
            settingsMenu.onBloomToggle,

        onVSyncToggle =
            settingsMenu.onVSyncToggle,

        onDebugToggle =
            settingsMenu.onDebugToggle,

        resetSettings =
            settingsMenu.resetSettings,
    }

    UI.loadXml("main_menu.xml", handlers)
    UI.addXml("settings_menu.xml", handlers)

    UI.setText("titleLabel", string.upper(Application.name))
    UI.setText("versionLabel", "v" .. Application.version)
    UI.setText("buildLabel", "Powered by Lindo")

end

function OnDestroy(scene)
    UI.clear()
end