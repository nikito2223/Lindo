#pragma once
#if defined(_WIN32)
  #if defined(LI_UI_BUILD)
    #define LI_UI_API __declspec(dllexport)
  #else
    #define LI_UI_API __declspec(dllimport)
  #endif
#else
  #define LI_UI_API __attribute__((visibility("default")))
#endif
