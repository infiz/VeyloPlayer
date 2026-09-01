cmake_minimum_required(VERSION 3.28)

project(VeyloPlayer
    # Release version: update this value manually for each release.
    VERSION 1.0.0
    DESCRIPTION "A modern open-source media player for Windows and macOS"
    LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

option(VEYLO_BUILD_TESTS "Build VeyloPlayer tests" ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Svg)
find_package(LibVLC 3 REQUIRED)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(VeyloPlayer
    WIN32
    MACOSX_BUNDLE
    MANUAL_FINALIZATION
    src/Application.cpp
    src/Application.h
    src/ChapterNavigation.cpp
    src/ChapterNavigation.h
    src/CurrentImageProvider.cpp
    src/CurrentImageProvider.h
    src/FolderSequence.cpp
    src/FolderSequence.h
    src/MediaFile.cpp
    src/MediaFile.h
    src/NaturalSort.cpp
    src/NaturalSort.h
    src/PlaybackResume.cpp
    src/PlaybackResume.h
    src/PlayerController.cpp
    src/PlayerController.h
    src/TrackPreference.cpp
    src/TrackPreference.h
    src/VideoSurfaceWindow.cpp
    src/VideoSurfaceWindow.h
    src/main.cpp
)

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(VeyloPlayer
    URI Veylo.UI
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/AddMediaButton.qml
        qml/IconButton.qml
        qml/PlaybackBar.qml
        qml/TimelineSlider.qml
        qml/Theme.qml
)

qt_add_resources(VeyloPlayer "branding"
    PREFIX "/branding"
    BASE "assets/branding"
    FILES
        assets/branding/veylo-player.png
)

target_link_libraries(VeyloPlayer PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Svg
    LibVLC::LibVLC
)

target_compile_definitions(VeyloPlayer PRIVATE
    VEYLO_APP_VERSION="${PROJECT_VERSION}"
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
)

if(MSVC)
    target_compile_options(VeyloPlayer PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(VeyloPlayer PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(VeyloPlayer PROPERTIES
    OUTPUT_NAME "VeyloPlayer"
    MACOSX_BUNDLE_BUNDLE_NAME "VeyloPlayer"
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.veyloplayer.app"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
)

if(APPLE)
    set_target_properties(VeyloPlayer PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Info.plist.in"
    )
endif()

include(GNUInstallDirs)
install(TARGETS VeyloPlayer
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(WIN32)
    enable_language(RC)
    target_sources(VeyloPlayer PRIVATE assets/branding/VeyloPlayer.rc)
    target_link_libraries(VeyloPlayer PRIVATE shell32)

    if(NOT LIBVLC_RUNTIME_DIR)
        message(FATAL_ERROR "LIBVLC_RUNTIME_DIR is required on Windows so LibVLC DLLs and plugins can be packaged")
    endif()

    add_custom_command(TARGET VeyloPlayer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${LIBVLC_RUNTIME_DIR}/libvlc.dll"
            "${LIBVLC_RUNTIME_DIR}/libvlccore.dll"
            "$<TARGET_FILE_DIR:VeyloPlayer>"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${LIBVLC_RUNTIME_DIR}/plugins"
            "$<TARGET_FILE_DIR:VeyloPlayer>/plugins"
        COMMENT "Deploying the private LibVLC runtime"
    )

    install(FILES
        "${LIBVLC_RUNTIME_DIR}/libvlc.dll"
        "${LIBVLC_RUNTIME_DIR}/libvlccore.dll"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    install(DIRECTORY "${LIBVLC_RUNTIME_DIR}/plugins"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

qt_generate_deploy_qml_app_script(
    TARGET VeyloPlayer
    OUTPUT_SCRIPT qt_deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${qt_deploy_script})

if(VEYLO_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()

set(CPACK_PACKAGE_NAME "VeyloPlayer")
set(CPACK_PACKAGE_VENDOR "VeyloPlayer contributors")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A modern open-source media player")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "VeyloPlayer")
set(CPACK_PACKAGE_FILE_NAME "VeyloPlayer-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_PACKAGE_EXECUTABLES "VeyloPlayer" "VeyloPlayer")

if(WIN32)
    set(CPACK_GENERATOR "WIX;ZIP")
    set(CPACK_WIX_VERSION 4)
    set(CPACK_WIX_TEMPLATE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WIX.template.in")
    set(CPACK_WIX_UPGRADE_GUID "A35D3F09-80E8-4D16-A9F0-0D2AE6016D5B")
    set(CPACK_WIX_PROGRAM_MENU_FOLDER "VeyloPlayer")
    set(CPACK_WIX_PRODUCT_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/branding/veylo-player.ico")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
endif()

include(CPack)

qt_finalize_executable(VeyloPlayer)
