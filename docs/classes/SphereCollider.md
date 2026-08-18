# SphereCollider (Сферический коллайдер)

**Файлы:**
- [src/Physics/Collider/SphereCollider.h](../../src/Physics/Collider/SphereCollider.h)
- [src/Physics/Collider/SphereCollider.cpp](../../src/Physics/Collider/SphereCollider.cpp)

## Назначение
Коллайдер в форме сферы. Один из самых простых и быстрых типов коллайдеров. Идеален для:
- **Шариков и округлых объектов** — натуральная форма.
- **Диапазонных триггеров** — быстрая проверка пересечения с другими формами.
- **Игроков** — часто используется как простой коллайдер для персонажа.

## Ключевые компоненты

### Основные поля

| Поле | Тип | Описание |
|------|-----|---------|
| `radius` | float | Локальный радиус сферы (скалируется мировым масштабом) |

### Основные методы

**Конфигурация:**
```cpp
void SetRadius(float newRadius);      // Установить радиус (гарантирует мин 0.0001)
float GetRadius() const;              // Получить локальный радиус
```

**Мировые координаты:**
```cpp
float GetWorldRadius() const override;        // Мировой радиус (с учетом gameObject scale)
glm::vec3 GetWorldCenter() const override;    // Центр сферы (gameObject.position + offset)
Lindo::Math::AABB GetAABB() const override;   // Axis-aligned bounding box
```

**Коллизии:**
```cpp
// Полиморфный dispatch
bool CheckCollision(Collider* other, CollisionInfo& outInfo) const override;

// Специализированные реализации
bool CheckCollision(const BoxCollider* other, CollisionInfo& outInfo) const;
bool CheckCollision(const SphereCollider* other, CollisionInfo& outInfo) const;
bool CheckCollision(const CapsuleCollider* other, CollisionInfo& outInfo) const;
```

## Алгоритмы коллизии

### Sphere vs Sphere

**Принцип**: две сферы пересекаются если расстояние между центрами < суммы радиусов.

**Реализация**:
```cpp
glm::vec3 delta = centerB - centerA;
float distSq = dot(delta, delta);
float radiusSum = radiusA + radiusB;

if (distSq > radiusSum * radiusSum) {
    return false;  // Нет пересечения
}

float dist = sqrt(distSq);
glm::vec3 normal = (dist > 1e-8f) ? (delta / dist) : glm::vec3(0, 1, 0);

// Контактная точка — на поверхности сферы A в сторону сферы B
outInfo.contactPoint = centerA + normal * radiusA;
outInfo.penetrationDepth = radiusSum - dist;
```

**Сложность**: $O(1)$ — всего несколько скалярных операций и один корень.

### Sphere vs Box

**Принцип**: найти ближайшую точку на коробке к центру сферы, вычислить расстояние.

**Реализация**:
1. Трансформировать центр сферы в локальное пространство коробки.
2. Найти ближайшую точку на AABB коробки (зажать координаты).
3. Если сфера вне коробки:
   - Расстояние от центра до ближайшей точки.
   - Если расстояние > радиус → нет коллизии.
4. Если центр сферы внутри коробки:
   - Найти сторону коробки с минимальным проникновением.
   - Отолкнуть сферу наружу вдоль этой стороны.

**Код**:
```cpp
// Sphere vs Box
glm::vec3 localCenter = transpose(rotB) * (sphereCenter - boxCenter);
glm::vec3 closest = ClosestPointOnAABB(-halfB, halfB, localCenter);
glm::vec3 diff = localCenter - closest;
float distSq = dot(diff, diff);

if (distSq > sphereRadius * sphereRadius) {
    return false;  // Вне коробки
}

// Коллизия детектирована
if (distSq > 1e-8f) {
    float dist = sqrt(distSq);
    glm::vec3 normal = diff / dist;
    outInfo.contactNormal = normalize(rotB * normal);
    outInfo.contactPoint = boxCenter + rotB * closest;
    outInfo.penetrationDepth = sphereRadius - dist;
} else {
    // Центр сферы внутри коробки
    // Найти сторону с минимальным проникновением
    // ... (подробнее см. в коде)
}
```

### Sphere vs Capsule

**Принцип**: найти ближайшую точку на центральном сегменте капсулы к центру сферы.

**Реализация**:
```cpp
glm::vec3 closestOnSeg = ClosestPointOnSegment(bottom, top, sphereCenter);
glm::vec3 diff = sphereCenter - closestOnSeg;
float dist = length(diff);
float combinedRadius = sphereRadius + capsuleRadius;

if (dist > combinedRadius) {
    return false;  // Нет коллизии
}

glm::vec3 normal = (dist > 1e-8f) ? (diff / dist) : glm::vec3(0, 1, 0);
outInfo.contactPoint = sphereCenter + normal * sphereRadius;
outInfo.penetrationDepth = max(0, combinedRadius - dist);
```

## Практический пример использования

```cpp
// Простой шарик с физикой
auto* ball = new GameObject("Ball");
auto* sphereCol = ball->addComponent<SphereCollider>(0.5f);  // Радиус 0.5м
sphereCol->SetRestitution(0.8f);  // Прыгучий мяч
sphereCol->SetFriction(0.3f);

// Триггер для детектирования входа в зону
auto* zone = new GameObject("DamageZone");
auto* triggerSphere = zone->addComponent<SphereCollider>(5.0f);
triggerSphere->SetTrigger(true);
triggerSphere->SetTriggerCallback(
    CollisionEvent::Enter,
    [](Collider* other) {
        if (auto* player = dynamic_cast<Player*>(other->gameObject)) {
            player->TakeDamage(10);
        }
    }
);

// Игрок с смещенным коллайдером (например, для шеи)
auto* player = new GameObject("Player");
auto* headCollider = player->addComponent<SphereCollider>(0.3f);
headCollider->SetOffset(glm::vec3(0, 1.8f, 0));  // На уровне головы
```

## Оптимизация

**Производительность:**
- **Sphere vs Sphere**: $O(1)$ — очень быстро.
- **Sphere vs Box**: $O(1)$ — находится ближайшая точка за константное время.
- **Sphere vs Capsule**: $O(1)$ — проекция на отрезок.

**Broad-phase**: AABB сферы — это просто куб со стороной $2r$ вокруг центра.

**Рекомендации:**
- Для игрока используйте `SphereCollider` — самый быстрый вариант.
- Для триггеров зон/эффектов — идеален для дальности срабатывания.
- Если масштаб object'а неоднороден (например, 2x0.5x2) — используется максимальный компонент масштаба.

## Как это отрисовывается для отладки (DebugDraw)

При отладке сфера рисуется как wireframe сфера (набор окружностей):

1. **В Renderer.cpp** при обходе коллайдеров:
   ```cpp
   auto* sphere = dynamic_cast<SphereCollider*>(collider);
   if (sphere) {
       glm::vec3 center = sphere->GetWorldCenter();
       float radius = sphere->GetWorldRadius();
       
       // Рисуются окружности (обычно 3 перпендикулярные окружности)
       for (int i = 0; i < segments; i++) {
           float angle = (i / (float)segments) * 2.0f * M_PI;
           glm::vec3 p1 = center + glm::vec3(radius * cos(angle), 0, radius * sin(angle));
           glm::vec3 p2 = center + glm::vec3(radius * cos(angle + dAngle), 0, radius * sin(angle + dAngle));
           m_debugDraw->DrawLine(p1, p2, glm::vec3(1, 1, 0));
       }
   }
   ```

2. **DebugDraw** рисует эти линии через GL_LINES.

## Взаимосвязь с другими классами

- **Collider** — базовый класс.
- **BoxCollider** — может сталкиваться с `SphereCollider`.
- **CapsuleCollider** — может сталкиваться с `SphereCollider`.
- **VectorMath** — используется для нахождения ближайшей точки на отрезке.
- **AABB** — используется для broad-phase и отладки.
- **Rigidbody** (если используется) — применяет импульсы коллизии.

## Примечание: масштабирование

```cpp
float GetWorldRadius() const {
    glm::vec3 scale = GetWorldScale();
    float maxScale = max(max(scale.x, scale.y), scale.z);
    return radius * max(maxScale, 0.0001f);
}
```

Радиус масштабируется по максимальному компоненту масштаба. Это сохраняет форму сферы при неоднородном масштабировании.
