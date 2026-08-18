cmake_minimum_required(VERSION 3.15)
project(Lindo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==============================================================================
# НАСТРОЙКА ВЫХОДНЫХ ДИРЕКТОРИЙ (bin/ и lib/)
# ==============================================================================
# Направляем все .exe и .dll файлы в папку bin/ внутри директории сборки
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Lindo/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Lindo/bin")
# Статические библиотеки (.lib / .a) убираем в lib/
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Lindo/lib")

# Для MSVC / Multi-configuration генераторов (Debug/Release), чтобы не создавались
# лишние подпапки bin/Debug, а всё падало строго в bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Lindo/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Lindo/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Lindo/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Lindo/bin")
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
add_executable(Lindo WIN32 src/main.cpp)

if(MSVC)
    target_link_options(Lindo PRIVATE /ENTRY:mainCRTStartup)
endif()

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
            ${CMAKE_CURRENT_SOURCE_DIR}/res ${CMAKE_BINARY_DIR}/Lindo/res)

# Если в проекте есть папка config/, копируем её в build/config/
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config")
    add_custom_command(
        TARGET Lindo
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
                ${CMAKE_CURRENT_SOURCE_DIR}/config ${CMAKE_BINARY_DIR}/Lindo/config)
else()
    # Если папки config еще нет в исходниках, просто создадим пустую папку build/config
    add_custom_command(
        TARGET Lindo
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/config)
endif()