// CryptoUtils.h
#pragma once

#include <vector>
#include <string>

class CryptoUtils {
public:
    // Получить текущий ключ шифрования
    static const std::vector<unsigned char>& getKey();

    // Установить ключ (вызывается один раз при инициализации сцены)
    static void setKey(const std::vector<unsigned char>& key);

    // Применить XOR к данным (шифрование/дешифрование)
    static void xorTransform(std::vector<char>& data);

    // Прочитать и расшифровать файл как строку (для текстовых данных)
    static std::string decryptFile(const std::string& path);

    // Прочитать и расшифровать файл как вектор байт (для бинарных данных)
    static std::vector<char> decryptFileBinary(const std::string& path);

private:
    static std::vector<unsigned char> s_key;
};