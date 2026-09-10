#include "OpenGLRenderAPI.h"
#include <glad/glad.h>
#include <core/OGL.h>
#include "Debug/DebugLogger.h"

namespace Lindo::Graphics {

    GraphicsAPI IRenderAPI::s_API = GraphicsAPI::OpenGL;

    void OpenGLRenderAPI::Init() {
        // Инициализируем GLAD при старте контекста
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            LOG_CRITICAL("Failed to initialize GLAD!");
            throw std::runtime_error("GLAD init failed");
        }

        LOG_INFO("[GPU Vendor]   : " + std::string((const char*)glGetString(GL_VENDOR)));
        LOG_INFO("[GPU Renderer] : " + std::string((const char*)glGetString(GL_RENDERER)));

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_MULTISAMPLE);
    }

    void OpenGLRenderAPI::SetViewport(int x, int y, int width, int height) {
        glViewport(x, y, width, height);
    }

    void OpenGLRenderAPI::SetClearColor(const glm::vec4& color) {
        glClearColor(color.r, color.g, color.b, color.a);
    }

    void OpenGLRenderAPI::Clear(bool colorBuffer, bool depthBuffer) {
        GLbitfield mask = 0;
        if (colorBuffer) mask |= GL_COLOR_BUFFER_BIT;
        if (depthBuffer) mask |= GL_DEPTH_BUFFER_BIT;
        glClear(mask);
    }

    void OpenGLRenderAPI::SetDepthTest(bool enabled) {
        if (enabled) glEnable(GL_DEPTH_TEST);
        else glDisable(GL_DEPTH_TEST);
    }

    void OpenGLRenderAPI::SetDepthWrite(bool enabled) {
        glDepthMask(enabled ? GL_TRUE : GL_FALSE);
    }

    void OpenGLRenderAPI::SetDepthFuncLess() {
        glDepthFunc(GL_LESS);
    }

    void OpenGLRenderAPI::SetDepthFuncLEqual() {
        glDepthFunc(GL_LEQUAL);
    }

    void OpenGLRenderAPI::SetCullFace(bool enabled, bool backFace) {
        if (enabled) {
            glEnable(GL_CULL_FACE);
            glCullFace(backFace ? GL_BACK : GL_FRONT);
            glFrontFace(GL_CCW);
        }
        else {
            glDisable(GL_CULL_FACE);
        }
    }

    void OpenGLRenderAPI::SetBlending(bool enabled) {
        if (enabled) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        else {
            glDisable(GL_BLEND);
        }
    }

    void OpenGLRenderAPI::SetMultisampling(bool enabled) {
        if (enabled) glEnable(GL_MULTISAMPLE);
        else glDisable(GL_MULTISAMPLE);
    }

}