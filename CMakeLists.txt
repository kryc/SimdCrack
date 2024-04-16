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
if (NOT AVX OR AVX STREQUAL "" OR AVX STREQUAL "256")
    set(AVXFLAG "-mavx2" CACHE STRING "" FORCE)
elseif(AVX STREQUAL "512")
    set(AVXFLAG "-mavx512bw" CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Werror ${AVXFLAG}")
set(CMAKE_EXE_LINKER_FLAGS "${AVXFLAG} -lpthread -lgmpxx -lgmp")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -funroll-loops")
set(CMAKE_CXX_FLAGS_DEBUG "-g -ggdb -O0 -fno-omit-frame-pointer -DDEBUG")

if(APPLE)
    set(HOMEBREW_PREFIX "/usr/local" CACHE PATH "Path to Homebrew installation")
    set(HOMEBREW_INCLUDE ${HOMEBREW_PREFIX}/include)
    include_directories(/usr/local/include)
    link_directories(/usr/local/lib)
else()
    set(HOMEBREW_INCLUDE "")
endif()

file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(simdcrack ${SOURCES})
target_include_directories(simdcrack
                            PUBLIC
                                ./src/
                                ./SimdHash/src/
                                ./libdispatchqueue/include/
                        )
target_link_libraries(simdcrack simdhash dispatchqueue)