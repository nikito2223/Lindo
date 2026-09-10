---@meta
---=============================================================================
--- Lindo Engine Lua API Annotations for Lua Language Server (LLS)
---=============================================================================

--------------------------------------------------------------------------------
--- GLM / Math Types
--------------------------------------------------------------------------------

---@class vec3
---@field x number
---@field y number
---@field z number

---@class Transform
---@field position vec3
---@field rotation vec3
---@field scale vec3

--------------------------------------------------------------------------------
--- Core Scene Types
--------------------------------------------------------------------------------

---@class GameObject
---@field name string Имя игрового объекта
---@field tag string Тег объекта
---@field active boolean Флаг активности объекта
---@field transform Transform Трансформ объекта
local GameObject = {}

---Возвращает мировую позицию объекта
---@return vec3
function GameObject:getWorldPosition() end

---Устанавливает мировую позицию объекта
---@param position vec3
function GameObject:setWorldPosition(position) end

---Добавляет компонент к объекту по типу
---@param type "Player"|"Camera"|"MeshRenderer"|"Material"|"BoxCollider"|"SphereCollider"|"CapsuleCollider"|"MeshCollider"|"DirectionalLight"
function GameObject:addComponent(type) end

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

---Создает новый GameObject на сцене
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
--- UI Components & API
--------------------------------------------------------------------------------

---@class UIWidget
---@field visible boolean Видимость виджета
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


--- Модуль работы с пользовательским интерфейсом
UI = {}

---Создает новую панель
---@return UIPanel
function UI.createPanel() end

---Создает текстовую метку
---@param text string
---@return UILabel
function UI.createLabel(text) end

---Создает кнопку с функцией обратного вызова
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


--- Управление приложением
Application = {}

---Завершает работу приложения и закрывает окно
function Application.quit() end