# applies Qt specific configuration
# for GUI applications, QtGuiAppConfig must be included before
# after including this module, AppTarget must be included

# add the Core module as it is always required
# also add additional Qt/KF modules which must have been specified before if required
# the Gui/Widgets/Quick modules should be added by including QtGuiAppConfig
set(QT_REPOS base ${ADDITIONAL_QT_REPOS})
set(QT_MODULES Core ${ADDITIONAL_QT_MODULES})
set(KF_MODULES ${ADDITIONAL_KF_MODULES})

include(QtLinkage)

# check whether D-Bus interfaces need to be processed
if(DBUS_FILES)
    message(STATUS "Project has D-Bus interface declarations which will be processed.")
    # the D-Bus Qt module is required
    list(APPEND QT_MODULES DBus)
endif()

if(SVG_SUPPORT OR ((USE_STATIC_QT_BUILD AND (WIDGETS_GUI OR QUICK_GUI)) AND SVG_ICON_SUPPORT))
    list(APPEND QT_MODULES Svg)
    list(APPEND QT_REPOS svg)
endif()

list(REMOVE_DUPLICATES QT_REPOS)
list(REMOVE_DUPLICATES QT_MODULES)
if(KF_MODULES)
    list(REMOVE_DUPLICATES KF_MODULES)
endif()

# actually find the required Qt/KF modules
foreach(QT_MODULE ${QT_MODULES})
    # using those helpers allows using static Qt 5 build
    find_qt5_module(${QT_MODULE} REQUIRED)
    use_qt5_module(${QT_MODULE} REQUIRED)
endforeach()
foreach(KF_MODULE ${KF_MODULES})
    # only shared KF5 modules supported
    find_package(KF5${KF_MODULE} REQUIRED)
    set(KF5_${KF_MODULE}_DYNAMIC_LIB KF5::${KF_MODULE})
    link_against_library(KF5_${KF_MODULE} "AUTO_LINKAGE" REQUIRED)
endforeach()

# tune for static build
if(USE_STATIC_QT5_CORE AND (WIDGETS_GUI OR QUICK_GUI))
    # include plugins statically
    if(WIN32)
        list(APPEND LIBRARIES Qt5::static::QWindowsIntegrationPlugin)
        list(APPEND STATIC_LIBRARIES Qt5::static::QWindowsIntegrationPlugin)
    endif()
    if(SVG_ICON_SUPPORT)
        list(APPEND LIBRARIES Qt5::QSvgPlugin)
        list(APPEND STATIC_LIBRARIES Qt5::QSvgPlugin)
    endif()

    # workaround for missing compile definition GRAPHITE2_STATIC
    #set_property(TARGET Qt5::Gui APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS GRAPHITE2_STATIC)

endif()

# option for built-in translations
option(BUILTIN_TRANSLATIONS "enables/disables built-in translations" OFF)

# determine relevant Qt translation files
set(QT_TRANSLATION_FILES)
set(QT_TRANSLATION_SEARCH_PATHS)
if(CMAKE_FIND_ROOT_PATH)
    list(APPEND QT_TRANSLATION_SEARCH_PATHS "${CMAKE_FIND_ROOT_PATH}/share/qt/translations")
endif()
list(APPEND QT_TRANSLATION_SEARCH_PATHS "${CMAKE_INSTALL_PREFIX}/share/qt/translations")
list(APPEND QT_TRANSLATION_SEARCH_PATHS "/usr/share/qt/translations")
foreach(QT_TRANSLATION_PATH ${QT_TRANSLATION_SEARCH_PATHS})
    if(IS_DIRECTORY "${QT_TRANSLATION_PATH}")
        foreach(QT_REPO ${QT_REPOS})
            file(GLOB QT_QM_FILES "${QT_TRANSLATION_PATH}/qt${QT_REPO}_*.qm")
            if(QT_QM_FILES)
                # add file to list of built-in translations (if that configuration is enabled)
                if(BUILTIN_TRANSLATIONS)
                    file(COPY ${QT_QM_FILES} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
                    list(APPEND QM_FILES ${QT_QM_FILES})
                endif()
                list(APPEND QT_TRANSLATION_FILES "qt${QT_REPO}")
            endif()
        endforeach()
        break()
    endif()
endforeach()

# make relevant Qt translations available as array via config.h
include(ListToString)
list_to_string(", " "QStringLiteral(\"" "\")" "${QT_TRANSLATION_FILES}" QT_TRANSLATION_FILES_ARRAY)

# enable lrelease and add install target for localization
if(TS_FILES)
    message(STATUS "Project has translations which will be released.")

    # the LinguistTools module is required
    # (but not add it to QT_MODULES because we don't link against it)
    find_package(Qt5LinguistTools REQUIRED)

    # adds the translations and a target for it
    qt5_create_translation(QM_FILES
        ${HEADER_FILES} ${SRC_FILES}
        ${WIDGETS_HEADER_FILES} ${WIDGETS_SRC_FILES} ${WIDGETS_UI_FILES}
        ${QML_HEADER_FILES} ${QML_SRC_FILES} ${QML_RES_FILES}
        ${TS_FILES}
    )
    add_custom_target(${META_PROJECT_NAME}_translations ALL DEPENDS ${QM_FILES})

    # add installs and install target for translations
    install(FILES ${QM_FILES}
        DESTINATION share/${META_PROJECT_NAME}/translations
        COMPONENT localization
    )
    if(NOT TARGET install-localization)
        set(LOCALIZATION_TARGET "install-localization")
        add_custom_target(${LOCALIZATION_TARGET}
            DEPENDS ${META_PROJECT_NAME}_translations
            COMMAND "${CMAKE_COMMAND}" -DCMAKE_INSTALL_COMPONENT=localization -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
        )
    endif()

    # built-in translations
    if(BUILTIN_TRANSLATIONS)
        # write a qrc file for the qm files and add it to the resource files
        set(TRANSLATIONS_QRC_FILE "${CMAKE_CURRENT_BINARY_DIR}/translations.qrc")
        file(WRITE "${TRANSLATIONS_QRC_FILE}" "<RCC><qresource prefix=\"/translations\">")
        foreach(QM_FILE ${QM_FILES})
            get_filename_component(QM_FILE_NAME "${QM_FILE}" NAME)
            file(APPEND "${TRANSLATIONS_QRC_FILE}" "<file>${QM_FILE_NAME}</file>")
        endforeach()
        file(APPEND "${TRANSLATIONS_QRC_FILE}" "</qresource></RCC>")
        list(APPEND RES_FILES "${TRANSLATIONS_QRC_FILE}")
    endif()
endif()

# generate DBus interfaces
if(DBUS_FILES)
    qt5_add_dbus_interfaces(SRC_FILES ${DBUS_FILES})
endif()

# add icons to be built-in
if(REQUIRED_ICONS)
    set(BUILTIN_ICON_THEMES "" CACHE STRING "specifies icon themes to be built-in")
    if(BUILTIN_ICON_THEMES)
        set(ICON_THEME_FILES)
        set(ICON_SEARCH_PATHS)
        if(CMAKE_FIND_ROOT_PATH)
            # find icons from the regular prefix when cross-compiling
            list(APPEND ICON_SEARCH_PATHS "${CMAKE_FIND_ROOT_PATH}/share/icons")
        endif()
        list(APPEND ICON_SEARCH_PATHS "${CMAKE_INSTALL_PREFIX}/share/icons")
        list(APPEND ICON_SEARCH_PATHS "/usr/share/icons")
        set(BUILDIN_ICONS_DIR "${CMAKE_CURRENT_BINARY_DIR}/icons")
        set(DEFAULT_THEME_INDEX_FILE "${BUILDIN_ICONS_DIR}/default/index.theme")
        file(REMOVE_RECURSE "${BUILDIN_ICONS_DIR}")
        file(MAKE_DIRECTORY "${BUILDIN_ICONS_DIR}")
        foreach(ICON_THEME ${BUILTIN_ICON_THEMES})
            string(REGEX MATCHALL "([^:]+|[^:]+$)" ICON_THEME_PARTS "${ICON_THEME}")
            list(LENGTH ICON_THEME_PARTS ICON_THEME_PARTS_LENGTH)
            if("${ICON_THEME_PARTS_LENGTH}" STREQUAL 2)
                list(GET ICON_THEME_PARTS 0 ICON_THEME)
                list(GET ICON_THEME_PARTS 1 NEW_ICON_THEME_NAME)
            else()
                set(NEW_ICON_THEME_NAME "${ICON_THEME}")
            endif()
            foreach(ICON_SEARCH_PATH ${ICON_SEARCH_PATHS})
                set(ICON_THEME_PATH "${ICON_SEARCH_PATH}/${ICON_THEME}")
                set(NEW_ICON_THEME_PATH "${ICON_SEARCH_PATH}/${ICON_THEME}")
                if(IS_DIRECTORY "${ICON_THEME_PATH}")
                    # find index files
                    file(GLOB GLOBBED_ICON_THEME_INDEX_FILES LIST_DIRECTORIES false "${ICON_THEME_PATH}/index.theme" "${ICON_THEME_PATH}/icon-theme.cache")
                    # make the first specified built-in the default theme
                    if(NOT EXISTS "${DEFAULT_THEME_INDEX_FILE}")
                        file(MAKE_DIRECTORY "${BUILDIN_ICONS_DIR}/default")
                        file(WRITE "${DEFAULT_THEME_INDEX_FILE}" "[Icon Theme]\nInherits=${NEW_ICON_THEME_NAME}")
                        list(APPEND ICON_THEME_FILES "<file>default/index.theme</file>")
                    endif()
                    # find required icons
                    set(GLOB_PATTERNS)
                    foreach(REQUIRED_ICON ${REQUIRED_ICONS})
                        list(APPEND GLOB_PATTERNS
                            "${ICON_THEME_PATH}/${REQUIRED_ICON}"
                            "${ICON_THEME_PATH}/${REQUIRED_ICON}.*"
                            "${ICON_THEME_PATH}/*/${REQUIRED_ICON}"
                            "${ICON_THEME_PATH}/*/${REQUIRED_ICON}.*"
                        )
                    endforeach()
                    file(GLOB_RECURSE GLOBBED_ICON_THEME_FILES LIST_DIRECTORIES false ${GLOB_PATTERNS})
                    # make temporary copy of required icons and create resource list for Qt
                    foreach(ICON_THEME_FILE ${GLOBBED_ICON_THEME_INDEX_FILES} ${GLOBBED_ICON_THEME_FILES})
                        #message(STATUS "relpath: ${ICON_THEME_FILE_RELATIVE_PATH}, new relpath: ${NEW_ICON_THEME_FILE_RELATIVE_PATH}")
                        # resolve symlinks (use
                        if(IS_SYMLINK "${ICON_THEME_FILE}")
                            string(REGEX REPLACE "^${ICON_SEARCH_PATH}/" "" ICON_THEME_FILE_RELATIVE_PATH "${ICON_THEME_FILE}")
                            string(REGEX REPLACE "(^[^/\\]+)" "${NEW_ICON_THEME_NAME}" NEW_ICON_THEME_FILE_RELATIVE_PATH "${ICON_THEME_FILE_RELATIVE_PATH}")
                            set(ICON_THEME_FILE_ALIAS " alias=\"${NEW_ICON_THEME_FILE_RELATIVE_PATH}\"")
                            get_filename_component(ICON_THEME_FILE "${ICON_THEME_FILE}" REALPATH)
                        else()
                            unset(ICON_THEME_FILE_ALIAS)
                        endif()
                        string(REGEX REPLACE "^${ICON_SEARCH_PATH}/" "" ICON_THEME_FILE_RELATIVE_PATH "${ICON_THEME_FILE}")
                        string(REGEX REPLACE "(^[^/\\]+)" "${NEW_ICON_THEME_NAME}" NEW_ICON_THEME_FILE_RELATIVE_PATH "${ICON_THEME_FILE_RELATIVE_PATH}")
                        get_filename_component(ICON_THEME_FILE_DIR "${ICON_THEME_FILE_RELATIVE_PATH}" DIRECTORY)
                        string(REGEX REPLACE "(^[^/\\]+)" "${NEW_ICON_THEME_NAME}" NEW_ICON_THEME_FILE_DIR "${ICON_THEME_FILE_DIR}")
                        file(MAKE_DIRECTORY "${BUILDIN_ICONS_DIR}/${NEW_ICON_THEME_FILE_DIR}")
                        file(COPY "${ICON_THEME_FILE}" DESTINATION "${BUILDIN_ICONS_DIR}/${NEW_ICON_THEME_FILE_DIR}")
                        list(APPEND ICON_THEME_FILES "<file${ICON_THEME_FILE_ALIAS}>${NEW_ICON_THEME_FILE_RELATIVE_PATH}</file>")
                    endforeach()
                    break()
                endif()
                unset(ICON_THEME_PATH)
            endforeach()
            if(NOT ICON_THEME_PATH)
                message(FATAL_ERROR "The specified icon theme \"${ICON_THEME}\" coud not be found.")
            endif()
        endforeach()
        set(BUILTIN_ICON_THEMES_QRC_FILE "${CMAKE_CURRENT_BINARY_DIR}/icons/builtinicons.qrc")
        list(REMOVE_DUPLICATES ICON_THEME_FILES)
        string(CONCAT BUILTIN_ICON_THEMES_QRC_FILE_CONTENT "<RCC><qresource prefix=\"/icons\">" ${ICON_THEME_FILES} "</qresource></RCC>")
        file(WRITE "${BUILTIN_ICON_THEMES_QRC_FILE}" "${BUILTIN_ICON_THEMES_QRC_FILE_CONTENT}")
        list(APPEND RES_FILES "${BUILTIN_ICON_THEMES_QRC_FILE}")
    endif()
endif()

# enable moc, uic and rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
if(WIDGETS_UI_FILES AND WIDGETS_GUI)
    set(CMAKE_AUTOUIC ON)
    if(INSTALL_UI_HEADER)
        # also add install for header files generated by uic
        foreach(UI_FILE WIDGETS_UI_FILES)
            get_filename_component(UI_NAME "${UI_FILE}" NAME_WE)
            install(
                FILES "${CMAKE_CURRENT_BINARY_DIR}/ui_${UI_NAME}.h"
                DESTINATION "include/${META_PROJECT_NAME}/ui"
                COMPONENT ui-header
            )
        endforeach()
    endif()
endif()
