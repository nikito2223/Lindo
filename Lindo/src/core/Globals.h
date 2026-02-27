#ifndef GLOBALS_H
#define GLOBALS_H

#include <glm/glm.hpp>
#include <camera/FirstPersonCamera.h>
#include <string>
#include <render/UI/UIWidget.h>

class FirstPersonCamera;
class Player;

FirstPersonCamera* getActiveCamera();
Player* getPlayer();

extern unsigned int SCR_WIDTH;
extern unsigned int SCR_HEIGHT;
extern FirstPersonCamera camera;
extern float deltaTime;
extern float lastFrame;
extern bool useRawResources;      // только объявление
extern bool DebugMode;      // только объявление
extern float lastX;
extern float lastY;
extern bool firstMouse;
extern const std::string PathData; // только объявление
extern bool uiActive;
extern std::shared_ptr<UIPanel> g_rootPanel;
#endif