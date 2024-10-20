cmake_minimum_required(VERSION 3.16)

project(ZcAIDanmuGenerator VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED Widgets Multimedia MultimediaWidgets NetWork)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})


target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Multimedia Qt6::MultimediaWidgets Qt6::Network)


