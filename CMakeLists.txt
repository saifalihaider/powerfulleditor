cmake_minimum_required(VERSION 3.16)
project(MediaFileManager VERSION 1.0.0 LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# CUDA configuration
option(USE_CUDA "Enable CUDA support" ON)
if(USE_CUDA)
    enable_language(CUDA)
    find_package(CUDA REQUIRED)
    add_definitions(-DWITH_CUDA)
    set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};-O3;-gencode arch=compute_52,code=sm_52)
endif()

# Find required packages
find_package(Qt6 COMPONENTS Core Gui Widgets Multimedia Network Test REQUIRED)
find_package(FFmpeg REQUIRED COMPONENTS AVCODEC AVFORMAT AVUTIL SWSCALE AVFILTER)
find_package(GTest REQUIRED)

# Configure version information
configure_file(
    "${CMAKE_SOURCE_DIR}/src/version.h.in"
    "${CMAKE_BINARY_DIR}/version.h"
)

# CUDA source files
if(USE_CUDA)
    set(CUDA_SOURCES
        src/cuda/frame_processor.cu
        src/cuda/effects_processor.cu
        src/cuda/scaler.cu
    )
endif()

# Main application
add_executable(MediaFileManager WIN32
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/mediaprocessor.cpp
    src/mediaprocessor.h
    src/timelineview.cpp
    src/timelineview.h
    src/timelinetrack.cpp
    src/timelinetrack.h
    src/timelineclip.cpp
    src/timelineclip.h
    src/timelineruler.cpp
    src/timelineruler.h
    src/videoeffect.cpp
    src/videoeffect.h
    src/effectsmanager.cpp
    src/effectsmanager.h
    src/effectsdialog.cpp
    src/effectsdialog.h
    src/audioeffect.cpp
    src/audioeffect.h
    src/audiotrack.cpp
    src/audiotrack.h
    src/audiomanager.cpp
    src/audiomanager.h
    src/texteffect.cpp
    src/texteffect.h
    src/textmanager.cpp
    src/textmanager.h
    src/keyframe.cpp
    src/keyframe.h
    src/animation.cpp
    src/animation.h
    src/exportsettings.cpp
    src/exportsettings.h
    src/videoexporter.cpp
    src/videoexporter.h
    src/proxymanager.cpp
    src/proxymanager.h
    src/framecache.cpp
    src/framecache.h
    src/medialibrary.cpp
    src/medialibrary.h
    src/propertyinspector.cpp
    src/propertyinspector.h
    src/propertywidget.cpp
    src/propertywidget.h
    src/updatechecker.cpp
    src/updatechecker.h
    src/analytics.cpp
    src/analytics.h
    src/bugreporter.cpp
    src/bugreporter.h
    src/gpumanager.cpp
    src/gpumanager.h
    src/highresprocessor.cpp
    src/highresprocessor.h
    ${CUDA_SOURCES}
    resources/resources.qrc
)

target_include_directories(MediaFileManager PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}
    ${FFMPEG_INCLUDE_DIRS}
    ${CUDA_INCLUDE_DIRS}
)

target_link_libraries(MediaFileManager PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Network
    ${FFMPEG_LIBRARIES}
    ${CUDA_LIBRARIES}
    ${CUDA_CUDA_LIBRARY}
    ${CUDA_CUDART_LIBRARY}
)

if(USE_CUDA)
    set_target_properties(MediaFileManager PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON
        CUDA_ARCHITECTURES "52;61;72;75;86"
    )
endif()

# Tests
enable_testing()

add_executable(MediaFileManagerTests
    tests/main_test.cpp
    tests/video_test.cpp
    tests/ui_test.cpp
)

target_link_libraries(MediaFileManagerTests PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Network
    Qt6::Test
    ${FFMPEG_LIBRARIES}
    ${CUDA_LIBRARIES}
    GTest::GTest
    GTest::Main
)

target_include_directories(MediaFileManagerTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}
    ${FFMPEG_INCLUDE_DIRS}
    ${CUDA_INCLUDE_DIRS}
)

# Register tests
include(GoogleTest)
gtest_discover_tests(MediaFileManagerTests)

# Installation
install(TARGETS MediaFileManager
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install Qt dependencies
include(InstallRequiredSystemLibraries)
get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)

if(WIN32)
    # Install Qt plugins
    install(DIRECTORY "${_qt_bin_dir}/../plugins"
        DESTINATION bin
        COMPONENT Runtime
        FILES_MATCHING
        PATTERN "*.dll"
        PATTERN "*d.dll" EXCLUDE
    )
    
    # Install FFmpeg
    install(DIRECTORY "${FFMPEG_ROOT}/bin/"
        DESTINATION bin
        COMPONENT Runtime
        FILES_MATCHING
        PATTERN "*.dll"
        PATTERN "*.exe"
    )

    # Install CUDA Runtime
    if(USE_CUDA)
        install(FILES "${CUDA_TOOLKIT_ROOT_DIR}/bin/cudart64_*.dll"
            DESTINATION bin
            COMPONENT Runtime
        )
    endif()
endif()

# Packaging
set(CPACK_PACKAGE_NAME "Professional Video Editor")
set(CPACK_PACKAGE_VENDOR "Your Company")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Professional video editing software with high-resolution support")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/resources/icon.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/resources/icon.ico")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\MediaFileManager.exe")
    set(CPACK_NSIS_DISPLAY_NAME ${CPACK_PACKAGE_NAME})
    set(CPACK_NSIS_PACKAGE_NAME ${CPACK_PACKAGE_NAME})
    set(CPACK_NSIS_HELP_LINK "https://example.com/support")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://example.com")
    set(CPACK_NSIS_MODIFY_PATH ON)
    
    # Create start menu shortcuts
    set(CPACK_NSIS_MENU_LINKS
        "bin/MediaFileManager.exe" "Professional Video Editor"
        "https://example.com/support" "Support"
        "https://example.com/docs" "Documentation"
    )
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME ${CPACK_PACKAGE_NAME})
    set(CPACK_DMG_FORMAT "UDZO")
else()
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Your Name <your.email@example.com>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "video")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "ffmpeg, qt6-base-dev, nvidia-cuda-toolkit")
endif()

include(CPack)
