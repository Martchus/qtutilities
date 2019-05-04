cmake_minimum_required(VERSION 3.3.0 FATAL_ERROR)

# defines helper to link against Qt dynamically or statically

# prevent multiple inclusion
if (DEFINED QT_LINKAGE_DETERMINED)
    return()
endif ()
set(QT_LINKAGE_DETERMINED ON)

# include validate_visibility from c++utilities' 3rdParty module
include(3rdParty)

# by default, require Qt 5.6 or higher
if (NOT META_QT5_VERSION)
    set(META_QT5_VERSION 5.6)
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
        find_package("${ARGS_PREFIX}${ARGS_MODULE}" "${META_QT5_VERSION}" REQUIRED)
        foreach (TARGET ${ARGS_TARGETS})
            if (NOT TARGET "${TARGET}")
                message(FATAL_ERROR "The ${ARGS_PREFIX}${ARGS_MODULE} does not provide the target ${TARGET}.")
            endif ()
            if ("${TARGET}" IN_LIST "${ARGS_LIBRARIES_VARIABLE}")
                continue()
            endif ()
            set("${ARGS_LIBRARIES_VARIABLE}" "${${ARGS_LIBRARIES_VARIABLE}};${TARGET}")
            set("PKG_CONFIG_${ARGS_PREFIX}_${ARGS_MODULE}" "${ARGS_PREFIX}${ARGS_MODULE}")
            message(STATUS "Linking ${META_TARGET_NAME} against Qt 5 module ${TARGET}.")

            # hack for "StaticQt5": re-assign INTERFACE_LINK_LIBRARIES_RELEASE to INTERFACE_LINK_LIBRARIES
            get_target_property("${ARGS_MODULE}_INTERFACE_LINK_LIBRARIES_RELEASE" "${TARGET}"
                                INTERFACE_LINK_LIBRARIES_RELEASE)
            if ("${ARGS_MODULE}_INTERFACE_LINK_LIBRARIES_RELEASE")
                set_target_properties("${TARGET}"
                                      PROPERTIES INTERFACE_LINK_LIBRARIES
                                                 "${${ARGS_MODULE}_INTERFACE_LINK_LIBRARIES_RELEASE}")
            endif ()
        endforeach ()
    endif ()

    # find and use plugins
    foreach (PLUGIN ${ARGS_PLUGINS})
        if (NOT TARGET "${ARGS_PREFIX}::Q${PLUGIN}Plugin")
            find_package("${PREFIX}${MODULE}" "${META_QT5_VERSION}" REQUIRED)
        endif ()
        if (NOT TARGET "${ARGS_PREFIX}::Q${PLUGIN}Plugin")
            message(FATAL_ERROR "The ${ARGS_PREFIX}${MODULE} does not provide the target ${ARGS_PREFIX}::Q${PLUGIN}Plugin.")
        endif ()
        if ("${ARGS_PREFIX}::Q${PLUGIN}Plugin" IN_LIST "${ARGS_LIBRARIES_VARIABLE}")
            continue()
        endif ()
        set("${ARGS_LIBRARIES_VARIABLE}" "${${ARGS_LIBRARIES_VARIABLE}};${ARGS_PREFIX}::Q${PLUGIN}Plugin")
        message(STATUS "Linking ${META_TARGET_NAME} against Qt 5 plugin ${ARGS_PREFIX}::Q${PLUGIN}Plugin.")
    endforeach ()

    # unset variables (can not simply use a function because Qt's variables need to be exported)
    foreach (ARGUMENT ${OPTIONAL_ARGS} ${ONE_VALUE_ARGS} ${MULTI_VALUE_ARGS})
        unset(ARGS_${ARGUMENT})
    endforeach ()
endmacro ()

# define function to make qmake variable available within CMake
function (query_qmake_variable QMAKE_VARIABLE)
    # prevent queries for variables already known
    if (NOT "${${QMAKE_VARIABLE}}" STREQUAL "")
        return()
    endif ()

    # execute qmake
    get_target_property(QMAKE_BIN Qt5::qmake IMPORTED_LOCATION)
    execute_process(COMMAND "${QMAKE_BIN}" -query "${QMAKE_VARIABLE}"
                    RESULT_VARIABLE "${QMAKE_VARIABLE}_RESULT"
                    OUTPUT_VARIABLE "${QMAKE_VARIABLE}")
    if (NOT "${${QMAKE_VARIABLE}_RESULT}" STREQUAL 0 OR "${${QMAKE_VARIABLE}}" STREQUAL "")
        message(
            FATAL_ERROR
                "Unable to read qmake variable ${QMAKE_VARIABLE} via \"${QMAKE_BIN} -query ${QMAKE_VARIABLE}\"; output was \"${${QMAKE_VARIABLE}}\"."
            )
    endif ()

    # remove new-line character at the end
    string(REGEX
           REPLACE "\n$"
                   ""
                   "${QMAKE_VARIABLE}"
                   "${${QMAKE_VARIABLE}}")

    # export variable to parent scope
    set("${QMAKE_VARIABLE}" "${${QMAKE_VARIABLE}}" PARENT_SCOPE)
    message(STATUS "qmake variable ${QMAKE_VARIABLE} is ${${QMAKE_VARIABLE}}")
endfunction ()
