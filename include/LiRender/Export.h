#pragma once

#if defined(_WIN32)
  #if defined(LI_RENDER_BUILD)
    #define LI_RENDER_API __declspec(dllexport)
  #else
    #define LI_RENDER_API __declspec(dllimport)
  #endif
#else
  #define LI_RENDER_API __attribute__((visibility("default")))
#endif
