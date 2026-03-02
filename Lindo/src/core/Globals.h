#ifndef GLOBALS_H
#define GLOBALS_H

#include <glm/glm.hpp>
#include <string>
#include <Graphics/ui/UIWidget.h>
#include <Window/Camera.h>

class Camera;
class Player;

Camera* getActiveCamera();
Player* getPlayer();

extern unsigned int SCR_WIDTH;
extern unsigned int SCR_HEIGHT;
extern Camera camera;
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