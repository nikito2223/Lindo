# --- Lindo Engine ---

**Lindo** — 3D игровой движок на **C++**, созданный для разработки игр и экспериментов с собственным игровым рендерингом, физикой, UI, аудио и другими системами.

> 🎮 **One Engine — Multiple Games**

---

## ✨ Возможности

На данный момент Lindo включает:

* 🎨 3D Rendering
* 🖥️ UI System
* 🎮 Input System
* 📦 Asset Manager
* 🧱 Collision & Physics
* 💡 Lighting & Shadows
* 🌌 Skybox
* 🔊 OpenAL Audio
* 🧩 Components
* 🌍 Scene System
* 🐛 Debug Overlay

Проект находится в активной разработке.

---

# 🛠️ Требования

## Windows

Для сборки Lindo на Windows потребуется:

* **Windows 10/11**
* **Visual Studio 2022**
* **C++ Desktop Development**
* **CMake 3.15+**
* **Git**
* **vcpkg**

### Visual Studio 2022

При установке Visual Studio необходимо установить workload:

> **Desktop development with C++**

Также должны быть доступны:

* MSVC C++ compiler
* Windows SDK
* CMake tools for Windows

---

## 📦 vcpkg

Lindo использует **vcpkg** для установки C++ зависимостей.

Если vcpkg ещё не установлен:

```bash
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat
```

После этого можно установить зависимости проекта через `vcpkg.json`.

---

# 📚 Зависимости

Все основные зависимости проекта находятся в:

```text
vcpkg.json
```

Текущий список:

* OpenGL
* GLFW
* GLEW
* GLM
* Assimp
* nlohmann-json
* GLAD
* OpenAL
* FreeType

### Установка зависимостей

Находясь в корневой папке проекта:

```bash
vcpkg install
```

vcpkg автоматически прочитает `vcpkg.json` и установит необходимые библиотеки.

---

# 🔨 Сборка на Windows

После установки всех необходимых компонентов:

### 1. Клонировать репозиторий

```bash
git clone <REPOSITORY_URL>
cd Lindo
```

### 2. Установить зависимости

```bash
vcpkg install
```

### 3. Создать папку сборки

```bash
mkdir build
cd build
```

### 4. Сконфигурировать CMake

```bash
cmake ..
```

Или, если CMake не видит установленный vcpkg:

```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=<VCPKG_PATH>/scripts/buildsystems/vcpkg.cmake
```

### 5. Собрать проект

```bash
cmake --build . --config Release
```

После успешной сборки исполняемый файл Lindo будет находиться в папке сборки.

---

# 🐧 Linux

Linux-версия Lindo **не требует vcpkg**.

Зависимости можно установить непосредственно через пакетный менеджер системы.

Для Ubuntu/Debian:

```bash
sudo apt update
sudo apt install \
    build-essential \
    cmake \
    git \
    libglfw3-dev \
    libglew-dev \
    libglm-dev \
    libassimp-dev \
    nlohmann-json3-dev \
    libopenal-dev \
    libfreetype6-dev
```

Также требуется OpenGL:

```bash
sudo apt install libgl1-mesa-dev
```

---

## 🔨 Сборка на Linux

Клонируем проект:

```bash
git clone <REPOSITORY_URL>
cd Lindo
```

Создаём папку сборки:

```bash
mkdir build
cd build
```

Запускаем CMake:

```bash
cmake ..
```

Собираем:

```bash
cmake --build . -j$(nproc)
```

Запуск:

```bash
./Lindo
```

---

# 📁 Структура проекта

Примерная структура:

```text
Lindo/
│
├── CMakeLists.txt
├── CMakePresets.json
├── vcpkg.json
│
├── include/
│   └── Lindo/
│
├── src/
│   ├── core/
│   ├── graphics/
│   ├── physics/
│   ├── world/
│   ├── components/
│   └── ...
│
├── res/
│   ├── models/
│   ├── textures/
│   ├── shaders/
│   ├── fonts/
│   └── ...
│
└── build/
```

> `build/` не должен храниться в Git-репозитории.

---

# 🧩 CMake

Lindo использует **CMake** как основную систему сборки.

Минимальная версия CMake:

```text
3.15
```

Если у вас уже установлен CMake 3.15 — **этого достаточно**, если `CMakeLists.txt` проекта не требует более новую версию.

Проверить версию:

```bash
cmake --version
```

---

# 🪟 IDE

Основная IDE для разработки Lindo:

**Visual Studio 2022**

Также проект можно разрабатывать в других редакторах, поддерживающих CMake и C++.

Например:

* Visual Studio 2022
* VS Code
* CLion
* другие CMake-compatible IDE

---

# 🌐 Платформы

| Platform   | Status                     | Dependencies    |
| ---------- | -------------------------- | --------------- |
| 🪟 Windows | ✅ Supported                | vcpkg           |
| 🐧 Linux   | 🚧 Supported / Development | System packages |
| 🍎 macOS   | 🚧 Planned                 | —               |

---

# 🚀 Development

Lindo создаётся как универсальный движок:

```text
             ┌─────────────┐
             │    Lindo    │
             │    Engine   │
             └──────┬──────┘
                    │
        ┌───────────┼───────────┐
        ▼           ▼           ▼
     Game 01     Game 02     Game 03
```

Идея заключается в том, чтобы основные системы движка не зависели от конкретной игры.

Игровая логика, сцены и ресурсы могут использовать Lindo как основу для разных проектов.

---

# 📌 Roadmap

* [x] Rendering
* [x] Input
* [x] UI
* [x] Audio
* [x] Asset Manager
* [x] Collision
* [x] Physics
* [x] Lighting
* [x] Shadows
* [x] Skybox
* [x] Debug Tools
* [ ] Editor
* [ ] Advanced Physics
* [ ] Networking
* [ ] More rendering features

Roadmap может изменяться по мере разработки.

---

# 📄 License

License: **TBD**

---

<div align="center">

### 🟧 Lindo Engine

**One Engine — Multiple Games**

Made with ❤️ and C++

</div>
