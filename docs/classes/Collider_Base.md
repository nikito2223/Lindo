# Collider (Базовый класс)

**Файлы:**
- [src/Physics/Collider/Collider.h](../../src/Physics/Collider/Collider.h)
- [src/Physics/Collider/Collider.cpp](../../src/Physics/Collider/Collider.cpp)

## Назначение
Базовый класс для всех коллайдеров в физической системе. Отвечает за:
- **Регистрацию в физической системе** — при старте компоненты регистрируются в `PhysicsSystem`, при уничтожении — удаляются.
- **Хранение физических свойств** — плотность (density), трение (friction), упругость (restitution).
- **Управление коллизиями и триггерами** — отслеживание Enter/Stay/Exit событий.
- **Callback-система** — регистрация функций обратного вызова для коллизий и триггеров.
- **Трансформация в мировых координатах** — преобразование локального пространства коллайдера в мировое (через gameObject).

## Ключевые компоненты

### Структуры

**PhysicsMaterial**
```cpp
struct PhysicsMaterial {
    float density = 1.0f;      // Масштаб массы
    float friction = 0.5f;     // Трение [0, 1]
    float restitution = 0.2f;  // Упругость/прыгучесть [0, 1]
};
```

**CollisionInfo**
```cpp
struct CollisionInfo {
    Collider* other = nullptr;           // Второй коллайдер
    glm::vec3 contactPoint;               // Точка контакта
    glm::vec3 contactNormal;              // Нормаль контакта
    float penetrationDepth = 0.0f;        // Глубина проникновения
    float relativeVelocity = 0.0f;        // Относительная скорость
};
```

### Основные поля

| Поле | Тип | Описание |
|------|-----|---------|
| `isTrigger` | bool | Является ли триггером (не физический, только уведомления) |
| `isEnabled` | bool | Включен ли коллайдер |
| `offset` | vec3 | Локальное смещение относительно позиции gameObject |
| `colliderScale` | vec3 | Локальный масштаб коллайдера |
| `usegameObjectScale` | bool | Использовать масштаб gameObject или свой |
| `material` | PhysicsMaterial | Физические свойства материала |
| `currentCollisions` | vector<Collider*> | Активные коллизии (для отслеживания Enter/Exit) |
| `currentTriggers` | vector<Collider*> | Активные триггеры |

### Жизненный цикл

```
GameObject создается
    ↓
Component.OnStart() → PhysicsSystem.RegisterCollider(this)
    ↓
Физическая система каждый frame:
  1. Broad-phase (AABB тесты)
  2. Narrow-phase (точные коллизии через CheckCollision)
  3. Вызов callbacks Enter/Stay/Exit
    ↓
Component.OnDestroy() → PhysicsSystem.UnregisterCollider(this)
```

### Основные методы

**Конфигурация поведения:**
```cpp
void SetTrigger(bool trigger);      // Сделать триггером
bool IsTrigger() const;
void SetEnabled(bool enabled);      // Включить/отключить
bool IsEnabled() const;
```

**Локальное смещение:**
```cpp
void SetOffset(const glm::vec3& newOffset);
glm::vec3 GetOffset() const;
```

**Масштабирование:**
```cpp
void SetColliderScale(const glm::vec3& scale);   // Свой масштаб
glm::vec3 GetColliderScale() const;
void SetUsegameObjectScale(bool use);                 // Использовать gameObject scale
```

**Физические свойства:**
```cpp
void SetFriction(float friction);        // [0, 1]
float GetFriction() const;
void SetRestitution(float restitution);  // [0, 1]
float GetRestitution() const;
void SetDensity(float density);
float GetDensity() const;
```

**Мировые координаты:**
```cpp
glm::vec3 GetWorldPosition() const;     // gameObject.position + offset
glm::vec3 GetWorldCenter() const;       // То же (может быть переопределено)
glm::vec3 GetWorldScale() const;        // gameObject.scale * colliderScale или colliderScale
```

**Callback система:**
```cpp
// Для обычных коллизий
void SetCollisionCallback(CollisionEvent event, std::function<void(const CollisionInfo&)> callback);

// Для триггеров
void SetTriggerCallback(CollisionEvent event, std::function<void(Collider*)> callback);
```

Где `CollisionEvent`:
- `CollisionEvent::Enter` — первый контакт
- `CollisionEvent::Stay` — сохранение контакта
- `CollisionEvent::Exit` — конец контакта

### Виртуальные методы (для наследования)

```cpp
// Должны быть реализованы в подклассах
virtual bool CheckCollision(Collider* other, CollisionInfo& outInfo) const;
virtual Lindo::Math::AABB GetAABB() const;
virtual void OnDrawGizmos();  // Отрисовка debug-линий
```

## Поток обработки коллизий

1. **Регистрация**: При `OnStart()` коллайдер добавляется в `PhysicsSystem::m_colliders`.
2. **Broad-phase**: Физическая система проверяет пересечение AABB всех коллайдеров.
3. **Narrow-phase**: Для пар с пересекающимися AABB вызывается `CheckCollision()`.
4. **Event tracking**: Сравнивается `currentCollisions` с новыми коллизиями:
   - Новые → вызов `OnCollisionEnter()` / `OnTriggerEnter()`
   - Продолжающиеся → вызов `OnCollisionStay()` / `OnTriggerStay()`
   - Закончившиеся → вызов `OnCollisionExit()` / `OnTriggerExit()`

## Как расширить (написать свой коллайдер)

1. Наследуйте от `Collider`.
2. Реализуйте:
   - `GetAABB()` — вернуть ограничивающий прямоугольник для broad-phase.
   - `CheckCollision(Collider*, CollisionInfo&)` — полиморфный dispatch на другие типы.
   - Перегрузки `CheckCollision()` для каждого типа коллайдера, который может сталкиваться с вашим.
   - `OnDrawGizmos()` — отрисовка debug-линий (используется в DebugDraw).

## Советы при использовании

- **Для static объектов**: используйте `SetEnabled(false)` после инициализации или установите коллайдер в сцене как непокидаемый.
- **Для триггеров**: установите `SetTrigger(true)` — коллайдер перестанет участвовать в физике, только уведомлять о пересечениях.
- **Для производительности**: группируйте коллайдеры по типам (static vs dynamic), так как физическая система может оптимизировать broad-phase.
- **Для сложных mesh-объектов**: используйте `MeshCollider` вместо ручного создания множества коллайдеров.

## Взаимосвязь с другими классами

- **PhysicsSystem** — регистрирует коллайдеры, управляет broad-phase/narrow-phase, вызывает callbacks.
- **GameObject** — содержит коллайдер как компонент, предоставляет трансформацию.
- **AABB** — используется для broad-phase и отладки.
- **Подклассы** (BoxCollider, SphereCollider, CapsuleCollider, MeshCollider) — специализированные реализации для разных форм.
