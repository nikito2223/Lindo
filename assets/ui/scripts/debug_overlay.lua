local DebugOverlay = {}

function DebugOverlay.init()
    UI.loadXml("debug_overlay.xml", {})
    
    -- Установка системных констант
    UI.setText("lbl_engine", "Engine: " .. Application.name)
    UI.setText("lbl_version", "Version: " .. Application.version)
end

function DebugOverlay.update(dt)
    if Input.getKeyDown("F3") then
        local visible = not DebugOverlay.visible
        DebugOverlay.visible = visible
        UI.setVisible("debug_panel", visible)
    end

    if not DebugOverlay.visible then return end

    -- 1. FPS & Frame Time
    local fps = System.getFPS()
    local frameTime = fps > 0 and (1000.0 / fps) or 0.0
    UI.setText("lbl_fps", string.format("FPS: %d (%.1f ms)", fps, frameTime))

    -- 2. Position & Speed
    local p = System.getPlayerInfo()
    UI.setText("lbl_pos", string.format("Position: X: %.2f | Y: %.2f | Z: %.2f", p.x or 0, p.y or 0, p.z or 0))
    UI.setText("lbl_speed", string.format("Speed: %.2f m/s", p.speed or 0))

    -- 3. Memory & Objects
    UI.setText("lbl_memory", string.format("Memory: %.1f MB", System.getMemoryMB()))
    UI.setText("lbl_objects", string.format("Scene Objects: %d", System.getObjectCount()))
end

return DebugOverlay