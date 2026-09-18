cmake_minimum_required(VERSION 3.15)
project(Lindo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==============================================================================
# Директории вывода: бинарники и .dll в bin/, статические либы в lib/
# ==============================================================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Lindo/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Lindo/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Lindo/lib")

# Для MSVC / Multi-config (Debug/Release), чтобы не создавались подпапки
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_BINARY_DIR}/Lindo/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Lindo/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_BINARY_DIR}/Lindo/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Lindo/bin")
# ==============================================================================

execute_process(COMMAND ${CMAKE_COMMAND} --version)

option(LINDO_BUILD_APPDIR "Pack linux build" OFF)
option(LINDO_BUILD_TESTS  "Build tests"      OFF)

add_compile_definitions(VC_BUILD_NAME="${VC_BUILD_NAME}")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    if(VCPKG_TARGET_TRIPLET MATCHES "static")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
endif()

add_subdirectory(src)

# ==============================================================================
# Иконка приложения (только Windows) через .rc файл
# ==============================================================================
set(LINDO_APP_SOURCES src/main.cpp)

if(WIN32)
    # Файл ресурсов, в котором описана иконка (см. assets/app.rc)
    set(LINDO_RC_FILE "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/Windows/icon.rc")

    if(EXISTS "${LINDO_RC_FILE}")
        list(APPEND LINDO_APP_SOURCES "${LINDO_RC_FILE}")

        # Для MSVC включаем кодировку UTF-8 для rc-компилятора,
        # чтобы пути с кириллицей/юникодом не ломались.
        if(MSVC)
            set_source_files_properties("${LINDO_RC_FILE}" PROPERTIES
                COMPILE_FLAGS "-c65001")
        endif()
    else()
        message(WARNING "app.rc not found, executable will use default icon: ${LINDO_RC_FILE}")
    endif()
endif()

add_executable(Lindo WIN32 ${LINDO_APP_SOURCES})

if(MSVC)
    target_link_options(Lindo PRIVATE /ENTRY:mainCRTStartup)
endif()

if(LINDO_BUILD_APPDIR)
    include(${CMAKE_CURRENT_SOURCE_DIR}/dev/cmake/BuildAppdir.cmake)
endif()

target_link_libraries(Lindo PRIVATE LindoSrc
                                    $<$<PLATFORM_ID:Windows>:winmm>)

target_link_options(Lindo PRIVATE $<$<CXX_COMPILER_ID:GNU>:-no-pie>)

# Deploy assets -> build/Lindo/assets
add_custom_command(
    TARGET Lindo
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/Lindo/assets)

# config/ -> build/Lindo/config
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config")
    add_custom_command(
        TARGET Lindo
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
                ${CMAKE_CURRENT_SOURCE_DIR}/config ${CMAKE_BINARY_DIR}/Lindo/config)
else()
    add_custom_command(
        TARGET Lindo
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/Lindo/config)
endif()