cmake_minimum_required(VERSION 3.8)
project(crazyflie_webots_cpp)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

# Dipendenze ROS 2 Jazzy e Webots Cyberbotics (come da guida ufficiale C++)
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(pluginlib REQUIRED)
find_package(webots_ros2_driver REQUIRED)
find_package(crazyflie_interfaces REQUIRED)

# 1. Esporta il file XML di configurazione del plugin C++ per webots_ros2_driver
pluginlib_export_plugin_description_file(webots_ros2_driver crazyflie_driver.xml)

# 2. Libreria condivisa del plugin C++ CrazyflieDriver
add_library(
  ${PROJECT_NAME}
  SHARED
  src/CrazyflieDriver.cpp
)
target_include_directories(
  ${PROJECT_NAME}
  PRIVATE
  include
)
ament_target_dependencies(
  ${PROJECT_NAME}
  pluginlib
  rclcpp
  geometry_msgs
  crazyflie_interfaces
  webots_ros2_driver
)
install(TARGETS
  ${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

# 3. Installazione Header C++ e Directory di Risorse (launch, resource, worlds, config)
install(
  DIRECTORY include/
  DESTINATION include
)
install(
  DIRECTORY launch resource worlds config
  DESTINATION share/${PROJECT_NAME}/
)

# 4. Installazione Server Crazyswarm2
install(
  PROGRAMS
    scripts/webots_crazyflie_server.py
  DESTINATION lib/${PROJECT_NAME}
)

ament_export_include_directories(
  include
)
ament_export_libraries(
  ${PROJECT_NAME}
)
ament_package()
