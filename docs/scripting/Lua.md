# Lua Scripting — Lindo Engine

> Полное руководство по написанию игровой логики на Lua в движке Lindo.

Lindo предоставляет Unity-подобный API для Lua: жизненный цикл компонентов, работа с игровыми объектами, физикой, UI, вводом и настройками. Скрипты не требуют пересборки C++ — просто положите `.lua` файл и подключите его.

---

## 📑 Содержание

1. [Быстрый старт](#-быстрый-старт)
2. [Жизненный цикл скрипта](#-жизненный-цикл-скрипта)
3. [GameObject](#-gameobject)
4. [Transform](#-transform)
5. [Компоненты и физика](#-компоненты-и-физика)
6. [RigidBody](#-rigidbody)
7. [Сцены](#-сцены)
8. [UI — интерфейс](#-ui--интерфейс)
9. [XML-разметка](#-xml-разметка)
10. [Input — ввод](#-input--ввод)
11. [Time — время](#-time--время)
12. [Renderer — рендер](#-renderer--рендер)
13. [Settings и DisplaySettings](#-settings-и-displaysettings)
14. [LayerMask](#-layermask)
15. [Application](#-application)
16. [Консольные команды](#-консольные-команды)
17. [Примеры](#-примеры)
18. [Обработка ошибок](#-обработка-ошибок)

---

## 🚀 Быстрый старт

1. Создайте файл `res/scripts/rotator.lua`:

```lua
function Update(self, deltaTime)
    local pos = self.transform.position
    pos.x = pos.x + deltaTime
    self.transform.position = pos
end
```

2. Подключите его к игровому объекту из C++:

```cpp
object->addComponent<Lindo::Scripting::LuaScript>("scripts/rotator.lua");
```

3. Готово. Скрипт запустится при следующем кадре.

> 💡 **Совет.** Скрипты можно перезагружать без пересборки проекта — движок перечитывает файл при старте сцены.

---

## 🔄 Жизненный цикл скрипта

`LuaScript` вызывает стандартные Unity-подобные колбэки. Все они опциональны — определяйте только те, что нужны.

```lua
function Awake(self)
    -- Вызывается один раз при создании компонента
    print("Awake: " .. self.name)
end

function Start(self)
    -- Вызывается один раз перед первым Update
end

function Update(self, deltaTime)
    -- Вызывается каждый кадр
end

function OnDestroy(self)
    -- Вызывается при удалении объекта или компонента
end
```

| Колбэк | Когда вызывается | Аргументы |
|---|---|---|
| `Awake(self)` | Один раз при создании | `self` — GameObject |
| `Start(self)` | Один раз перед первым `Update` | `self` — GameObject |
| `Update(self, dt)` | Каждый кадр | `self`, `deltaTime` |
| `OnDestroy(self)` | При уничтожении | `self` — GameObject |

### Доступные глобальные объекты

- `self` и `gameObject` — владеющий `GameObject`
- `self.name`, `self.tag`, `self.active`
- `self.transform` — `Transform`
- `Time`, `Input`, `UI`, `Scenes`, `Renderer`, `Application`, `LayerMask`

---

## 🎮 GameObject

Основной объект сцены. Все свойства можно читать и изменять.

### Свойства

| Свойство | Тип | Описание |
|---|---|---|
| `name` | `string` | Имя объекта |
| `tag` | `string` | Тег объекта |
| `layer` | `integer` | Числовой слой |
| `layerName` | `string` | Имя слоя (чтение/запись по имени) |
| `active` | `boolean` | Активен ли объект |
| `transform` | `Transform` | Трансформ объекта |

### Методы

```lua
-- Мировая позиция
local pos = self:getWorldPosition()   -- vec3
self:setWorldPosition(Vector3(1, 2, 3))

-- Проверка тега
if self:compareTag("Enemy") then
    -- ...
end

-- Работа с компонентами
self:addComponent("RigidBody")
self:addComponent("BoxCollider")

local rb = self:getRigidBody()   -- RigidBody | nil

-- Модель и материал
self:setModel("models/player.gltf")
self:setMaterialColor(Vector3(1, 0, 0))   -- RGB 0..1

-- Подгонка коллайдеров под AABB модели
self:fitCollider()
```

### Поддерживаемые типы для `addComponent`

| Тип | Компонент |
|---|---|
| `"Player"` | `Player` |
| `"Camera"` | `Camera` |
| `"MeshRenderer"` | `MeshRenderer` |
| `"Material"` | `Material` |
| `"BoxCollider"` | `BoxCollider` |
| `"SphereCollider"` | `SphereCollider` |
| `"CapsuleCollider"` | `CapsuleCollider` |
| `"MeshCollider"` | `MeshCollider` |
| `"DirectionalLight"` | `DirectionalLight` |
| `"RigidBody"` | `RigidBody` |

### Пример

```lua
function Start(self)
    self:addComponent("MeshRenderer")
    self:addComponent("BoxCollider")
    self:setModel("models/crate.gltf")
    self:setMaterialColor(Vector3(0.8, 0.4, 0.1))
    self:fitCollider()

    self:addComponent("RigidBody")
    local rb = self:getRigidBody()
    rb.mass = 5.0
    rb.restitution = 0.4
end
```

---

## 🧭 Transform

```lua
---@class Transform
---@field position vec3
---@field rotation vec3
---@field scale vec3
```

### Пример

```lua
function Update(self, dt)
    local t = self.transform

    -- Вращение
    local rot = t.rotation
    rot.y = rot.y + dt * 90   -- 90°/сек
    t.rotation = rot

    -- Пульсация масштаба
    local s = 1.0 + math.sin(Time.time * 2) * 0.1
    t.scale = Vector3(s, s, s)
end
```

> ⚠️ **Важно.** `vec3` — это value-тип. Изменения нужно присваивать обратно:
> ```lua
> local p = self.transform.position
> p.x = p.x + 1
> self.transform.position = p   -- обязательно!
> ```

---

## 🧩 Компоненты и физика

### RigidBody

```lua
---@class RigidBody
---@field mass number
---@field invMass number
---@field velocity vec3
---@field angularVelocity vec3
---@field acceleration vec3
---@field useGravity boolean
---@field gravityScale number
---@field restitution number
---@field friction number
---@field linearDamping number
---@field angularDamping number
---@field isKinematic boolean
---@field isSleeping boolean
---@field isGrounded boolean
```

### Методы RigidBody

| Метод | Описание |
|---|---|
| `applyForce(vec3)` | Приложить силу |
| `applyForceAtPoint(vec3, vec3)` | Сила в точке |
| `applyImpulse(vec3)` | Импульс |
| `applyImpulseAtPoint(vec3, vec3)` | Импульс в точке |
| `applyTorque(vec3)` | Крутящий момент |
| `clearForces()` | Сбросить накопленные силы |
| `wake()` | Разбудить тело |

### Пример: прыжок

```lua
function Update(self, dt)
    local rb = self:getRigidBody()
    if not rb then return end

    if Input.actionDown("Jump") and rb.isGrounded then
        rb:applyImpulse(Vector3(0, 8, 0))
    end
end
```

### Пример: гравитационная пушка

```lua
function Update(self, dt)
    if Input.actionDown("Fire") then
        local rb = self:getRigidBody()
        if rb then
            local forward = Vector3(0, 0, 1)
            rb:applyImpulse(forward * 20.0)
        end
    end
end
```

---

## 🎬 Сцены

### Регистрация и загрузка

Все сцены регистрируются в `res/scripts/bootstrap.lua`:

```lua
Scenes.register("Credits", "credits_scene.lua")
Scenes.register("Menu",    "menu_scene.lua")
Scenes.register("Game",    "game_scene.lua")

Scenes.load("Menu")
```

### API

| Функция | Описание |
|---|---|
| `Scenes.current()` | Текущая активная `Scene` или `nil` |
| `Scenes.load(name)` | Запросить загрузку сцены по имени |
| `Scenes.register(name, script)` | Зарегистрировать фабрику сцены |
| `Scenes.setSkybox(path, res?)` | Установить скайбокс (дубль `Renderer.setSkybox`) |

### Жизненный цикл сцены

Скрипт сцены — обычный Lua-файл с двумя колбэками:

```lua
function OnCreate(scene)
    -- Сцена создана
    local camera = scene:create("MenuCamera")
    camera.transform.position = Vector3(0, 0, 5)
    camera:addComponent("Camera")
end

function Update(scene, deltaTime)
    -- Каждый кадр
end
```

| Колбэк | Аргументы |
|---|---|
| `OnCreate(scene)` | `Scene` — созданная сцена |
| `Update(scene, dt)` | `Scene`, `deltaTime` |

### Методы Scene

| Метод | Возвращает | Описание |
|---|---|---|
| `scene:create(name)` | `GameObject` | Создать объект |
| `scene:find(name)` | `GameObject\|nil` | Найти объект по имени |
| `scene:destroy(name)` | — | Удалить объект |

### Свойства Scene

| Свойство | Тип |
|---|---|
| `name` | `string` |
| `active` | `boolean` |

### Пример: процедурная сцена

```lua
function OnCreate(scene)
    for i = 1, 10 do
        local cube = scene:create("Cube_" .. i)
        cube:addComponent("MeshRenderer")
        cube:setModel("models/cube.gltf")
        cube.transform.position = Vector3(i * 2.0, 0, 0)
    end
end
```

---

## 🖼 UI — интерфейс

Lindo поддерживает **два способа** создания интерфейса: программно через Lua API и декларативно через XML-разметку.

### UI-виджеты

Иерархия классов:

```
UIWidget
├── UIPanel
├── UILabel
├── UIButton
├── UITextInput
├── UISlider
├── UIDropDown
└── UIToggle
```

### Базовый UIWidget

| Свойство / метод | Тип | Описание |
|---|---|---|
| `tag` | `string` | Тег |
| `layer` | `integer` | Слой |
| `layerName` | `string` | Имя слоя |
| `zOrder` | `integer` | Порядок отрисовки |
| `visible` | `boolean` | Видимость |
| `setPosition(x, y)` | — | Позиция |
| `setSize(w, h)` | — | Размер |
| `setVisible(bool)` | — | Видимость |
| `compareTag(tag)` | `boolean` | Сравнение тега |

### UI API

| Функция | Возвращает | Описание |
|---|---|---|
| `UI.root()` | `UIPanel\|nil` | Корневая панель |
| `UI.createPanel()` | `UIPanel` | Создать панель |
| `UI.createLabel(text)` | `UILabel` | Создать метку |
| `UI.createButton(text, cb)` | `UIButton` | Создать кнопку |
| `UI.clear()` | — | Очистить динамические виджеты |
| `UI.loadXml(path, handlers)` | `boolean` | Загрузить XML с полной очисткой |
| `UI.addXml(path, handlers)` | `boolean` | Добавить XML без очистки |
| `UI.setText(id, text)` | — | Установить текст по ID |
| `UI.setVisible(id, bool)` | — | Видимость по ID |
| `UI.setChecked(id, bool)` | — | Состояние Toggle по ID |
| `UI.isChecked(id)` | `boolean` | Состояние Toggle по ID |
| `UI.getValue(id)` | `any` | Значение виджета по ID |

### Пример: программное создание UI

```lua
function Start(self)
    local root = UI.root()
    if not root then return end

    local panel = UI.createPanel()
    panel:setPosition(20, 20)
    panel:setSize(300, 200)
    root:addChild(panel)

    local label = UI.createLabel("Здоровье: 100")
    label:setPosition(10, 10)
    label:setSize(280, 40)
    panel:addChild(label)

    local button = UI.createButton("Старт", function()
        print("Кнопка нажата!")
        Scenes.load("Game")
    end)
    button:setPosition(10, 60)
    button:setSize(280, 40)
    panel:addChild(button)
end
```

### Пример: XML + программное управление

```lua
-- Загрузка разметки
UI.loadXml("main_menu.xml", {
    start  = function() Scenes.load("Game") end,
    quit   = function() Application.quit() end,
    reload = function() UI.setText("status", "Обновлено!") end,
})

-- Позже — управление по ID из XML
UI.setVisible("status", true)
UI.setText("status", "Загрузка...")
```

---

## 📄 XML-разметка

XML-файлы кладутся в `res/ui/layouts/`. Путь в `UI.loadXml` указывается относительно этой папки.

### Поддерживаемые теги

| Тег | Атрибуты | Описание |
|---|---|---|
| `<UI>` | — | Корневой контейнер (игнорируется) |
| `<Panel>` | `color`, + общие | Панель-контейнер |
| `<Label>` | `text`, `color`, + общие | Текстовый элемент |
| `<Button>` | `text`, `onClick`, `textColor`, `color`, + общие | Кнопка |
| `<Input>` | `placeholder`, `onSubmit`, + общие | Поле ввода |
| `<Slider>` | `min`, `max`, `value`, `onChange`, + общие | Слайдер |
| `<DropDown>` | `options`, `onSelect`, + общие | Выпадающий список |
| `<Toggle>` | `checked`, `label`, `onChange`, + общие | Переключатель |

### Общие атрибуты

| Атрибут | Тип | По умолчанию |
|---|---|---|
| `id` | `string` | — |
| `x`, `y` | `number` | `0` |
| `width`, `height` | `number` | `100`, `40` |
| `fontSize` | `number` | `24` |
| `zOrder` / `z` | `integer` | `0` |
| `visible` | `"true"`/`"false"` | `true` |
| `color` | `"r,g,b[,a]"` | зависит от элемента |
| `textColor` | `"r,g,b[,a]"` | `"1,1,1,1"` |

> 🎨 Цвета — значения от `0` до `1` через запятую: `"1,0.5,0,1"`.

### Пример XML

```xml
<UI>
    <Panel id="menu" x="100" y="100" width="400" height="300" color="0.1,0.1,0.15,0.9">
        <Label id="title" x="20" y="20" width="360" height="60"
               text="Главное меню" fontSize="32" color="1,1,1,1"/>

        <Button id="btn_start" x="20" y="100" width="360" height="50"
                text="Играть" onClick="start"
                color="0.2,0.4,0.8,1" textColor="1,1,1,1"/>

        <Button id="btn_quit" x="20" y="170" width="360" height="50"
                text="Выход" onClick="quit"
                color="0.5,0.2,0.2,1"/>

        <Toggle id="sounds" x="20" y="240" width="200" height="40"
                checked="true" label="Звук" onChange="toggle_sound"/>

        <Slider id="volume" x="20" y="290" width="360" height="30"
                min="0" max="1" value="0.8" onChange="set_volume"/>

        <DropDown id="quality" x="20" y="330" width="360" height="40"
                  options="Low,Medium,High,Ultra" onSelect="set_quality"/>

        <Input id="name" x="20" y="380" width="360" height="40"
               placeholder="Введите имя..." onSubmit="set_name"/>
    </Panel>
</UI>
```

### Обработчики событий в Lua

```lua
UI.loadXml("main_menu.xml", {
    start          = function() Scenes.load("Game") end,
    quit           = function() Application.quit() end,
    toggle_sound   = function(state) print("Звук:", state) end,
    set_volume     = function(value) print("Громкость:", value) end,
    set_quality    = function(idx, name) print("Качество:", idx, name) end,
    set_name       = function(text) print("Имя:", text) end,
})
```

---

## ⌨ Input — ввод

### Функции

| Функция | Возвращает | Описание |
|---|---|---|
| `Input.action(name)` | `boolean` | Действие зажато |
| `Input.actionDown(name)` | `boolean` | Действие нажато в этом кадре |
| `Input.actionUp(name)` | `boolean` | Действие отпущено в этом кадре |
| `Input.getKey(name)` | `boolean` | Клавиша зажата |
| `Input.getKeyDown(name)` | `boolean` | Клавиша нажата в этом кадре |
| `Input.getKeyUp(name)` | `boolean` | Клавиша отпущена в этом кадре |
| `Input.consumeEscape()` | `boolean` | Поглотить Escape |
| `Input.axis(pos, neg)` | `number` | Ось от `-1` до `1` |
| `Input.setUIActive(bool)` | — | Активировать режим UI |
| `Input.isUIActive()` | `boolean` | Активен ли режим UI |

### Пример: движение WASD

```lua
function Update(self, dt)
    local moveX = Input.axis("Right", "Left")
    local moveZ = Input.axis("Forward", "Back")

    local pos = self.transform.position
    pos.x = pos.x + moveX * dt * 5.0
    pos.z = pos.z + moveZ * dt * 5.0
    self.transform.position = pos
end
```

### Пример: переход в игровой режим

```lua
function Start(self)
    Input.setUIActive(true)   -- меню активно
end

function OnPlayClicked()
    Input.setUIActive(false)  -- игровой режим
    Scenes.load("Game")
end
```

> 💡 Пока активно UI или открыта консоль, курсор остаётся видимым, а игровой ввод блокируется.

---

## ⏱ Time — время

### Свойства

| Свойство | Тип | Описание |
|---|---|---|
| `Time.deltaTime` | `number` | Время между кадрами (с учётом `timeScale`) |
| `Time.unscaledDeltaTime` | `number` | Время между кадрами (без `timeScale`) |
| `Time.time` | `number` | Общее время с запуска |
| `Time.timeScale` | `number` | Множитель времени |

### Методы

| Метод | Описание |
|---|---|
| `Time.setTimeScale(value)` | Установить масштаб времени |

### Пример: пауза

```lua
function Update(self, dt)
    if Input.actionDown("Pause") then
        if Time.timeScale == 0 then
            Time.setTimeScale(1.0)
        else
            Time.setTimeScale(0.0)
        end
    end
end
```

### Пример: анимация без учёта паузы

```lua
function Update(self, dt)
    -- Крутим что-то, даже когда игра на паузе
    local rot = self.transform.rotation
    rot.y = rot.y + Time.unscaledDeltaTime * 45
    self.transform.rotation = rot
end
```

---

## 🎥 Renderer — рендер

| Функция | Возвращает | Описание |
|---|---|---|
| `Renderer.setSkybox(path, res?)` | `boolean` | Установить скайбокс (по умолчанию `res = 1024`) |

### Пример

```lua
function Start(self)
    Renderer.setSkybox("textures/skyboxes/sunset.hdr", 2048)
end
```

---

## ⚙ Settings и DisplaySettings

Глобальные singleton-настройки. Доступ через `Settings.get()` и `DisplaySettings.get()`.

### Settings

| Свойство | Тип |
|---|---|
| `debugMode` | `boolean` |
| `showFPS` | `boolean` |
| `wireframeMode` | `boolean` |
| `msaaSamples` | `integer` |
| `shadowQuality` | `ShadowQuality` |
| `textureFiltering` | `TextureFiltering` |
| `enableShadows` | `boolean` |
| `fov`, `nearPlane`, `farPlane` | `number` |
| `enableBloom`, `enableFXAA`, `enableSSAO` | `boolean` |
| `gamma`, `exposure` | `number` |
| `masterVolume`, `musicVolume`, `sfxVolume` | `number` |
| `muteAudio` | `boolean` |

**Методы:** `apply(path?)`, `resetToDefaults()`, `saveToFile(path?)`, `loadFromFile(path?)`.

### DisplaySettings

| Свойство | Тип |
|---|---|
| `windowWidth`, `windowHeight` | `integer` |
| `fullscreen`, `borderless`, `vsync` | `boolean` |
| `targetFPS` | `integer` |
| `useFixedTimestep` | `boolean` |
| `fixedTimestep` | `number` |

**Методы:** `getAspectRatio()`, `apply(path?)`, `saveToFile(path?)`, `loadFromFile(path?)`.

### Enums

```lua
ShadowQuality = { Off, Low, Medium, High, Ultra }

TextureFiltering = {
    Bilinear, Trilinear,
    Anisotropic2x, Anisotropic8x, Anisotropic16x
}
```

### Пример: меню настроек

```lua
local s = Settings.get()

s.showFPS = true
s.enableBloom = true
s.shadowQuality = ShadowQuality.High
s.textureFiltering = TextureFiltering.Anisotropic16x
s.masterVolume = 0.7
s:apply()

local ds = DisplaySettings.get()
ds.fullscreen = true
ds.vsync = true
ds.targetFPS = 144
ds:apply()

print("Aspect:", ds:getAspectRatio())
```

### Пример: сохранение и загрузка

```lua
function OnSaveClicked()
    Settings.get():saveToFile()
    DisplaySettings.get():saveToFile()
end

function OnLoadClicked()
    Settings.get():loadFromFile()
    DisplaySettings.get():loadFromFile()
    Settings.get():apply()
    DisplaySettings.get():apply()
end
```

---

## 🗂 LayerMask

Глобальная таблица для работы со слоями.

| Функция | Возвращает | Описание |
|---|---|---|
| `LayerMask.getMask(...)` | `integer` | Битовая маска по именам |
| `LayerMask.nameToLayer(name)` | `integer` | Индекс слоя по имени |
| `LayerMask.layerToName(layer)` | `string` | Имя слоя по индексу |

### Пример

```lua
-- Маска для рейкаста по игроку и врагам
local mask = LayerMask.getMask("Player", "Enemies")
print("Mask:", mask)

-- Узнать слой игрока
self.layerName = "Player"
print("Layer index:", LayerMask.nameToLayer("Player"))
print("Layer name:", LayerMask.layerToName(self.layer))
```

---

## 🚪 Application

| Функция | Описание |
|---|---|
| `Application.quit()` | Закрывает окно и завершает работу движка |

### Пример

```lua
UI.loadXml("main_menu.xml", {
    quit = function()
        print("Выходим...")
        Application.quit()
    end,
})
```

---

## 💻 Консольные команды

Встроенная консоль поддерживает следующие команды:

| Команда | Описание |
|---|---|
| `scene_list` | Список зарегистрированных сцен (`*` — активная) |
| `scene_current` | Имя текущей сцены |
| `scene_load <name>` | Загрузить сцену по имени |
| `pause [0\|1]` | Пауза (`1`) / продолжить (`0`) |

**Tab-автодополнение:**
- Фильтрует по первому слову команды
- Циклически перебирает только подходящие команды
- Сбрасывается при редактировании, навигации или вводе аргументов

---

## 📚 Примеры

### 🌀 Rotator — вращение объекта

```lua
-- res/scripts/rotator.lua
local speed = 90.0

function Start(self)
    print("Rotator started on " .. self.name)
end

function Update(self, dt)
    local rot = self.transform.rotation
    rot.y = rot.y + speed * dt
    self.transform.rotation = rot
end

function OnDestroy(self)
    print("Rotator destroyed")
end
```

### 🏃 Player Controller

```lua
local SPEED = 6.0
local JUMP_FORCE = 8.0

function Start(self)
    self:addComponent("RigidBody")
    self:addComponent("CapsuleCollider")

    local rb = self:getRigidBody()
    rb.mass = 1.0
    rb.friction = 0.1
    rb.restitution = 0.0
end

function Update(self, dt)
    local rb = self:getRigidBody()
    if not rb then return end

    -- Горизонтальное движение
    local mx = Input.axis("Right", "Left")
    local mz = Input.axis("Forward", "Back")
    local v = rb.velocity
    rb.velocity = Vector3(mx * SPEED, v.y, mz * SPEED)

    -- Прыжок
    if Input.actionDown("Jump") and rb.isGrounded then
        rb:applyImpulse(Vector3(0, JUMP_FORCE, 0))
    end
end
```

### 🎬 Bootstrap — инициализация сцен

```lua
-- res/scripts/bootstrap.lua

Scenes.register("Menu",    "menu_scene.lua")
Scenes.register("Game",    "game_scene.lua")
Scenes.register("Credits", "credits_scene.lua")

Renderer.setSkybox("textures/skyboxes/default.hdr", 1024)
Scenes.load("Menu")

print("[bootstrap] Scenes registered")
```

### 🖥 Menu Scene

```lua
-- res/scripts/menu_scene.lua

function OnCreate(scene)
    local camera = scene:create("MenuCamera")
    camera:addComponent("Camera")
    camera.transform.position = Vector3(0, 0, 5)

    Input.setUIActive(true)

    UI.loadXml("main_menu.xml", {
        play  = function()
            Input.setUIActive(false)
            Scenes.load("Game")
        end,
        quit  = function()
            Application.quit()
        end,
    })
end

function Update(scene, dt)
    -- Анимация камеры
    local cam = scene:find("MenuCamera")
    if cam then
        local p = cam.transform.position
        p.x = math.sin(Time.time * 0.5) * 0.5
        cam.transform.position = p
    end
end
```

### 🧪 Procedural scene

```lua
function OnCreate(scene)
    -- Пол
    local ground = scene:create("Ground")
    ground:addComponent("MeshRenderer")
    ground:setModel("models/plane.gltf")
    ground:setMaterialColor(Vector3(0.3, 0.5, 0.2))
    ground:addComponent("MeshCollider")
    ground:fitCollider()

    -- Ряд кубов
    for i = 1, 5 do
        local cube = scene:create("Cube_" .. i)
        cube:addComponent("MeshRenderer")
        cube:addComponent("BoxCollider")
        cube:setModel("models/cube.gltf")
        cube:setMaterialColor(Vector3(i / 5, 0.5, 1.0 - i / 5))
        cube.transform.position = Vector3((i - 3) * 2, 0.5, 0)
        cube:addComponent("RigidBody")

        cube:fitCollider()
    end

    -- Свет
    local light = scene:create("SunLight")
    light:addComponent("DirectionalLight")
    light.transform.rotation = Vector3(-45, 30, 0)
end
```

### 🎨 Динамический UI

```lua
local score = 0

function Start(self)
    Input.setUIActive(true)

    UI.loadXml("hud.xml", {
        restart = function()
            score = 0
            UI.setText("score", "Счёт: 0")
        end,
    })

    UI.setText("score", "Счёт: 0")
end

function Update(self, dt)
    if Input.actionDown("Score") then
        score = score + 1
        UI.setText("score", "Счёт: " .. score)
    end
end
```

### ⚙ Меню настроек с сохранением

```lua
local function applySettings()
    local s = Settings.get()
    s:apply()
    DisplaySettings.get():apply()
end

UI.loadXml("settings.xml", {
    toggle_fps = function(state)
        Settings.get().showFPS = state
        applySettings()
    end,

    set_shadows = function(idx, name)
        Settings.get().shadowQuality = ShadowQuality[name]
        applySettings()
    end,

    set_volume = function(v)
        Settings.get().masterVolume = v
        applySettings()
    end,

    save = function()
        Settings.get():saveToFile()
        DisplaySettings.get():saveToFile()
        print("Настройки сохранены")
    end,

    reset = function()
        Settings.get():resetToDefaults()
        DisplaySettings.get():resetToDefaults()
        applySettings()
        print("Настройки сброшены")
    end,
})
```

### 🌍 Смена скайбокса и сцены

```lua
UI.loadXml("level_select.xml", {
    desert = function()
        Renderer.setSkybox("textures/skyboxes/desert.hdr", 2048)
        Scenes.load("DesertLevel")
    end,
    night = function()
        Renderer.setSkybox("textures/skyboxes/night.hdr", 2048)
        Scenes.load("NightLevel")
    end,
    space = function()
        Renderer.setSkybox("textures/skyboxes/space.hdr", 2048)
        Scenes.load("SpaceLevel")
    end,
})
```

---

## ⚠ Обработка ошибок

Ошибки в Lua-скриптах **не крашат движок** и не завершают сцену. Они логируются через движковый логгер (`LOG_ERROR`).

Пример безопасного колбэка кнопки:

```lua
-- Внутри C++ уже обёрнуто в sol::protected_function
-- Если ваш callback бросит ошибку — она попадёт в лог, но игра продолжит работу
UI.loadXml("menu.xml", {
    boom = function()
        error("Что-то пошло не так")
    end,
})
-- В логе: [Lua UI] Button callback failed: Что-то пошло не так
```

### Рекомендации

- Всегда проверяйте `nil` перед вызовом методов:
  ```lua
  local rb = self:getRigidBody()
  if rb then rb:applyImpulse(...) end
  ```
- Используйте `pcall` для рискованных операций:
  ```lua
  local ok, err = pcall(function()
      -- что-то рискованное
  end)
  if not ok then print("Ошибка: " .. tostring(err)) end
  ```
- Логируйте через `print` — он перенаправляется в движковый логгер.

---

## 🗺 Карта API

```
Application     -> quit
Renderer        -> setSkybox
Scenes          -> current, load, register, setSkybox
Settings        -> get, apply, saveToFile, loadFromFile, resetToDefaults
DisplaySettings -> get, getAspectRatio, apply, saveToFile, loadFromFile
LayerMask       -> getMask, nameToLayer, layerToName
Input           -> action, actionDown, actionUp,
                   getKey, getKeyDown, getKeyUp, consumeEscape,
                   axis, setUIActive, isUIActive
UI              -> root, createPanel, createLabel, createButton,
                   clear, loadXml, addXml,
                   setText, setVisible, setChecked, isChecked, getValue
```

---

> 📝 **Файл аннотаций.** Для автодополнения в Lua Language Server используйте `LuaApi.lua` (директива `---@meta`). Он синхронизирован с `LuaApi.cpp`.

> 🔗 **См. также:** `res/scripts/bootstrap.lua`, `res/scripts/menu_scene.lua`, `res/ui/layouts/*.xml`