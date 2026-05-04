cmake_minimum_required(VERSION 3.8)
project(navio)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(navio_interfaces REQUIRED)

# ---- Shared library (your existing code) ----
add_library(${PROJECT_NAME} SHARED
  src/truck_node.cpp
  src/Common/Util.cpp
  src/Common/I2Cdev.cpp
  src/Navio2/ADC_Navio2.cpp
  src/Common/MS5611.cpp
  src/Common/MPU9250.cpp
  src/Navio2/LSM9DS1.cpp
  src/Common/Ublox.cpp
  src/Navio2/RGBled.cpp
  src/Common/gpio.cpp
  src/Navio2/PWM.cpp
)

ament_target_dependencies(${PROJECT_NAME} rclcpp std_msgs sensor_msgs navio_interfaces)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_compile_features(${PROJECT_NAME} PUBLIC c_std_99 cxx_std_17)

# ---- Executable that runs the node ----
add_executable(truck_node src/truck_node.cpp)
ament_target_dependencies(truck_node rclcpp)
target_link_libraries(truck_node ${PROJECT_NAME})

# ---- Install ----
# Install the executable where ros2 run expects it:
install(TARGETS truck_node
  DESTINATION lib/${PROJECT_NAME}
)

# Install the shared library + export targets
install(TARGETS ${PROJECT_NAME}
  EXPORT navioTargets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(
  DIRECTORY include/
  DESTINATION include
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_export_targets(navioTargets HAS_LIBRARY_TARGET)
ament_export_dependencies(rclcpp std_msgs navio_interfaces)
ament_package()