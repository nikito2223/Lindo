#ifndef SHADER_H
#define SHADER_H

#include "./core/OGL.h"
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <unordered_map>
#include "../antires/CryptoUtils.h"

class Shader
{
public:
    unsigned int ID;

    // Конструкторы для разных комбинаций шейдеров
    // ------------------------------------------------------------------------

    // Vertex + Fragment
    Shader(const char* vertexPath, const char* fragmentPath)
    {
        compileShader(vertexPath, nullptr, fragmentPath);
    }

    // Vertex + Geometry + Fragment
    Shader(const char* vertexPath, const char* geometryPath, const char* fragmentPath)
    {
        compileShader(vertexPath, geometryPath, fragmentPath);
    }

    // Конструктор для загрузки из строк (для динамической генерации шейдеров)
    Shader(const std::string& vertexCode, const std::string& fragmentCode, bool fromString = true)
    {
        if (fromString) {
            compileFromString(vertexCode, "", fragmentCode);
        }
    }

    Shader(const std::string& vertexCode, const std::string& geometryCode,
        const std::string& fragmentCode, bool fromString = true)
    {
        if (fromString) {
            compileFromString(vertexCode, geometryCode, fragmentCode);
        }
    }

    // Деструктор
    ~Shader()
    {
        glDeleteProgram(ID);
    }

    // Копирование запрещено (или реализуйте правильное копирование)
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    // Move конструктор и оператор
    Shader(Shader&& other) noexcept : ID(other.ID)
    {
        other.ID = 0;
    }

    Shader& operator=(Shader&& other) noexcept
    {
        if (this != &other) {
            glDeleteProgram(ID);
            ID = other.ID;
            other.ID = 0;
        }
        return *this;
    }

    // ------------------------------------------------------------------------
    void use() const
    {
        glUseProgram(ID);
    }

    // 🔧 ДОБАВЛЕНО: Отключение шейдера
    static void unuse()
    {
        glUseProgram(0);
    }

    // 🔧 ДОБАВЛЕНО: Получение ID программы
    unsigned int getID() const { return ID; }

    // 🔧 ДОБАВЛЕНО: Проверка валидности шейдера
    bool isValid() const { return ID != 0; }

    // utility uniform functions
    // ------------------------------------------------------------------------
    void setBool(const std::string& name, bool value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }

    void setInt(const std::string& name, int value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setFloat(const std::string& name, float value) const
    {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setVec2(const std::string& name, const glm::vec2& value) const
    {
        glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void setVec2(const std::string& name, float x, float y) const
    {
        glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
    }

    void setVec3(const std::string& name, const glm::vec3& value) const
    {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void setVec3(const std::string& name, float x, float y, float z) const
    {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
    }

    void setVec4(const std::string& name, const glm::vec4& value) const
    {
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void setVec4(const std::string& name, float x, float y, float z, float w) const
    {
        glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
    }

    void setMat2(const std::string& name, const glm::mat2& mat) const
    {
        glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    void setMat3(const std::string& name, const glm::mat3& mat) const
    {
        glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    void setMat4(const std::string& name, const glm::mat4& mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    // 🔧 ДОБАВЛЕНО: Установка массива матриц
    void setMat4Array(const std::string& name, const std::vector<glm::mat4>& matrices) const
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()),
            static_cast<GLsizei>(matrices.size()), GL_FALSE, &matrices[0][0][0]);
    }

    // 🔧 ДОБАВЛЕНО: Установка массива float
    void setFloatArray(const std::string& name, const float* values, int count) const
    {
        glUniform1fv(glGetUniformLocation(ID, name.c_str()), count, values);
    }

    // 🔧 ДОБАВЛЕНО: Установка массива vec3
    void setVec3Array(const std::string& name, const glm::vec3* values, int count) const
    {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), count, &values[0][0]);
    }

    // 🔧 ДОБАВЛЕНО: Получение uniform location (для кэширования)
    int getUniformLocation(const std::string& name) const
    {
        return glGetUniformLocation(ID, name.c_str());
    }

    // 🔧 ДОБАВЛЕНО: Установка uniform с кэшированием (опционально)
    void setBoolCached(const std::string& name, bool value) const
    {
        static std::unordered_map<std::string, int> locationCache;
        auto it = locationCache.find(name);
        if (it == locationCache.end()) {
            int loc = glGetUniformLocation(ID, name.c_str());
            locationCache[name] = loc;
            glUniform1i(loc, (int)value);
        }
        else {
            glUniform1i(it->second, (int)value);
        }
    }

    // 🔧 ДОБАВЛЕНО: Релоад шейдера (горячая перезагрузка)
    bool reload(const char* vertexPath = nullptr, const char* geometryPath = nullptr, const char* fragmentPath = nullptr)
    {
        unsigned int oldID = ID;

        if (vertexPath && fragmentPath) {
            if (geometryPath) {
                compileShader(vertexPath, geometryPath, fragmentPath);
            }
            else {
                compileShader(vertexPath, nullptr, fragmentPath);
            }
        }

        if (ID == 0) {
            // Если компиляция не удалась, восстанавливаем старый шейдер
            ID = oldID;
            std::cout << "ERROR::SHADER::RELOAD_FAILED: Keeping old shader." << std::endl;
            return false;
        }

        // Удаляем старый шейдер
        glDeleteProgram(oldID);
        std::cout << "Shader reloaded successfully!" << std::endl;
        return true;
    }

    // 🔧 ДОБАВЛЕНО: Методы для отладки
    void printUniforms() const
    {
        GLint numUniforms = 0;
        glGetProgramiv(ID, GL_ACTIVE_UNIFORMS, &numUniforms);

        std::cout << "Active uniforms in shader " << ID << ":" << std::endl;

        for (GLint i = 0; i < numUniforms; ++i) {
            char name[256];
            GLsizei length;
            GLint size;
            GLenum type;

            glGetActiveUniform(ID, i, sizeof(name), &length, &size, &type, name);

            std::string typeStr;
            switch (type) {
            case GL_FLOAT: typeStr = "float"; break;
            case GL_FLOAT_VEC2: typeStr = "vec2"; break;
            case GL_FLOAT_VEC3: typeStr = "vec3"; break;
            case GL_FLOAT_VEC4: typeStr = "vec4"; break;
            case GL_INT: typeStr = "int"; break;
            case GL_BOOL: typeStr = "bool"; break;
            case GL_FLOAT_MAT4: typeStr = "mat4"; break;
            default: typeStr = "unknown"; break;
            }

            std::cout << "  " << name << " (" << typeStr << ")" << std::endl;
        }
    }

private:

    // Основная функция компиляции
    void compileShader(const char* vertexPath, const char* geometryPath, const char* fragmentPath)
    {
        std::string vertexCode;
        std::string geometryCode;
        std::string fragmentCode;

        // Чтение файлов
        try {
            if (vertexPath) {
                vertexCode = CryptoUtils::decryptFile(vertexPath);
            }
            if (geometryPath) {
                geometryCode = CryptoUtils::decryptFile(geometryPath);
            }
            if (fragmentPath) {
                fragmentCode = CryptoUtils::decryptFile(fragmentPath);
            }

            // Вершинный шейдер
            std::ifstream vShaderFile(vertexPath);
            if (!vShaderFile.is_open()) {
                throw std::runtime_error(std::string("Cannot open vertex shader: ") + vertexPath);
            }
            std::stringstream vShaderStream;
            vShaderStream << vShaderFile.rdbuf();
            vertexCode = vShaderStream.str();
            vShaderFile.close();

            // Геометрический шейдер (если есть)
            if (geometryPath) {
                std::ifstream gShaderFile(geometryPath);
                if (!gShaderFile.is_open()) {
                    throw std::runtime_error(std::string("Cannot open geometry shader: ") + geometryPath);
                }
                std::stringstream gShaderStream;
                gShaderStream << gShaderFile.rdbuf();
                geometryCode = gShaderStream.str();
                gShaderFile.close();
            }

            // Фрагментный шейдер
            std::ifstream fShaderFile(fragmentPath);
            if (!fShaderFile.is_open()) {
                throw std::runtime_error(std::string("Cannot open fragment shader: ") + fragmentPath);
            }
            std::stringstream fShaderStream;
            fShaderStream << fShaderFile.rdbuf();
            fragmentCode = fShaderStream.str();
            fShaderFile.close();
        }
        catch (std::exception& e) {
            std::cout << "ERROR::SHADER::FILE_READ_ERROR: " << e.what() << std::endl;
            return;
        }

        compileFromString(vertexCode, geometryCode, fragmentCode);
    }

    // Компиляция из строк
    void compileFromString(const std::string& vertexCode,
        const std::string& geometryCode,
        const std::string& fragmentCode)
    {
        const char* vShaderCode = vertexCode.c_str();
        const char* gShaderCode = geometryCode.empty() ? nullptr : geometryCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        // Компиляция шейдеров
        unsigned int vertex, geometry = 0, fragment;

        // Вершинный шейдер
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        // Геометрический шейдер (если есть)
        if (gShaderCode) {
            geometry = glCreateShader(GL_GEOMETRY_SHADER);
            glShaderSource(geometry, 1, &gShaderCode, NULL);
            glCompileShader(geometry);
            checkCompileErrors(geometry, "GEOMETRY");
        }

        // Фрагментный шейдер
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        // Создание программы
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        if (geometry) glAttachShader(ID, geometry);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        // Удаление шейдеров
        glDeleteShader(vertex);
        if (geometry) glDeleteShader(geometry);
        glDeleteShader(fragment);
    }

    // Проверка ошибок компиляции/линковки
    void checkCompileErrors(GLuint shader, std::string type)
    {
        GLint success;
        GLchar infoLog[1024];

        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n"
                    << infoLog << "\n -- --------------------------------------------------- -- "
                    << std::endl;
            }
        }
        else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n"
                    << infoLog << "\n -- --------------------------------------------------- -- "
                    << std::endl;
            }
            else {
                // 🔧 ДОБАВЛЕНО: Успешная линковка
                std::cout << "Shader program linked successfully! (ID: " << ID << ")" << std::endl;
            }
        }
    }
};

#endif