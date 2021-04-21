# CMake에서 주석은 #을 사용해 작성합니다
set(AUTHOR_NAME "bradkim06")
cmake_minimum_required(VERSION 3.14)    # CMake 버전을 명시
set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_definitions(__MSP430F415__)
set(IAR_BASE_ROOT /mnt/d/Project/inc)
project(TurbinMeter)

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS
    "*.h"
    "*.c"
    )
SET(App ${SRC})
ADD_EXECUTABLE(App ${SRC})
include_directories(
  ${IAR_BASE_ROOT}
)
