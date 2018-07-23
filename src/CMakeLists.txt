cmake_minimum_required(VERSION 2.6)

add_library(Shape
            flst.cpp
            shape.h shape.cpp
            tree.h tree.cpp)

option(Boundary "Store boundary of shapes" ON)
if(Boundary)
  add_definitions(-DBOUNDARY)
endif()

add_subdirectory(libImage)
add_executable(test test.cpp)
target_link_libraries(test image Shape)

add_executable(test_oldFLST test_oldFLST.cpp ClassicalFLST/oldFlst.cpp)
target_link_libraries(test_oldFLST image Shape)

find_package(Imagine)
if(Imagine_FOUND)
  add_executable(boundaries main.cpp)
  ImagineUseModules(boundaries Images)
  target_link_libraries(boundaries Shape)
endif()
