#pragma once
#if defined(_WIN32)
  #if defined(LI_PHYSICS_BUILD)
    #define LI_PHYSICS_API __declspec(dllexport)
  #else
    #define LI_PHYSICS_API __declspec(dllimport)
  #endif
#else
  #define LI_PHYSICS_API __attribute__((visibility("default")))
#endif
