cmake_minimum_required (VERSION 3.15)

project ("XitEngine" VERSION 0.1 LANGUAGES C CXX)

find_package(imgui CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(minhook CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

# --- ALVO 1: A DLL do SpeedHack ---
# Criamos uma SHARED library para gerar o arquivo .dll
add_library(speedhack SHARED src/dll/speedhack.cpp)
target_link_libraries(speedhack PRIVATE minhook::minhook)
set_target_properties(speedhack PROPERTIES PREFIX "" SUFFIX ".dll")

file(GLOB SRC_FILES src/*.cpp)

# --- ALVO 2: O Executável Principal ---
add_executable(XitEngine ${SRC_FILES})

target_include_directories(XitEngine PRIVATE include)
target_link_libraries(XitEngine PRIVATE 
    imgui::imgui 
    glfw 
    OpenGL::GL 
)

target_compile_definitions(XitEngine PRIVATE DLL_PATH="speedhack.dll")