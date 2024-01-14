cmake_minimum_required(VERSION 3.10)

# set the project name
project(simdcrack VERSION 0.1)

# Set build type default Release
if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

add_subdirectory(./SimdHash/)
add_subdirectory(./libdispatchqueue/)

# specify clang
SET(CMAKE_C_COMPILER clang)
SET(CMAKE_CXX_COMPILER clang++)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# set compiler flags
set(STD_COMPILE_FLAGS "-Wall -Werror -fno-exceptions -g -O3")
set(STD_LINKER_FLAGS "-lpthread")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${STD_COMPILE_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${STD_COMPILE_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${STD_LINKER_FLAGS}")

set(CMAKE_CXX_FLAGS_DEBUG "-g -ggdb -O0 -fno-omit-frame-pointer -DDEBUG")

if(APPLE)
    set(HOMEBREW_PREFIX "/usr/local" CACHE PATH "Path to Homebrew installation")
    set(HOMEBREW_INCLUDE ${HOMEBREW_PREFIX}/include)
else()
    set(HOMEBREW_INCLUDE "")
endif()

file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(simdcrack ${SOURCES})
target_include_directories(simdcrack
                            PUBLIC
                                ./src/
                                ./SimdHash/SimdHash/
                                ./libdispatchqueue/include/
                                ${HOMEBREW_INCLUDE}
                        )
target_link_libraries(simdcrack simdhash dispatchqueue)