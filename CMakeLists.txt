cmake_minimum_required(VERSION 3.16)

project(LABS VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools Multimedia Core5Compat)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools Multimedia Core5Compat Network)

set(TS_FILES LABS_zh_CN.ts)

set(PROJECT_SOURCES
        src/main.cpp
        src/widget.cpp
        src/widget.h
        src/widget.ui
        ${TS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(LABS
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        resources/logo.rc
        resources/pic.qrc
        resources/res.qrc
        resources/qss.qrc

        src/processobj.h
        src/processobj.cpp
        src/networkobj.h
        src/networkobj.cpp
        src/thememanager.h
        src/thememanager.cpp
        src/biliinfo.h
        src/songmanager.h
        src/songmanager.cpp

        src/lyricswidget.h
        src/lyricswidget.cpp
        src/lyricswidget.ui
        src/notificationwidget.h
        src/notificationwidget.cpp
        src/notificationwidget.ui
        src/visualiserwidget.h
        src/visualiserwidget.cpp
        src/visualiserwidget.ui
        src/titlewidget.h
        src/titlewidget.cpp
        src/titlewidget.ui
        src/betterslider.h
        src/betterslider.cpp
        src/betterslider.ui
        src/bettertableview.h
        src/bettertableview.cpp
        src/bettertableview.ui
        src/biliinfowidget.h
        src/biliinfowidget.cpp
        src/biliinfowidget.ui

    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET LABS APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

target_include_directories(LABS
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(LABS SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(LABS
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

if(MSVC)
    target_compile_options(LABS PRIVATE /W4 /WX)
else()
    target_compile_options(LABS PRIVATE 
        -Wall -Wextra -Wpedantic -Werror
        -Wconversion -Wsign-conversion -Wunused -Wshadow
        -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Woverloaded-virtual
    )
endif()


target_link_directories(LABS PRIVATE bin)
target_link_libraries(LABS PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Multimedia Qt6::Core5Compat Qt6::Network 
tag libfftw3-3 avutil-60 avformat-62 avfilter-11 avdevice-62 avcodec-62 swresample-6 swscale-9
    dwmapi
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.LABS)
endif()
set_target_properties(LABS PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    # set WIN32_EXECUTABLE FALSE to enable qDebug output if you are using vscode 
    WIN32_EXECUTABLE $<CONFIG:Release>
)

include(GNUInstallDirs)
include_directories(SYSTEM PRIVATE include/taglib/ include/fftw3/ include/ffmpeg/ src/)
install(TARGETS LABS
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(LABS)
endif()
