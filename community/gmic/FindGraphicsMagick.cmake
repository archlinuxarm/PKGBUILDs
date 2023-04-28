find_package(PkgConfig)

pkg_check_modules(GRAPHICSMAGICK GraphicsMagick++)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GraphicsMagick
  REQUIRED_VARS GRAPHICSMAGICK_LIBRARIES
)

add_library(GraphicsMagick::GraphicsMagick++ INTERFACE IMPORTED)
set_target_properties(GraphicsMagick::GraphicsMagick++ PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${GRAPHICSMAGICK_INCLUDE_DIRS}"
  INTERFACE_LINK_LIBRARIES "${GRAPHICSMAGICK_LIBRARIES}"
)
