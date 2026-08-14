//#include "WindowCallbacks.h"
//#include <core/Globals.h>
//#include <Component\PlayerController\Player.h>
//namespace Lindo {
//    void framebuffer_size_callback(GLFWwindow*, int width, int height)
//    {
//        glViewport(0, 0, width, height);
//    }
//
//    void mouse_callback(GLFWwindow*, double xposIn, double yposIn)
//    {
//        if (Globals::uiActive && Globals::rootPanel) {
//            Globals::rootPanel->onMouseMove((float)xposIn, (float)yposIn);
//            return; // Не двигаем камеру, когда UI активен
//        }
//
//        float xpos = (float)xposIn;
//        float ypos = (float)yposIn;
//
//        if (Globals::firstMouse)
//        {
//            lastX = xpos;
//            lastY = ypos;
//            firstMouse = false;
//        }
//
//        float xoffset = xpos - lastX;
//        float yoffset = lastY - ypos;
//
//        lastX = xpos;
//        lastY = ypos;
//
//        Lindo::Components::Controller::Player* player = getPlayer();
//        if (!player) return;
//        auto* cam = player->owner->getComponent<Lindo::Components::Rendering::Camera>();
//        cam->processMouseMovement(xoffset, yoffset, true);
//    }
//
//    void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
//        Lindo::Components::Controller::Player* player = getPlayer();
//        if (!player) return;
//
//        auto* cam = player->owner->getComponent<Lindo::Components::Rendering::Camera>();
//        cam->processMouseScroll(static_cast<float>(yoffset));
//    }
//}