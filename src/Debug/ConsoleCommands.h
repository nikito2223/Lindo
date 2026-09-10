#pragma once

namespace Lindo {
    class EngineContext;
    class Window;

    class ConsoleCommands {
    public:
        // Регистрирует все консольные команды движка
        static void RegisterAll(EngineContext* context, Window* window);

    private:
        // Регистрация команд управления настройками (CVars)
        static void RegisterSettingsCommands(EngineContext* context, Window* window);
    };
}