add_definitions(-DTHIS_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

set(SRC image.hpp
        sample.hpp
        pixelTypes.hpp
        image_converter.hpp
        image_crop.hpp
        image_io.hpp
        image_io.cpp
        image_drawing.hpp)

add_library(image ${SRC})

# zlib is needed by libPNG
find_package(ZLIB REQUIRED)
if(NOT ZLIB_FOUND)
  add_subdirectory(../../third_party/zlib ../../third_party/zlib)
endif(NOT ZLIB_FOUND)
include_directories(${ZLIB_INCLUDE_DIRS})

find_package(PNG REQUIRED)
if(NOT PNG_FOUND)
    add_subdirectory(../../third_party/png ../../third_party/png)
endif(NOT PNG_FOUND)
include_directories(${PNG_INCLUDE_DIRS})

find_package(JPEG REQUIRED)
if(NOT JPEG_FOUND)
    add_subdirectory(../../third_party/jpeg ../../third_party/jpeg)
endif(NOT JPEG_FOUND)
include_directories(${JPEG_INCLUDE_DIR})

add_definitions(${PNG_DEFINITIONS})

target_link_libraries(image ${PNG_LIBRARIES} ${JPEG_LIBRARIES})

if(UNIX)
    set_target_properties(image PROPERTIES
                          COMPILE_FLAGS "-std=c++98 -Wall -Wextra -Werror")
endif(UNIX)

if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(WIN32)

# unit_test(image "image")
# unit_test(image_drawing "image")
# unit_test(image_io "image")
