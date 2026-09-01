# Locate a LibVLC SDK and expose LibVLC::LibVLC.
#
# Hints:
#   -DLIBVLC_ROOT=/path/to/vlc-sdk-or-runtime-root
#
# Results:
#   LibVLC_FOUND
#   LIBVLC_INCLUDE_DIR
#   LIBVLC_LIBRARY
#   LIBVLC_RUNTIME_DIR

set(_LIBVLC_HINTS)
if(LIBVLC_ROOT)
    list(APPEND _LIBVLC_HINTS "${LIBVLC_ROOT}")
endif()
if(DEFINED ENV{LIBVLC_ROOT})
    list(APPEND _LIBVLC_HINTS "$ENV{LIBVLC_ROOT}")
endif()

find_path(LIBVLC_INCLUDE_DIR
    NAMES vlc/vlc.h
    HINTS ${_LIBVLC_HINTS}
    PATH_SUFFIXES sdk/include include
)

find_library(LIBVLC_LIBRARY
    NAMES libvlc vlc
    HINTS ${_LIBVLC_HINTS}
    PATH_SUFFIXES sdk/lib lib
)

if(WIN32)
    find_path(LIBVLC_RUNTIME_DIR
        NAMES libvlc.dll libvlccore.dll
        HINTS ${_LIBVLC_HINTS}
    )
elseif(APPLE)
    find_path(LIBVLC_RUNTIME_DIR
        NAMES libvlc.dylib
        HINTS ${_LIBVLC_HINTS}
        PATH_SUFFIXES lib
    )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibVLC
    REQUIRED_VARS LIBVLC_INCLUDE_DIR LIBVLC_LIBRARY
    VERSION_VAR LibVLC_VERSION
)

if(LibVLC_FOUND AND NOT TARGET LibVLC::LibVLC)
    add_library(LibVLC::LibVLC UNKNOWN IMPORTED)
    set_target_properties(LibVLC::LibVLC PROPERTIES
        IMPORTED_LOCATION "${LIBVLC_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBVLC_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(LIBVLC_INCLUDE_DIR LIBVLC_LIBRARY LIBVLC_RUNTIME_DIR)
