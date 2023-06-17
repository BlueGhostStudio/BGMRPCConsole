cmake_minimum_required(VERSION 3.14)

project(BGMRPCConsole LANGUAGES CXX)

add_subdirectory(daemon)
add_subdirectory(client)

#set(CMAKE_AUTOUIC ON)
#set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTORCC ON)

#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)

#find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
#find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

#add_executable(BGMRPCConsole
#  main.cpp
#  dbusrpcclient.h
#  dbusrpcclient.cpp
#)
#target_link_libraries(BGMRPCConsole Qt${QT_VERSION_MAJOR}::Core)

#install(TARGETS BGMRPCConsole
#    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#)
