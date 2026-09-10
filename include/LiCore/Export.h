#pragma once

#if defined(_WIN32)
  #if defined(LI_CORE_BUILD)
    #define LI_CORE_API __declspec(dllexport)
  #else
    #define LI_CORE_API __declspec(dllimport)
  #endif
#else
  #define LI_CORE_API __attribute__((visibility("default")))
#endif

#ifndef LI_API
#define LI_API LI_CORE_API
#endif
