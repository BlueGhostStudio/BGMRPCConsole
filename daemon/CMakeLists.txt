cmake_minimum_required(VERSION 3.14)

project(
  BGMRPCConsoleDaemon
  VERSION 0.1
  LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core WebSockets DBus)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core WebSockets DBus)

include_directories(../BGMRPCClient)

set(daemon_SRCS)
qt_add_dbus_adaptor(daemon_SRCS ../common/daemon.xml bgmrpcconsoledaemon.h
                    BGMRPCConsoleDaemon daemon_adaptor)

add_executable(BGMRPCConsoleDaemon main.cpp bgmrpcconsoledaemon.h
                                   bgmrpcconsoledaemon.cpp ${daemon_SRCS})
target_link_libraries(
  BGMRPCConsoleDaemon Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::DBus
  Qt${QT_VERSION_MAJOR}::WebSockets BGMRPCClient)

set_target_properties(
  BGMRPCConsoleDaemon
  PROPERTIES
    OUTPUT_NAME BGMRPCConsoleDaemon
    INSTALL_RPATH_USE_LINK_PATH ON
    INSTALL_RPATH
    ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}:${BGMRPC_CLIENT}/${CMAKE_INSTALL_LIBDIR}
)

install(
  TARGETS BGMRPCConsoleDaemon
  LIBRARY DESTINATION ${BGMRPC_CLIENT}/whisp/${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${BGMRPC_CLIENT}/whisp/${CMAKE_INSTALL_BINDIR})

install(
  CODE "execute_process(COMMAND ln -sf ${BGMRPC_CLIENT}/whisp/${CMAKE_INSTALL_BINDIR}/BGMRPCConsoleDaemon /usr/local/bin/BGMRPCConsoleDaemon)"
)
