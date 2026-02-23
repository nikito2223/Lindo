#include "WindowCallbacks.h"
#include "Globals.h"
#include <render/Scene/Scene.h>

void framebuffer_size_callback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow*, double xposIn, double yposIn)
{
    float xpos = (float)xposIn;
    float ypos = (float)yposIn;

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    FirstPersonCamera* cam = getActiveCamera();
    if (cam) {
        cam->processMouseMovement(xoffset, yoffset, true);
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    FirstPersonCamera* cam = getActiveCamera();
    if (cam) {
        cam->processMouseScroll(static_cast<float>(yoffset));
    }
}
