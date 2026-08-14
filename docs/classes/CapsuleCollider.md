# CapsuleCollider (Капсульный коллайдер)

**Файлы:**
- [src/Physics/Collider/CapsuleCollider.h](../../src/Physics/Collider/CapsuleCollider.h)
- [src/Physics/Collider/CapsuleCollider.cpp](../../src/Physics/Collider/CapsuleCollider.cpp)

## Назначение
Коллайдер в форме капсулы (цилиндра с полусферическими шапками на концах). Идеален для:
- **Цилиндрических объектов** — трубы, столбы, цилиндры.
- **Удлиненных объектов** — бревна, мечи, палки.
- **Персонажей** — часто используется для моделирования туловища/ног.

**Преимущество**: скругленные концы позволяют объектам скользить по углам без застревания, в отличие от Box.

## Ключевые компоненты

### Основные поля

| Поле | Тип | Описание |
|------|-----|---------|
| `radius` | float | Радиус цилиндра и полусфер (локальный) |
| `height` | float | Полная высота капсулы (включая полусферы) |
| `direction` | Direction | Оси выравнивания (X, Y, Z) — вдоль какой оси ориентирована капсула |

**Direction enum:**
```cpp
enum class Direction {
    X = 0,  // Капсула вытянута вдоль оси X
    Y = 1,  // Капсула вытянута вдоль оси Y (по умолчанию)
    Z = 2   // Капсула вытянута вдоль оси Z
};
```

### Основные методы

**Конфигурация:**
```cpp
void SetRadius(float newRadius);           // Установить радиус
float GetRadius() const;
void SetHeight(float newHeight);           // Установить полную высоту
float GetHeight() const;
void SetDirection(Direction dir);          // Установить ось выравнивания
Direction GetDirection() const;
```

**Мировые координаты:**
```cpp
float GetWorldRadius() const;              // Мировой радиус (с масштабом)
glm::mat3 GetWorldRotationMatrix() const;  // Матрица ориентации
void GetEndpoints(glm::vec3& outTop, glm::vec3& outBottom) const;  // Концы центрального сегмента
glm::vec3 GetWorldCenter() const override; // Центр капсулы
Lindo::Math::AABB GetAABB() const override; // Ограничивающий прямоугольник
```

**Коллизии:**
```cpp
bool CheckCollision(Collider* other, CollisionInfo& outInfo) const override;
bool CheckCollision(const BoxCollider* other, CollisionInfo& outInfo) const;
bool CheckCollision(const SphereCollider* other, CollisionInfo& outInfo) const;
bool CheckCollision(const CapsuleCollider* other, CollisionInfo& outInfo) const;
```

## Геометрия капсулы

```
Высота = 3, Радиус = 0.5

Полусфера (вершина)
        /\
       /  \
      |    |  <- Цилиндр  (height - 2*radius)
       \  /
        \/
Полусфера (основание)

Концы сегмента:
- Top:    центр + direction * (height/2 - radius)
- Bottom: центр - direction * (height/2 - radius)

Между этими точками натягивается "трубка" радиуса R
```

## Алгоритмы коллизии

### Capsule vs Sphere

**Принцип**: найти ближайшую точку на центральном сегменте капсулы к центру сферы.

**Реализация**:
```cpp
glm::vec3 bottom, top;
GetEndpoints(bottom, top);
float capsuleRadius = GetWorldRadius();
glm::vec3 sphereCenter = other->GetWorldCenter();
float sphereRadius = other->GetWorldRadius();

// Ближайшая точка на отрезке bottom-top к sphereCenter
glm::vec3 closestOnSeg = ClosestPointOnSegment(bottom, top, sphereCenter);

// Вектор от капсулы (препятствие) к сфере (игрок)
glm::vec3 diff = sphereCenter - closestOnSeg;
float dist = length(diff);
float combinedRadius = capsuleRadius + sphereRadius;

if (dist > combinedRadius) {
    return false;  // Нет коллизии
}

// Нормаль контакта — в сторону сферы
glm::vec3 normal = (dist > 1e-8f) ? (diff / dist) : glm::vec3(0, 1, 0);
outInfo.contactPoint = closestOnSeg + normal * capsuleRadius;
outInfo.penetrationDepth = max(0, combinedRadius - dist);
```

**Сложность**: $O(1)$ — проекция точки на отрезок.

### Capsule vs Box

**Принцип**: найти ближайшую точку на отрезке к коробке, затем проверить расстояние.

**Реализация**:
1. Трансформировать оба конца отрезка в локальное пространство коробки.
2. Найти ближайшую точку отрезка к AABB коробки.
3. Найти ближайшую точку на AABB к этой точке.
4. Вычислить расстояние и нормаль.

**Код**:
```cpp
// Capsule vs Box
glm::vec3 localTop = transpose(rotB) * (top - boxCenter);
glm::vec3 localBottom = transpose(rotB) * (bottom - boxCenter);

// Ближайшая точка отрезка к AABB коробки
glm::vec3 segClosest = ClosestPointOnSegment(localBottom, localTop, 
                                              ClosestPointOnAABB(-half, half, localTop));

glm::vec3 closestOnBox = ClosestPointOnAABB(-half, half, segClosest);
glm::vec3 diff = segClosest - closestOnBox;
float dist = length(diff);

if (dist > capsuleRadius) {
    return false;  // Нет коллизии
}

glm::vec3 normal = (dist > 1e-8f) ? (diff / dist) : glm::vec3(0, 1, 0);
outInfo.contactNormal = normalize(rotB * normal);
outInfo.penetrationDepth = max(0, capsuleRadius - dist);
```

### Capsule vs Capsule

**Принцип**: найти ближайшие точки между двумя отрезками, вычислить расстояние.

**Реализация**:
```cpp
// Ближайшие точки между двумя отрезками
glm::vec3 closestA, closestB;
ClosestPointsBetweenSegments(bottomA, topA, bottomB, topB, closestA, closestB);

// Вектор от капсулы A к капсуле B
glm::vec3 diff = closestB - closestA;
float dist = length(diff);
float combinedRadius = radiusA + radiusB;

if (dist > combinedRadius) {
    return false;  // Нет коллизии
}

glm::vec3 normal = (dist > 1e-8f) ? (diff / dist) : glm::vec3(0, 1, 0);
outInfo.contactPoint = (closestA + closestB) * 0.5f;
outInfo.penetrationDepth = max(0, combinedRadius - dist);
```

**Алгоритм ClosestPointsBetweenSegments**: найти пару точек на двух отрезках с минимальным расстоянием. Использует параметрические уравнения отрезков и решает систему уравнений.

## Практический пример использования

```cpp
// Столб/цилиндр
auto* pillar = new GameObject("Pillar");
auto* capsule = pillar->addComponent<CapsuleCollider>(0.5f, 3.0f, CapsuleCollider::Direction::Y);
capsule->SetFriction(0.8f);  // Шероховатый

// Палка/меч (горизонтальная)
auto* sword = new GameObject("Sword");
auto* swordCollider = sword->addComponent<CapsuleCollider>(0.1f, 1.5f, CapsuleCollider::Direction::X);
swordCollider->SetTrigger(true);  // Триггер для урона
swordCollider->SetTriggerCallback(
    CollisionEvent::Enter,
    [](Collider* other) {
        if (auto* enemy = dynamic_cast<Enemy*>(other->owner)) {
            enemy->TakeDamage(20);
        }
    }
);

// Туловище персонажа
auto* player = new GameObject("Player");
auto* bodyCollider = player->addComponent<CapsuleCollider>(0.4f, 1.6f, CapsuleCollider::Direction::Y);
bodyCollider->SetOffset(glm::vec3(0, 0.8f, 0));  // Посередине туловища
```

## Масштабирование

```cpp
// GetWorldRadius() использует максимальный компонент масштаба
float maxScale = max(scale.x, scale.y, scale.z);
float worldRadius = radius * maxScale;

// Высота масштабируется по компоненте вдоль оси выравнивания
// Если direction = Y, то используется scale.y
float axisScale = (direction == Y) ? scale.y : ...;
float worldHeight = height * axisScale;
```

## Как это отрисовывается для отладки (DebugDraw)

При отладке капсула рисуется как wireframe цилиндр + две полусферы:

1. **В Renderer.cpp** (при наличии соответствующего кода):
   ```cpp
   auto* capsule = dynamic_cast<CapsuleCollider*>(collider);
   if (capsule) {
       glm::vec3 top, bottom;
       capsule->GetEndpoints(bottom, top);
       float r = capsule->GetWorldRadius();
       
       // Рисуются окружности на концах
       for (int i = 0; i < segments; i++) {
           float angle = (i / (float)segments) * 2.0f * M_PI;
           // Окружность на top
           m_debugDraw->DrawLine(top + circle[i], top + circle[i+1], color);
           // Окружность на bottom
           m_debugDraw->DrawLine(bottom + circle[i], bottom + circle[i+1], color);
       }
       
       // Вертикальные линии (рёбра цилиндра)
       for (int i = 0; i < ribs; i++) {
           glm::vec3 offset = circlePoint[i] * r;
           m_debugDraw->DrawLine(top + offset, bottom + offset, color);
       }
   }
   ```

2. **Результат**: видна вытянутая капсула в нужной ориентации.

## Оптимизация

**Производительность:**
- **Capsule vs Sphere**: $O(1)$ — проекция точки на отрезок.
- **Capsule vs Box**: $O(1)$ — несколько проекций точек.
- **Capsule vs Capsule**: $O(1)$ — решение системы уравнений.

**Рекомендации:**
- Капсулы идеальны для персонажей — гладкие углы не зацепляются.
- Для множества капсул используйте broad-phase (AABB) для отсеивания пар.
- Ось выравнивания можно менять runtime-ом без перестроения (меняется только GetEndpoints).

## Взаимосвязь с другими классами

- **Collider** — базовый класс.
- **BoxCollider** — может сталкиваться с `CapsuleCollider`.
- **SphereCollider** — может сталкиваться с `CapsuleCollider`.
- **VectorMath** — `ClosestPointOnSegment`, `ClosestPointsBetweenSegments`.
- **AABB** — для broad-phase.
- **Direction enum** — определяет ориентацию капсулы.

## Примечание: высота vs радиус

- **height** — полная высота капсулы, включая полусферы.
- **Центральный сегмент**: `height - 2*radius`.
- Если `height < 2*radius`, то сегмент вырождается (капсула становится более похожей на сферу).

Рекомендуется: `height >= 2*radius` для нормальной капсулы.
