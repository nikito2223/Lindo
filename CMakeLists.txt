cmake_minimum_required(VERSION 3.15)
project(Lindo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==============================================================================
# НАСТРОЙКА ВЫХОДНЫХ ДИРЕКТОРИЙ (bin/ и lib/)
# ==============================================================================
# Направляем все .exe и .dll файлы в папку bin/ внутри директории сборки
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
# Статические библиотеки (.lib / .a) убираем в lib/
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# Для MSVC / Multi-configuration генераторов (Debug/Release), чтобы не создавались
# лишние подпапки bin/Debug, а всё падало строго в bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin")
# ==============================================================================

execute_process(COMMAND ${CMAKE_COMMAND} --version)

option(LINDO_BUILD_APPDIR "Pack linux build" OFF)
option(LINDO_BUILD_TESTS "Build tests" OFF)

add_compile_definitions(VC_BUILD_NAME="${VC_BUILD_NAME}")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    if(VCPKG_TARGET_TRIPLET MATCHES "static")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
endif()

add_subdirectory(src)
add_executable(Lindo src/main.cpp)

if(LINDO_BUILD_APPDIR)
    include(${CMAKE_CURRENT_SOURCE_DIR}/dev/cmake/BuildAppdir.cmake)
endif()

target_link_libraries(Lindo PRIVATE LindoSrc
                                    $<$<PLATFORM_ID:Windows>:winmm>)

target_link_options(Lindo PRIVATE $<$<CXX_COMPILER_ID:GNU>:-no-pie>)

# Deploy res to build/bin/res
# Благодаря $<TARGET_FILE_DIR:Lindo>, CMake автоматически скопирует папку res
# прямо туда, где лежит Lindo.exe (то есть в build/bin/res/)
add_custom_command(
    TARGET Lindo
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/res $<TARGET_FILE_DIR:Lindo>/res)