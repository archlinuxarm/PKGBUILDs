set(HEADER_URL "https://gmic.eu/gmic_stdlib.h")
set(HEADER_DIR ${CMAKE_SOURCE_DIR}/src)
set(HEADER_NAME gmic_stdlib.h)
set(HEADER_PATH ${HEADER_DIR}/${HEADER_NAME})

# gmic_stdlib.h header
if(NOT EXISTS ${HEADER_PATH})
  file(DOWNLOAD ${HEADER_URL} ${HEADER_PATH} STATUS download_status)

  list(GET download_status 0 status_code)
  if(NOT ${status_code} EQUAL 0)
    message(FATAL_ERROR "Missing ${HEADER_NAME} and unable to obtain it. Please download it from ${HEADER_URL} and save it to src/ directory.")
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GMicStdlib
  REQUIRED_VARS HEADER_PATH
)

add_library(GMicStdlib::Stdlib INTERFACE IMPORTED)

set_target_properties(GMicStdlib::Stdlib PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${HEADER_DIR}"
)
