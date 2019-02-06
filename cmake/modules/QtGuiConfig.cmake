cmake_minimum_required(VERSION 3.3.0 FATAL_ERROR)

if (NOT BASIC_PROJECT_CONFIG_DONE)
    message(FATAL_ERROR "Before including the QtGuiConfig module, the BasicConfig module must be included.")
endif ()
if (QT_CONFIGURED)
    message(FATAL_ERROR "The QtGuiConfig module can not be included when Qt usage has already been configured.")
endif ()
if (TARGET_CONFIG_DONE)
    message(FATAL_ERROR "Can not include QtGuiConfig module when targets are already configured.")
endif ()

# enable Qt Widgets GUI
if (WIDGETS_GUI)
    list(APPEND META_PRIVATE_COMPILE_DEFINITIONS GUI_QTWIDGETS)
    list(APPEND META_PUBLIC_COMPILE_DEFINITIONS ${META_PROJECT_VARNAME_UPPER}_GUI_QTWIDGETS)
    list(APPEND WIDGETS_FILES
                ${WIDGETS_HEADER_FILES}
                ${WIDGETS_SRC_FILES}
                ${WIDGETS_RES_FILES}
                ${WIDGETS_UI_FILES})
    list(APPEND ADDITIONAL_HEADER_FILES ${WIDGETS_HEADER_FILES})
    if (WIDGETS_FILES)
        list(APPEND ADDITIONAL_QT_MODULES Widgets)
        message(STATUS "Building with Qt Widgets GUI.")
    else ()
        message(STATUS "Qt Widgets GUI is not available.")
    endif ()
else ()
    message(STATUS "Building WITHOUT Qt Widgets GUI.")
endif ()

# enable Qt Quick GUI
if (QUICK_GUI)
    list(APPEND META_PRIVATE_COMPILE_DEFINITIONS GUI_QTQUICK)
    list(APPEND META_PUBLIC_COMPILE_DEFINITIONS ${META_PROJECT_VARNAME_UPPER}_GUI_QTQUICK)
    list(APPEND QML_FILES
                ${QML_HEADER_FILES}
                ${QML_SRC_FILES}
                ${QML_RES_FILES})
    list(APPEND ADDITIONAL_HEADER_FILES ${QML_HEADER_FILES})
    if (QML_FILES)
        list(APPEND ADDITIONAL_QT_MODULES Qml Quick)
        list(APPEND ADDITIONAL_QT_REPOS "declarative")
        message(STATUS "Building with Qt Quick GUI.")

        # enable QML debugging
        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
            list(APPEND META_PRIVATE_COMPILE_DEFINITIONS QT_QML_DEBUG)
        endif ()

        # enable Qt Quick Controls 2
        if (META_USE_QQC2)
            list(APPEND ADDITIONAL_QT_MODULES QuickControls2)
        endif ()
    else ()
        message(STATUS "Qt Quick GUI is not available.")
    endif ()
else ()
    message(STATUS "Building WITHOUT Qt Quick GUI.")
endif ()

# do further GUI-related configuration only if at least one kind of GUI is enabled (tageditor allows building without GUI so
# this is a valid configuration)
if (WIDGETS_GUI OR QUICK_GUI)
    if (WIN32)
        # set "GUI-type" to WIN32 to hide console under Windows
        set(GUI_TYPE WIN32)
    elseif (APPLE)
        # make the GUI application a "bundle" under MacOSX
        set(GUI_TYPE MACOSX_BUNDLE)
    endif ()

    # add source files requried by both GUI variants
    list(APPEND SRC_FILES ${GUI_SRC_FILES})
    list(APPEND ADDITIONAL_HEADER_FILES ${GUI_HEADER_FILES})

    # add option for enabling/disabling static Qt plugins
    option(SVG_SUPPORT "whether to link against the SVG image format plugin (only relevant when using static Qt)" ON)
    option(SVG_ICON_SUPPORT "whether to link against the SVG icon engine (only relevant when using static Qt)" ON)
    set(IMAGE_FORMAT_SUPPORT "Gif;ICO;Jpeg"
        CACHE STRING "specifies the image format plugins to link against (only relevant when using static Qt)")

    if (ANDROID)
        list(APPEND ADDITIONAL_QT_MODULES Svg)
    endif ()
endif ()
