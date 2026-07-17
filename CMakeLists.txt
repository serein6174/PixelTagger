cmake_minimum_required(VERSION 3.20)

project(AnnotaVision LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(OpenCV REQUIRED COMPONENTS core imgproc)

add_executable(AnnotaVision
    src/main.cpp

    src/app/Application.h
    src/app/Application.cpp

    src/common/types/EntityIds.h
    src/common/types/Result.h
    src/common/types/ViewModelChange.h
    src/common/presentation/AnnotationRenderData.h
    src/common/presentation/LabelViewData.h

    src/model/AnnotationModel.h
    src/model/ImageModel.h
    src/model/LabelModel.h
    src/model/ProjectModel.h
    src/model/ProjectModel.cpp

    src/repository/JsonProjectRepository.h
    src/repository/JsonProjectRepository.cpp

    src/exporter/YoloExporter.h
    src/exporter/YoloExporter.cpp

    src/processor/ImageConverter.h
    src/processor/ImageConverter.cpp
    src/processor/ImageProcessor.h
    src/processor/ImageProcessor.cpp

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
    src/viewmodel/ProjectViewModel.h
    src/viewmodel/ProjectViewModel.cpp
    src/viewmodel/ProcessViewModel.h
    src/viewmodel/ProcessViewModel.cpp
)

target_include_directories(AnnotaVision PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(AnnotaVision PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    ${OpenCV_LIBS}
)

if(MSVC)
    target_compile_options(AnnotaVision PRIVATE /utf-8)
endif()

if(WIN32)
    add_custom_command(
        TARGET AnnotaVision
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:AnnotaVision>/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::QWindowsIntegrationPlugin>"
                "$<TARGET_FILE_DIR:AnnotaVision>/platforms/"
        COMMENT "Deploying the Qt Windows platform plugin"
        VERBATIM
    )
endif()

include(CTest)

if(BUILD_TESTING)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)

    function(add_annotavision_test target source)
        add_executable(${target}
            ${source}
            ${ARGN}
            tests/TestFixtures.h

            src/common/types/EntityIds.h
            src/common/types/Result.h
            src/common/types/ViewModelChange.h
            src/common/presentation/AnnotationRenderData.h
            src/common/presentation/LabelViewData.h

            src/model/AnnotationModel.h
            src/model/ImageModel.h
            src/model/LabelModel.h
            src/model/ProjectModel.h
            src/model/ProjectModel.cpp

            src/viewmodel/AnnotationViewModel.h
            src/viewmodel/AnnotationViewModel.cpp
            src/viewmodel/LabelViewModel.h
            src/viewmodel/LabelViewModel.cpp
        )

        target_include_directories(${target} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/tests
        )

        target_link_libraries(${target} PRIVATE
            Qt${QT_VERSION_MAJOR}::Widgets
            Qt${QT_VERSION_MAJOR}::Test
        )

        if(MSVC)
            target_compile_options(${target} PRIVATE /utf-8)
        endif()

        add_test(
            NAME ${target}
            COMMAND ${CMAKE_COMMAND} -E env
                    "PATH=$<TARGET_FILE_DIR:Qt${QT_VERSION_MAJOR}::Core>;$ENV{PATH}"
                    $<TARGET_FILE:${target}>
        )
    endfunction()

    add_annotavision_test(
        ProjectModelLabelTests
        tests/model/ProjectModelLabelTests.cpp
    )
    add_annotavision_test(
        MultiLabelViewModelTests
        tests/viewmodel/MultiLabelViewModelTests.cpp
    )
    add_annotavision_test(
        AnnotationEditingTests
        tests/annotation/AnnotationEditingTests.cpp
    )
    add_annotavision_test(
        MainWindowAnnotationTests
        tests/view/MainWindowAnnotationTests.cpp
        src/view/canvas/CoordinateMapper.h
        src/view/canvas/CoordinateMapper.cpp
        src/view/canvas/ImageCanvas.h
        src/view/canvas/ImageCanvas.cpp
        src/view/MainWindow.h
        src/view/MainWindow.cpp
    )
    add_annotavision_test(
        ImageCanvasInteractionTests
        tests/view/ImageCanvasInteractionTests.cpp
        src/view/canvas/CoordinateMapper.h
        src/view/canvas/CoordinateMapper.cpp
        src/view/canvas/ImageCanvas.h
        src/view/canvas/ImageCanvas.cpp
    )

    function(add_image_processing_test target source)
        add_executable(${target}
            ${source}
            ${ARGN}

            src/common/types/Result.h
            src/processor/ImageConverter.h
            src/processor/ImageConverter.cpp
            src/processor/ImageProcessor.h
            src/processor/ImageProcessor.cpp
        )

        target_include_directories(${target} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${OpenCV_INCLUDE_DIRS}
        )

        target_link_libraries(${target} PRIVATE
            Qt${QT_VERSION_MAJOR}::Widgets
            Qt${QT_VERSION_MAJOR}::Test
            ${OpenCV_LIBS}
        )

        if(MSVC)
            target_compile_options(${target} PRIVATE /utf-8)
        endif()

        add_test(
            NAME ${target}
            COMMAND ${CMAKE_COMMAND} -E env
                    "PATH=$<TARGET_FILE_DIR:Qt${QT_VERSION_MAJOR}::Core>;$<TARGET_FILE_DIR:opencv_core>;$ENV{PATH}"
                    $<TARGET_FILE:${target}>
        )
    endfunction()

    add_image_processing_test(
        ImageProcessorTests
        tests/processor/ImageProcessorTests.cpp
    )
    add_image_processing_test(
        ProcessViewModelTests
        tests/viewmodel/ProcessViewModelTests.cpp
        src/viewmodel/ProcessViewModel.h
        src/viewmodel/ProcessViewModel.cpp
    )

    function(add_yolo_export_test target source)
        add_executable(${target}
            ${source}
            ${ARGN}

            src/common/types/EntityIds.h
            src/common/types/Result.h
            src/common/types/ViewModelChange.h

            src/model/AnnotationModel.h
            src/model/ImageModel.h
            src/model/LabelModel.h
            src/model/ProjectModel.h
            src/model/ProjectModel.cpp

            src/exporter/YoloExporter.h
            src/exporter/YoloExporter.cpp
        )

        target_include_directories(${target} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
        )

        target_link_libraries(${target} PRIVATE
            Qt${QT_VERSION_MAJOR}::Widgets
            Qt${QT_VERSION_MAJOR}::Test
        )

        if(MSVC)
            target_compile_options(${target} PRIVATE /utf-8)
        endif()

        add_test(
            NAME ${target}
            COMMAND ${CMAKE_COMMAND} -E env
                    "PATH=$<TARGET_FILE_DIR:Qt${QT_VERSION_MAJOR}::Core>;$ENV{PATH}"
                    $<TARGET_FILE:${target}>
        )
    endfunction()

    add_yolo_export_test(
        YoloExporterTests
        tests/exporter/YoloExporterTests.cpp
    )
    add_yolo_export_test(
        ProjectExportViewModelTests
        tests/viewmodel/ProjectExportViewModelTests.cpp
        src/repository/JsonProjectRepository.h
        src/repository/JsonProjectRepository.cpp
        src/viewmodel/ProjectViewModel.h
        src/viewmodel/ProjectViewModel.cpp
    )
endif()
