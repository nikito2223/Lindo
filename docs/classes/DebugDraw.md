**DebugDraw**

Файлы:
- [src/Graphics/core/DebugDraw.h](src/Graphics/core/DebugDraw.h#L1-L36)
- [src/Graphics/core/DebugDraw.cpp](src/Graphics/core/DebugDraw.cpp#L1-L99)

Краткое назначение:
- Отвечает за отрисовку отладочных линий и хитбоксов (gizmos) поверх сцены.

Основной поток данных (как задаются хитбоксы шейдеру):
1. Рендерер сцены вызывает `m_debugDraw->begin(view, projection)` — сохраняет матрицы `view` и `projection` и очищает буфер вершин.
2. В коде сцены/рендера при обходе коллайдеров вызывается `m_debugDraw->DrawLine(start, end, color)` — это пушит две вершины `{ position, color }` в `m_vertices`.
3. После накопления всех линий `m_debugDraw->render()`:
   - Привязывает VAO/VBO и загружает содержимое `m_vertices` в VBO через `glBufferData`.
   - Активирует шейдер (`m_shader->use()`), вызывает `m_shader->setMat4("view", m_view)` и `m_shader->setMat4("projection", m_projection)` — тем самым передаёт матрицы в шейдер как uniform-переменные.
   - Выключает `GL_CULL_FACE` и `GL_DEPTH_TEST` (временно), вызывает `glDrawArrays(GL_LINES, ...)` — GPU читает атрибуты вершин и передаёт их в вершинный шейдер.

Как шейдер получает данные вершин:
- В `DebugDraw.cpp` есть встроенные строки шейдера `debugLineVertex` / `debugLineFragment`.
- Вершинный шейдер объявляет `layout(location = 0) in vec3 aPos;` и `layout(location = 1) in vec3 aColor;`.
- В `init()` выполняется `glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);` и
  `glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));` — это связывает поля структуры `Vertex` с атрибутами `aPos` и `aColor`.
- Таким образом шейдер получает координаты и цвет каждой вершины напрямую из буфера.

Где формируются хитбоксы:
- В `src/Graphics/core/Renderer.cpp` (см. блок, где обрабатываются `Collider`): код вычисляет угловые точки коробки, преобразует их в мировые координаты и вызывает `DrawLine` для каждой из 12 рёбер — поэтому линии хитбокса оказываются на шейдере как пары вершин.

Как менять поведение/цвет/шейдеры:
- Изменить цвет конкретного ребра: передавайте другой `color` в `DrawLine` (в `Renderer.cpp` в местах вызова). Это самый быстрый путь.
- Изменить глобально: можно добавить цвет как uniform в шейдер и в `render()` выставлять его перед рисованием (после чего игнорировать `aColor`), либо изменить вызовы `DrawLine` чтобы они нормализовали/модифицировали цвет.
- Изменить сам шейдер: отредактировать строки `debugLineVertex`/`debugLineFragment` в `DebugDraw.cpp` или заменить загрузкой шейдера из файла (и пересоздавать `Shader` при изменениях).
- Изменить ширину линий: добавить `glLineWidth(...)` в `render()` перед `glDrawArrays`.

Советы при правках:
- После изменений в строках шейдера нужно пересоздать объект `Shader` (вызвать `init()` или реинит `m_shader`), иначе изменения не применятся.
- Если хотите, чтобы хитбоксы рендерились с тестом глубины (например, чтобы скрыть за объектами), не отключайте `GL_DEPTH_TEST` или отключайте/включайте его только для нужных наборов линий.
- Если планируете рисовать много линий часто — подумайте о использовании `glBufferSubData` и persistent mapping для производительности.

Ключевые места кода для быстрой навигации:
- [src/Graphics/core/DebugDraw.h](src/Graphics/core/DebugDraw.h#L1-L36)
- [src/Graphics/core/DebugDraw.cpp](src/Graphics/core/DebugDraw.cpp#L1-L99)
- [src/Graphics/core/Renderer.cpp](src/Graphics/core/Renderer.cpp#L150-L230)
