#include "CryptoUtils.h"
#include <fstream>
#include <iterator>
#include <stdexcept>

std::vector<unsigned char> CryptoUtils::s_key;

const std::vector<unsigned char>& CryptoUtils::getKey() {
    return s_key;
}

void CryptoUtils::setKey(const std::vector<unsigned char>& key) {
    s_key = key;
}

// Исправлено: один параметр, используем внутренний ключ s_key
void CryptoUtils::xorTransform(std::vector<char>& data) {
    if (s_key.empty()) return;
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] ^= s_key[i % s_key.size()];
    }
}

std::string CryptoUtils::decryptFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot open file: " + path);

    std::vector<char> encrypted((std::istreambuf_iterator<char>(file)), {});
    xorTransform(encrypted);  // теперь один аргумент
    return std::string(encrypted.begin(), encrypted.end());
}

std::vector<char> CryptoUtils::decryptFileBinary(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot open file: " + path);

    std::vector<char> encrypted((std::istreambuf_iterator<char>(file)), {});
    xorTransform(encrypted);
    return encrypted;
}