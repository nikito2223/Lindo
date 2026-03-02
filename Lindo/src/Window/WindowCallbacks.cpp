#include "WindowCallbacks.h"
#include <core/Globals.h>
#include "Objects/Player.h"

void framebuffer_size_callback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow*, double xposIn, double yposIn)
{
    if (uiActive && g_rootPanel) {
        g_rootPanel->onMouseMove((float)xposIn, (float)yposIn);
        return; // Не двигаем камеру, когда UI активен
    }

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

    Player* player = getPlayer();
    if (!player) return;

    Camera& cam = player->getCamera();
    cam.processMouseMovement(xoffset, yoffset, true);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    Player* player = getPlayer();
    if (!player) return;

    Camera& cam = player->getCamera();
    cam.processMouseScroll(static_cast<float>(yoffset));
}