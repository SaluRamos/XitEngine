cmake_minimum_required (VERSION 3.15)

project ("RobloxEng" VERSION 0.1 LANGUAGES C CXX)

find_package(imgui CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

file(GLOB SRC_FILES src/*.cpp)

add_executable(RobloxEng ${SRC_FILES})

target_include_directories(RobloxEng PRIVATE include)
target_link_libraries(RobloxEng PRIVATE 
    imgui::imgui 
    glfw 
    OpenGL::GL 
)
