cmake_minimum_required(VERSION 3.17.0 FATAL_ERROR)

if (NOT BASIC_PROJECT_CONFIG_DONE)
    message(FATAL_ERROR "Before including the QtGuiConfig module, the BasicConfig module must be included.")
endif ()
if (QT_CONFIGURED)
    message(FATAL_ERROR "The QtGuiConfig module can not be included when Qt usage has already been configured.")
endif ()
if (TARGET_CONFIG_DONE)
    message(FATAL_ERROR "Can not include QtGuiConfig module when targets are already configured.")
endif ()

if (NOT WIDGETS_GUI AND NOT QUICK_GUI)
    message(STATUS "GUI is completely disabled.")
    return()
endif ()

list(APPEND ADDITIONAL_QT_MODULES Gui)

# enable Qt Widgets GUI
if (WIDGETS_GUI)
    list(APPEND META_PRIVATE_COMPILE_DEFINITIONS GUI_QTWIDGETS)
    list(APPEND META_PUBLIC_COMPILE_DEFINITIONS ${META_PROJECT_VARNAME_UPPER}_GUI_QTWIDGETS)
    list(APPEND WIDGETS_FILES ${WIDGETS_HEADER_FILES} ${WIDGETS_SRC_FILES} ${WIDGETS_RES_FILES} ${WIDGETS_UI_FILES})
    list(APPEND ADDITIONAL_HEADER_FILES ${WIDGETS_HEADER_FILES})
    if (WIDGETS_FILES OR META_HAS_WIDGETS_GUI)
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
    list(APPEND QML_FILES ${QML_HEADER_FILES} ${QML_SRC_FILES} ${QML_RES_FILES})
    list(APPEND ADDITIONAL_HEADER_FILES ${QML_HEADER_FILES})
    if (META_QUICK_GUI_MODES)
        if (NOT DEFAULT_QUICK_GUI_MODES)
            set(DEFAULT_QUICK_GUI_MODES "${META_QUICK_GUI_MODES}")
        endif ()
        set(QUICK_GUI_ENABLED_MODES
            "${DEFAULT_QUICK_GUI_MODES}"
            CACHE STRING "the Qt Quick GUI modes to enable")
        set(QUICK_GUI_ENABLED_MODED_STR "")
        foreach (MODE ${QUICK_GUI_ENABLED_MODES})
            if (NOT MODE IN_LIST META_QUICK_GUI_MODES)
                message(
                    FATAL_ERROR
                        "Configured Qt Quick GUI mode \"${MODE}\" is invalid. Valid modes are: ${META_QUICK_GUI_MODES}")
            endif ()
            string(TOUPPER "${MODE}" MODE_UPPER)
            set(QUICK_GUI_ENABLED_MODED_STR "${QUICK_GUI_ENABLED_MODED_STR}${MODE} ")
            list(APPEND META_PUBLIC_COMPILE_DEFINITIONS "${META_PROJECT_VARNAME_UPPER}_GUI_QTQUICK_MODE_${MODE_UPPER}")
            list(APPEND QML_FILES ${QML_HEADER_FILES_${MODE_UPPER}} ${QML_SRC_FILES_${MODE_UPPER}}
                 ${QML_RES_FILES_${MODE_UPPER}})
        endforeach ()
        list(GET QUICK_GUI_ENABLED_MODES 0 QTQUICK_GUI_PRIMARY_MODE)
        list(APPEND META_PRIVATE_COMPILE_DEFINITIONS GUI_QTQUICK_PRIMARY_MODE="${QTQUICK_GUI_PRIMARY_MODE}")
        list(APPEND META_PRIVATE_COMPILE_DEFINITIONS GUI_QTQUICK_MODES="${QUICK_GUI_ENABLED_MODED_STR}")
    endif ()
    if (QML_FILES OR META_HAS_QUICK_GUI)
        list(APPEND ADDITIONAL_QT_MODULES Qml Quick)
        list(APPEND ADDITIONAL_QT_REPOS "declarative")
        set(QUICK_GUI_CONTROLS_STYLE
            ""
            CACHE STRING "sets the Qt Quick Controls style")
        message(STATUS "Building with Qt Quick GUI.")

        # enable QML debugging
        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
            list(APPEND META_PRIVATE_COMPILE_DEFINITIONS QT_QML_DEBUG)
        endif ()

        # enable Qt Quick Controls 2 (only useful if runtime style selection is wanted)
        if (META_USE_QQC2)
            list(APPEND ADDITIONAL_QT_MODULES QuickControls2)
        endif ()
    else ()
        message(STATUS "Qt Quick GUI is not available.")
    endif ()
else ()
    message(STATUS "Building WITHOUT Qt Quick GUI.")
endif ()
if (META_QUICK_GUI_MODES)
    foreach (MODE ${META_QUICK_GUI_MODES})
        list(APPEND EXCLUDED_FILES ${QML_HEADER_FILES_${MODE_UPPER}} ${QML_SRC_FILES_${MODE_UPPER}}
             ${QML_RES_FILES_${MODE_UPPER}})
    endforeach ()
endif ()

# allow selecting Qt Quick Controls style at build time
set(QT_QUICK_STYLE_EXPERIMENTAL_NOTICE
    "It is considered experimental and might be replaced by a different helper using \"import QtQuick.Controls.Native\" and file selectors in the next minor release."
)
function (qt_utilities_change_qt_quick_controls_style QML_FILES_VARIABLE)
    message(
        WARNING
            "The CMake function qt_utilities_change_qt_quick_controls_style() is used. ${QT_QUICK_STYLE_EXPERIMENTAL_NOTICE}"
    )
    if (NOT QUICK_GUI_CONTROLS_STYLE)
        return()
    endif ()
    find_program(PERL_BIN perl)
    if (NOT PERL_BIN)
        message(FATAL_ERROR "Unable to find Perl, set PERL_BIN to the path of Perl's executable.")
    endif ()
    if (QUICK_GUI_CONTROLS_STYLE STREQUAL "dynamic")
        set(OVERRIDE "QtQuick.Controls")
    else ()
        set(OVERRIDE "QtQuick.Controls.${QUICK_GUI_CONTROLS_STYLE}")
    endif ()
    set(CHANGED_QML_FILES "")
    foreach (QML_FILE ${${QML_FILES_VARIABLE}})
        if (CMAKE_SYSTEM_NAME MATCHES "Windows.*")
            set(CHANGED_QML_FILE "${CMAKE_CURRENT_SOURCE_DIR}/qmltmp/${QML_FILE}")
        else ()
            set(CHANGED_QML_FILE "${CMAKE_CURRENT_BINARY_DIR}/qmltmp/${QML_FILE}")
        endif ()
        add_custom_command(
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${QML_FILE}"
            OUTPUT "${CHANGED_QML_FILE}"
            COMMAND
                "${PERL_BIN}" ARGS -p -e "s|QtQuick.Controls.Material|${OVERRIDE}|g;" -e
                "s|Material.accent(?!\\:)|palette.accent|g;" -e "s|.*Material\\..*||g;"
                "${CMAKE_CURRENT_SOURCE_DIR}/${QML_FILE}" > "${CHANGED_QML_FILE}"
            COMMENT "Changing Qt Quick Controls style to ${QUICK_GUI_CONTROLS_STYLE}"
            VERBATIM)
        set_source_files_properties("${CHANGED_QML_FILE}" PROPERTIES QT_RESOURCE_ALIAS "${QML_FILE}")
        if ("${QML_FILE}" IN_LIST QML_SINGLETON_FILES)
            set_source_files_properties("${CHANGED_QML_FILE}" PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
        endif ()
        list(APPEND CHANGED_QML_FILES "${CHANGED_QML_FILE}")
    endforeach ()

    set_source_files_properties(${${QML_FILES_VARIABLE}} PROPERTIES HEADER_FILE_ONLY TRUE)
    source_group("Unprocessed QML" FILES ${${QML_FILES_VARIABLE}})
    source_group("Generated QML (DO NOT EDIT)" FILES ${CHANGED_QML_FILES})

    set(UNPROCESSED_QML_FILES
        "${UNPROCESSED_QML_FILES};${${QML_FILES_VARIABLE}}"
        PARENT_SCOPE)
    set("${QML_FILES_VARIABLE}"
        "${CHANGED_QML_FILES}"
        PARENT_SCOPE)
endfunction ()

# allow linking against the concrete Qt Quick Controls 2 module for the selected style
macro (qt_utilities_configure_qt_quick_controls DEFAULT_STYLE RELEVANT_SOURCES)
    message(
        WARNING "The CMake macro qt_utilities_configure_qt_quick_controls() is used. ${QT_QUICK_STYLE_EXPERIMENTAL_NOTICE}")
    if (QUICK_GUI_CONTROLS_STYLE)
        set(QUICK_GUI_CONTROLS_SELECTED_STYLE "${QUICK_GUI_CONTROLS_STYLE}")
    else ()
        set(QUICK_GUI_CONTROLS_SELECTED_STYLE "${DEFAULT_STYLE}")
    endif ()
    if (QUICK_GUI_CONTROLS_SELECTED_STYLE STREQUAL "dynamic")
        set(QUICK_GUI_CONTROLS_COMPILE_TIME_STYLE "")
    else ()
        set(QUICK_GUI_CONTROLS_COMPILE_TIME_STYLE "${QUICK_GUI_CONTROLS_STYLE}")
    endif ()
    if (QUICK_GUI_CONTROLS_SELECTED_STYLE STREQUAL "dynamic" OR QUICK_GUI_CONTROLS_SELECTED_STYLE STREQUAL "FluentWinUI3")
        list(APPEND ADDITIONAL_QT_MODULES "QuickControls2")
    else ()
        list(APPEND ADDITIONAL_QT_MODULES "QuickControls2${QUICK_GUI_CONTROLS_SELECTED_STYLE}")
    endif ()
    if (QUICK_GUI_CONTROLS_COMPILE_TIME_STYLE AND RELEVANT_SOURCES)
        set_property(
            SOURCE ${RELEVANT_SOURCES}
            APPEND
            PROPERTY COMPILE_DEFINITIONS
                     ${META_PROJECT_VARNAME_UPPER}_QUICK_GUI_CONTROLS_STYLE="${QUICK_GUI_CONTROLS_COMPILE_TIME_STYLE}")
    endif ()
endmacro ()

# set platform-specific GUI-type
if (WIN32)
    # set "GUI-type" to WIN32 to hide console under Windows
    set(GUI_TYPE WIN32)
    set(BUILD_CLI_WRAPPER_DEFAULT ON)
    # add option to use console application with detached console instead of creating a GUI app
    option(USE_DETACHED_CONSOLE "use console app with detached console instead of creating a GUI app" OFF)
    if (USE_DETACHED_CONSOLE)
        unset(GUI_TYPE)
        set(BUILD_CLI_WRAPPER_DEFAULT OFF)
        set(${META_PROJECT_VARNAME_UPPER}_WINDOWS_CONSOLE_ALLOCATION_POLICY
            "detached"
            CACHE STRING "sets the console allocation policy for ${META_PROJECT_NAME}" FORCE)
    endif ()
    # add option for building CLI-wrapper
    option(BUILD_CLI_WRAPPER "whether to build a CLI wrapper" "${BUILD_CLI_WRAPPER_DEFAULT}")
elseif (APPLE)
    # make the GUI application a "bundle" under MacOSX
    set(GUI_TYPE MACOSX_BUNDLE)
endif ()

# add source files required by both GUI variants
list(APPEND SRC_FILES ${GUI_SRC_FILES})
list(APPEND ADDITIONAL_HEADER_FILES ${GUI_HEADER_FILES})

# add option for enabling/disabling static Qt plugins
option(SVG_SUPPORT "whether to link against the SVG image format plugin (only relevant when using static Qt)" ON)
option(SVG_ICON_SUPPORT "whether to link against the SVG icon engine (only relevant when using static Qt)" ON)
set(IMAGE_FORMAT_SUPPORT
    "Gif;ICO;Jpeg"
    CACHE STRING "specifies the image format plugins to link against (only relevant when using static Qt)")

# always enable the Svg module under Android
if (ANDROID)
    list(APPEND ADDITIONAL_QT_MODULES Svg)
endif ()
