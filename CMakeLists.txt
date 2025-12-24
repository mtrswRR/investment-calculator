# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

project(calculator LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Core Gui Widgets Network REQUIRED)

add_executable(calculator main.cpp calculator.cpp calculator.h)

target_link_libraries(calculator PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Network)