#include "Globals.h"

// 🔧 ИЗМЕНЕНО: инициализируем без const
unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;

FirstPersonCamera camera(glm::vec3(0.0f, 0.0f,0.0f));

float deltaTime = 0.0f;
float lastFrame = 0.0f;

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;