qt_add_executable(VeyloCoreTests
    tst_core.cpp
    ../src/ChapterNavigation.cpp
    ../src/FolderSequence.cpp
    ../src/MediaFile.cpp
    ../src/NaturalSort.cpp
    ../src/PlaybackResume.cpp
    ../src/TrackPreference.cpp
    ../src/WindowPlacement.cpp
)

target_include_directories(VeyloCoreTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src")
target_link_libraries(VeyloCoreTests PRIVATE Qt6::Core Qt6::Test)

if(MSVC)
    target_compile_options(VeyloCoreTests PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(VeyloCoreTests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME VeyloCoreTests COMMAND VeyloCoreTests)

if(WIN32)
    qt_add_executable(VeyloPlayerRecoveryTests
        tst_player_recovery.cpp
        ../src/PlayerController.cpp
        ../src/VideoSurfaceWindow.cpp
        ../src/GraphicsDeviceMonitor.cpp
        ../src/ChapterNavigation.cpp
        ../src/FolderSequence.cpp
        ../src/MediaFile.cpp
        ../src/NaturalSort.cpp
        ../src/PlaybackResume.cpp
        ../src/TrackPreference.cpp
    )
    target_include_directories(VeyloPlayerRecoveryTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src")
    target_link_libraries(VeyloPlayerRecoveryTests PRIVATE
        Qt6::Core Qt6::Gui Qt6::Test LibVLC::LibVLC d3d11 dxgi)
    add_test(NAME VeyloPlayerRecoveryTests COMMAND VeyloPlayerRecoveryTests)
    set_tests_properties(VeyloPlayerRecoveryTests PROPERTIES TIMEOUT 90)
endif()
