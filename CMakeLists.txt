cmake_minimum_required(VERSION 3.20)

project(AnnotaVision LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_executable(AnnotaVision
    src/main.cpp

    src/model/ImageModel.h
    src/model/ProjectModel.h

    src/view/ImageCanvas.h
    src/view/ImageCanvas.cpp
    src/view/MainWindow.h
    src/view/MainWindow.cpp

    src/viewmodel/ImageViewModel.h
    src/viewmodel/ImageViewModel.cpp
)

target_include_directories(AnnotaVision PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(AnnotaVision PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
)
