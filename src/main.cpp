#include "core/Application.h"
#include "core/Globals.h"
#include <array>
#include <string>
#include <iostream>

int main() {
    try {
        // Создание и запуск приложения
        Lindo::Application app;
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}