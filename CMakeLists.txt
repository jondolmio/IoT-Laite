cmake_minimum_required(VERSION 3.16)

project(IoTLaite C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(iot_laite
    src/camera_db.c
    src/freertos_host_shim.c
    src/main.c
)

target_include_directories(iot_laite PRIVATE include)
target_compile_options(iot_laite PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(iot_laite PRIVATE m)
