cmake_minimum_required(VERSION 3.3.0 FATAL_ERROR)

# defines helper to link against Qt dynamically or statically

# prevent multiple inclusion
if(DEFINED QT_LINKAGE_DETERMINED)
    return()
endif()
set(QT_LINKAGE_DETERMINED ON)

include(3rdParty)

# by default, require Qt 5.6 or higher
if(NOT META_QT5_VERSION)
    set(META_QT5_VERSION 5.6)
endif()

# determine whether to use dynamic or shared version of Qt (or both)
set(QT_LINKAGE "AUTO_LINKAGE" CACHE STRING "specifies whether to link statically or dynamically against Qt 5")
if(BUILD_STATIC_LIBS OR ("${QT_LINKAGE}" STREQUAL "AUTO_LINKAGE" AND ((STATIC_LINKAGE AND "${META_PROJECT_TYPE}" STREQUAL "application") OR (STATIC_LIBRARY_LINKAGE AND ("${META_PROJECT_TYPE}" STREQUAL "" OR "${META_PROJECT_TYPE}" STREQUAL "library")))) OR ("${QT_LINKAGE}" STREQUAL "STATIC"))
    set(USE_STATIC_QT_BUILD ON)
    message(STATUS "Checking for static Qt 5 libraries to use in project ${TARGET_PREFIX}${META_PROJECT_NAME}${TARGET_SUFFIX}")
endif()
if(("${QT_LINKAGE}" STREQUAL "AUTO_LINKAGE" AND (NOT (STATIC_LINKAGE AND "${META_PROJECT_TYPE}" STREQUAL "application") OR NOT (STATIC_LIBRARY_LINKAGE AND ("${META_PROJECT_TYPE}" STREQUAL "" OR "${META_PROJECT_TYPE}" STREQUAL "library")))) OR ("${QT_LINKAGE}" STREQUAL "SHARED"))
    set(USE_SHARED_QT_BUILD ON)
    message(STATUS "Checking for dynamic Qt 5 libraries to use in project ${TARGET_PREFIX}${META_PROJECT_NAME}${TARGET_SUFFIX}")
endif()

macro(find_qt5_module MODULE REQUIRED)
    # determine whether the library is required or optional
    # FIXME: improve passing required argument
    if("${REQUIRED}" STREQUAL "OPTIONAL")
        unset(QT_5_${MODULE}_REQUIRED)
    elseif("${REQUIRED}" STREQUAL "REQUIRED")
        set(QT_5_${MODULE}_REQUIRED "REQUIRED")
    else()
        message(FATAL_ERROR "Invalid use of link_against_library; must specify either REQUIRED or OPTIONAL.")
    endif()

    # find static version
    if(USE_STATIC_QT_BUILD)
        # check for 'Static'-prefixed CMake module first
        # - patched mingw-w64-qt5 packages providing those files are available in my PKGBUILDs repository
        # - has the advantage that usage of dynamic and static Qt during the same build is possible
        find_package(StaticQt5${MODULE} ${META_QT5_VERSION})
        if(StaticQt5${MODULE}_FOUND)
            if(TARGET StaticQt5::${MODULE})
                set(QT5_${MODULE}_STATIC_PREFIX "StaticQt5::")
            else()
                set(QT5_${MODULE}_STATIC_PREFIX "Qt5::static::")
            endif()
            set(QT5_${MODULE}_STATIC_LIB "${QT5_${MODULE}_STATIC_PREFIX}${MODULE}")
            set(QT5_${MODULE}_ASSUME_STATIC OFF)
            set(QT5_${MODULE}_FOUND ON)
            # reverse lookup for pkg-config
            set(PC_PKG_STATIC_Qt5_${MODULE} "StaticQt5${MODULE}")
            set(PC_PKG_STATIC_StaticQt5_${MODULE} "StaticQt5${MODULE}")
            set(PC_PKG_STATIC_Qt5_static_${MODULE} "StaticQt5${MODULE}")
        else()
            # consider the regular Qt package (without "Static" prefix) the static version if
            # static Qt is required and Qt package with "Static" prefix doesn't exist
            # (fallback if not using patched version of Qt mentioned above)
            find_package(Qt5${MODULE} ${META_QT5_VERSION} ${QT_5_${MODULE}_REQUIRED})
            if(Qt5${MODULE}_FOUND)
                set(QT5_${MODULE}_STATIC_PREFIX "Qt5::")
                set(QT5_${MODULE}_STATIC_LIB "${QT5_${MODULE}_STATIC_PREFIX}${MODULE}")
                set(QT5_${MODULE}_ASSUME_STATIC ON)
                set(QT5_${MODULE}_FOUND ON)
                # reverse lookup for pkg-config
                set(PC_PKG_STATIC_Qt5_${MODULE} "Qt5${MODULE}")
                message(WARNING "Building static libs and/or static Qt linkage has been enabled. Hence assuming provided Qt 5 module ${MODULE} is static.")
            endif()
        endif()

        # use INTERFACE_LINK_LIBRARIES_RELEASE of the imported target as general INTERFACE_LINK_LIBRARIES to get correct transitive dependencies
        # under any configuration
        if(StaticQt5${MODULE}_FOUND OR Qt5${MODULE}_FOUND)
            get_target_property(QT5_${MODULE}_STATIC_LIB_DEPS "${QT5_${MODULE}_STATIC_LIB}" INTERFACE_LINK_LIBRARIES_RELEASE)
            set_target_properties("${QT5_${MODULE}_STATIC_LIB}" PROPERTIES INTERFACE_LINK_LIBRARIES "${QT5_${MODULE}_STATIC_LIB_DEPS}")
        endif()
    endif()

    # find dynamic version
    if(USE_SHARED_QT_BUILD)
        if(QT5_${MODULE}_ASSUME_STATIC)
            message(FATAL_ERROR "The provided Qt 5 module ${MODULE} is assumed to be static. However, a shared version is required for building dynamic libs and/or dynamic Qt linkage.")
        endif()
        find_package(Qt5${MODULE} ${META_QT5_VERSION} ${QT_5_${MODULE}_REQUIRED})
        if(Qt5${MODULE}_FOUND)
            set(QT5_${MODULE}_DYNAMIC_LIB Qt5::${MODULE})
            set(QT5_${MODULE}_FOUND ON)
            # reverse lookup for pkg-config
            set(PC_PKG_SHARED_Qt5_${MODULE} "Qt5${MODULE}")
        endif()
    endif()
endmacro()

macro(use_qt5_module MODULE REQUIRED)
    if(${MODULE} IN_LIST META_PUBLIC_QT_MODULES)
        list(APPEND META_PUBLIC_SHARED_LIB_DEPENDS "${QT5_${MODULE}_DYNAMIC_LIB}")
        list(APPEND META_PUBLIC_STATIC_LIB_DEPENDS "${QT5_${MODULE}_STATIC_LIB}")
    endif()
    link_against_library("QT5_${MODULE}" "${QT_LINKAGE}" "${REQUIRED}")
endmacro()

macro(use_static_qt5_plugin MODULE PLUGIN FOR_SHARED_TARGET FOR_STATIC_TARGET)
    if("${FOR_SHARED_TARGET}")
        list(APPEND PRIVATE_LIBRARIES "${QT5_${MODULE}_STATIC_PREFIX}Q${PLUGIN}Plugin")
        message(STATUS "Linking ${TARGET_PREFIX}${META_PROJECT_NAME}${TARGET_SUFFIX} against static Qt 5 plugin ${QT5_${MODULE}_STATIC_PREFIX}Q${PLUGIN}Plugin")
    endif()
    if("${FOR_STATIC_TARGET}")
        list(APPEND PRIVATE_STATIC_LIBRARIES "${QT5_${MODULE}_STATIC_PREFIX}Q${PLUGIN}Plugin")
        message(STATUS "Adding static Qt 5 plugin ${QT5_${MODULE}_STATIC_PREFIX}Q${PLUGIN}Plugin to dependencies of static ${TARGET_PREFIX}${META_PROJECT_NAME}${TARGET_SUFFIX}")
    endif()
endmacro()

macro(query_qmake_variable QMAKE_VARIABLE)
    get_target_property(QMAKE_BIN Qt5::qmake IMPORTED_LOCATION)
    execute_process(
        COMMAND "${QMAKE_BIN}" -query "${QMAKE_VARIABLE}"
        RESULT_VARIABLE "${QMAKE_VARIABLE}_RESULT"
        OUTPUT_VARIABLE "${QMAKE_VARIABLE}"
    )
    if(NOT "${${QMAKE_VARIABLE}_RESULT}" STREQUAL 0 OR "${${QMAKE_VARIABLE}}" STREQUAL "")
        message(FATAL_ERROR "Unable to read qmake variable ${QMAKE_VARIABLE} via \"${QMAKE_BIN} -query ${QMAKE_VARIABLE}\"; output was \"${${QMAKE_VARIABLE}}\".")
    endif()
    string(REGEX REPLACE "\n$" "" "${QMAKE_VARIABLE}" "${${QMAKE_VARIABLE}}")
    message(STATUS "qmake variable ${QMAKE_VARIABLE} is ${${QMAKE_VARIABLE}}")
endmacro()
