# The set of languages for which implicit dependencies are needed:
SET(CMAKE_DEPENDS_LANGUAGES
  )
# The set of files for implicit dependencies of each language:

# Preprocessor definitions for this target.
SET(CMAKE_TARGET_DEFINITIONS
  "HAVE_LIBCURL"
  "QT_CORE_LIB"
  "QT_GUI_LIB"
  "QT_NETWORK_LIB"
  "QT_NO_DEBUG"
  "QT_WEBKIT_LIB"
  "SC_DATA_DIR=\"/usr/share/SuperCollider\""
  )

# Targets to which this target links.
SET(CMAKE_TARGET_LINKED_INFO_FILES
  )

# The include file search paths:
SET(CMAKE_C_TARGET_INCLUDE_PATH
  "common"
  "../include/common"
  "../include/plugin_interface"
  "../external_libraries/yaml-cpp-0.3.0/include"
  "../external_libraries/boost"
  ".."
  "../editors/sc-ide/widgets/util"
  "../editors/sc-ide/widgets"
  "editors/sc-ide"
  "/usr/include/qt4"
  "/usr/include/qt4/QtWebKit"
  "/usr/include/qt4/QtGui"
  "/usr/include/qt4/QtNetwork"
  "/usr/include/qt4/QtCore"
  )
SET(CMAKE_CXX_TARGET_INCLUDE_PATH ${CMAKE_C_TARGET_INCLUDE_PATH})
SET(CMAKE_Fortran_TARGET_INCLUDE_PATH ${CMAKE_C_TARGET_INCLUDE_PATH})
SET(CMAKE_ASM_TARGET_INCLUDE_PATH ${CMAKE_C_TARGET_INCLUDE_PATH})
