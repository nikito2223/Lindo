# MeshCollider (Коллайдер на основе сетки)

**Файлы:**
- [src/Physics/Collider/MeshCollider.h](../../src/Physics/Collider/MeshCollider.h)
- [src/Physics/Collider/MeshCollider.cpp](../../src/Physics/Collider/MeshCollider.cpp)

## Назначение
Коллайдер, автоматически получающий свою форму из геометрии 3D-модели (mesh) компонента `MeshRenderer`. Вычисляет ограничивающий прямоугольник по vertices модели и использует его как `BoxCollider`. Полезен для:
- **Сложных static объектов** — зданий, скал, мебели (когда нужна примерная форма).
- **Быстрой настройки** — не нужно вручную создавать коллайдер, он генерируется автоматически.
- **Соответствия визуальной геометрии** — форма коллайдера примерно соответствует отрисовываемому mesh.

**Ограничения:**
- Не точен для неправильных форм (только приближение через AABB).
- Наследуется от `BoxCollider`, поэтому всегда прямоугольная форма.
- Для точных коллизий со сложными mesh требуется использовать несколько простых коллайдеров.

## Ключевые компоненты

### Наследование
```
Collider (базовый класс)
    ↓
BoxCollider (используется как основа)
    ↓
MeshCollider
```

Т.е. `MeshCollider` — это специализированный `BoxCollider`, который инициализируется из bounds модели.

### Основные поля (добавлены в MeshCollider)

| Поле | Тип | Описание |
|------|-----|---------|
| `localMin` | vec3 | Минимальная точка AABB в локальном пространстве модели |
| `localMax` | vec3 | Максимальная точка AABB в локальном пространстве модели |

### Основные методы

**Конфигурация:**
```cpp
void SetLocalBounds(const glm::vec3& minBounds, const glm::vec3& maxBounds);
// Установить локальные границы вручную и пересчитать размер/смещение
```

**Инициализация из mesh:**
```cpp
void UpdateFromMeshRenderer();
// Извлечь bounds из MeshRenderer и применить их
// Вызывается автоматически в OnStart()
```

**Наследованные методы из BoxCollider:**
```cpp
// Полностью наследуются из BoxCollider
void SetSize(const glm::vec3& newSize);
glm::vec3 GetSize() const;
glm::vec3 GetWorldHalfExtents() const;
// ... и все остальные методы BoxCollider
```

## Жизненный цикл

```
GameObject создается с MeshRenderer и MeshCollider
    ↓
Component.OnStart()
    ↓
    MeshCollider::OnStart()
        ↓
        UpdateFromMeshRenderer()  // Извлечь bounds из MeshRenderer
            ↓
            SetLocalBounds(meshBBoxMin, meshBBoxMax)
                ↓
                UpdateBounds()  // Пересчитать размер и смещение для BoxCollider
                    ↓
                    SetSize(size)       // Установить размер
                    SetOffset(center)   // Установить смещение
        ↓
        Collider::OnStart()  // Регистрация в PhysicsSystem
    ↓
Готово к коллизиям
```

## Практический пример использования

```cpp
// Автоматическое создание коллайдера из модели
auto* building = new GameObject("Building");
auto* meshRenderer = building->addComponent<MeshRenderer>("assets/models/house.obj");
auto* meshCollider = building->addComponent<MeshCollider>();
// MeshCollider автоматически вытянет bounds из meshRenderer

// Вручную установить границы
auto* rock = new GameObject("Rock");
auto* rockMesh = rock->addComponent<MeshRenderer>("assets/models/rock.obj");
auto* rockCollider = rock->addComponent<MeshCollider>();
// Или вручную, если нужно переопределить:
rockCollider->SetLocalBounds(glm::vec3(-2, -1, -2), glm::vec3(2, 3, 2));

// Проверить, что это работает
std::cout << "Collider size: " << glm::to_string(rockCollider->GetSize()) << "\n";
std::cout << "Collider offset: " << glm::to_string(rockCollider->GetOffset()) << "\n";
```

## Как это работает внутри

### UpdateFromMeshRenderer()

```cpp
void MeshCollider::UpdateFromMeshRenderer() {
    if (!owner) return;
    
    // 1. Получить компонент MeshRenderer
    auto* meshRenderer = owner->getComponent<MeshRenderer>();
    if (!meshRenderer || !meshRenderer->hasBBox) {
        return;  // Нет mesh данных
    }
    
    // 2. Извлечь bounds из mesh
    localMin = meshRenderer->bboxMin;  // Минимальная точка vertices
    localMax = meshRenderer->bboxMax;  // Максимальная точка vertices
    
    // 3. Пересчитать размер и смещение
    UpdateBounds();
}
```

### UpdateBounds()

```cpp
void MeshCollider::UpdateBounds() {
    // Размер = разница между max и min
    glm::vec3 size = localMax - localMin;
    SetSize(glm::abs(size));  // Абсолютное значение (на случай инвертированных осей)
    
    // Смещение = центр между min и max
    glm::vec3 center = (localMax + localMin) * 0.5f;
    SetOffset(center);
    // Это смещает центр BoxCollider так, чтобы его boundaries совпали с mesh bounds
}
```

## Визуализация преобразования

```
Исходный mesh:
    localMin = (-1, 0, -1)
    localMax = (3, 2, 2)
    
Вычисленные параметры:
    size = abs((3, 2, 2) - (-1, 0, -1)) = (4, 2, 3)
    center = ((3, 2, 2) + (-1, 0, -1)) * 0.5f = (1, 1, 0.5)
    
Результат:
    BoxCollider с размером (4, 2, 3) и смещением (1, 1, 0.5)
    
Визуально:
    Исходный mesh                  →  BoxCollider вокруг него
    [o o o o o]                      [─────────]
    [o o o o o]         →            [─────────]
    [o o o o o]                      [─────────]
    (-1 to 3 по X)                   (centered at 1)
```

## Алгоритмы коллизии

`MeshCollider` наследует все коллизионные методы из `BoxCollider`:
- **Box vs Box** — используется SAT (см. [BoxCollider.md](BoxCollider.md))
- **Box vs Sphere** — ближайшая точка на AABB
- **Box vs Capsule** — проекция отрезка на AABB
- **Sphere vs Box** — ближайшая точка на AABB
- **Capsule vs Box** — проекция отрезка на AABB

## Производительность и советы

**Достоинства:**
- ✅ Автоматическое создание из mesh.
- ✅ Простое использование.
- ✅ Хорошее приближение для многих объектов.

**Недостатки:**
- ❌ Не может точно представить сложные, вогнутые формы.
- ❌ Использует только AABB (ориентированный прямоугольник), теряет детали.
- ❌ Для сложной геометрии нужно ручное разбиение на несколько простых коллайдеров.

**Рекомендации:**
- Для **static объектов** (стены, полы, потолки) — отлично.
- Для **простых объектов** (кубы, цилиндры) — идеально.
- Для **сложных форм** (статуи, растения) — лучше вручную создать несколько `BoxCollider`/`SphereCollider`/`CapsuleCollider`.
- Если нужна точность — используйте **mesh collider с вогнутостью** (но это требует более сложной реализации, чем текущая).

## Взаимосвязь с другими классами

- **BoxCollider** — родительский класс, на котором базируется вся физика.
- **Collider** — базовый класс для всех коллайдеров.
- **MeshRenderer** — источник данных (bounds извлекаются отсюда).
- **GameObject** — содержит как MeshRenderer, так и MeshCollider.
- **PhysicsSystem** — регистрирует и управляет коллайдером.

## Примечание о bbox в MeshRenderer

MeshRenderer должен иметь поля:
```cpp
struct MeshRenderer {
    bool hasBBox = false;       // Есть ли вычисленные bounds
    glm::vec3 bboxMin;          // Минимальная точка mesh
    glm::vec3 bboxMax;          // Максимальная точка mesh
    // ... другие поля
};
```

Если `hasBBox == false`, `MeshCollider::UpdateFromMeshRenderer()` просто выйдет и не установит bounds.

## Когда переопределять bounds

```cpp
// Если автоматические bounds не подходят
auto* collider = obj->getComponent<MeshCollider>();
collider->SetLocalBounds(
    glm::vec3(-2, -1, -1),  // Минимум
    glm::vec3(2, 3, 1)       // Максимум
);
// Bounds пересчитаются, и коллайдер обновится

// Или получить текущие bounds через наследованные методы
glm::vec3 size = collider->GetSize();
glm::vec3 offset = collider->GetOffset();
```
