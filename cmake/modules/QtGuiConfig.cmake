# after including this module, AppConfig must be included

# enable Qt Widgets GUI
set(WIDGETS_GUI "yes" CACHE STRING "enables/disables building the Qt Widgets GUI: yes (default) or no")
if(${WIDGETS_GUI} STREQUAL "yes")
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
elseif(${WIDGETS_GUI} STREQUAL "no")
    message(STATUS "Building WITHOUT Qt Widgets GUI.")
else()
    message(FATAL_ERROR "Specification whether to build with Qt Widgets GUI is invalid (must be either yes or no).")
endif()

# enable Qt Quick GUI
set(QUICK_GUI "yes" CACHE STRING "enables/disables building the Qt Quick GUI: yes (default) or no")
if(${QUICK_GUI} STREQUAL "yes")
    add_definitions(
        -DGUI_QTQUICK
    )
    list(APPEND QML_FILES ${QML_HEADER_FILES} ${QML_SRC_FILES} ${QML_RES_FILES})
    list(APPEND ADDITIONAL_HEADER_FILES ${QML_HEADER_FILES})
    if(QML_FILES)
        list(APPEND ADDITIONAL_QT_MODULES Quick)
        message(STATUS "Building with Qt Quick GUI.")
    else()
        message(STATUS "Qt Quick GUI is not available.")
    endif()
elseif(${QUICK_GUI} STREQUAL "no")
    message(STATUS "Building WITHOUT Qt Quick GUI.")
else()
    message(FATAL_ERROR "Specification whether to build with Qt Quick GUI is invalid (must be either yes or no).")
endif()

# set "GUI-type" to WIN32 to hide console under windows
if(WIN32)
    if(${WIDGETS_GUI} STREQUAL "yes" OR ${QUICK_GUI} STREQUAL "yes")
        list(APPEND QT_MODULES Gui)
        set(GUI_TYPE WIN32)
    endif()
endif()
