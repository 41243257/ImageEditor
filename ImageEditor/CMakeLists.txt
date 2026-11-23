cmake_minimum_required(VERSION 3.16)

project(ImageEditor VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# 同時支援 Qt 6 和 Qt 5
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# MSVC 預設不把原始碼當成 UTF-8，中文字會變亂碼
if(MSVC)
    add_compile_options(/utf-8)
endif()

add_executable(ImageEditor
    main.cpp
    mainwindow.cpp
    mainwindow.h
)

target_link_libraries(ImageEditor PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(ImageEditor PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)
