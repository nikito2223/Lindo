#ifndef GLOBALS_H
#define GLOBALS_H

#include <glm/glm.hpp>
#include <camera/FirstPersonCamera.h>

class FirstPersonCamera;
class Player;

FirstPersonCamera* getActiveCamera();
Player* getPlayer();

// 🔧 ИЗМЕНЕНО: убрали const, чтобы можно было менять разрешение
extern unsigned int SCR_WIDTH;
extern unsigned int SCR_HEIGHT;

extern FirstPersonCamera camera;

extern float deltaTime;
extern float lastFrame;

extern float lastX;
extern float lastY;
extern bool firstMouse;

#endif