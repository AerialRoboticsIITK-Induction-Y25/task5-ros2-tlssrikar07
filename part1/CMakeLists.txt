cmake_minimum_required(VERSION 3.10)
project(drone_oop_sim CXX)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include_directories(include)

add_executable(drone_sim
    src/main.cpp
    src/vehicle.cpp
    src/drone.cpp
    src/mission_drone.cpp
    src/autonomous_drone.cpp
)

set_target_properties(drone_sim PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
