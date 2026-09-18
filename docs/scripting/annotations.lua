---@meta
---=============================================================================
--- Lindo Engine Lua API Annotations for Lua Language Server (LLS)
--- Автоматически синхронизировано с LuaApi.cpp
---=============================================================================

--------------------------------------------------------------------------------
--- GLM / Math Types
--------------------------------------------------------------------------------

---@class vec3
---@field x number
---@field y number
---@field z number

---@class quat
---@field x number
---@field y number
---@field z number
---@field w number

---@class Transform
---@field position vec3
---@field rotation vec3 Мировые углы Эйлера (в движке хранится как кватернион)
---@field scale vec3

--------------------------------------------------------------------------------
--- Enums
--------------------------------------------------------------------------------

---@alias ShadowQuality
---| "Off"
---| "Low"
---| "Medium"
---| "High"
---| "Ultra"

---@alias TextureFiltering
---| "Bilinear"
---| "Trilinear"
---| "Anisotropic2x"
---| "Anisotropic8x"
---| "Anisotropic16x"

--------------------------------------------------------------------------------
--- Settings
--------------------------------------------------------------------------------

---@class Settings
---@field debugMode boolean
---@field showFPS boolean
---@field wireframeMode boolean
---@field msaaSamples integer
---@field shadowQuality ShadowQuality
---@field textureFiltering TextureFiltering
---@field enableShadows boolean
---@field fov number
---@field nearPlane number
---@field farPlane number
---@field enableBloom boolean
---@field enableFXAA boolean
---@field enableSSAO boolean
---@field gamma number
---@field exposure number
---@field masterVolume number
---@field musicVolume number
---@field sfxVolume number
---@field muteAudio boolean
local Settings = {}

---Применяет настройки (опционально — из указанного пути)
---@param path? string
function Settings:apply(path) end

---Сбрасывает настройки на значения по умолчанию
function Settings:resetToDefaults() end

---Сохраняет настройки в файл
---@param path? string
function Settings:saveToFile(path) end

---Загружает настройки из файла
---@param path? string
function Settings:loadFromFile(path) end

---Возвращает singleton-экземпляр настроек
---@return Settings
function Settings.get() end


---@class DisplaySettings
---@field windowWidth integer
---@field windowHeight integer
---@field fullscreen boolean
---@field borderless boolean
---@field vsync boolean
---@field targetFPS integer
---@field useFixedTimestep boolean
---@field fixedTimestep number
local DisplaySettings = {}

---@return number
function DisplaySettings:getAspectRatio() end

---@param path? string
function DisplaySettings:apply(path) end

---@param path? string
function DisplaySettings:saveToFile(path) end

---@param path? string
function DisplaySettings:loadFromFile(path) end

---@return DisplaySettings
function DisplaySettings.get() end

--------------------------------------------------------------------------------
--- Core Scene Types
--------------------------------------------------------------------------------

---@class GameObject
---@field name string Имя игрового объекта
---@field tag string Тег объекта
---@field layer integer Числовой слой
---@field layerName string Имя слоя (get/set по имени)
---@field active boolean Флаг активности объекта
---@field transform Transform Трансформ объекта
local GameObject = {}

---Возвращает мировую позицию объекта
---@return vec3
function GameObject:getWorldPosition() end

---Устанавливает мировую позицию объекта
---@param position vec3
function GameObject:setWorldPosition(position) end

---Проверяет, совпадает ли тег объекта с указанным
---@param tag string
---@return boolean
function GameObject:compareTag(tag) end

---Добавляет компонент к объекту по типу
---@param type "Player"|"Camera"|"MeshRenderer"|"Material"|"BoxCollider"|"SphereCollider"|"CapsuleCollider"|"MeshCollider"|"DirectionalLight"|"RigidBody"
function GameObject:addComponent(type) end

---Возвращает RigidBody-компонент, если он есть
---@return RigidBody|nil
function GameObject:getRigidBody() end

---Загружает и устанавливает 3D-модель для MeshRenderer
---@param path string Путь к файлу модели
function GameObject:setModel(path) end

---Устанавливает цвет материала
---@param color vec3 RGB цвет (от 0.0 до 1.0)
function GameObject:setMaterialColor(color) end

---Подгоняет коллайдеры объекта под габариты AABB модели
function GameObject:fitCollider() end


---@class Scene
---@field name string Название сцены
---@field active boolean Флаг активности сцены
local Scene = {}

---Создаёт новый GameObject на сцене
---@param name string Имя создаваемого объекта
---@return GameObject
function Scene:create(name) end

---Ищет GameObject по имени
---@param name string
---@return GameObject|nil
function Scene:find(name) end

---Удаляет GameObject по имени
---@param name string
function Scene:destroy(name) end

---@class LuaScene : Scene

--------------------------------------------------------------------------------
--- RigidBody (Physics)
--------------------------------------------------------------------------------

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
local RigidBody = {}

---@param force vec3
function RigidBody:applyForce(force) end

---@param force vec3
---@param point vec3
function RigidBody:applyForceAtPoint(force, point) end

---@param impulse vec3
function RigidBody:applyImpulse(impulse) end

---@param impulse vec3
---@param point vec3
function RigidBody:applyImpulseAtPoint(impulse, point) end

---@param torque vec3
function RigidBody:applyTorque(torque) end

function RigidBody:clearForces() end

function RigidBody:wake() end

--------------------------------------------------------------------------------
--- LayerMask
--------------------------------------------------------------------------------

---@class LayerMask
LayerMask = {}

---Возвращает битовую маску по именам слоёв
---@param ... string
---@return integer
function LayerMask.getMask(...) end

---@param name string
---@return integer
function LayerMask.nameToLayer(name) end

---@param layer integer
---@return string
function LayerMask.layerToName(layer) end

--------------------------------------------------------------------------------
--- UI Components & API
--------------------------------------------------------------------------------

---@class UIWidget
---@field tag string
---@field layer integer
---@field layerName string
---@field zOrder integer
---@field visible boolean
local UIWidget = {}

---Устанавливает позицию виджета
---@param x number
---@param y number
function UIWidget:setPosition(x, y) end

---Устанавливает размер виджета
---@param width number
---@param height number
function UIWidget:setSize(width, height) end

---Управляет видимостью виджета
---@param visible boolean
function UIWidget:setVisible(visible) end

---Проверяет, совпадает ли тег виджета
---@param tag string
---@return boolean
function UIWidget:compareTag(tag) end


---@class UIPanel : UIWidget
local UIPanel = {}

---Добавляет дочерний виджет в панель
---@param child UIWidget
function UIPanel:addChild(child) end


---@class UILabel : UIWidget
local UILabel = {}

---Устанавливает текст метки
---@param text string
function UILabel:setText(text) end


---@class UIButton : UIWidget
local UIButton = {}


---@class UITextInput : UIWidget
local UITextInput = {}

---@param text string
function UITextInput:setText(text) end

---@return string
function UITextInput:getText() end


---@class UISlider : UIWidget
local UISlider = {}

---@param value number
function UISlider:setValue(value) end

---@return number
function UISlider:getValue() end


---@class UIDropDown : UIWidget
local UIDropDown = {}

---@return integer
function UIDropDown:getSelectedIndex() end

---@return string
function UIDropDown:getSelectedOption() end

---@param index integer
function UIDropDown:setSelectedIndex(index) end


---@class UIToggle : UIWidget
local UIToggle = {}

---@param checked boolean
function UIToggle:setChecked(checked) end

---@return boolean
function UIToggle:isChecked() end

---@param label string
function UIToggle:setLabel(label) end

---@return string
function UIToggle:getLabel() end


--- Модуль работы с пользовательским интерфейсом
UI = {}

---Создаёт новую панель
---@return UIPanel
function UI.createPanel() end

---Создаёт текстовую метку
---@param text string
---@return UILabel
function UI.createLabel(text) end

---Создаёт кнопку с функцией обратного вызова
---@param text string
---@param callback fun()
---@return UIButton
function UI.createButton(text, callback) end

---Возвращает корневую панель интерфейса
---@return UIPanel|nil
function UI.root() end

---Очищает динамический интерфейс (сохраняя постоянные элементы)
function UI.clear() end

---Загружает XML-разметку с полной очисткой текущих динамических виджетов
---@param path string Путь к XML
---@param handlers table<string, fun()> Таблица обработчиков событий (onClick)
---@return boolean
function UI.loadXml(path, handlers) end

---Добавляет XML-разметку без полной очистки (для оверлеев/панелей)
---@param path string Путь к XML
---@param handlers table<string, fun()> Таблица обработчиков событий
---@return boolean
function UI.addXml(path, handlers) end

---Устанавливает текст элемента по его ID в XML
---@param id string
---@param text string
function UI.setText(id, text) end

---Управляет видимостью элемента по его ID в XML
---@param id string
---@param visible boolean
function UI.setVisible(id, visible) end

---Устанавливает состояние Toggle по ID
---@param id string
---@param checked boolean
function UI.setChecked(id, checked) end

---Возвращает состояние Toggle по ID
---@param id string
---@return boolean
function UI.isChecked(id) end

---Возвращает значение виджета по ID (Slider/Input/DropDown/Toggle)
---@param id string
---@return number|string|boolean|nil
function UI.getValue(id) end

--------------------------------------------------------------------------------
--- Input API
--------------------------------------------------------------------------------

--- Модуль управления вводом
Input = {}

---Проверяет, зажата ли клавиша/действие
---@param name string
---@return boolean
function Input.action(name) end

---Проверяет, было ли нажатие действия в текущем кадре
---@param name string
---@return boolean
function Input.actionDown(name) end

---Проверяет, было ли отпущено действие в текущем кадре
---@param name string
---@return boolean
function Input.actionUp(name) end

---Проверяет, зажата ли клавиша по имени
---@param name string
---@return boolean
function Input.getKey(name) end

---Проверяет, была ли клавиша нажата в текущем кадре
---@param name string
---@return boolean
function Input.getKeyDown(name) end

---Проверяет, была ли клавиша отпущена в текущем кадре
---@param name string
---@return boolean
function Input.getKeyUp(name) end

---Потребляет событие Escape (например, чтобы не закрывать окно)
---@return boolean
function Input.consumeEscape() end

---Возвращает значение оси между двумя действиями (от -1.0 до 1.0)
---@param positive string
---@param negative string
---@return number
function Input.axis(positive, negative) end

---Переключает режим активности UI для ввода
---@param active boolean
function Input.setUIActive(active) end

---Проверяет, активен ли ввод для UI
---@return boolean
function Input.isUIActive() end

--------------------------------------------------------------------------------
--- Renderer
--------------------------------------------------------------------------------

--- Модуль управления рендерингом
Renderer = {}

---Устанавливает скайбокс
---@param path string
---@param resolution? integer По умолчанию 1024
---@return boolean
function Renderer.setSkybox(path, resolution) end

--------------------------------------------------------------------------------
--- Scenes & Application API
--------------------------------------------------------------------------------

--- Управление сценами движка
Scenes = {}

---Возвращает текущую активную сцену
---@return Scene|nil
function Scenes.current() end

---Запрашивает загрузку сцены по имени
---@param name string
function Scenes.load(name) end

---Регистрирует фабрику сцены из Lua-скрипта
---@param name string Имя сцены
---@param script string Путь к Lua-скрипту сцены
function Scenes.register(name, script) end

---Устанавливает скайбокс (дублирует Renderer.setSkybox)
---@param path string
---@param resolution? integer
---@return boolean
function Scenes.setSkybox(path, resolution) end


--- Управление приложением
Application = {}

---Завершает работу приложения и закрывает окно
function Application.quit() end

--------------------------------------------------------------------------------
--- XML-разметка UI (для UI.loadXml / UI.addXml)
--------------------------------------------------------------------------------
--- Поддерживаемые теги:
---   <UI>           корневой контейнер (игнорируется)
---   <Panel>        панель-контейнер
---   <Label>        текстовая метка
---   <Button>       кнопка (onClick="handlerName")
---   <Input>        поле ввода (placeholder, onSubmit="handlerName")
---   <Slider>       слайдер (min, max, value, onChange="handlerName")
---   <DropDown>     выпадающий список (options="a,b,c", onSelect="handlerName")
---   <Toggle>       переключатель (checked="true|false", label, onChange="handlerName")
---
--- Общие атрибуты: id, x, y, width, height, fontSize, zOrder/z, visible,
---                  color="r,g,b[,a]", textColor, text
--------------------------------------------------------------------------------