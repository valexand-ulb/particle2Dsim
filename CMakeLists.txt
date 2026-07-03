cmake_minimum_required(VERSION 4.2)
project(particle2Dsim)

set(CMAKE_CXX_STANDARD 20)

find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(Vulkan REQUIRED)


add_executable(particle2Dsim src/main.cpp
        src/BasicApp.cpp
        src/BasicApp.h
        external/stb_image_impl.cpp
        external/tinyob_impl.cpp)

target_link_libraries(particle2Dsim
        glfw
        Vulkan::Vulkan
)
