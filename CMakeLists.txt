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

if(APPLE)
    enable_language(OBJCXX)
endif()

option(VEYLO_BUILD_TESTS "Build VeyloPlayer tests" ON)
set(VEYLO_VLC_VERSION "unknown" CACHE STRING "Version of the VLC runtime packaged with VeyloPlayer")
set(VEYLO_SOURCE_REVISION "" CACHE STRING "Public Git revision used for this build")
set(VEYLO_MSVC_RUNTIME_DIR "" CACHE PATH "Directory containing the x64 Visual C++ runtime DLLs")

# Qt's Gui package probes for optional Vulkan headers even on macOS, where
# VeyloPlayer uses Qt's native Metal/OpenGL rendering paths. Keep Qt required
# while silencing status output for optional packages that are not used.
find_package(Qt6 6.8 QUIET REQUIRED COMPONENTS Core Gui Quick QuickControls2 Svg)
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
    src/LegalInfo.cpp
    src/LegalInfo.h
    src/MediaFile.cpp
    src/MediaFile.h
    src/NaturalSort.cpp
    src/NaturalSort.h
    src/PlaybackResume.cpp
    src/PlaybackResume.h
    src/PlayerController.cpp
    src/PlayerController.h
    src/SystemIntegration.cpp
    src/SystemIntegration.h
    src/TrackPreference.cpp
    src/TrackPreference.h
    src/VideoSurfaceWindow.cpp
    src/VideoSurfaceWindow.h
    src/WindowPlacement.cpp
    src/WindowPlacement.h
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

qt_add_resources(VeyloPlayer "legal"
    PREFIX "/legal"
    BASE "."
    FILES
        LICENSE
        THIRD_PARTY_NOTICES.md
)

target_link_libraries(VeyloPlayer PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Svg
    LibVLC::LibVLC
)

if(APPLE)
    target_sources(VeyloPlayer PRIVATE src/SystemIntegration_mac.mm)
    target_link_libraries(VeyloPlayer PRIVATE
        "-framework AppKit"
        "-framework UniformTypeIdentifiers"
    )
endif()

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
    set(macos_app_icon "${CMAKE_CURRENT_SOURCE_DIR}/assets/branding/veylo-player.icns")
    set_source_files_properties("${macos_app_icon}" PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    target_sources(VeyloPlayer PRIVATE "${macos_app_icon}")
    set_target_properties(VeyloPlayer PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Info.plist.in"
        MACOSX_BUNDLE_ICON_FILE "veylo-player.icns"
    )
endif()

include(GNUInstallDirs)
install(TARGETS VeyloPlayer
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(NOT VEYLO_SOURCE_REVISION)
    execute_process(
        COMMAND git rev-parse HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE VEYLO_SOURCE_REVISION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()
if(NOT VEYLO_SOURCE_REVISION)
    set(VEYLO_SOURCE_REVISION "source-archive")
endif()
set(VEYLO_GIT_COMMIT "${VEYLO_SOURCE_REVISION}")
set(VEYLO_SOURCE_STATE "clean")
set(VEYLO_SOURCE_ARCHIVE
    "https://github.com/infiz/VeyloPlayer/archive/${VEYLO_GIT_COMMIT}.tar.gz")
execute_process(
    COMMAND git status --porcelain --untracked-files=normal
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE VEYLO_GIT_STATUS
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(VEYLO_GIT_STATUS)
    set(VEYLO_SOURCE_STATE "dirty development build - not for redistribution")
    set(VEYLO_SOURCE_ARCHIVE
        "UNAVAILABLE: commit the complete source and rebuild before distribution")
endif()
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/SOURCE_OFFER.txt.in"
    "${CMAKE_CURRENT_BINARY_DIR}/SOURCE_OFFER.txt"
    @ONLY
)

install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
    "${CMAKE_CURRENT_SOURCE_DIR}/INSTALLER_LICENSE.txt"
    "${CMAKE_CURRENT_SOURCE_DIR}/SOURCE_CODE.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/THIRD_PARTY_NOTICES.md"
    "${CMAKE_CURRENT_BINARY_DIR}/SOURCE_OFFER.txt"
    DESTINATION licenses
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/LICENSES/"
    DESTINATION licenses
)

# Qt's online installer supplies SPDX SBOMs containing the exact copyright and
# third-party license inventory for each module. Include the modules that can
# contribute runtime files to VeyloPlayer when those SBOMs are available.
get_filename_component(VEYLO_QT_PREFIX "${Qt6_DIR}/../../.." ABSOLUTE)
foreach(qt_module qtbase qtdeclarative qtsvg qtshadertools)
    set(qt_sbom "${VEYLO_QT_PREFIX}/sbom/${qt_module}-${Qt6_VERSION}.spdx")
    if(EXISTS "${qt_sbom}")
        install(FILES "${qt_sbom}" DESTINATION licenses/qt-sbom)
    endif()
endforeach()

if(WIN32)
    enable_language(RC)
    target_sources(VeyloPlayer PRIVATE assets/branding/VeyloPlayer.rc)
    target_link_libraries(VeyloPlayer PRIVATE shell32)

    if(NOT LIBVLC_RUNTIME_DIR)
        message(FATAL_ERROR "LIBVLC_RUNTIME_DIR is required on Windows so LibVLC DLLs and plugins can be packaged")
    endif()
    if(MSVC AND (NOT VEYLO_MSVC_RUNTIME_DIR
            OR NOT EXISTS "${VEYLO_MSVC_RUNTIME_DIR}/vcruntime140.dll"))
        message(FATAL_ERROR "VEYLO_MSVC_RUNTIME_DIR must contain the x64 Visual C++ runtime DLLs")
    endif()

    if(MSVC)
        file(GLOB VEYLO_MSVC_RUNTIME_FILES CONFIGURE_DEPENDS
            "${VEYLO_MSVC_RUNTIME_DIR}/*.dll")
        add_custom_command(TARGET VeyloPlayer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${VEYLO_MSVC_RUNTIME_FILES}
                "$<TARGET_FILE_DIR:VeyloPlayer>"
            COMMENT "Deploying the app-local Visual C++ runtime"
        )
        install(FILES ${VEYLO_MSVC_RUNTIME_FILES}
            DESTINATION ${CMAKE_INSTALL_BINDIR})
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
    foreach(vlc_notice COPYING.txt AUTHORS.txt README.txt)
        if(EXISTS "${LIBVLC_RUNTIME_DIR}/${vlc_notice}")
            install(FILES "${LIBVLC_RUNTIME_DIR}/${vlc_notice}"
                DESTINATION licenses/vlc)
        endif()
    endforeach()
endif()

qt_generate_deploy_qml_app_script(
    TARGET VeyloPlayer
    OUTPUT_SCRIPT qt_deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_TOOL_OPTIONS --no-system-dxc-compiler --no-compiler-runtime
)
install(SCRIPT ${qt_deploy_script})

if(VEYLO_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 QUIET REQUIRED COMPONENTS Test)
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

    # WiX requires RTF. Generate it from the same notices and complete GNU
    # license texts installed beside the application so the installer cannot
    # silently drift from the distributed legal payload.
    set(installer_license_text "")
    foreach(license_file
            INSTALLER_LICENSE.txt
            THIRD_PARTY_NOTICES.md
            LICENSE
            LICENSES/GPL-2.0.txt
            LICENSES/LGPL-2.1.txt
            LICENSES/LGPL-3.0.txt)
        file(READ "${CMAKE_CURRENT_SOURCE_DIR}/${license_file}" license_part)
        string(APPEND installer_license_text
            "\n\n===== ${license_file} =====\n\n${license_part}")
    endforeach()
    file(READ "${CMAKE_CURRENT_BINARY_DIR}/SOURCE_OFFER.txt" source_offer_text)
    string(APPEND installer_license_text
        "\n\n===== SOURCE_OFFER.txt =====\n\n${source_offer_text}")
    string(REPLACE "\\" "\\\\" installer_license_rtf "${installer_license_text}")
    string(REPLACE "{" "\\{" installer_license_rtf "${installer_license_rtf}")
    string(REPLACE "}" "\\}" installer_license_rtf "${installer_license_rtf}")
    string(REPLACE "\r\n" "\n" installer_license_rtf "${installer_license_rtf}")
    string(REPLACE "\n" "\\par\n" installer_license_rtf "${installer_license_rtf}")
    set(CPACK_WIX_LICENSE_RTF "${CMAKE_CURRENT_BINARY_DIR}/VeyloPlayer-License.rtf")
    file(WRITE "${CPACK_WIX_LICENSE_RTF}"
        "{\\rtf1\\ansi\\deff0{\\fonttbl{\\f0 Segoe UI;}}\\f0\\fs18\n${installer_license_rtf}\n}")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
endif()

include(CPack)

qt_finalize_executable(VeyloPlayer)
