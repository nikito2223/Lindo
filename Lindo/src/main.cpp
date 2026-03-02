#include "core/Application.h"
#include "core/Globals.h"
#include "antires/CryptoUtils.h"
#include <array>
#include <string>
#include <iostream>

// Функция генерации ключа (можно вынести в отдельный файл, но для простоты оставим здесь)
std::vector<unsigned char> generateEncryptionKey() {
    // Стабильные факторы, не зависящие от сцены
    const std::string gameName = "Lindo";
    const std::string version = "1.0.0";   // меняйте только при необходимости перешифровать ресурсы
    const std::string salt = "G4m3D3v"; // дополнительная соль

    std::string combined = gameName + version + salt;

    // Простой XOR-хеш для получения 16 байт
    std::array<unsigned char, 16> key = {};
    for (size_t i = 0; i < combined.size(); ++i) {
        key[i % 16] ^= static_cast<unsigned char>(combined[i]);
    }

    // Дополнительное смешивание с фиксированными константами для надёжности
    const unsigned char fixed[] = {
        0xA5, 0x5A, 0x3C, 0xC3, 0x69, 0x96, 0x12, 0x21,
        0x34, 0x43, 0x56, 0x65, 0x78, 0x87, 0x9A, 0xBC
    };
    for (int i = 0; i < 16; ++i) {
        key[i] ^= fixed[i];
    }

    return std::vector<unsigned char>(key.begin(), key.end());
}

int main() {
    try {
        // Инициализация криптографии (если используются зашифрованные ресурсы)
        auto encryptionKey = generateEncryptionKey();
        CryptoUtils::setKey(encryptionKey);

        // Создание и запуск приложения
        Application app;
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}