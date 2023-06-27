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

install(
  TARGETS whisp
  LIBRARY DESTINATION ${BGMRPC_CLIENT}/whisp/${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${BGMRPC_CLIENT}/whisp/${CMAKE_INSTALL_BINDIR})

add_executable(cmsUpdate api/cms/updateForm.cpp)
target_link_libraries(cmsUpdate PRIVATE newt ncurses
                                        Qt${QT_VERSION_MAJOR}::Core)
install(TARGETS cmsUpdate RUNTIME DESTINATION ${WHISPDATALOCATION}/api/cms)

add_custom_target(
  api ALL
  SOURCES api/default/initial.sh
          api/default/table.jq
          api/default/watch_state.sh
          api/default/watch_keepalive.sh
          api/account/initial.sh
          api/account/watch_state.sh
          api/account/login.sh
          api/account/logout.sh
          api/cms/initial.sh
          api/cms/prepare_path.sh
          api/cms/cd.sh
          api/cms/list.sh
          api/cms/new.sh
          api/cms/remove.sh
          api/cms/edit.sh
          api/cms/update.sh
          api/cms/watch_signal.sh
          api/cms/watch_state.sh
          api/cms/export.sh
          api/cms/import.sh
          api/todo/initial.sh
          api/todo/open.sh)

install(
  DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/api
  DESTINATION ${WHISPDATALOCATION}
  FILES_MATCHING
  PATTERN
    "*.sh"
    PERMISSIONS
      OWNER_EXECUTE
      OWNER_WRITE
      OWNER_READ
      GROUP_EXECUTE
      GROUP_READ
      WORLD_EXECUTE
      WORLD_READ
  PATTERN
    "*.jq"
    PERMISSIONS
      OWNER_EXECUTE
      OWNER_WRITE
      OWNER_READ
      GROUP_EXECUTE
      GROUP_READ
      WORLD_EXECUTE
      WORLD_READ
  PATTERN "vim" EXCLUDE)

execute_process(
  COMMAND vim -es -c "exec \"!echo \" . $VIM"
  OUTPUT_VARIABLE VIM_RUNTIME
  OUTPUT_STRIP_TRAILING_WHITESPACE)

add_custom_target(
  whispVimPlug ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/vim
          ${CMAKE_CURRENT_BINARY_DIR}/vim
  SOURCES vim/plugin/WhispOutput.vim)

add_custom_target(
  cmsVimPlug ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_CURRENT_LIST_DIR}/api/cms/vim ${CMAKE_CURRENT_BINARY_DIR}/vim
  SOURCES api/cms/vim/plugin/WhispCMSBuf.vim api/cms/vim/plugin/WhispCMSCmd.vim)

install(
  DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/vim/plugin
  DESTINATION ${VIM_RUNTIME}/vimfiles/after
  PATTERN
    "after/plugin"
    PERMISSIONS
      OWNER_WRITE
      OWNER_READ
      OWNER_EXECUTE
      GROUP_READ
      GROUP_EXECUTE
      WORLD_READ
      WORLD_EXECUTE
  PATTERN "*.vim" PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ)

install(
  CODE "execute_process(COMMAND ln -sf ${BGMRPC_CLIENT}/whisp/${CMAKE_INSTALL_BINDIR}/whisp /usr/local/bin/whisp)"
)
