cmake_minimum_required(VERSION 3.3.0 FATAL_ERROR)

# defines helper to link against Qt dynamically or statically

# prevent multiple inclusion
if (DEFINED QT_LINKAGE_DETERMINED)
    return()
endif ()
set(QT_LINKAGE_DETERMINED ON)

# include validate_visibility from c++utilities' 3rdParty module
include(3rdParty)

# determine the minimum Qt version
if (NOT META_QT_VERSION)
    if (META_QT5_VERSION)
        # use deprecated META_QT5_VERSION variable (for compatibility)
        set(META_QT_VERSION "${META_QT5_VERSION}")
    else ()
        # require Qt 5.6 or higher by default
        set(META_QT_VERSION 5.6)
    endif ()
endif ()

# avoid "add_custom_target cannot create target "apk" because another target…" errors produced by Qt's Android support module
# (which can not cope with Qt CMake modules already pulled in by a dependency)
if (ANDROID AND NOT ${PROJECT_NAME}-MultiAbiBuild)
    set(${PROJECT_NAME}-MultiAbiBuild ON)
endif ()

# define function for using Qt and KDE Frameworks modules and static plugins
macro (use_qt_module)
    # parse arguments
    set(OPTIONAL_ARGS ONLY_PLUGINS)
    set(ONE_VALUE_ARGS PREFIX MODULE VISIBILITY LIBRARIES_VARIABLE)
    set(MULTI_VALUE_ARGS TARGETS PLUGINS)
    cmake_parse_arguments(ARGS "${OPTIONAL_ARGS}" "${ONE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN})

    # validate values
    if (NOT ARGS_PREFIX)
        message(FATAL_ERROR "use_qt_module called without PREFIX.")
    endif ()
    if (NOT ARGS_MODULE)
        message(FATAL_ERROR "use_qt_module called without MODULE.")
    endif ()
    if (ARGS_VISIBILITY)
        validate_visibility(${ARGS_VISIBILITY})
    else ()
        set(ARGS_VISIBILITY PRIVATE)
    endif ()
    if (NOT ARGS_LIBRARIES_VARIABLE)
        set(ARGS_LIBRARIES_VARIABLE "${ARGS_VISIBILITY}_LIBRARIES")
    endif ()
    if (NOT ARGS_TARGETS)
        if (${MODULE}_MODULE_TARGETS)
            set(ARGS_TARGETS "${${MODULE}_MODULE_TARGETS}")
        else ()
            set(ARGS_TARGETS "${ARGS_PREFIX}::${ARGS_MODULE}")
        endif ()
    endif ()
    if (ARGS_ONLY_PLUGINS AND NOT ARGS_PLUGINS)
        message(FATAL_ERROR "ONLY_PLUGINS specified but no plugins.")
    endif ()

    # find and use module
    if (NOT ONLY_PLUGINS)
        find_package("${ARGS_PREFIX}${ARGS_MODULE}" "${META_QT_VERSION}" REQUIRED)
        foreach (TARGET ${ARGS_TARGETS})
            if (NOT TARGET "${TARGET}")
                message(FATAL_ERROR "The ${ARGS_PREFIX}${ARGS_MODULE} does not provide the target ${TARGET}.")
            endif ()
            if ("${TARGET}" IN_LIST "${ARGS_LIBRARIES_VARIABLE}")
                continue()
            endif ()
            set("${ARGS_LIBRARIES_VARIABLE}" "${${ARGS_LIBRARIES_VARIABLE}};${TARGET}")
            set("PKG_CONFIG_${ARGS_PREFIX}_${ARGS_MODULE}" "${ARGS_PREFIX}${ARGS_MODULE}")
            message(STATUS "Linking ${META_TARGET_NAME} against Qt module ${TARGET}.")

            # hack for "StaticQt5": re-assign INTERFACE_LINK_LIBRARIES_RELEASE to INTERFACE_LINK_LIBRARIES
            get_target_property("${ARGS_MODULE}_INTERFACE_LINK_LIBRARIES_RELEASE" "${TARGET}"
                                INTERFACE_LINK_LIBRARIES_RELEASE)
            if ("${ARGS_MODULE}_INTERFACE_LINK_LIBRARIES_RELEASE")
                set_target_properties("${TARGET}" PROPERTIES INTERFACE_LINK_LIBRARIES
                                                             "${${ARGS_MODULE}_INTERFACE_LINK_LIBRARIES_RELEASE}")
            endif ()
        endforeach ()
    endif ()

    # find and use plugins
    foreach (PLUGIN ${ARGS_PLUGINS})
        set(PLUGIN_TARGET "${ARGS_PREFIX}::Q${PLUGIN}Plugin")
        if (NOT TARGET "${PLUGIN_TARGET}")
            find_package("${ARGS_PREFIX}${ARGS_MODULE}" "${META_QT_VERSION}" REQUIRED)
        endif ()
        if (NOT TARGET "${PLUGIN_TARGET}"
            AND PLUGIN MATCHES "Svg.*"
            AND ARGS_MODULE STREQUAL "Svg")
            # look for Svg plugins within the Gui module's directory as well
            find_package("${ARGS_PREFIX}Gui" "${META_QT_VERSION}" REQUIRED)
            set(PLUGIN_CONFIG "${${ARGS_PREFIX}Gui_DIR}/${ARGS_PREFIX}Q${PLUGIN}PluginConfig.cmake")
            if (EXISTS "${PLUGIN_CONFIG}")
                include("${PLUGIN_CONFIG}")
            endif ()
        endif ()
        if (NOT TARGET "${PLUGIN_TARGET}")
            message(
                FATAL_ERROR
                    "The ${ARGS_PREFIX}${ARGS_MODULE} package does not provide the target ${ARGS_PREFIX}::Q${PLUGIN}Plugin.")
        endif ()
        if ("${PLUGIN_TARGET}" IN_LIST "${ARGS_LIBRARIES_VARIABLE}")
            continue()
        endif ()
        set("${ARGS_LIBRARIES_VARIABLE}" "${${ARGS_LIBRARIES_VARIABLE}};${ARGS_PREFIX}::Q${PLUGIN}Plugin")
        message(STATUS "Linking ${META_TARGET_NAME} against Qt plugin ${ARGS_PREFIX}::Q${PLUGIN}Plugin.")
    endforeach ()

    # unset variables (can not simply use a function because Qt's variables need to be exported)
    foreach (ARGUMENT ${OPTIONAL_ARGS} ${ONE_VALUE_ARGS} ${MULTI_VALUE_ARGS})
        unset(ARGS_${ARGUMENT})
    endforeach ()
endmacro ()

# define function to make Qt variable available; queries qmake or uses variables provided by Qt 6
function (query_qmake_variable QMAKE_VARIABLE)
    # prevent queries for variables already known
    if (NOT "${${QMAKE_VARIABLE}}" STREQUAL "")
        return()
    endif ()

    # check configuration variables provided by Qt 6
    if (QMAKE_VARIABLE MATCHES "QT_(.*)")
        set(VARIABLE_NAME "${CMAKE_MATCH_1}")
        if (QT6_${VARIABLE_NAME})
            set(VARIABLE_NAME QT6_${VARIABLE_NAME})
        endif ()
        if (NOT "${${VARIABLE_NAME}}" STREQUAL "")
            set("${QMAKE_VARIABLE}"
                "${${VARIABLE_NAME}}"
                PARENT_SCOPE)
            message(STATUS "Qt variable ${QMAKE_VARIABLE} taken from ${VARIABLE_NAME}: ${${VARIABLE_NAME}}")
            return()
        endif ()
    endif ()

    # execute qmake
    get_target_property(QMAKE_BIN "${QT_QMAKE_TARGET}" IMPORTED_LOCATION)
    execute_process(
        COMMAND "${QMAKE_BIN}" -query "${QMAKE_VARIABLE}"
        RESULT_VARIABLE "${QMAKE_VARIABLE}_RESULT"
        OUTPUT_VARIABLE "${QMAKE_VARIABLE}")
    if (NOT "${${QMAKE_VARIABLE}_RESULT}" STREQUAL 0 OR "${${QMAKE_VARIABLE}}" STREQUAL "")
        message(
            FATAL_ERROR
                "Unable to read qmake variable ${QMAKE_VARIABLE} via \"${QMAKE_BIN} -query ${QMAKE_VARIABLE}\"; output was \"${${QMAKE_VARIABLE}}\"."
        )
    endif ()

    # remove new-line character at the end
    string(REGEX REPLACE "\n$" "" "${QMAKE_VARIABLE}" "${${QMAKE_VARIABLE}}")

    # export variable to parent scope
    set("${QMAKE_VARIABLE}"
        "${${QMAKE_VARIABLE}}"
        PARENT_SCOPE)
    message(STATUS "Qt variable ${QMAKE_VARIABLE} queried from qmake: ${${QMAKE_VARIABLE}}")
endfunction ()

# define function to make Qt variable available, resolving relative paths via CMAKE_FIND_ROOT_PATH
function (query_qmake_variable_path QMAKE_VARIABLE)
    # query the variable itself
    query_qmake_variable("${QMAKE_VARIABLE}")
    set(VARIABLE_VALUE "${${QMAKE_VARIABLE}}")
    if (NOT VARIABLE_VALUE)
        message(WARNING "Unable to resolve Qt variable ${QMAKE_VARIABLE}: it is empty")
        return()
    endif ()

    # pass the variable as-is if it points to an existing dir/file
    if (EXISTS "${VARIABLE_VALUE}")
        get_filename_component(VARIABLE_VALUE "${VARIABLE_VALUE}" REALPATH)
        message(STATUS "Qt variable ${QMAKE_VARIABLE} resolved to path: ${VARIABLE_VALUE}")
        set("${QMAKE_VARIABLE}"
            "${VARIABLE_VALUE}"
            PARENT_SCOPE)
        return()
    endif ()

    # assume VARIABLE_VALUE is relative within CMAKE_FIND_ROOT_PATH, e.g. QT_INSTALL_TRANSLATIONS might be set to
    # "share/qt6/translations"
    foreach (ROOT_PATH ${CMAKE_FIND_ROOT_PATH} "")
        foreach (PREFIX_PATH ${CMAKE_PREFIX_PATH} "")
            string(JOIN "/" FULL_PATH ${ROOT_PATH} ${PREFIX_PATH} ${VARIABLE_VALUE})
            if (EXISTS "${FULL_PATH}")
                set("${QMAKE_VARIABLE}"
                    "${FULL_PATH}"
                    PARENT_SCOPE)
                message(STATUS "Qt variable ${QMAKE_VARIABLE} resolved to path: ${FULL_PATH}")
                return()
            else ()
                list(APPEND CHECKED_PATHS "${FULL_PATH}")
            endif ()
        endforeach ()
    endforeach ()
    message(
        WARNING "Unable to resolve Qt variable ${QMAKE_VARIABLE} to an existing path, was checking for: ${CHECKED_PATHS}")
endfunction ()
