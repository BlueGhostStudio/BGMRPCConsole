cmake_minimum_required(VERSION 3.14)

project(BGMRPCClientDaemon LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core DBus)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core DBus)

set(daemon_SRCS)
qt_add_dbus_adaptor(daemon_SRCS
    ../common/daemon.xml
    bgmrpcclientdaemon.h
    BGMRPCClientDaemon
    daemon_adaptor
)

add_executable(BGMRPCClientDaemon
  main.cpp
  bgmrpcclientdaemon.h
  bgmrpcclientdaemon.cpp
  ${daemon_SRCS}
)
target_link_libraries(BGMRPCClientDaemon Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::DBus)

install(TARGETS BGMRPCClientDaemon
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
