cmake_minimum_required(VERSION 3.5)

project(yolo-ros)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED COMPONENTS core highgui imgcodecs imgproc videoio)

# ONNX Runtime C++ SDK
set(ORT_DIR "$ENV{HOME}/onnxruntime")
find_library(ORT_LIB onnxruntime PATHS ${ORT_DIR}/lib REQUIRED NO_DEFAULT_PATH)

# Standalone executable
add_executable(cam2image_node
  src/cam2image.cpp
  src/remote_capture.cpp
  src/image_processor.cpp
)

ament_target_dependencies(cam2image_node
  rclcpp
  sensor_msgs
  std_msgs
  cv_bridge
  OpenCV
)

target_include_directories(cam2image_node PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include/${PROJECT_NAME}>"
  ${ORT_DIR}/include
)

target_link_libraries(cam2image_node ${ORT_LIB})

# Install only the executable
install(TARGETS
  cam2image_node
  DESTINATION lib/${PROJECT_NAME}
)

# Install headers
install(DIRECTORY include/ DESTINATION include/${PROJECT_NAME})

ament_package()