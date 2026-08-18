#pragma once

namespace Lindo {
    class EngineContext;
    class Window;

    class ConsoleCommands {
    public:
        // Регистрирует все консольные команды движка
        static void RegisterAll(EngineContext* context, Window* window);
    };
}