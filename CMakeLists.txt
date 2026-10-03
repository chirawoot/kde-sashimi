cmake_minimum_required(VERSION 3.16)
project(kde-sashimi VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Multimedia MultimediaWidgets Pdf DBus)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(kde-sashimi main.cpp)

target_link_libraries(kde-sashimi PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    Qt6::Pdf
    Qt6::DBus
)

install(TARGETS kde-sashimi RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
