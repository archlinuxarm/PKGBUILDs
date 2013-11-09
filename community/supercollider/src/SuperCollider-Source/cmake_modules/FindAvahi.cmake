find_library(AVAHI_LIBRARY-COMMON NAMES avahi-common)
find_library(AVAHI_LIBRARY-CLIENT NAMES avahi-client)
find_path(AVAHI_INCLUDE_DIR avahi-client/publish.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Avahi DEFAULT_MSG AVAHI_LIBRARY-COMMON AVAHI_LIBRARY-CLIENT AVAHI_INCLUDE_DIR)

if(AVAHI_FOUND)
    set(AVAHI_LIBRARIES ${AVAHI_LIBRARY-COMMON} ${AVAHI_LIBRARY-CLIENT})
    set(AVAHI_INCLUDE_DIRS ${AVAHI_INCLUDE_DIR})
endif()
