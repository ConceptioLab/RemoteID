cmake_minimum_required(VERSION 3.5)
project(RemoteID)

# Especifique as fontes do seu projeto
set(SOURCES
    src/bluetooth/remote.cpp
    src/bluetooth/advle.c
    src/bluetooth/scan.c
)

set(ROS_INCLUDE_DIRS
   /opt/ros/humble/include/rclcpp
   /opt/ros/humble/include/rcl
   /opt/ros/humble/include/rcutils
   /opt/ros/humble/include/rmw
   /opt/ros/humble/include/rcl_yaml_param_parser
   /opt/ros/humble/include/rosidl_runtime_c
   /opt/ros/humble/include/rosidl_typesupport_interface
   /opt/ros/humble/include/rcpputils
   /opt/ros/humble/include/builtin_interfaces
   /opt/ros/humble/include/rosidl_runtime_cpp
   /opt/ros/humble/include/tracetools
   /opt/ros/humble/include/rcl_interfaces
   /opt/ros/humble/include/libstatistics_collector
   /opt/ros/humble/include/statistics_msgs
   /opt/ros/humble/include/std_msgs
)

# Encontre as bibliotecas que você precisa
find_package(PkgConfig REQUIRED)
pkg_check_modules(BLUETOOTH REQUIRED bluez)
pkg_check_modules(GPS REQUIRED libgps)
pkg_check_modules(CONFIG REQUIRED libconfig)

# Adicione o executável
add_executable(remote ${SOURCES})

# Inclua diretórios de cabeçalho
target_include_directories(remote PRIVATE
    ${BLUETOOTH_INCLUDE_DIRS}
    ${GPS_INCLUDE_DIRS}
    ${CONFIG_INCLUDE_DIRS}
)

# Especifique as bibliotecas a serem vinculadas
target_link_libraries(remote
    ${BLUETOOTH_LIBRARIES}
    ${GPS_LIBRARIES}
    ${CONFIG_LIBRARIES}
    -lm
    -pthread
)