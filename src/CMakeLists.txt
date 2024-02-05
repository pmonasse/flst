cmake_minimum_required(VERSION 3.5)
project(Boundaries)

add_library(Shape
            flst.cpp
            shape.h shape.cpp
            tree.h tree.cpp)

option(Boundary "Store boundary of shapes" ON)
if(Boundary)
  add_definitions(-DBOUNDARY)
endif()

find_package(PNG)
find_package(JPEG)
if(PNG_FOUND AND JPEG_FOUND)
    add_subdirectory(libImage)
    add_executable(check_FLST check_FLST.cpp)
    target_link_libraries(check_FLST image Shape)

    add_executable(test_FLST test_FLST.cpp)
    target_link_libraries(test_FLST image Shape)

    add_executable(test_oldFLST test_oldFLST.cpp ClassicalFLST/oldFlst.cpp)
    target_link_libraries(test_oldFLST image Shape)
endif()                           

find_package(Imagine COMPONENTS Images)
if(Imagine_FOUND)
  add_executable(boundaries main.cpp)
  target_link_libraries(boundaries Shape Imagine::Images)
endif()
