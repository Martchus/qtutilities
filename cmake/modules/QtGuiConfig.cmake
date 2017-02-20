cmake_minimum_required(VERSION 3.3.0 FATAL_ERROR)

if(NOT BASIC_PROJECT_CONFIG_DONE)
    message(FATAL_ERROR "Before including the QtGuiConfig module, the BasicConfig module must be included.")
endif()
if(QT_CONFIGURED)
    message(FATAL_ERROR "The QtGuiConfig module can not be included when Qt usage has already been configured.")
endif()
if(TARGET_CONFIG_DONE)
    message(FATAL_ERROR "Can not include QtGuiConfig module when targets are already configured.")
endif()

# enable Qt Widgets GUI
if(WIDGETS_GUI)
    list(APPEND META_PRIVATE_COMPILE_DEFINITIONS GUI_QTWIDGETS MODEL_UNDO_SUPPORT)
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
    list(APPEND META_PRIVATE_COMPILE_DEFINITIONS GUI_QTQUICK)
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
    # set "GUI-type" to WIN32 to hide console under windows
    if(WIN32)
        set(GUI_TYPE WIN32)
    endif()

    # add source files requried by both GUI variants
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
