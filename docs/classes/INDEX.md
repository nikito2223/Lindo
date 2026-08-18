# Документация классов Lindo Engine

Полная документация по ключевым классам движка, с объяснением работы, алгоритмов и примерами использования.

## 📚 Содержание

### Graphics & Debug Rendering
- **[DebugDraw](classes/DebugDraw.md)** — Отрисовка отладочных линий и гизмо (wireframe коробки, сферы, капсулы коллайдеров).
  - Как линии отправляются на шейдер
  - Поток данных vertex → GPU → шейдер
  - Как менять цвета и шейдеры

### Physics & Collision
Система коллайдеров построена иерархически:

#### Базовый класс
- **[Collider](classes/Collider_Base.md)** — Основа для всех коллайдеров.
  - Регистрация в PhysicsSystem
  - Управление событиями Enter/Stay/Exit
  - Callback-система
  - Физические свойства (friction, restitution, density)

#### Специализированные коллайдеры
- **[BoxCollider](classes/BoxCollider.md)** — Ориентированный прямоугольник (OBB).
  - SAT (Separating Axis Theorem) для Box vs Box
  - Box vs Sphere (ближайшая точка на AABB)
  - Вращение и масштабирование

- **[SphereCollider](classes/SphereCollider.md)** — Сферический коллайдер.
  - $O(1)$ производительность для всех типов
  - Sphere vs Sphere (простое расстояние)
  - Sphere vs Box (ближайшая точка)
  - Sphere vs Capsule (проекция на отрезок)

- **[CapsuleCollider](classes/CapsuleCollider.md)** — Капсула (цилиндр + полусферы).
  - Ориентация вдоль оси (X, Y, Z)
  - Идеален для персонажей
  - Capsule vs Capsule (ближайшие точки между отрезками)
  - Скругленные края для гладкого скольжения

- **[MeshCollider](classes/MeshCollider.md)** — Коллайдер на основе сетки.
  - Автоматическое вычисление AABB из mesh
  - Наследуется от BoxCollider
  - Для static объектов (здания, скалы)

### UI & Overlay
- **[UIRenderer](classes/UIRenderer.md)** (будет добавлен)
- **[DebugOverlay](classes/DebugOverlay.md)** (будет добавлен)

---

## 🔍 Быстрая навигация

### По назначению

**Я хочу...**
- Понять, как работают коллайдеры → [Collider](classes/Collider_Base.md)
- Создать физический объект → [BoxCollider](classes/BoxCollider.md) или [SphereCollider](classes/SphereCollider.md)
- Создать триггер для зоны → любой коллайдер + `SetTrigger(true)`
- Создать персонажа → [CapsuleCollider](classes/CapsuleCollider.md)
- Использовать модель как коллайдер → [MeshCollider](classes/MeshCollider.md)
- Отладить коллайдеры визуально → [DebugDraw](classes/DebugDraw.md)
- Узнать о коллизиях между типами → см. таблицы в каждом классе

### По типам коллизий

| Тип | Алгоритм | Класс | Сложность |
|-----|----------|-------|-----------|
| Box vs Box | SAT (15 осей) | [BoxCollider](classes/BoxCollider.md) | O(1) |
| Box vs Sphere | Ближайшая точка на AABB | [BoxCollider](classes/BoxCollider.md) | O(1) |
| Box vs Capsule | Проекция отрезка на AABB | [BoxCollider](classes/BoxCollider.md) | O(1) |
| Sphere vs Sphere | Расстояние центр-центр | [SphereCollider](classes/SphereCollider.md) | O(1) |
| Sphere vs Box | Ближайшая точка на AABB | [SphereCollider](classes/SphereCollider.md) | O(1) |
| Sphere vs Capsule | Проекция на отрезок | [SphereCollider](classes/SphereCollider.md) | O(1) |
| Capsule vs Capsule | Ближайшие точки между отрезками | [CapsuleCollider](classes/CapsuleCollider.md) | O(1) |
| Capsule vs Box | Проекция отрезка на AABB | [CapsuleCollider](classes/CapsuleCollider.md) | O(1) |
| Capsule vs Sphere | Проекция на отрезок | [CapsuleCollider](classes/CapsuleCollider.md) | O(1) |

---

## 📋 Структура документов

Каждый документ следует одному шаблону:

1. **Назначение** — Для чего нужен класс и когда его использовать
2. **Ключевые компоненты** — Главные поля и методы с таблицами
3. **Алгоритмы** — Детальное объяснение каждого алгоритма коллизии
4. **Практические примеры** — Copy-paste код для быстрого начала
5. **Оптимизация** — Советы по производительности и рекомендации
6. **Взаимосвязь** — Какие классы взаимодействуют

---

## 🗄️ JSON-база классов

Для автоматизированного анализа кода также доступна структурированная база:

```bash
docs/database/classes.json
```

Содержит:
- Пути к файлам `.h` и `.cpp`
- Описания и методы
- Информацию о наследовании
- Поддерживаемые типы коллизий

Можно использовать для:
- Автоматического перехода на файлы
- Генерации графиков наследования
- Документирования API
- IDE-интеграции

---

## 🚀 Частые задачи

### Создать простой коллайдер

```cpp
auto* sphere = obj->addComponent<SphereCollider>(0.5f);
sphere->SetFriction(0.8f);
sphere->SetRestitution(0.2f);
```

### Создать триггер (без физики)

```cpp
auto* trigger = zone->addComponent<BoxCollider>(glm::vec3(5, 5, 5));
trigger->SetTrigger(true);
trigger->SetTriggerCallback(
    CollisionEvent::Enter,
    [](Collider* other) { std::cout << "Entered!\n"; }
);
```

### Отладить коллайдеры

1. Убедитесь, что [DebugDraw](classes/DebugDraw.md) инициализирован.
2. Включите debug mode (обычно клавиша F10 или похожая).
3. Хитбоксы рисуются проволочными линиями поверх объектов.

### Создать коллайдер из модели

```cpp
auto* building = new GameObject("Building");
auto* mesh = building->addComponent<MeshRenderer>("model.obj");
auto* collider = building->addComponent<MeshCollider>();
// Коллайдер автоматически вычислит AABB из mesh
```

---

## 🔗 Связанные файлы

**Исходные коды:**
- `src/Physics/Collider/` — все классы коллайдеров
- `src/Graphics/core/DebugDraw.{h,cpp}` — отрисовка отладочных линий
- `src/Physics/PhysicsSystem.{h,cpp}` — система управления коллайдерами

**Вспомогательные:**
- `src/Physics/Math/AABB.{h,cpp}` — ограничивающий прямоугольник
- `src/Physics/Math/VectorMath.{h,cpp}` — математические функции

---

## 📖 Примечания

- Все расчеты используют `glm::vec3` и `glm::mat4` для математики.
- Углы указываются в **градусах** (преобразуются в радианы внутри).
- Все расстояния и размеры в **единицах мира** (обычно метры).
- Коллайдеры автоматически следят за трансформацией gameObject.

---

**Последнее обновление:** 2026-08-12  
**Версия документации:** 1.0  
**Для вопросов:** см. комментарии в исходных кодах
