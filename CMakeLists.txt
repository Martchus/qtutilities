cmake_minimum_required(VERSION 3.17.0 FATAL_ERROR)

# meta data
set(META_PROJECT_NAME qtutilities)
set(META_PROJECT_VARNAME QT_UTILITIES)
set(META_APP_NAME "Qt Utilities")
set(META_APP_AUTHOR "Martchus")
set(META_APP_URL "https://github.com/${META_APP_AUTHOR}/${META_PROJECT_NAME}")
set(META_APP_DESCRIPTION
    "Common Qt related C++ classes and routines used by my applications such as dialogs, widgets and models")
set(META_VERSION_MAJOR 6)
set(META_VERSION_MINOR 14)
set(META_VERSION_PATCH 0)
set(META_APP_VERSION ${META_VERSION_MAJOR}.${META_VERSION_MINOR}.${META_VERSION_PATCH})

project(${META_PROJECT_NAME})

# add project files
set(HEADER_FILES
    misc/xmlparsermacros.h
    misc/undefxmlparsermacros.h
    misc/trylocker.h
    misc/adoptlocker.h
    misc/dialogutils.h
    misc/desktoputils.h
    misc/conversion.h
    misc/compat.h
    models/checklistmodel.h
    resources/qtconfigarguments.h
    resources/resources.h
    resources/importplugin.h)
set(SRC_FILES misc/dialogutils.cpp misc/desktoputils.cpp models/checklistmodel.cpp resources/qtconfigarguments.cpp
              resources/resources.cpp)
set(RES_FILES resources/qtutilsicons.qrc)

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
    misc/recentmenumanager.h)
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
    misc/recentmenumanager.cpp)
set(WIDGETS_UI_FILES
    aboutdialog/aboutdialog.ui
    enterpassworddialog/enterpassworddialog.ui
    settingsdialog/settingsdialog.ui
    settingsdialog/qtappearanceoptionpage.ui
    settingsdialog/qtlanguageoptionpage.ui
    settingsdialog/qtenvoptionpage.ui
    paletteeditor/paletteeditor.ui)
set(QT_TESTS buttonoverlay dialogs)
set(CMAKE_MODULE_FILES
    cmake/modules/AndroidApk.cmake cmake/modules/QtConfig.cmake cmake/modules/QtGuiConfig.cmake
    cmake/modules/QtLinkage.cmake cmake/modules/QtWebViewProviderConfig.cmake cmake/modules/QtJsProviderConfig.cmake)
set(CMAKE_TEMPLATE_FILES cmake/templates/qtconfig.h.in cmake/templates/webviewdefs.h.in cmake/templates/webviewincludes.h.in
                         cmake/templates/jsdefs.h.in cmake/templates/jsincludes.h.in)
if (ANDROID)
    list(APPEND CMAKE_MODULE_FILES)
    list(APPEND CMAKE_TEMPLATE_FILES cmake/templates/android-deployment.json.in)
endif ()

set(TS_FILES translations/${META_PROJECT_NAME}_zh_CN.ts translations/${META_PROJECT_NAME}_de_DE.ts
             translations/${META_PROJECT_NAME}_en_US.ts)

set(DOC_FILES README.md)

set(REQUIRED_ICONS
    dialog-cancel
    dialog-ok
    dialog-ok-apply
    document-open
    document-open-recent
    edit-clear
    go-next
    preferences-desktop-icons
    preferences-desktop-locale
    qtcreator
    system-file-manager
    system-run
    system-search
    window-close)

set(SCRIPT_FILES scripts/required_icons.sh)

# required to include CMake modules from own project directory
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules" "${CMAKE_MODULE_PATH}")

# find c++utilities
set(CONFIGURATION_PACKAGE_SUFFIX
    ""
    CACHE STRING "sets the suffix for find_package() calls to packages configured via c++utilities")
set(PACKAGE_NAMESPACE
    ""
    CACHE STRING "sets the namespace (prefix) for find_package() calls to packages configured via c++utilities")
if (PACKAGE_NAMESPACE)
    set(PACKAGE_NAMESPACE_PREFIX "${PACKAGE_NAMESPACE}-")
endif ()
find_package(${PACKAGE_NAMESPACE_PREFIX}c++utilities${CONFIGURATION_PACKAGE_SUFFIX} 5.5.0 REQUIRED)
use_cpp_utilities()

# configure platform specific capslock detection for enterpassworddialog.cpp
if (WIN32)
    set(HAVE_PLATFORM_SPECIFIC_CAPSLOCK_DETECTION ON)
else ()
    include(3rdParty)
    use_package(TARGET_NAME X11::X11 PACKAGE_NAME X11 OPTIONAL)
    if (TARGET X11::X11)
        set_property(
            SOURCE enterpassworddialog/enterpassworddialog.cpp
            APPEND
            PROPERTY COMPILE_DEFINITIONS X_AVAILABLE)
        set(HAVE_PLATFORM_SPECIFIC_CAPSLOCK_DETECTION ON)
    endif ()
endif ()
option(CAPSLOCK_DETECTION "enables capslock detection" ${HAVE_PLATFORM_SPECIFIC_CAPSLOCK_DETECTION})
if (CAPSLOCK_DETECTION)
    if (NOT HAVE_PLATFORM_SPECIFIC_CAPSLOCK_DETECTION)
        message(FATAL_ERROR "No backend for capslock detection found (WinAPI or X11 must be provided)")
    endif ()
    set_property(
        SOURCE enterpassworddialog/enterpassworddialog.cpp
        APPEND
        PROPERTY COMPILE_DEFINITIONS ${META_PROJECT_VARNAME}_PLATFORM_SPECIFIC_CAPSLOCK_DETECTION)
endif ()

# configure support for D-Bus notifications
if (UNIX
    AND NOT APPLE
    AND NOT ANDROID)
    set(ENABLE_DBUS_NOTIFICATIONS_BY_DEFAULT ON)
else ()
    set(ENABLE_DBUS_NOTIFICATIONS_BY_DEFAULT OFF)
endif ()
option(DBUS_NOTIFICATIONS "enables support for D-Bus notifications" ${ENABLE_DBUS_NOTIFICATIONS_BY_DEFAULT})
set(DBUS_NOTIFICATIONS_FILE_NAME misc/dbusnotification)
if (DBUS_NOTIFICATIONS)
    list(APPEND HEADER_FILES ${DBUS_NOTIFICATIONS_FILE_NAME}.h)
    list(APPEND SRC_FILES ${DBUS_NOTIFICATIONS_FILE_NAME}.cpp)
    list(APPEND DBUS_FILES dbus/org.freedesktop.Notifications.xml)
    list(APPEND META_PUBLIC_COMPILE_DEFINITIONS ${META_PROJECT_VARNAME}_SUPPORT_DBUS_NOTIFICATIONS)
    list(APPEND QT_TESTS dbusnotification)
    message(STATUS "D-Bus notifications enabled")
else ()
    list(APPEND EXCLUDED_FILES ${DBUS_NOTIFICATIONS_FILE_NAME}.h ${DBUS_NOTIFICATIONS_FILE_NAME}.cpp)
    message(STATUS "D-Bus notifications disabled")
endif ()

# include modules to apply configuration
include(BasicConfig)
include(QtGuiConfig)

# add further Qt modules (which are not automatically detected)
set(META_PUBLIC_QT_MODULES Core ${ADDITIONAL_QT_MODULES})

# include further modules to apply configuration
include(QtConfig)
include(WindowsResources)
include(LibraryTarget)
include(Doxygen)
include(ConfigHeader)

# configure test target
include(TestUtilities)
list(APPEND QT_TEST_LIBRARIES ${CPP_UTILITIES_LIB} ${META_TARGET_NAME})
use_qt_module(LIBRARIES_VARIABLE "QT_TEST_LIBRARIES" PREFIX "${QT_PACKAGE_PREFIX}" MODULE "Test")
foreach (TEST ${QT_TESTS})
    configure_test_target(TEST_NAME "${TEST}_tests" SRC_FILES "tests/${TEST}.cpp" LIBRARIES "${QT_TEST_LIBRARIES}")
endforeach ()
