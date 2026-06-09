cmake_minimum_required(VERSION 3.10...3.30)
project(drone_fleet)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(std_srvs REQUIRED)

include_directories(include)

add_executable(drone_node 
    src/Drone_Node.cpp
)
target_include_directories(drone_node PRIVATE include)
ament_target_dependencies(drone_node rclcpp std_msgs std_srvs)

add_executable(fleet_manager src/fleet_manager.cpp)
target_include_directories(fleet_manager PRIVATE include)
ament_target_dependencies(fleet_manager rclcpp std_msgs std_srvs)

add_executable(health_monitor src/health_monitor.cpp)
target_include_directories(health_monitor PRIVATE include)
ament_target_dependencies(health_monitor rclcpp std_msgs std_srvs)

install(TARGETS
  drone_node
  fleet_manager
  health_monitor
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}/
)

ament_package()
