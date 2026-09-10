#pragma once
#if defined(_WIN32)
  #if defined(LI_SCRIPTING_BUILD)
    #define LI_SCRIPTING_API __declspec(dllexport)
  #else
    #define LI_SCRIPTING_API __declspec(dllimport)
  #endif
#else
  #define LI_SCRIPTING_API __attribute__((visibility("default")))
#endif
