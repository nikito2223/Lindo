-- Unity-style Lindo script.
-- Attach with: gameObject:addComponent<LuaScript>("scripts/rotator.lua")

function Awake(self)
    print("Awake: " .. self.name)
end

function Start(self)
    self.transform.rotation = Vector3(0.0, 0.0, 0.0)
end

function Update(self, deltaTime)
    local rotation = self.transform.rotation
    rotation.y = rotation.y + 45.0 * deltaTime
    self.transform.rotation = rotation
end

function OnDestroy(self)
    print("Destroyed: " .. self.name)
end
