cmake_minimum_required(VERSION 3.3.0 FATAL_ERROR)

# determines the Qt linkage

if(NOT DEFINED QT_LINKAGE_DETERMINED)
    set(QT_LINKAGE_DETERMINED true)
    include(3rdParty)

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
            set(${MODULE}_REQUIRED "")
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
            find_package(StaticQt5${MODULE})
            if(StaticQt5${MODULE}_FOUND)
                if(TARGET StaticQt5::${MODULE})
                    set(QT5_${MODULE}_STATIC_LIB StaticQt5::${MODULE})
                else()
                    set(QT5_${MODULE}_STATIC_LIB Qt5::static::${MODULE})
                endif()
                set(QT5_${MODULE}_ASSUME_STATIC OFF)
                set(QT5_${MODULE}_FOUND ON)
                # reverse lookup for pkg-config
                set(PC_PKG_STATIC_Qt5_static_${MODULE} "StaticQt5${MODULE}")
            else()
                # consider the regular Qt package (without "Static" prefix) the static version if
                # static Qt is required and Qt package with "Static" prefix doesn't exist
                # (fallback if not using patched version of Qt mentioned above)
                if(USE_STATIC_QT_BUILD AND NOT StaticQt5${MODULE}_FOUND AND Qt5${MODULE}_FOUND)
                    find_package(Qt5${MODULE} ${QT_5_${MODULE}_REQUIRED})
                    if(Qt5${MODULE}_FOUND)
                        set(QT5_${MODULE}_STATIC_LIB Qt5::${MODULE})
                        set(QT5_${MODULE}_ASSUME_STATIC ON)
                        set(QT5_${MODULE}_FOUND ON)
                        # reverse lookup for pkg-config
                        set(PC_PKG_STATIC_Qt5_${MODULE} "Qt5${MODULE}")
                        message(WARNING "Building static libs and/or static Qt linkage has been enabled. Hence assuming provided Qt 5 ${MODULE} library is static.")
                    endif()
                endif()
            endif()
        endif()

        # find dynamic version
        if(USE_SHARED_QT_BUILD)
            if(QT5_${MODULE}_ASSUME_STATIC)
                message(FATAL_ERROR "The provided Qt 5 ${MODULE} is assumed to be static. However, a shared version is required for building dynamic libs and/or dynamic Qt linkage.")
            endif()
            find_package(Qt5${MODULE} ${QT_5_${MODULE}_REQUIRED})
            if(Qt5${MODULE}_FOUND)
                set(QT5_${MODULE}_DYNAMIC_LIB Qt5::${MODULE})
                set(QT5_${MODULE}_FOUND ON)
                # reverse lookup for pkg-config
                set(PC_PKG_SHARED_Qt5_${MODULE} "Qt5${MODULE}")
            endif()
        endif()
    endmacro()

    macro(use_qt5_module MODULE REQUIRED)
        link_against_library("QT5_${MODULE}" "${QT_LINKAGE}" "${REQUIRED}")
        if(${MODULE} IN_LIST META_PUBLIC_QT_MODULES)
            list(APPEND META_PUBLIC_SHARED_LIB_DEPENDS ${QT5_${MODULE}_DYNAMIC_LIB})
            list(APPEND META_PUBLIC_STATIC_LIB_DEPENDS ${QT5_${MODULE}_STATIC_LIB})
        endif()
    endmacro()

    macro(use_static_qt5_plugin PLUGIN)
        if(TARGET "Qt5::static::Q${PLUGIN}Plugin")
            list(APPEND LIBRARIES "Qt5::static::Q${PLUGIN}Plugin")
            list(APPEND STATIC_LIBRARIES "Qt5::static::Q${PLUGIN}Plugin")
        else()
            list(APPEND LIBRARIES "Qt5::Q${PLUGIN}Plugin")
            list(APPEND STATIC_LIBRARIES "Qt5::Q${PLUGIN}Plugin")
        endif()
    endmacro()

endif(NOT DEFINED QT_LINKAGE_DETERMINED)
