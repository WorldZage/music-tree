cmake_minimum_required(VERSION 3.16)

project(music_tree VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS QuickWidgets)
find_package(Qt6 REQUIRED COMPONENTS Sql)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appmusic_tree
    main.cpp
)

qt_add_qml_module(appmusic_tree
    URI music_tree
    VERSION 1.0
    QML_FILES
        Main.qml
    SOURCES
        discogsmanager.h
        discogsmanager.cpp
        graphviewitem.h graphviewitem.cpp
        artistservice.h artistservice.cpp
        artist.h artist.cpp
        databasemanager.h databasemanager.cpp
        sessionmanager.h sessionmanager.cpp
        sessionartistmodel.h sessionartistmodel.cpp
)

qt_add_resources(APP_RESOURCES resources.qrc)
target_sources(appmusic_tree PRIVATE ${APP_RESOURCES})


# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appmusic_tree PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appmusic_tree
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appmusic_tree
    PRIVATE Qt6::Widgets Qt6::Qml Qt6::Gui Qt6::Quick Qt6::QuickWidgets Qt6::Sql
  )


include(GNUInstallDirs)
install(TARGETS appmusic_tree
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
