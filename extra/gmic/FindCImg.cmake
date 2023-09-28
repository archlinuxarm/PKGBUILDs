set(HEADER_URL "https://github.com/dtschump/CImg/raw/master/CImg.h")
set(HEADER_DIR ${CMAKE_SOURCE_DIR}/src)
set(HEADER_NAME CImg.h)
set(HEADER_PATH ${HEADER_DIR}/${HEADER_NAME})

# CImg.h header
if(NOT EXISTS ${HEADER_PATH})
  file(DOWNLOAD ${HEADER_URL} ${HEADER_PATH} STATUS download_status)

  list(GET download_status 0 status_code)
  if(NOT ${status_code} EQUAL 0)
    message(FATAL_ERROR "Missing ${HEADER_NAME} and unable to obtain it. Please download it from ${HEADER_URL} and save it to src/ directory.")
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CImg
  REQUIRED_VARS HEADER_PATH
)

# Build options
option(ENABLE_CURL "Add support for curl" ON)
option(ENABLE_X "Add support for X11" ON)
option(ENABLE_XSHM "Add support for X11 Xshm extension" OFF)
option(ENABLE_FFMPEG "Add support for FFMpeg" ON)
option(ENABLE_FFTW "Add support for FFTW" ON)
option(ENABLE_GRAPHICSMAGICK "Add support for GrahicsMagick" ON)
option(ENABLE_JPEG "Add support for handling images in Jpeg format" ON)
option(ENABLE_OPENCV "Add support for OpenCV" OFF)
option(ENABLE_OPENEXR "Add support for handling images in EXR format" ON)
option(ENABLE_OPENMP "Add support for parallel processing" ON)
option(ENABLE_PNG "Add support for handling images in PNG format" ON)
option(ENABLE_TIFF "Add support for handling images in Tiff format" ON)
option(ENABLE_ZLIB "Add support for data compression via Zlib" ON)

set(COMPILE_FLAGS)
set(LINK_FLAGS)
set(CLI_COMPILE_FLAGS)
set(EXTRA_LIBRARY_TARGETS)

## Add dependencies

# OpenMP support
if(ENABLE_OPENMP)
  find_package(OpenMP)
  if(OpenMP_FOUND)
    list(APPEND COMPILE_FLAGS "cimg_use_openmp")
    list(APPEND EXTRA_LIBRARY_TARGETS OpenMP::OpenMP_CXX)
  endif()
endif()

# Zlib support
if(ENABLE_ZLIB)
  find_package(ZLIB)

  if(ZLIB_FOUND)
    list(APPEND COMPILE_FLAGS "cimg_use_zlib")
    list(APPEND EXTRA_LIBRARY_TARGETS ZLIB::ZLIB)
  endif()
endif()

# Curl support
if(ENABLE_CURL)
  find_package(CURL)

  if(CURL_FOUND)
    list(APPEND COMPILE_FLAGS "cimg_use_curl")
    list(APPEND EXTRA_LIBRARY_TARGETS CURL::libcurl)
  endif()
endif()

# X11 support
if(ENABLE_X)
  find_package(X11)

  if(X11_FOUND)
    list(APPEND COMPILE_FLAGS "cimg_display=1" "cimg_appname=\"gmic\"")
    list(APPEND EXTRA_LIBRARY_TARGETS X11::X11)
  else()
    list(APPEND COMPILE_FLAGS "cimg_display=0" "cimg_appname=\"gmic\"")
  endif()

  if(ENABLE_XSHM AND X11_XShm_FOUND)
    list(APPEND COMPILE_FLAGS "cimg_use_xshm")
    list(APPEND EXTRA_LIBRARY_TARGETS X11::Xext)
  endif()
endif()

if(ENABLE_FFTW)
  find_package(FFTW3)

  if(FFTW3_FOUND)
    list(APPEND COMPILE_FLAGS "cimg_use_fftw3")
    list(APPEND EXTRA_LIBRARY_TARGETS ${FFTW3_LIBRARIES} -lfftw3_threads)
  endif()
endif()

if(ENABLE_OPENCV)
  find_package(OpenCV)

  if(OPENCV_FOUND)
    list(APPEND CLI_COMPILE_FLAGS "cimg_use_opencv")
    list(APPEND EXTRA_LIBRARY_TARGETS ${OpenCV_LIBRARIES})
  endif()
endif()

if(ENABLE_GRAPHICSMAGICK)
  find_package(GraphicsMagick)

  if(GraphicsMagick_FOUND)
    list(APPEND CLI_COMPILE_FLAGS "cimg_use_magick")
    list(APPEND EXTRA_LIBRARY_TARGETS GraphicsMagick::GraphicsMagick++)
  endif()
endif()

if(ENABLE_TIFF)
  find_package(TIFF)

  if(TIFF_FOUND)
    list(APPEND CLI_COMPILE_FLAGS "cimg_use_tiff")
    list(APPEND EXTRA_LIBRARY_TARGETS TIFF::TIFF)
  endif()
endif()

if(ENABLE_PNG)
  find_package(PNG)

  if(PNG_FOUND)
    list(APPEND CLI_COMPILE_FLAGS "cimg_use_png")
    list(APPEND EXTRA_LIBRARY_TARGETS PNG::PNG)
  endif()
endif()

if(ENABLE_JPEG)
  find_package(JPEG)

  if(JPEG_FOUND)
    list(APPEND CLI_COMPILE_FLAGS "cimg_use_jpeg")
    list(APPEND EXTRA_LIBRARY_TARGETS JPEG::JPEG)
  endif()
endif()

if(ENABLE_OPENEXR)
  find_package(OpenEXR)

  if(OpenEXR_FOUND)
    list(APPEND CLI_COMPILE_FLAGS "cimg_use_openexr")
    list(APPEND EXTRA_LIBRARY_TARGETS OpenEXR::OpenEXR)
  endif()
endif()

if(MINGW)
  list(APPEND LINK_FLAGS "-Wl,--stack,16777216")
endif()

find_package(Threads)
if(Threads_FOUND)
  list(APPEND EXTRA_LIBRARY_TARGETS Threads::Threads)
endif()


# Library definition

add_library(CImg::CImg INTERFACE IMPORTED)

target_compile_definitions(CImg::CImg INTERFACE ${COMPILE_FLAGS} ${CLI_COMPILE_FLAGS})
target_link_options(CImg::CImg INTERFACE ${LINK_FLAGS})
target_link_libraries(CImg::CImg INTERFACE ${EXTRA_LIBRARY_TARGETS})
target_include_directories(CImg::CImg INTERFACE ${HEADER_DIR})
