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
#include <unordered_set>
#include <filesystem>

#include "Debug/DebugLogger.h"

namespace Lindo {
    namespace Graphics {
        class Shader
        {
        public:
            unsigned int ID = 0;

            // Храним пути для удобного hot-reload
            std::string m_vertexPath;
            std::string m_geometryPath;
            std::string m_fragmentPath;

            // Конструкторы
            // ------------------------------------------------------------------------
            Shader(const std::string& vertexPath, const std::string& fragmentPath)
                : m_vertexPath(vertexPath), m_fragmentPath(fragmentPath) {
                LOG_INFO("[Shader] Loading shader program: Vertex='" + vertexPath + "', Fragment='" + fragmentPath + "'");
                compileShader(vertexPath.c_str(), nullptr, fragmentPath.c_str());
            }

            Shader(const std::string& vertexPath, const std::string& geometryPath, const std::string& fragmentPath)
                : m_vertexPath(vertexPath), m_geometryPath(geometryPath), m_fragmentPath(fragmentPath) {
                const char* geomPtr = geometryPath.empty() ? nullptr : geometryPath.c_str();
                LOG_INFO("[Shader] Loading shader program: Vertex='" + vertexPath + "', Geometry='" + geometryPath + "', Fragment='" + fragmentPath + "'");
                compileShader(vertexPath.c_str(), geomPtr, fragmentPath.c_str());
            }

            static Shader FromString(const std::string& vertexCode, const std::string& fragmentCode) {
                LOG_INFO("[Shader] Creating shader program from raw strings (Vertex & Fragment)");
                return Shader(vertexCode, fragmentCode, true);
            }

            static Shader FromString(const std::string& vertexCode, const std::string& geometryCode, const std::string& fragmentCode) {
                LOG_INFO("[Shader] Creating shader program from raw strings (Vertex, Geometry & Fragment)");
                return Shader(vertexCode, geometryCode, fragmentCode, true);
            }

            // Деструктор
            ~Shader()
            {
                if (ID != 0) {
                    LOG_INFO("[Shader] Deleting shader program ID: " + std::to_string(ID));
                    glDeleteProgram(ID);
                }
            }

            // Копирование запрещено
            Shader(const Shader&) = delete;
            Shader& operator=(const Shader&) = delete;

            // Move конструктор и оператор
            Shader(Shader&& other) noexcept
                : ID(other.ID),
                m_vertexPath(std::move(other.m_vertexPath)),
                m_geometryPath(std::move(other.m_geometryPath)),
                m_fragmentPath(std::move(other.m_fragmentPath))
            {
                other.ID = 0;
            }

            Shader& operator=(Shader&& other) noexcept
            {
                if (this != &other) {
                    if (ID != 0) {
                        glDeleteProgram(ID);
                    }
                    ID = other.ID;
                    m_vertexPath = std::move(other.m_vertexPath);
                    m_geometryPath = std::move(other.m_geometryPath);
                    m_fragmentPath = std::move(other.m_fragmentPath);
                    other.ID = 0;
                }
                return *this;
            }

            // ------------------------------------------------------------------------
            void use() const
            {
                glUseProgram(ID);
            }

            static void unuse()
            {
                glUseProgram(0);
            }

            unsigned int getID() const { return ID; }
            bool isValid() const { return ID != 0; }

            // Uniform utility functions
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

            void setMat4Array(const std::string& name, const std::vector<glm::mat4>& matrices) const
            {
                glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()),
                    static_cast<GLsizei>(matrices.size()), GL_FALSE, &matrices[0][0][0]);
            }

            void setFloatArray(const std::string& name, const float* values, int count) const
            {
                glUniform1fv(glGetUniformLocation(ID, name.c_str()), count, values);
            }

            void setVec3Array(const std::string& name, const glm::vec3* values, int count) const
            {
                glUniform3fv(glGetUniformLocation(ID, name.c_str()), count, &values[0][0]);
            }

            int getUniformLocation(const std::string& name) const
            {
                return glGetUniformLocation(ID, name.c_str());
            }

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

            // Перезагрузка шейдера (hot-reload)
            bool reload(const char* vertexPath = nullptr, const char* geometryPath = nullptr, const char* fragmentPath = nullptr)
            {
                LOG_INFO("[Shader] Reloading shader ID: " + std::to_string(ID) + "...");
                const char* vPath = vertexPath ? vertexPath : (m_vertexPath.empty() ? nullptr : m_vertexPath.c_str());
                const char* gPath = geometryPath ? geometryPath : (m_geometryPath.empty() ? nullptr : m_geometryPath.c_str());
                const char* fPath = fragmentPath ? fragmentPath : (m_fragmentPath.empty() ? nullptr : m_fragmentPath.c_str());

                unsigned int oldID = ID;
                ID = 0;

                if (vPath && fPath) {
                    compileShader(vPath, gPath, fPath);
                }

                if (ID == 0) {
                    ID = oldID;
                    LOG_ERROR("[Shader] Reload failed! Retaining old shader program ID: " + std::to_string(ID));
                    return false;
                }

                if (oldID != 0) {
                    glDeleteProgram(oldID);
                }

                if (vertexPath) m_vertexPath = vertexPath;
                if (geometryPath) m_geometryPath = geometryPath;
                if (fragmentPath) m_fragmentPath = fragmentPath;

                LOG_INFO("[Shader] Shader reloaded successfully! New Program ID: " + std::to_string(ID));
                return true;
            }

            void printUniforms() const
            {
                GLint numUniforms = 0;
                glGetProgramiv(ID, GL_ACTIVE_UNIFORMS, &numUniforms);

                LOG_INFO("[Shader] Active uniforms in shader program ID " + std::to_string(ID) + ":");

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

                    LOG_INFO("  Uniform #" + std::to_string(i) + ": " + std::string(name) + " (" + typeStr + ")");
                }
            }

        private:

            Shader(const std::string& vertexCode, const std::string& fragmentCode, bool /*fromString*/) {
                compileFromString(vertexCode, "", fragmentCode);
            }

            Shader(const std::string& vertexCode, const std::string& geometryCode, const std::string& fragmentCode, bool /*fromString*/) {
                compileFromString(vertexCode, geometryCode, fragmentCode);
            }

            // Обработка инклюдов (#include "filename.glsl")
            std::string preprocessShader(const std::string& filepath, std::unordered_set<std::string>& includedFiles) {
                std::filesystem::path fullPath = std::filesystem::absolute(filepath);
                std::string canonicalPath = fullPath.string();

                if (includedFiles.find(canonicalPath) != includedFiles.end()) {
                    LOG_DEBUG("[Shader Preprocessor] Skipping already included file: " + canonicalPath);
                    return "";
                }
                includedFiles.insert(canonicalPath);

                LOG_DEBUG("[Shader Preprocessor] Reading file: " + filepath);
                std::ifstream file(filepath);
                if (!file.is_open()) {
                    LOG_ERROR("[Shader Preprocessor] Cannot open file: " + filepath);
                    return "";
                }

                std::string source;
                std::string line;
                std::filesystem::path parentDir = fullPath.parent_path();
                int lineNumber = 0;

                while (std::getline(file, line)) {
                    lineNumber++;
                    size_t firstNonSpace = line.find_first_not_of(" \t");
                    if (firstNonSpace != std::string::npos && line.compare(firstNonSpace, 8, "#include") == 0) {
                        size_t start = line.find('"', firstNonSpace + 8);
                        size_t end = line.find('"', start + 1);

                        if (start != std::string::npos && end != std::string::npos && start < end) {
                            std::string includeRelPath = line.substr(start + 1, end - start - 1);
                            std::string includeFullPath = (parentDir / includeRelPath).string();

                            LOG_INFO("[Shader Preprocessor] Including: '" + includeRelPath + "' from " + filepath + ":" + std::to_string(lineNumber));
                            source += "// --- BEGIN INCLUDE: " + includeRelPath + " ---\n";
                            source += preprocessShader(includeFullPath, includedFiles) + "\n";
                            source += "// --- END INCLUDE: " + includeRelPath + " ---\n";
                            continue;
                        }
                        else {
                            LOG_WARN("[Shader Preprocessor] Invalid #include syntax at " + filepath + ":" + std::to_string(lineNumber));
                        }
                    }
                    source += line + "\n";
                }

                return source;
            }

            void compileShader(const char* vertexPath, const char* geometryPath, const char* fragmentPath) {
                std::string vertexCode;
                std::string geometryCode;
                std::string fragmentCode;

                try {
                    if (vertexPath) {
                        LOG_INFO("[Shader] Preprocessing Vertex Shader: " + std::string(vertexPath));
                        std::unordered_set<std::string> includedFiles;
                        vertexCode = preprocessShader(vertexPath, includedFiles);
                    }
                    if (geometryPath) {
                        LOG_INFO("[Shader] Preprocessing Geometry Shader: " + std::string(geometryPath));
                        std::unordered_set<std::string> includedFiles;
                        geometryCode = preprocessShader(geometryPath, includedFiles);
                    }
                    if (fragmentPath) {
                        LOG_INFO("[Shader] Preprocessing Fragment Shader: " + std::string(fragmentPath));
                        std::unordered_set<std::string> includedFiles;
                        fragmentCode = preprocessShader(fragmentPath, includedFiles);
                    }
                }
                catch (const std::exception& e) {
                    LOG_ERROR("[Shader] Exception during preprocessing: " + std::string(e.what()));
                    return;
                }

                compileFromString(vertexCode, geometryCode, fragmentCode);
            }

            void compileFromString(const std::string& vertexCode,
                const std::string& geometryCode,
                const std::string& fragmentCode)
            {
                const char* vShaderCode = vertexCode.c_str();
                const char* gShaderCode = geometryCode.empty() ? nullptr : geometryCode.c_str();
                const char* fShaderCode = fragmentCode.c_str();

                unsigned int vertex = 0, geometry = 0, fragment = 0;

                // Vertex Shader
                LOG_DEBUG("[Shader Compilation] Compiling Vertex Shader...");
                vertex = glCreateShader(GL_VERTEX_SHADER);
                glShaderSource(vertex, 1, &vShaderCode, NULL);
                glCompileShader(vertex);
                checkCompileErrors(vertex, "VERTEX");

                // Geometry Shader
                if (gShaderCode) {
                    LOG_DEBUG("[Shader Compilation] Compiling Geometry Shader...");
                    geometry = glCreateShader(GL_GEOMETRY_SHADER);
                    glShaderSource(geometry, 1, &gShaderCode, NULL);
                    glCompileShader(geometry);
                    checkCompileErrors(geometry, "GEOMETRY");
                }

                // Fragment Shader
                LOG_DEBUG("[Shader Compilation] Compiling Fragment Shader...");
                fragment = glCreateShader(GL_FRAGMENT_SHADER);
                glShaderSource(fragment, 1, &fShaderCode, NULL);
                glCompileShader(fragment);
                checkCompileErrors(fragment, "FRAGMENT");

                // Program Linking
                LOG_DEBUG("[Shader Linking] Linking Shader Program...");
                ID = glCreateProgram();
                glAttachShader(ID, vertex);
                if (geometry) glAttachShader(ID, geometry);
                glAttachShader(ID, fragment);
                glLinkProgram(ID);
                checkCompileErrors(ID, "PROGRAM");

                // Cleanup
                glDeleteShader(vertex);
                if (geometry) glDeleteShader(geometry);
                glDeleteShader(fragment);
            }
            
            void checkCompileErrors(GLuint shader, std::string type)
            {
                GLint success;
                GLchar infoLog[1024];

                if (type != "PROGRAM") {
                    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
                    if (!success) {
                        glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                        LOG_ERROR("[Shader Compilation Error] Type: " + type + "\n" + std::string(infoLog));
                    }
                    else {
                        LOG_DEBUG("[Shader Compilation Success] Stage: " + type);
                    }
                }
                else {
                    glGetProgramiv(shader, GL_LINK_STATUS, &success);
                    if (!success) {
                        glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                        LOG_ERROR("[Shader Linker Error] Program Linking Failed!\n" + std::string(infoLog));
                        ID = 0;
                    }
                    else {
                        LOG_INFO("[Shader Linker Success] Program linked successfully (ID: " + std::to_string(ID) + ")");
                    }
                }
            }
        };
    }
}

#endif