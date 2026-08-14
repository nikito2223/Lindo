project(LindoSrc)

file(GLOB_RECURSE headers ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
list(REMOVE_ITEM sources ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
list(APPEND sources ${CMAKE_CURRENT_SOURCE_DIR}/Graphics/core/DebugDraw.cpp)

add_library(LindoSrc STATIC ${sources} ${headers})

find_package(OpenGL REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(glm REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # specific for vcpkg
    find_package(OpenAL CONFIG REQUIRED)
else()
    find_package(OpenAL REQUIRED)
endif()
find_package(glfw3 REQUIRED)
find_package(Freetype REQUIRED)
find_package(PNG REQUIRED)

target_include_directories(LindoSrc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(LindoSrc PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/FS>
)

target_link_libraries(
    LindoSrc
    PRIVATE glfw
            OpenGL::GL
            glad::glad
            assimp::assimp
            nlohmann_json::nlohmann_json
            OpenAL::OpenAL
            Freetype::Freetype
            PNG::PNG
    PUBLIC glm::glm # Need public for src/delegates.hpp, which including to
                    # main.cpp
)

target_link_options(
    LindoSrc
    PUBLIC
    $<$<CXX_COMPILER_ID:GNU>:
    -no-pie
    >
    # Need for static compilation on Windows with clang TODO: Make single build
    # on Windows to avoid dependence on combinations of platforms and compilers
    # and make it independent
    $<$<PLATFORM_ID:Windows>:$<$<CXX_COMPILER_ID:Clang>:-static>>)