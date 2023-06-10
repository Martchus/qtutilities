cmake_minimum_required(VERSION 3.3.0 FATAL_ERROR)

# determines the web view provider (either Qt WebKit or Qt WebEngine)

if (TARGET_CONFIG_DONE)
    message(FATAL_ERROR "Can not include QtWebViewProviderConfig module when targets are already configured.")
endif ()

# include required modules
include(QtLinkage)

# check whether Qt WebEngine is present
find_package("${QT_PACKAGE_PREFIX}WebEngineWidgets" "${META_QT_VERSION}")
set(WEBVIEW_PROVIDER_DEFAULT "none")
if ("${${QT_PACKAGE_PREFIX}WebEngineWidgets_FOUND}")
    set(WEBVIEW_PROVIDER_DEFAULT "webengine")
endif ()

# configure the specified web view provider
set(WEBVIEW_PROVIDER
    "${WEBVIEW_PROVIDER_DEFAULT}"
    CACHE STRING "specifies the web view provider: webengine (default), webkit or none")
if (WEBVIEW_PROVIDER STREQUAL "webkit")
    set(WEBVIEW_PROVIDER WebKitWidgets)
    set(WEBVIEW_DEFINITION "${META_PROJECT_VARNAME_UPPER}_USE_WEBKIT")
    message(STATUS "Using Qt WebKit as web view provider.")
elseif (WEBVIEW_PROVIDER STREQUAL "webengine")
    set(WEBVIEW_PROVIDER WebEngineWidgets)
    set(WEBVIEW_DEFINITION "${META_PROJECT_VARNAME_UPPER}_USE_WEBENGINE")
    list(APPEND ADDITIONAL_QT_REPOS "webengine")
    message(STATUS "Using Qt WebEngine as web view provider.")
elseif (WEBVIEW_PROVIDER STREQUAL "none")
    set(WEBVIEW_PROVIDER "")
    set(WEBVIEW_DEFINITION "${META_PROJECT_VARNAME_UPPER}_NO_WEBVIEW")
    message(STATUS "Built-in web view has been disabled.")
else ()
    message(FATAL_ERROR "The specified web view provider '${WEBVIEW_PROVIDER}' is unknown.")
endif ()

# add header files with some defines/includes to conveniently use the selected provider
if (WEBVIEW_PROVIDER)
    list(APPEND ADDITIONAL_QT_MODULES "${WEBVIEW_PROVIDER}")

    if (META_WEBVIEW_SRC_DIR)
        set(WEBVIEW_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${META_WEBVIEW_SRC_DIR}")
    else ()
        set(WEBVIEW_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/gui")
    endif ()

    include(TemplateFinder)
    find_template_file("webviewdefs.h" QT_UTILITIES WEBVIEWDEFS_H_TEMPLATE_FILE)
    configure_file(
        "${WEBVIEWDEFS_H_TEMPLATE_FILE}" "${WEBVIEW_HEADER_DIR}/webviewdefs.h" # simply add this to source to ease inclusion
        NEWLINE_STYLE UNIX # since this goes to sources ensure consistency
    )
    find_template_file("webviewincludes.h" QT_UTILITIES WEBVIEWINCLUDES_H_TEMPLATE_FILE)
    configure_file(
        "${WEBVIEWINCLUDES_H_TEMPLATE_FILE}" "${WEBVIEW_HEADER_DIR}/webviewincludes.h" # simply add this to source to ease
                                                                                       # inclusion
        NEWLINE_STYLE UNIX # since this goes to sources ensure consistency
    )
    list(APPEND WIDGETS_FILES "${WEBVIEW_HEADER_DIR}/webviewdefs.h" "${WEBVIEW_HEADER_DIR}/webviewincludes.h")
endif ()

list(APPEND META_PUBLIC_COMPILE_DEFINITIONS ${WEBVIEW_DEFINITION})
