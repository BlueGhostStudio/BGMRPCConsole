cmake_minimum_required(VERSION 3.14)

include(GNUInstallDirs)
set(WHISPDATALOCATION
    "${BGMRPC_CLIENT}/whisp/${CMAKE_INSTALL_DATADIR}"
    CACHE STRING "")
add_compile_definitions(WHISPDATALOCATION="${WHISPDATALOCATION}")

project(whisp LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core DBus)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core DBus)

set(client_SRCS)
qt_add_dbus_interface(client_SRCS ../common/daemon.xml client_interface)

add_executable(whisp main.cpp ${client_SRCS})
target_link_libraries(whisp Qt${QT_VERSION_MAJOR}::Core
                      Qt${QT_VERSION_MAJOR}::DBus)

add_custom_target(
  watchScripts ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/watch
          ${CMAKE_CURRENT_BINARY_DIR}/watch
  SOURCES watch/watch_keepalive.sh watch/watch_state.sh)

install(
  TARGETS whisp
  LIBRARY DESTINATION ${BGMRPC_CLIENT}/whisp/${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${BGMRPC_CLIENT}/whisp/${CMAKE_INSTALL_BINDIR})

install(
  DIRECTORY watch
  DESTINATION ${WHISPDATALOCATION}
  PATTERN
    "watch/*"
    PERMISSIONS
      OWNER_EXECUTE
      OWNER_WRITE
      OWNER_READ
      GROUP_EXECUTE
      GROUP_READ
      WORLD_EXECUTE
      WORLD_READ)

install(
  CODE "execute_process(COMMAND ln -sf ${BGMRPC_CLIENT}/whisp/${CMAKE_INSTALL_BINDIR}/whisp /usr/local/bin/whisp)"
)
