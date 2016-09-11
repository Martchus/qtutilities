# determines the Qt linkage

if(NOT DEFINED QT_LINKAGE_DETERMINED)
    set(QT_LINKAGE_DETERMINED true)

    set(QT_LINKAGE "AUTO_LINKAGE")
    if(BUILD_STATIC_LIBS AND BUILD_SHARED_LIBS)
        message(FATAL_ERROR "When using Qt/KDE modules it is not possible to build shared and static libraries at the same time.")
    endif()
    # set USE_STATIC_QT_BUILD variable to ON to use static Qt
    # this only works with patched mingw-w64-qt5-* packages found in my PKGBUILDs repository
    # in any other environment you must ensure that the available Qt version is in accordance with the specified STATIC_LINKAGE/STATIC_LIBRARY_LINKAGE options
    if(BUILD_STATIC_LIBS OR ("${QT_LINKAGE}" STREQUAL "AUTO_LINKAGE" AND ((STATIC_LINKAGE AND "${META_PROJECT_TYPE}" STREQUAL "application") OR (STATIC_LIBRARY_LINKAGE AND ("${META_PROJECT_TYPE}" STREQUAL "" OR "${META_PROJECT_TYPE}" STREQUAL "library")))) OR ("${QT_LINKAGE}" STREQUAL "STATIC"))
        set(USE_STATIC_QT_BUILD ON)
        message(STATUS "Linking ${META_PROJECT_NAME} statically against Qt 5.")
    elseif(("${QT_LINKAGE}" STREQUAL "AUTO_LINKAGE") OR ("${QT_LINKAGE}" STREQUAL "SHARED"))
        set(USE_STATIC_QT_BUILD OFF)
        message(STATUS "Linking ${META_PROJECT_NAME} dynamically against Qt 5.")
    endif()

endif(NOT DEFINED QT_LINKAGE_DETERMINED)
