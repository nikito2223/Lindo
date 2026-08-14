# BoxCollider (Ориентированный коллайдер-коробка)

**Файлы:**
- [src/Physics/Collider/BoxCollider.h](../../src/Physics/Collider/BoxCollider.h)
- [src/Physics/Collider/BoxCollider.cpp](../../src/Physics/Collider/BoxCollider.cpp)

## Назначение
Коллайдер в форме ориентированного прямоугольного параллелепипеда (Oriented Bounding Box, OBB). Следует трансформации owner (позиция, вращение, масштаб). Поддерживает:
- **Собственный размер** — определяется в локальном пространстве.
- **Вращение** — полностью ориентирован в пространстве.
- **Локальное смещение** — смещение центра коллайдера относительно owner.

## Ключевые компоненты

### Основные поля

| Поле | Тип | Описание |
|------|-----|---------|
| `size` | vec3 | Локальный размер коробки (width, height, depth) — полные размеры, не половинки |

### Основные методы

**Конфигурация:**
```cpp
void SetSize(const glm::vec3& newSize);   // Установить размер (гарантирует мин 0.0001)
glm::vec3 GetSize() const;                // Получить локальный размер
```

**Мировые координаты:**
```cpp
glm::vec3 GetWorldHalfExtents() const;    // Мировые полуразмеры (с учетом scale и offset)
glm::mat3 GetWorldRotationMatrix() const; // Матрица вращения 3x3 из owner
void GetAABB(glm::vec3& outMin, glm::vec3& outMax) const;  // Axis-aligned bounds
Lindo::Math::AABB GetAABB() const override;
std::vector<glm::vec3> GetWorldVertices() const;  // 8 угловых точек в мировом пространстве
```

**Коллизии:**
```cpp
// Полиморфный dispatch (сам решает, с какой реализацией вызвать)
bool CheckCollision(Collider* other, CollisionInfo& outInfo) const override;

// Специализированные реализации
bool CheckCollision(const BoxCollider* other, CollisionInfo& outInfo) const;
bool CheckCollision(const SphereCollider* other, CollisionInfo& outInfo) const;
bool CheckCollision(const CapsuleCollider* other, CollisionInfo& outInfo) const;
```

##算法 Алгоритмы коллизии

### Box vs Box (Separating Axis Theorem, SAT)

**Принцип**: две коробки не пересекаются если существует ось, на которую их проекции не пересекаются.

**Реализация**:
1. Вычисляются 15 потенциальных осей разделения:
   - 3 оси от коробки A (её локальные оси в мировом пространстве)
   - 3 оси от коробки B (её локальные оси в мировом пространстве)
   - 9 осей из перекрестных произведений каждой оси A с каждой осью B

2. Для каждой оси:
   - Проецируются оба параллелепипеда на ось (вычисляется радиус проекции как сумма скалярных произведений вектора оси на полуразмеры, повернутые в мировое пространство).
   - Вычисляется расстояние между центрами вдоль оси.
   - Если расстояние > сумме радиусов → ось разделяет, коллизии нет.

3. Если все оси проверены и ни одна не разделяет → коллизия. Осью с минимальным перекрытием задается нормаль контакта.

**Код**:
```cpp
// В CheckCollision(BoxCollider, BoxCollider):
// Оси A в кадре A — просто unit-векторы (1,0,0), (0,1,0), (0,0,1)
// Оси B в кадре A — столбцы матрицы R = transpose(rotA) * rotB
// Перекрестные произведения: cross(aAxes[i], bAxes[j])
// Проецируются параллелепипеды на ось:
//   projALen = |dot(axis, rotA[0])| * halfA.x + |dot(axis, rotA[1])| * halfA.y + |dot(axis, rotA[2])| * halfA.z
```

### Box vs Sphere

**Принцип**: найти ближайшую точку на коробке к центру сферы, вычислить расстояние.

**Реализация**:
1. Трансформировать центр сферы в локальное пространство коробки.
2. Найти ближайшую точку на AABB (зажав координаты в границы).
3. Вычислить расстояние и нормаль контакта.
4. Если сфера пересекает коробку (центр внутри), найти ось с минимальным проникновением и использовать её как нормаль.

**Код**:
```cpp
// bool CheckCollision(const SphereCollider* other, ...)
glm::vec3 localCenter = transpose(rot) * (sphereCenter - boxCenter);
glm::vec3 closest = VectorMath::ClosestPointOnAABB(-half, half, localCenter);
glm::vec3 diff = localCenter - closest;
float dist = sqrt(dot(diff, diff));
if (dist > sphereRadius) return false;  // Нет коллизии
// ... остальная обработка
```

## Практический пример использования

```cpp
// Создание
auto* box = gameObject->addComponent<BoxCollider>();
box->SetSize(glm::vec3(2.0f, 3.0f, 1.5f));  // 2м × 3м × 1.5м

// Смещение центра
box->SetOffset(glm::vec3(0.0f, 1.0f, 0.0f));  // На 1м выше owner

// Физика
box->SetFriction(0.7f);
box->SetRestitution(0.1f);  // Не прыгает

// Триггер без физики
auto* trigger = wall->addComponent<BoxCollider>();
trigger->SetSize(glm::vec3(5.0f, 5.0f, 0.2f));
trigger->SetTrigger(true);
trigger->SetTriggerCallback(
    CollisionEvent::Enter,
    [](Collider* other) {
        std::cout << "Игрок вошел в триггер!\n";
    }
);
```

## Как это отвязывается шейдеру (для DebugDraw)

При отладке хитбоксы рисуются как проволочные коробки:

1. **В Renderer.cpp** при обходе коллайдеров:
   ```cpp
   auto colliders = currentScene->FindComponentsOfType<BoxCollider>();
   for (auto* collider : colliders) {
       glm::mat4 world = collider->owner->getWorldMatrix();
       std::vector<glm::vec3> corners = collider->GetWorldVertices();  // 8 углов
       
       // Рисуются 12 рёбер (edge indices в коде)
       for (int idx = 0; idx < 24; idx += 2) {
           m_debugDraw->DrawLine(corners[edges[idx]], corners[edges[idx+1]], 
                                 glm::vec3(0.0f, 1.0f, 1.0f));  // Голубой цвет
       }
   }
   ```

2. **DebugDraw** накапливает линии в буфер вершин и отправляет на GPU через шейдер.

3. **Результат**: во время отладки видны все коллайдеры как проволочные коробки, ориентированные в мировом пространстве.

## Оптимизация и советы

**Производительность:**
- SAT для Box vs Box — $O(15)$ тестов осей, но точный результат.
- Для большого количества коллайдеров сначала выполняется broad-phase (AABB), а уж потом narrow-phase.

**Рекомендации:**
- Если объект static → оставьте коллайдер включенным, но в `Rigidbody` (если есть) установите массу = ∞.
- Для сложных mesh-форм используйте `MeshCollider` вместо ручного создания множества `BoxCollider`.
- Вращение `BoxCollider` бесплатно (просто используется owner-rotation), поэтому не бойтесь ротировать объекты.

## Взаимосвязь с другими классами

- **Collider** — базовый класс, откуда наследуется `BoxCollider`.
- **SphereCollider** — может сталкиваться с `BoxCollider`.
- **CapsuleCollider** — может сталкиваться с `BoxCollider`.
- **MeshCollider** — наследуется от `BoxCollider`.
- **DebugDraw** — рисует wireframe коробку через `GetWorldVertices()`.
- **VectorMath** — вспомогательные функции (ClosestPointOnAABB и т.д.).
