cmake_minimum_required(VERSION 2.6)
file(TO_CMAKE_PATH "$ENV{IMAGINEPP_ROOT}/CMake" p)
list(APPEND CMAKE_MODULE_PATH "${p}") #For old Imagine++
list(APPEND CMAKE_SYSTEM_FRAMEWORK_PATH /Library/Frameworks) #Mac, why not auto?
find_package(Imagine REQUIRED)

project(Boundaries)

add_library(Shape
            flst.cpp
            shape.h shape.cpp
            tree.h tree.cpp)

add_executable(boundaries main.cpp)
ImagineUseModules(boundaries Images)
target_link_libraries(boundaries Shape)
