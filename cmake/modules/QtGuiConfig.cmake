cmake_minimum_required(VERSION 3.3.0 FATAL_ERROR)

# after including this module, AppConfig must be included

# enable Qt Widgets GUI
if(WIDGETS_GUI)
    add_definitions(
        -DGUI_QTWIDGETS
        -DMODEL_UNDO_SUPPORT
    )
    list(APPEND WIDGETS_FILES ${WIDGETS_HEADER_FILES} ${WIDGETS_SRC_FILES} ${WIDGETS_RES_FILES} ${WIDGETS_UI_FILES})
    list(APPEND ADDITIONAL_HEADER_FILES ${WIDGETS_HEADER_FILES})
    if(WIDGETS_FILES)
        list(APPEND ADDITIONAL_QT_MODULES Widgets)
        message(STATUS "Building with Qt Widgets GUI.")
    else()
        message(STATUS "Qt Widgets GUI is not available.")
    endif()
else()
    message(STATUS "Building WITHOUT Qt Widgets GUI.")
endif()

# enable Qt Quick GUI
if(QUICK_GUI)
    add_definitions(
        -DGUI_QTQUICK
    )
    list(APPEND QML_FILES ${QML_HEADER_FILES} ${QML_SRC_FILES} ${QML_RES_FILES})
    list(APPEND ADDITIONAL_HEADER_FILES ${QML_HEADER_FILES})
    if(QML_FILES)
        list(APPEND ADDITIONAL_QT_MODULES Quick)
        list(APPEND ADDITIONAL_QT_REPOS "declarative")
        message(STATUS "Building with Qt Quick GUI.")
    else()
        message(STATUS "Qt Quick GUI is not available.")
    endif()
else()
    message(STATUS "Building WITHOUT Qt Quick GUI.")
endif()

if(WIDGETS_GUI OR QUICK_GUI)
    list(APPEND QT_MODULES Gui)
    # set "GUI-type" to WIN32 to hide console under windows
    if(WIN32)
        set(GUI_TYPE WIN32)
    endif()
endif()

# add source files requried by both GUI variants
if(WIDGETS_GUI OR QUICK_GUI)
    list(APPEND SRC_FILES ${GUI_SRC_FILES})
    list(APPEND ADDITIONAL_HEADER_FILES ${GUI_HEADER_FILES})
endif()

# add option for enabling/disabling static Qt plugins
if("Svg" IN_LIST ADDITIONAL_QT_MODULES)
    # Qt Svg module is not optional
    set(SVG_SUPPORT ON)
else()
    option(SVG_SUPPORT "enables/disables svg support for Qt GUI" OFF)
endif()
option(SVG_ICON_SUPPORT "enables/disables svg icon support for Qt GUI (only affects static builds where QSvgPlugin will be built-in if enabled)" ON)
