cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

# add project files
set(HEADER_FILES
    misc/xmlparsermacros.h
    misc/undefxmlparsermacros.h
    misc/trylocker.h
    misc/adoptlocker.h
    misc/dialogutils.cpp
    misc/desktoputils.cpp
    models/checklistmodel.h
    resources/qtconfigarguments.h
    resources/resources.h
)
set(SRC_FILES
    misc/dialogutils.h
    misc/desktoputils.h
    models/checklistmodel.cpp
    resources/qtconfigarguments.cpp
    resources/resources.cpp
    resources/qtutilsicons.qrc
)
set(WIDGETS_HEADER_FILES
    aboutdialog/aboutdialog.h
    enterpassworddialog/enterpassworddialog.h
    settingsdialog/optioncategory.h
    settingsdialog/optioncategoryfiltermodel.h
    settingsdialog/optioncategorymodel.h
    settingsdialog/optionpage.h
    settingsdialog/settingsdialog.h
    settingsdialog/qtsettings.h
    widgets/buttonoverlay.h
    widgets/clearcombobox.h
    widgets/clearlineedit.h
    widgets/clearplaintextedit.h
    widgets/clearspinbox.h
    widgets/iconbutton.h
    widgets/pathselection.h
    paletteeditor/paletteeditor.h
    paletteeditor/colorbutton.h
)
set(WIDGETS_SRC_FILES
    aboutdialog/aboutdialog.cpp
    enterpassworddialog/enterpassworddialog.cpp
    settingsdialog/optioncategory.cpp
    settingsdialog/optioncategoryfiltermodel.cpp
    settingsdialog/optioncategorymodel.cpp
    settingsdialog/optionpage.cpp
    settingsdialog/settingsdialog.cpp
    settingsdialog/qtsettings.cpp
    widgets/buttonoverlay.cpp
    widgets/clearcombobox.cpp
    widgets/clearlineedit.cpp
    widgets/clearplaintextedit.cpp
    widgets/clearspinbox.cpp
    widgets/iconbutton.cpp
    widgets/pathselection.cpp
    paletteeditor/paletteeditor.cpp
    paletteeditor/colorbutton.cpp
)
set(WIDGETS_UI_FILES
    aboutdialog/aboutdialog.ui
    enterpassworddialog/enterpassworddialog.ui
    settingsdialog/settingsdialog.ui
    settingsdialog/qtappearanceoptionpage.ui
    settingsdialog/qtlanguageoptionpage.ui
    settingsdialog/qtenvoptionpage.ui
    paletteeditor/paletteeditor.ui
)

# meta data
set(META_PROJECT_NAME qtutilities)
set(META_APP_NAME "Qt Utilities")
set(META_APP_AUTHOR "Martchus")
set(META_APP_URL "https://github.com/${META_APP_AUTHOR}/${META_PROJECT_NAME}")
set(META_APP_DESCRIPTION "Common Qt related C++ classes and routines used by my applications such as dialogs, widgets and models.")
set(META_VERSION_MAJOR 4)
set(META_VERSION_MINOR 0)
set(META_VERSION_PATCH 0)
set(META_APP_VERSION ${META_VERSION_MAJOR}.${META_VERSION_MINOR}.${META_VERSION_PATCH})

# stringification of meta data
set(META_PROJECT_NAME_STR "\"${META_PROJECT_NAME}\"")
set(META_APP_NAME_STR "\"${META_APP_NAME}\"")
set(META_APP_AUTHOR_STR "\"${META_APP_AUTHOR}\"")
set(META_APP_URL_STR "\"${META_APP_URL}\"")
set(META_APP_DESCRIPTION_STR "\"${META_APP_DESCRIPTION}\"")
set(META_APP_VERSION ${META_VERSION_MAJOR}.${META_VERSION_MINOR}.${META_VERSION_PATCH})
set(META_APP_VERSION_STR "\"${META_APP_VERSION}\"")

# define project
project(${META_PROJECT_NAME})

# add configuration header
configure_file(
    "${PROJECT_SOURCE_DIR}/resources/config.h.in"
    "${PROJECT_BINARY_DIR}/resources/config.h"
)
include_directories("${PROJECT_BINARY_DIR}")

# add windows resource file
if(MINGW)
    # create windows rc file from template
    set(WINDOWS_EXT "dll")
    configure_file(
        "${PROJECT_SOURCE_DIR}/resources/windows.rc.in"
        "${PROJECT_BINARY_DIR}/resources/windows.rc"
    )
    # set windres as resource compiler
    set(RES_FILES "${PROJECT_BINARY_DIR}/resources/windows.rc")
    set(CMAKE_RC_COMPILER_INIT windres)
    set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
    enable_language(RC)
endif(MINGW)

# remove library prefix when building with mingw-w64 (just for consistancy with qmake)
if(MINGW)
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif(MINGW)

# read cached variables
set(WIDGETS_GUI "yes" CACHE STRING "enables/disables Qt Widgets specific features: yes (default) or no")
if(${WIDGETS_GUI} STREQUAL "yes")
    message(STATUS "Building with Qt Widgets specific features.")
elseif(${WIDGETS_GUI} STREQUAL "no")
    message(STATUS "Building WITHOUT Qt Widgets specific features.")
else()
    message(FATAL_ERROR "Specification whether to build Qt Widgets specific features is invalid (must be either yes or no).")
endif()
set(GUI_QTQUICK "yes" CACHE STRING "enables/disables building Qt Quick specific features: yes (default) or no")
if(${GUI_QTQUICK} STREQUAL "yes")
    message(STATUS "Building with Qt Quick specific features.")
elseif(${GUI_QTQUICK} STREQUAL "no")
    message(STATUS "Building WITHOUT Qt Quick specific features.")
else()
    message(FATAL_ERROR "Specification whether to build Qt Quick specific features is invalid (must be either yes or no).")
endif()

# enable Qt Widgets specific features
if(${WIDGETS_GUI} STREQUAL "yes")
    add_definitions(
        -DGUI_QTWIDGETS
        -DMODEL_UNDO_SUPPORT
    )
endif()

# enable Qt Quick specific features
if(${WIDGETS_GUI} STREQUAL "yes")
    add_definitions(
        -DGUI_QTQUICK
    )
endif()

# disable new ABI (can't catch ios_base::failure with new ABI)
add_definitions(
    -D_GLIBCXX_USE_CXX11_ABI=0
)

# enable code for debugging
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG_BUILD)
    message(STATUS "Debug build enabled.")
endif()

# check required Qt 5 modules
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
if(${WIDGETS_GUI} STREQUAL "yes")
    find_package(Qt5Widgets REQUIRED)
endif()

# enable moc, uic and rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# stuff which is only required when linking against Qt Widgets
if(${WIDGETS_GUI} STREQUAL "yes")
    set(WIDGETS_STUFF ${GUI_TYPE} ${WIDGETS_HEADER_FILES} ${WIDGETS_SRC_FILES} ${WIDGETS_UI_FILES} ${QM_FILES} ${RES_FILES})
endif()

# executable and linking
add_library(${META_PROJECT_NAME} SHARED ${HEADER_FILES} ${SRC_FILES} ${WIDGETS_STUFF} ${RES_FILES} ${WINDOWS_ICON_PATH})
if(WIN32)
    # WinAPI provides functions required for capslock detection
    add_definitions(-DPLATFORM_SPECIFIC_CAPSLOCK_DETECTION)
else()
    # X11 can provide functions required for capslock detection under non-Windows environments
    find_package(X11)
    if(X11_FOUND)
        add_definitions(-DPLATFORM_SPECIFIC_CAPSLOCK_DETECTION -DX_AVAILABLE)
    endif()
endif()
target_link_libraries(${META_PROJECT_NAME} c++utilities Qt5::Core Qt5::Widgets ${X11_LIBRARIES})
set_target_properties(${META_PROJECT_NAME} PROPERTIES
    VERSION ${META_VERSION_MAJOR}.${META_VERSION_MINOR}.${META_VERSION_PATCH}
    SOVERSION ${META_VERSION_MAJOR}
    CXX_STANDARD 11
)
if(MINGW)
    # enable static library when building with mingw-w64
    add_library(${META_PROJECT_NAME}_static STATIC ${HEADER_FILES} ${SRC_FILES} ${RES_FILES} ${WINDOWS_ICON_PATH})
    target_link_libraries(${META_PROJECT_NAME}_static c++utilities Qt5::Core Qt5::Widgets ${X11_LIBRARIES})
    set_target_properties(${META_PROJECT_NAME}_static PROPERTIES
        VERSION ${META_VERSION_MAJOR}.${META_VERSION_MINOR}.${META_VERSION_PATCH}
        SOVERSION ${META_VERSION_MAJOR}
        OUTPUT_NAME ${META_PROJECT_NAME}
        CXX_STANDARD 11
    )
endif(MINGW)

# add install target
install(TARGETS ${META_PROJECT_NAME}
    RUNTIME DESTINATION bin
    COMPONENT binary
    LIBRARY DESTINATION lib
    COMPONENT binary
    ARCHIVE DESTINATION lib
    COMPONENT binary
)
if(MINGW)
    install(TARGETS ${META_PROJECT_NAME}_static
        RUNTIME DESTINATION bin
        COMPONENT binary
        LIBRARY DESTINATION lib
        COMPONENT binary
        ARCHIVE DESTINATION lib
        COMPONENT binary
    )
endif(MINGW)
foreach(HEADER_FILE ${HEADER_FILES})
    get_filename_component(HEADER_DIR ${HEADER_FILE} DIRECTORY)
    install(
        FILES ${HEADER_FILE}
        DESTINATION include/${META_PROJECT_NAME}/${HEADER_DIR}
        COMPONENT header
    )
endforeach()
if(NOT TARGET install-binary)
    add_custom_target(install-binary
        DEPENDS ${META_PROJECT_NAME}
        COMMAND "${CMAKE_COMMAND}" -DCMAKE_INSTALL_COMPONENT=binary -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
    )
endif()
if(NOT TARGET install-header)
    add_custom_target(install-header
        DEPENDS ${META_PROJECT_NAME}
        COMMAND "${CMAKE_COMMAND}" -DCMAKE_INSTALL_COMPONENT=header -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
    )
endif()
if(NOT TARGET install-mingw-w64)
    add_custom_target(install-mingw-w64
        DEPENDS install-binary install-header
    )
endif()
if(NOT TARGET install-binary-strip)
    add_custom_target(install-binary-strip
        DEPENDS ${META_PROJECT_NAME}
        COMMAND "${CMAKE_COMMAND}" -DCMAKE_INSTALL_DO_STRIP=1 -DCMAKE_INSTALL_COMPONENT=binary -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
    )
endif()
if(NOT TARGET install-mingw-w64-importlib-strip)
    add_custom_target(install-mingw-w64-importlib-strip
        DEPENDS install-binary-strip
        COMMAND "${CMAKE_FIND_ROOT_PATH}/bin/strip" --strip-unneeded "${CMAKE_INSTALL_PREFIX}/lib/lib${META_PROJECT_NAME}.dll.a"
    )
endif()
if(NOT TARGET install-mingw-w64-staticlib-strip)
    add_custom_target(install-mingw-w64-staticlib-strip
        DEPENDS install-binary-strip
        COMMAND "${CMAKE_FIND_ROOT_PATH}/bin/strip" -g "${CMAKE_INSTALL_PREFIX}/lib/lib${META_PROJECT_NAME}.a"
    )
endif()
if(NOT TARGET install-mingw-w64-strip)
    add_custom_target(install-mingw-w64-strip
        DEPENDS install-binary-strip install-mingw-w64-importlib-strip install-mingw-w64-staticlib-strip install-header
    )
endif()
