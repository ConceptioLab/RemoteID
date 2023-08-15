cmake_minimum_required(VERSION 3.5)
project(RemoteID)

# Especifique as fontes do seu projeto
set(SOURCES
    src/bluetooth/remoteros.cpp
    src/bluetooth/advle.c
    src/bluetooth/scan.c
)
set(ROS_INCLUDE_DIR
   /opt/ros/humble/include/rclcpp
)

# Encontre as bibliotecas que você precisa
find_package(PkgConfig REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
pkg_check_modules(BLUETOOTH REQUIRED bluez)
pkg_check_modules(GPS REQUIRED libgps)
pkg_check_modules(CONFIG REQUIRED libconfig)

# Adicione o executável
add_executable(remoteros ${SOURCES})

# Inclua diretórios de cabeçalho
target_include_directories(remoteros PRIVATE
    ${BLUETOOTH_INCLUDE_DIRS}
    ${GPS_INCLUDE_DIRS}
    ${CONFIG_INCLUDE_DIRS}
    ${ROS_INCLUDE_DIR}
)

# Especifique as bibliotecas a serem vinculadas
target_link_libraries(remoteros
    ${BLUETOOTH_LIBRARIES}
    ${GPS_LIBRARIES}
    ${CONFIG_LIBRARIES}
    -lm
    -pthread
)
