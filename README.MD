# --- Lindo Engine ---

**Lindo** — 3D движок, который разработан для собственных проектов

>- Проект находится в активной разработке.

# 🛠️ Требования

## Windows

>- Сборка под винду, требует -
>>- **Windows 10/11**
>>- **Visual Studio 2022**
>>- **C++ Desktop Development**
>>- **CMake 3.15+**
>>- **Git**
>>- **vcpkg**

### Visual Studio 2022

При установке Visual Studio необходимо установить workload:

> **Desktop development with C++**

Также должны быть доступны:

* MSVC C++ compiler
* Windows SDK
* CMake tools for Windows

---

## VCPKG часть

Lindo использует **vcpkg** для установки C++ зависимостей.

Если vcpkg ещё не установлен:

```bash
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat
```

После этого можно установить зависимости проекта через `vcpkg.json`.

---

# Зависимости

Все основные зависимости проекта находятся в:

```text
vcpkg.json
```

Текущий список -
#
*  OpenGL
* GLFW
* OpenAL Soft
* GLEW
* GLM
* Assimp
* GLAD
* JSON
* FreeType
* LibPNG
* Lua
#

# Сборка на Windows

После установки всех необходимых компонентов:

### 1. Клонировать репозиторий

```bash
git clone https://github.com/nikito2223/Lindo.git
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

### 5. Собрать проект

```bash
cmake --build . --config Release
```

После успешной сборки исполняемый файл Lindo будет находиться в папке сборки.

---

# Linux

Зависимости можно установить непосредственно через пакетный менеджер системы.

Пример для Ubuntu/Debian:

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

## Сборка на Linux

Клонируем проект:

```bash
git clone https://github.com/nikito2223/Lindo.git
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

# Структура проекта

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
│   ├── ui/
│   └── ...
│
└── build/
```

# IDE

Также проект можно разрабатывать в других редакторах, поддерживающих CMake и C++.

Например:

* Visual Studio 2022
* VS Code
* CLion
* другие CMake-compatible IDE

---

# 🌐 Платформы

| Platform   | Status                      | Dependencies    |
| ---------- | --------------------------  | --------------- |
| Windows | ✅ Supported                | vcpkg            |
| Linux   | 🚧 Planned                 | --              |
| macOS   | 🚧 Planned                 | —               |
---

# 📄 License

License: **GPL-3.0 license**

---
