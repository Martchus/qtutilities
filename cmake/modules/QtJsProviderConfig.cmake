# determines the JavaScript provider (either Qt Script or Qt Declarative)

if (TARGET_CONFIG_DONE)
    message(FATAL_ERROR "Can not include QtJsProviderConfig module when targets are already configured.")
endif ()

# configure the specified JavaScript provider
set(JS_PROVIDER
    "qml"
    CACHE STRING "specifies the JavaScript provider: qml (default), script or none")
if (JS_PROVIDER STREQUAL "script")
    set(JS_PROVIDER Script)
    set(JS_DEFINITION "${META_PROJECT_VARNAME_UPPER}_USE_SCRIPT")
    list(APPEND ADDITIONAL_QT_REPOS "script")
    message(STATUS "Using Qt Script as JavaScript provider.")
elseif (JS_PROVIDER STREQUAL "qml")
    set(JS_PROVIDER Qml)
    set(JS_DEFINITION "${META_PROJECT_VARNAME_UPPER}_USE_JSENGINE")
    list(APPEND ADDITIONAL_QT_REPOS "declarative")
    message(STATUS "Using Qt QML as JavaScript provider.")
elseif (JS_PROVIDER STREQUAL "none")
    set(JS_PROVIDER "")
    set(JS_DEFINITION "${META_PROJECT_VARNAME_UPPER}_NO_JSENGINE")
    message(STATUS "JavaScript provider has been disabled.")
else ()
    message(FATAL_ERROR "The specified JavaScript provider '${JS_PROVIDER}' is unknown.")
endif ()

# add header files with some defines/includes to conveniently use the selected provider
if (JS_PROVIDER)
    list(APPEND ADDITIONAL_QT_MODULES "${JS_PROVIDER}")

    if (META_JS_SRC_DIR)
        set(JS_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${META_JS_SRC_DIR}")
    else ()
        set(JS_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/gui")
    endif ()

    include(TemplateFinder)
    find_template_file("jsdefs.h" QT_UTILITIES JS_DEFS_H_TEMPLATE_FILE)
    configure_file(
        "${JS_DEFS_H_TEMPLATE_FILE}" "${JS_HEADER_DIR}/jsdefs.h" # simply add this to source to ease inclusion
        NEWLINE_STYLE UNIX # since this goes to sources ensure consistency
    )
    find_template_file("jsincludes.h" QT_UTILITIES JS_INCLUDES_H_TEMPLATE_FILE)
    configure_file(
        "${JS_INCLUDES_H_TEMPLATE_FILE}" "${JS_HEADER_DIR}/jsincludes.h" # simply add this to source to ease inclusion
        NEWLINE_STYLE UNIX # since this goes to sources ensure consistency
    )
    list(APPEND WIDGETS_FILES "${JS_HEADER_DIR}/jsdefs.h" "${JS_HEADER_DIR}/jsincludes.h")
endif ()

list(APPEND META_PUBLIC_COMPILE_DEFINITIONS ${JS_DEFINITION})
