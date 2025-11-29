cmake_minimum_required(VERSION 3.16)

project(Mystery_Gift_Injector VERSION 1.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        src/main.cpp
        # Core
        src/core/savefile.cpp
        src/core/savefile.h
        src/core/mysterygift.cpp
        src/core/mysterygift.h
        # ROM
        src/rom/gbaromreader.cpp
        src/rom/gbaromreader.h
        src/rom/romdatabase.cpp
        src/rom/romdatabase.h
        src/rom/romloader.cpp
        src/rom/romloader.h
        # Tickets
        src/tickets/ticketresource.cpp
        src/tickets/ticketresource.h
        src/tickets/ticketmanager.cpp
        src/tickets/ticketmanager.h
        # Rendering
        src/rendering/wondercardrenderer.cpp
        src/rendering/wondercardrenderer.h
        src/rendering/gen3fontrenderer.cpp
        src/rendering/gen3fontrenderer.h
        src/rendering/fallbackgraphics.cpp
        src/rendering/fallbackgraphics.h
        src/rendering/tileviewer.cpp
        src/rendering/tileviewer.h
        # UI
        src/ui/mainwindow.cpp
        src/ui/mainwindow.h
        src/ui/mainwindow.ui
        src/ui/editablewondercardwidget.cpp
        src/ui/editablewondercardwidget.h
        src/ui/authenticwondercardwidget.cpp
        src/ui/authenticwondercardwidget.h
        # Script
        src/script/scriptdisassembler.cpp
        src/script/scriptdisassembler.h
        # Resources
        resources.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(Mystery_Gift_Injector
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET Mystery_Gift_Injector APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(Mystery_Gift_Injector SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(Mystery_Gift_Injector
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(Mystery_Gift_Injector PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# Include directories for source organization
target_include_directories(Mystery_Gift_Injector PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/rom
    ${CMAKE_SOURCE_DIR}/src/tickets
    ${CMAKE_SOURCE_DIR}/src/rendering
    ${CMAKE_SOURCE_DIR}/src/ui
    ${CMAKE_SOURCE_DIR}/src/script
)

# Disable debug output in release builds
target_compile_definitions(Mystery_Gift_Injector PRIVATE
    $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>
)

# Copy Tickets folder to build output directory after build
add_custom_command(TARGET Mystery_Gift_Injector POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Tickets"
        "$<TARGET_FILE_DIR:Mystery_Gift_Injector>/Tickets"
    COMMENT "Copying Tickets folder to build directory"
)

# Copy documentation to build output directory
add_custom_command(TARGET Mystery_Gift_Injector POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/documentation.html"
        "$<TARGET_FILE_DIR:Mystery_Gift_Injector>/documentation.html"
    COMMENT "Copying documentation to build directory"
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.Mystery_Gift_Injector)
endif()
set_target_properties(Mystery_Gift_Injector PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS Mystery_Gift_Injector
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Mystery_Gift_Injector)
endif()
