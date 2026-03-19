# cmake/FindSignalHoundBB.cmake
# Locates Signal Hound BB60 SDK headers and library.
#
# Inputs (cache variables):
#   SIGNALHOUND_SDK_ROOT        - Root of the extracted SDK zip
#   SIGNALHOUND_BB_INCLUDE_DIR  - Override include path
#   SIGNALHOUND_BB_LIBRARY      - Override library path
#
# Outputs:
#   SIGNALHOUND_BB_FOUND
#   SIGNALHOUND_BB_INCLUDE_DIR
#   SIGNALHOUND_BB_LIBRARY
#
# Auto-discovery order:
#   1. SIGNALHOUND_BB_INCLUDE_DIR / SIGNALHOUND_BB_LIBRARY if set
#   2. SIGNALHOUND_SDK_ROOT if set
#   3. ENV SIGNALHOUND_SDK_ROOT if set
#   4. ~/software/signalhound/sdk/signal_hound_sdk (developer convenience)
#   5. Standard system paths

set(_sh_sdk_hints "")

if(DEFINED SIGNALHOUND_SDK_ROOT)
    list(APPEND _sh_sdk_hints "${SIGNALHOUND_SDK_ROOT}")
endif()

if(DEFINED ENV{SIGNALHOUND_SDK_ROOT})
    list(APPEND _sh_sdk_hints "$ENV{SIGNALHOUND_SDK_ROOT}")
endif()

# Developer convenience path — CI must NOT rely on this
if(UNIX)
    list(APPEND _sh_sdk_hints "$ENV{HOME}/software/signalhound/sdk/signal_hound_sdk")
endif()

find_path(SIGNALHOUND_BB_INCLUDE_DIR
    NAMES bb_api.h
    HINTS ${_sh_sdk_hints}
    PATH_SUFFIXES device_apis/bb_series/include
)

if(UNIX)
    # Prefer Ubuntu 18.04 library; allow override via SIGNALHOUND_SDK_ROOT.
    # The SDK ships versioned shared objects (e.g. libbb_api.so.5.0.9) without
    # creating an unversioned symlink, so we list explicit versioned names as
    # fallbacks. The unversioned stem "bb_api" is listed first so that a user
    # who manually creates libbb_api.so -> libbb_api.so.5.0.9 gets the usual
    # behaviour. NOTE: if the SDK updates its version number, update the
    # versioned names below or add the new version to NAMES.
    # TODO Task 11: expose SIGNALHOUND_BB_SO_VERSION as a cache variable.
    find_library(SIGNALHOUND_BB_LIBRARY
        NAMES bb_api libbb_api.so.5.0.9 libbb_api.so.5
        HINTS ${_sh_sdk_hints}
        PATH_SUFFIXES
            "device_apis/bb_series/lib/linux_x64/Ubuntu 18.04"
            "device_apis/bb_series/lib/linux_x64/Ubuntu 14.04"
            "device_apis/bb_series/lib/linux_x64/CentOS 7"
            "device_apis/bb_series/lib/linux_x64/Red Hat 8"
            "device_apis/bb_series/lib/linux_x64/Red Hat 7"
            device_apis/bb_series/lib/aarch64
    )
else()
    find_library(SIGNALHOUND_BB_LIBRARY
        NAMES bb_api
        HINTS ${_sh_sdk_hints}
        PATH_SUFFIXES device_apis/bb_series/lib/win_x64
    )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SignalHoundBB
    REQUIRED_VARS SIGNALHOUND_BB_INCLUDE_DIR SIGNALHOUND_BB_LIBRARY
)

mark_as_advanced(SIGNALHOUND_BB_INCLUDE_DIR SIGNALHOUND_BB_LIBRARY)
