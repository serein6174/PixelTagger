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

    src/app/Application.h
    src/app/Application.cpp

    src/common/types/EntityIds.h
    src/common/types/Result.h
    src/common/presentation/AnnotationViewData.h

    src/model/AnnotationModel.h
    src/model/ImageModel.h
    src/model/LabelModel.h
    src/model/ProjectModel.h
    src/model/ProjectModel.cpp

    src/service/ImageImportService.h
    src/service/ImageImportService.cpp

    src/view/canvas/CoordinateMapper.h
    src/view/canvas/CoordinateMapper.cpp
    src/view/canvas/ImageCanvas.h
    src/view/canvas/ImageCanvas.cpp
    src/view/MainWindow.h
    src/view/MainWindow.cpp

    src/viewmodel/AnnotationViewModel.h
    src/viewmodel/AnnotationViewModel.cpp
    src/viewmodel/ImageViewModel.h
    src/viewmodel/ImageViewModel.cpp
    src/viewmodel/LabelViewModel.h
    src/viewmodel/LabelViewModel.cpp
)

target_include_directories(AnnotaVision PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(AnnotaVision PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
)

if(MSVC)
    target_compile_options(AnnotaVision PRIVATE /utf-8)
endif()
