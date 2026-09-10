#pragma once
#if defined(_WIN32)
  #if defined(LI_AUDIO_BUILD)
    #define LI_AUDIO_API __declspec(dllexport)
  #else
    #define LI_AUDIO_API __declspec(dllimport)
  #endif
#else
  #define LI_AUDIO_API __attribute__((visibility("default")))
#endif
