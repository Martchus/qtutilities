cmake_minimum_required(VERSION 3.3.0 FATAL_ERROR)

# applies Qt specific configuration notes: For GUI applications, QtGuiConfig must be included before. This module must always
# be included before AppTarget/LibraryTarget.

# ensure generated sources are processed by AUTOMOC and AUTOUIC
if (POLICY CMP0071)
    cmake_policy(SET CMP0071 NEW)
endif ()

# verify inclusion order
if (NOT BASIC_PROJECT_CONFIG_DONE)
    message(FATAL_ERROR "Before including the QtConfig module, the BasicConfig module must be included.")
endif ()
if (QT_CONFIGURED)
    message(FATAL_ERROR "The QtConfig module can not be included when Qt usage has already been configured.")
endif ()
if (TARGET_CONFIG_DONE)
    message(FATAL_ERROR "Can not include QtConfig module when targets are already configured.")
endif ()

# include required modules
include(ListToString)
include(TemplateFinder)
include(QtLinkage)

# add the Core module as it is always required and also add additional Qt/KF modules which must have been specified before if
# required note: The Gui/Widgets/Quick modules should be added by including QtGuiConfig.
set(QT_REPOS ${ADDITIONAL_QT_REPOS} base)
set(QT_MODULES ${ADDITIONAL_QT_MODULES} Core)
set(KF_MODULES ${ADDITIONAL_KF_MODULES})

# disable auto-inclusion of QML plugins because these seem to pull in unwanted dependencies like PostgreSQL (required as of
# Qt 6)
set(QT_SKIP_AUTO_QML_PLUGIN_INCLUSION ON)

# disable deprecated features
option(DISABLE_DEPRECATED_QT_FEATURES "specifies whether deprecated Qt features should be disabled" OFF)
if (DISABLE_DEPRECATED_QT_FEATURES)
    list(APPEND META_PRIVATE_COMPILE_DEFINITIONS QT_DEPRECATED_WARNINGS QT_DISABLE_DEPRECATED_BEFORE=0x060000)
endif ()

# allow specifying a custom directory for Qt plugins
set(QT_PLUGIN_DIR
    ""
    CACHE STRING "specifies the directory to install Qt plugins")

# check whether D-Bus interfaces need to be processed
if (DBUS_FILES)
    message(STATUS "Project has D-Bus interface declarations which will be processed.")
    # the D-Bus Qt module is required
    list(APPEND QT_MODULES DBus)
endif ()

# remove duplicates
list(REMOVE_DUPLICATES QT_REPOS)
list(REMOVE_DUPLICATES QT_MODULES)
if (IMPORTED_QT_MODULES)
    list(REMOVE_DUPLICATES IMPORTED_QT_MODULES)
endif ()
if (KF_MODULES)
    list(REMOVE_DUPLICATES KF_MODULES)
endif ()
if (IMPORTED_KF_MODULES)
    list(REMOVE_DUPLICATES IMPORTED_KF_MODULES)
endif ()

# find and use the required Qt/KF modules
set(QT_PACKAGE_PREFIX
    "Qt5"
    CACHE STRING "specifies the prefix for Qt packages")
string(TOLOWER "${QT_PACKAGE_PREFIX}" QT_PACKAGE_PREFIX_LOWER)
set(QT_LINGUIST_TOOLS_PACKAGE "${QT_PACKAGE_PREFIX}LinguistTools")
set(QT_QMAKE_TARGET "${QT_PACKAGE_PREFIX}::qmake")
foreach (MODULE ${QT_MODULES})
    unset(MODULE_OPTIONS)
    if ("${MODULE}" IN_LIST META_PUBLIC_QT_MODULES)
        list(APPEND MODULE_OPTIONS VISIBILITY PUBLIC)
    endif ()
    use_qt_module(PREFIX "${QT_PACKAGE_PREFIX}" MODULE "${MODULE}" ${MODULE_OPTIONS})
endforeach ()
set(KF_PACKAGE_PREFIX
    "KF5"
    CACHE STRING "specifies the prefix for KDE Frameworks packages")
foreach (MODULE ${KF_MODULES})
    unset(MODULE_OPTIONS)
    if ("${MODULE}" IN_LIST META_PUBLIC_KF_MODULES)
        list(APPEND MODULE_OPTIONS VISIBILITY PUBLIC)
    endif ()
    use_qt_module(PREFIX "${KF_PACKAGE_PREFIX}" MODULE "${MODULE}" ${MODULE_OPTIONS})
endforeach ()

# hacks for using static Qt 5 via "StaticQt5" prefix: find regular Qt5Core module as well so Qt version is defined and use
# regular Qt5LinguistTools module and regular qmake target
if (QT_PACKAGE_PREFIX STREQUAL "StaticQt5")
    find_package(Qt5Core)
    set(QT_LINGUIST_TOOLS_PACKAGE Qt5LinguistTools)
    set(QT_QMAKE_TARGET Qt5::qmake)
endif ()

# find transitively required Qt/KF modules
foreach (MODULE ${IMPORTED_QT_MODULES})
    if (NOT "${QT_MODULE}" IN_LIST QT_MODULES)
        find_package("${QT_PACKAGE_PREFIX}${MODULE}" REQUIRED)
    endif ()
endforeach ()
foreach (MODULE ${IMPORTED_KF_MODULES})
    if (NOT "${KF_MODULE}" IN_LIST KF_MODULES)
        find_package("${KF_PACKAGE_PREFIX}${MODULE}" REQUIRED)
    endif ()
endforeach ()

# enable TLS support by default when using Qt Network (relevant for linking against static plugin)
if (NOT DEFINED TLS_SUPPORT)
    if (Network IN_LIST QT_MODULES OR Network IN_LIST IMPORTED_QT_MODULES)
        set(TLS_SUPPORT ON)
    else ()
        set(TLS_SUPPORT OFF)
    endif ()
endif ()

# built-in platform, imageformat and iconengine plugins when linking statically against Qt
if (TARGET "${QT_PACKAGE_PREFIX}::Core")
    get_target_property(QT_TARGET_TYPE "${QT_PACKAGE_PREFIX}::Core" TYPE)
endif ()
if (STATIC_LINKAGE OR QT_TARGET_TYPE STREQUAL STATIC_LIBRARY)
    if (META_PROJECT_IS_APPLICATION)
        set(QT_PLUGINS_LIBRARIES_VARIABLE PRIVATE_LIBRARIES)
    else ()
        set(QT_PLUGINS_LIBRARIES_VARIABLE QT_TEST_LIBRARIES)
    endif ()
    message(
        STATUS
            "Linking ${META_PROJECT_NAME} (${QT_PLUGINS_LIBRARIES_VARIABLE}) against Qt plugins because static linkage is enabled or a static Qt build is used."
    )

    if (Gui IN_LIST QT_MODULES
        OR Widgets IN_LIST QT_MODULES
        OR Quick IN_LIST QT_MODULES)
        if (WIN32 AND TARGET "${QT_PACKAGE_PREFIX}::QWindowsIntegrationPlugin")
            use_qt_module(
                LIBRARIES_VARIABLE
                "${QT_PLUGINS_LIBRARIES_VARIABLE}"
                PREFIX
                "${QT_PACKAGE_PREFIX}"
                MODULE
                Gui
                PLUGINS
                WindowsIntegration
                ONLY_PLUGINS)
        elseif (APPLE AND TARGET "${QT_PACKAGE_PREFIX}::QCocoaIntegrationPlugin")
            use_qt_module(
                LIBRARIES_VARIABLE
                "${QT_PLUGINS_LIBRARIES_VARIABLE}"
                PREFIX
                "${QT_PACKAGE_PREFIX}"
                MODULE
                Gui
                PLUGINS
                CocoaIntegration
                ONLY_PLUGINS)
        endif ()
        if (UNIX AND TARGET "${QT_PACKAGE_PREFIX}::QOffscreenIntegrationPlugin")
            use_qt_module(
                LIBRARIES_VARIABLE
                "${QT_PLUGINS_LIBRARIES_VARIABLE}"
                PREFIX
                "${QT_PACKAGE_PREFIX}"
                MODULE
                Gui
                PLUGINS
                OffscreenIntegration
                ONLY_PLUGINS)
        endif ()
        if (TARGET "${QT_PACKAGE_PREFIX}::QXcbIntegrationPlugin")
            use_qt_module(
                LIBRARIES_VARIABLE
                "${QT_PLUGINS_LIBRARIES_VARIABLE}"
                PREFIX
                "${QT_PACKAGE_PREFIX}"
                MODULE
                Gui
                PLUGINS
                XcbIntegration
                ONLY_PLUGINS)
        endif ()
        if (TARGET "${QT_PACKAGE_PREFIX}::QWaylandIntegrationPlugin")
            use_qt_module(
                LIBRARIES_VARIABLE
                "${QT_PLUGINS_LIBRARIES_VARIABLE}"
                PREFIX
                "${QT_PACKAGE_PREFIX}"
                MODULE
                Gui
                PLUGINS
                WaylandIntegration
                ONLY_PLUGINS)
            use_qt_module(
                LIBRARIES_VARIABLE
                "${QT_PLUGINS_LIBRARIES_VARIABLE}"
                PREFIX
                "${QT_PACKAGE_PREFIX}"
                MODULE
                WaylandClient
                PLUGINS
                WaylandXdgShellIntegration
                WaylandWlShellIntegration
                WaylandIviShellIntegration
                WaylandQtShellIntegration
                ONLY_PLUGINS
                PLUGINS_OPTIONAL)
        endif ()
    endif ()

    # ensure a TLS plugin is built-in when available and when creating an app using Qt Network - required since Qt 6.2.0
    # which "pluginized" TLS support
    if (TLS_SUPPORT)
        set(KNOWN_TLS_PLUGINS ${META_TLS_PLUGINS} SchannelBackend TlsBackendOpenSSL)
        set(USED_TLS_PLUGINS)
        foreach (TLS_PLUGIN ${KNOWN_TLS_PLUGINS})
            if (TARGET "${QT_PACKAGE_PREFIX}::Q${TLS_PLUGIN}Plugin")
                use_qt_module(
                    LIBRARIES_VARIABLE
                    "${QT_PLUGINS_LIBRARIES_VARIABLE}"
                    PREFIX
                    "${QT_PACKAGE_PREFIX}"
                    MODULE
                    Network
                    PLUGINS
                    ${TLS_PLUGIN}
                    ONLY_PLUGINS)
                list(APPEND USED_TLS_PLUGINS "${TLS_PLUGIN}")
                break() # one plugin is sufficient
            endif ()
        endforeach ()

        # allow importing TLS plugins via qtconfig.h
        if (USED_TLS_PLUGINS)
            list_to_string(" " "\\\n    Q_IMPORT_PLUGIN(Q" ")" "${USED_TLS_PLUGINS}" USED_TLS_PLUGINS_ARRAY)
        endif ()
    endif ()

    # ensure all available widget style plugins are built-in when creating a Qt Widgets application - required since Qt 5.10
    # because the styles have been "pluginized" (see commit 4f3249f)
    set(KNOWN_WIDGET_STYLE_PLUGINS WindowsVistaStyle MacStyle AndroidStyle)
    set(USED_WIDGET_STYLE_PLUGINS)
    if (Widgets IN_LIST QT_MODULES)
        foreach (WIDGET_STYLE_PLUGIN ${KNOWN_WIDGET_STYLE_PLUGINS})
            if (TARGET "${QT_PACKAGE_PREFIX}::Q${WIDGET_STYLE_PLUGIN}Plugin")
                use_qt_module(
                    LIBRARIES_VARIABLE
                    "${QT_PLUGINS_LIBRARIES_VARIABLE}"
                    PREFIX
                    "${QT_PACKAGE_PREFIX}"
                    MODULE
                    Widgets
                    PLUGINS
                    ${WIDGET_STYLE_PLUGIN}
                    ONLY_PLUGINS)
                list(APPEND USED_WIDGET_STYLE_PLUGINS "${WIDGET_STYLE_PLUGIN}")
            endif ()
        endforeach ()

        # allow importing widget style plugins via qtconfig.h
        if (USED_WIDGET_STYLE_PLUGINS)
            list_to_string(" " "\\\n    Q_IMPORT_PLUGIN(Q" "Plugin)" "${USED_WIDGET_STYLE_PLUGINS}"
                           WIDGET_STYLE_PLUGINS_ARRAY)
        endif ()
    endif ()

    # ensure image format plugins (beside SVG) are built-in if configured
    if (IMAGE_FORMAT_SUPPORT)
        foreach (IMAGE_FORMAT ${IMAGE_FORMAT_SUPPORT})
            if (IMAGE_FORMAT EQUAL "Svg")
                # the image format plugin of the Qt Svg module is handled separately
                set(SVG_SUPPORT ON)
                list(REMOVE_ITEM IMAGE_FORMAT_SUPPORT Svg)
            else ()
                use_qt_module(
                    LIBRARIES_VARIABLE
                    "${QT_PLUGINS_LIBRARIES_VARIABLE}"
                    PREFIX
                    "${QT_PACKAGE_PREFIX}"
                    MODULE
                    Gui
                    PLUGINS
                    ${IMAGE_FORMAT}
                    ONLY_PLUGINS)
            endif ()
        endforeach ()

        # allow importing image format plugins via qtconfig.h
        list_to_string(" " "\\\n    Q_IMPORT_PLUGIN(Q" "Plugin)" "${IMAGE_FORMAT_SUPPORT}" IMAGE_FORMAT_SUPPORT_ARRAY)
    endif ()

    # ensure SVG plugins are built-in if configured
    if ((SVG_SUPPORT OR SVG_ICON_SUPPORT) AND NOT Svg IN_LIST QT_MODULES)
        use_qt_module(LIBRARIES_VARIABLE "${QT_PLUGINS_LIBRARIES_VARIABLE}" PREFIX "${QT_PACKAGE_PREFIX}" MODULE Svg)
    endif ()
    if (SVG_SUPPORT)
        use_qt_module(
            LIBRARIES_VARIABLE
            "${QT_PLUGINS_LIBRARIES_VARIABLE}"
            PREFIX
            "${QT_PACKAGE_PREFIX}"
            MODULE
            Svg
            PLUGINS
            Svg
            ONLY_PLUGINS)
    endif ()
    if (SVG_ICON_SUPPORT)
        use_qt_module(
            LIBRARIES_VARIABLE
            "${QT_PLUGINS_LIBRARIES_VARIABLE}"
            PREFIX
            "${QT_PACKAGE_PREFIX}"
            MODULE
            Svg
            PLUGINS
            SvgIcon
            ONLY_PLUGINS)
    endif ()
endif ()

# option for built-in translations
option(BUILTIN_TRANSLATIONS "enables/disables built-in translations when building applications and libraries" OFF)
option(BUILTIN_TRANSLATIONS_OF_QT "enables/disables built-in translations of Qt itself when building applications"
       "${BUILTIN_TRANSLATIONS}")

# determine relevant Qt translation files
set(QT_TRANSLATION_FILES)
set(QT_TRANSLATION_SEARCH_PATHS)
query_qmake_variable_path(QT_INSTALL_TRANSLATIONS)
if (QT_INSTALL_TRANSLATIONS)
    list(APPEND QT_TRANSLATION_SEARCH_PATHS "${QT_INSTALL_TRANSLATIONS}")
endif ()
if (CMAKE_FIND_ROOT_PATH)
    foreach (ROOT_PATH ${CMAKE_FIND_ROOT_PATH})
        list(APPEND QT_TRANSLATION_SEARCH_PATHS "${ROOT_PATH}/${CMAKE_INSTALL_DATAROOTDIR}/qt/translations"
             "${ROOT_PATH}/${CMAKE_INSTALL_DATAROOTDIR}/${QT_PACKAGE_PREFIX_LOWER}/translations")
    endforeach ()
endif ()
list(
    APPEND
    QT_TRANSLATION_SEARCH_PATHS
    "${CMAKE_INSTALL_FULL_DATAROOTDIR}/qt/translations"
    "${CMAKE_INSTALL_FULL_DATAROOTDIR}/${QT_PACKAGE_PREFIX_LOWER}/translations"
    "/usr/${CMAKE_INSTALL_DATAROOTDIR}/qt/translations"
    "/usr/${CMAKE_INSTALL_DATAROOTDIR}/${QT_PACKAGE_PREFIX_LOWER}/translations")
list(REMOVE_DUPLICATES QT_TRANSLATION_SEARCH_PATHS)
set(QT_TRANSLATIONS_FOUND NO)
foreach (QT_TRANSLATION_PATH ${QT_TRANSLATION_SEARCH_PATHS})
    if (NOT IS_DIRECTORY "${QT_TRANSLATION_PATH}")
        continue()
    endif ()
    foreach (QT_REPO ${QT_REPOS})
        file(GLOB QT_QM_FILES "${QT_TRANSLATION_PATH}/qt${QT_REPO}_*.qm")
        if (NOT QT_QM_FILES)
            continue()
        endif ()
        # add files to list of built-in translations but only if that configuration is enabled and if we're building the
        # final application
        if (BUILTIN_TRANSLATIONS_OF_QT AND "${META_PROJECT_TYPE}" STREQUAL "application")
            file(COPY ${QT_QM_FILES} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
            list(APPEND EXTERNAL_QM_FILES ${QT_QM_FILES})
        endif ()
        list(APPEND QT_TRANSLATION_FILES "qt${QT_REPO}")
    endforeach ()
    set(QT_TRANSLATIONS_FOUND YES)
    break()
endforeach ()

# make list of Qt translation files even if translations are not found at build time
if (NOT QT_TRANSLATIONS_FOUND)
    foreach (QT_REPO ${QT_REPOS})
        list(APPEND QT_TRANSLATION_FILES "qt${QT_REPO}")
    endforeach ()
endif ()

# emit warning if no Qt translations found but built-in translations are enabled
if (BUILTIN_TRANSLATIONS AND NOT QT_TRANSLATION_FILES)
    message(
        WARNING
            "Unable to find translations of Qt itself so Qt's translation files will not be built-in. Be sure Qt translations (https://code.qt.io/cgit/qt/qttranslations.git) are installed. Was looking under: ${QT_TRANSLATION_SEARCH_PATHS}"
    )
endif ()

# make relevant Qt translations available as array via qtconfig.h
if (QT_TRANSLATION_FILES)
    list_to_string("," " \\\n    QStringLiteral(\"" "\")" "${QT_TRANSLATION_FILES}" QT_TRANSLATION_FILES_ARRAY)
endif ()

# enable lrelease and add install target for localization
option(ENABLE_QT_TRANSLATIONS "specifies whether Qt translations should be updated/released" ON)
if (ENABLE_QT_TRANSLATIONS AND TS_FILES)
    message(STATUS "Project has translations which will be released.")
    set(APP_SPECIFIC_QT_TRANSLATIONS_AVAILABLE YES)

    # require the LinguistTools module (not adding it to QT_MODULES because we don't link against it)
    find_package("${QT_LINGUIST_TOOLS_PACKAGE}")
    if (NOT "${${QT_LINGUIST_TOOLS_PACKAGE}_FOUND}" AND QT_HOST_PATH)
        # find the module within the host path when set (required for cross compilation with Qt 6 as the module is absent in
        # the target install tree)
        find_package("${QT_LINGUIST_TOOLS_PACKAGE}" PATHS "${QT_HOST_PATH}" "${QT_HOST_PATH}/lib/cmake"
                     NO_CMAKE_FIND_ROOT_PATH NO_DEFAULT_PATH)
    endif ()
    if (NOT "${${QT_LINGUIST_TOOLS_PACKAGE}_FOUND}")
        message(
            FATAL_ERROR
                "Qt translations are enabled but the CMake module \"${QT_LINGUIST_TOOLS_PACKAGE}\" could not be found.")
    endif ()

    if (NOT COMMAND qt_create_translation)
        macro (qt_create_translation)
            qt5_create_translation(${ARGV})
        endmacro ()
    endif ()

    set(LUPDATE_OPTIONS
        ""
        CACHE STRING "specifies options passed to lupdate")

    # adds the translations and a target for it
    qt_create_translation(
        QM_FILES
        ${HEADER_FILES}
        ${SRC_FILES}
        ${WIDGETS_HEADER_FILES}
        ${WIDGETS_SRC_FILES}
        ${WIDGETS_UI_FILES}
        ${QML_HEADER_FILES}
        ${QML_SRC_FILES}
        ${QML_RES_FILES}
        ${EXCLUDED_FILES}
        ${TS_FILES}
        OPTIONS
        ${LUPDATE_OPTIONS})
    add_custom_target(${META_PROJECT_NAME}_translations DEPENDS ${QM_FILES})
    if (NOT TARGET translations)
        add_custom_target(translations DEPENDS ${META_PROJECT_NAME}_translations)
    else ()
        add_dependencies(translations ${META_PROJECT_NAME}_translations)
    endif ()

    # add install target for translations
    if (NOT META_NO_INSTALL_TARGETS
        AND ENABLE_INSTALL_TARGETS
        AND NOT BUILTIN_TRANSLATIONS)
        install(
            FILES ${QM_FILES}
            DESTINATION "${META_DATA_DIR}/translations"
            COMPONENT localization)
        if (NOT TARGET install-localization)
            set(LOCALIZATION_TARGET "install-localization")
            add_custom_target(${LOCALIZATION_TARGET} COMMAND "${CMAKE_COMMAND}" -DCMAKE_INSTALL_COMPONENT=localization -P
                                                             "${CMAKE_BINARY_DIR}/cmake_install.cmake")
            add_dependencies(${LOCALIZATION_TARGET} ${META_PROJECT_NAME}_translations)
        endif ()
    endif ()

    list(APPEND APP_SPECIFIC_QT_TRANSLATION_FILES "${META_PROJECT_NAME}")
endif ()

# make application specific translation available as array via qtconfig.h (even if this project has no translations, there
# might be some from dependencies)
if (APP_SPECIFIC_QT_TRANSLATION_FILES)
    list_to_string("," " \\\n    QStringLiteral(\"" "\")" "${APP_SPECIFIC_QT_TRANSLATION_FILES}"
                   APP_SPECIFIC_QT_TRANSLATION_FILES_ARRAY)
else ()
    set(APP_SPECIFIC_QT_TRANSLATIONS_AVAILABLE NO)
endif ()

# built-in translations
if (BUILTIN_TRANSLATIONS AND (QM_FILES OR EXTERNAL_QM_FILES))
    # write a qrc file for the qm files and add it to the resource files
    set(TRANSLATIONS_QRC_FILE_NAME "${META_PROJECT_VARNAME_LOWER}_translations.qrc")
    set(TRANSLATIONS_QRC_FILE "${CMAKE_CURRENT_BINARY_DIR}/${TRANSLATIONS_QRC_FILE_NAME}")
    file(WRITE "${TRANSLATIONS_QRC_FILE}" "<RCC><qresource prefix=\"/translations\">")
    foreach (QM_FILE ${QM_FILES} ${EXTERNAL_QM_FILES})
        get_filename_component(QM_FILE_NAME "${QM_FILE}" NAME)
        file(APPEND "${TRANSLATIONS_QRC_FILE}" "<file>${QM_FILE_NAME}</file>")
    endforeach ()
    file(APPEND "${TRANSLATIONS_QRC_FILE}" "</qresource></RCC>")
    list(APPEND RES_FILES "${TRANSLATIONS_QRC_FILE}")
    list(APPEND AUTOGEN_DEPS ${QM_FILES} ${EXTERNAL_QM_FILES})
    list(APPEND BUILTIN_TRANSLATION_FILES "${TRANSLATIONS_QRC_FILE_NAME}")
endif ()

# generate DBus interfaces
if (DBUS_FILES)
    if (NOT COMMAND qt_add_dbus_interfaces)
        macro (qt_add_dbus_interfaces)
            qt5_add_dbus_interfaces(${ARGV})
        endmacro ()
    endif ()
    qt_add_dbus_interfaces(GENERATED_DBUS_FILES ${DBUS_FILES})
endif ()

# add icons to be built-in
if (REQUIRED_ICONS)
    set(BUILTIN_ICON_THEMES
        ""
        CACHE STRING "specifies icon themes to be built-in")
    option(BUILTIN_ICON_THEMES_IN_LIBRARIES "specifies whether icon themes should also be built-in when building libraries"
           OFF)
    if (BUILTIN_ICON_THEMES AND (BUILTIN_ICON_THEMES_IN_LIBRARIES OR (NOT "${META_PROJECT_TYPE}" STREQUAL "library")))
        set(ICON_THEME_FILES)
        set(ICON_SEARCH_PATHS)
        if (CMAKE_FIND_ROOT_PATH)
            foreach (ROOT_PATH ${CMAKE_FIND_ROOT_PATH})
                list(APPEND ICON_SEARCH_PATHS "${ROOT_PATH}/${CMAKE_INSTALL_DATAROOTDIR}/icons")
            endforeach ()
        endif ()
        list(APPEND ICON_SEARCH_PATHS "${CMAKE_INSTALL_FULL_DATAROOTDIR}/icons")
        list(APPEND ICON_SEARCH_PATHS "/usr/${CMAKE_INSTALL_DATAROOTDIR}/icons") # find icons from regular prefix when cross-
                                                                                 # compiling
        list(REMOVE_DUPLICATES ICON_SEARCH_PATHS)
        set(BUILTIN_ICONS_DIR "${CMAKE_CURRENT_BINARY_DIR}/icons")
        set(DEFAULT_THEME_INDEX_FILE "${BUILTIN_ICONS_DIR}/default/index.theme")
        file(REMOVE_RECURSE "${BUILTIN_ICONS_DIR}")
        file(MAKE_DIRECTORY "${BUILTIN_ICONS_DIR}")
        foreach (ICON_THEME ${BUILTIN_ICON_THEMES})
            string(REGEX MATCHALL "([^:]+|[^:]+$)" ICON_THEME_PARTS "${ICON_THEME}")
            list(LENGTH ICON_THEME_PARTS ICON_THEME_PARTS_LENGTH)
            if ("${ICON_THEME_PARTS_LENGTH}" STREQUAL 2)
                list(GET ICON_THEME_PARTS 0 ICON_THEME)
                list(GET ICON_THEME_PARTS 1 NEW_ICON_THEME_NAME)
            else ()
                set(NEW_ICON_THEME_NAME "${ICON_THEME}")
            endif ()
            foreach (ICON_SEARCH_PATH ${ICON_SEARCH_PATHS})
                set(ICON_THEME_PATH "${ICON_SEARCH_PATH}/${ICON_THEME}")
                set(NEW_ICON_THEME_PATH "${ICON_SEARCH_PATH}/${ICON_THEME}")
                if (IS_DIRECTORY "${ICON_THEME_PATH}")
                    message(
                        STATUS
                            "The specified icon theme \"${ICON_THEME}\" has been located under \"${ICON_THEME_PATH}\" and will be built-in."
                    )
                    # find index files
                    if (NOT ICON_THEME STREQUAL FALLBACK_ICON_THEME)
                        file(
                            GLOB GLOBBED_ICON_THEME_INDEX_FILES
                            LIST_DIRECTORIES false
                            "${ICON_THEME_PATH}/index.theme" "${ICON_THEME_PATH}/icon-theme.cache")
                    else ()
                        # only index.theme required when icons are provided as fallback anyways
                        file(
                            GLOB GLOBBED_ICON_THEME_INDEX_FILES
                            LIST_DIRECTORIES false
                            "${ICON_THEME_PATH}/index.theme")
                    endif ()
                    # make the first specified built-in the default theme
                    if (NOT EXISTS "${DEFAULT_THEME_INDEX_FILE}")
                        file(MAKE_DIRECTORY "${BUILTIN_ICONS_DIR}/default")
                        file(WRITE "${DEFAULT_THEME_INDEX_FILE}" "[Icon Theme]\nInherits=${NEW_ICON_THEME_NAME}")
                        list(APPEND ICON_THEME_FILES "<file>default/index.theme</file>")
                    endif ()
                    # find required icons, except the icon theme is provided as fallback anyways
                    if (NOT ICON_THEME STREQUAL FALLBACK_ICON_THEME)
                        set(GLOB_PATTERNS)
                        foreach (REQUIRED_ICON ${REQUIRED_ICONS})
                            list(APPEND GLOB_PATTERNS "${ICON_THEME_PATH}/${REQUIRED_ICON}"
                                 "${ICON_THEME_PATH}/${REQUIRED_ICON}.*" "${ICON_THEME_PATH}/*/${REQUIRED_ICON}"
                                 "${ICON_THEME_PATH}/*/${REQUIRED_ICON}.*")
                        endforeach ()
                        file(
                            GLOB_RECURSE GLOBBED_ICON_THEME_FILES
                            LIST_DIRECTORIES false
                            ${GLOB_PATTERNS})
                    else ()
                        message(
                            STATUS
                                "Icon files for specified theme \"${ICON_THEME}\" are skipped because these are provided as fallback anyways."
                        )
                        set(GLOBBED_ICON_THEME_FILES)
                    endif ()
                    # make temporary copy of required icons and create resource list for Qt
                    foreach (ICON_THEME_FILE ${GLOBBED_ICON_THEME_INDEX_FILES} ${GLOBBED_ICON_THEME_FILES})
                        # resolve symlinks
                        if (IS_SYMLINK "${ICON_THEME_FILE}")
                            string(REGEX REPLACE "^${ICON_SEARCH_PATH}/" "" ICON_THEME_FILE_RELATIVE_PATH
                                                 "${ICON_THEME_FILE}")
                            string(REGEX REPLACE "(^[^/\\]+)" "${NEW_ICON_THEME_NAME}" NEW_ICON_THEME_FILE_RELATIVE_PATH
                                                 "${ICON_THEME_FILE_RELATIVE_PATH}")
                            set(ICON_THEME_FILE_ALIAS " alias=\"${NEW_ICON_THEME_FILE_RELATIVE_PATH}\"")
                            get_filename_component(ICON_THEME_FILE "${ICON_THEME_FILE}" REALPATH)
                        else ()
                            unset(ICON_THEME_FILE_ALIAS)
                        endif ()
                        string(REGEX REPLACE "^${ICON_SEARCH_PATH}/" "" ICON_THEME_FILE_RELATIVE_PATH "${ICON_THEME_FILE}")
                        string(REGEX REPLACE "(^[^/\\]+)" "${NEW_ICON_THEME_NAME}" NEW_ICON_THEME_FILE_RELATIVE_PATH
                                             "${ICON_THEME_FILE_RELATIVE_PATH}")
                        get_filename_component(ICON_THEME_FILE_DIR "${ICON_THEME_FILE_RELATIVE_PATH}" DIRECTORY)
                        string(REGEX REPLACE "(^[^/\\]+)" "${NEW_ICON_THEME_NAME}" NEW_ICON_THEME_FILE_DIR
                                             "${ICON_THEME_FILE_DIR}")
                        file(MAKE_DIRECTORY "${BUILTIN_ICONS_DIR}/${NEW_ICON_THEME_FILE_DIR}")
                        file(COPY "${ICON_THEME_FILE}" DESTINATION "${BUILTIN_ICONS_DIR}/${NEW_ICON_THEME_FILE_DIR}")
                        list(APPEND ICON_THEME_FILES
                             "<file${ICON_THEME_FILE_ALIAS}>${NEW_ICON_THEME_FILE_RELATIVE_PATH}</file>")
                    endforeach ()
                    break()
                endif ()
                unset(ICON_THEME_PATH)
            endforeach ()
            if (NOT ICON_THEME_PATH)
                message(FATAL_ERROR "The specified icon theme \"${ICON_THEME}\" could not be found.")
            endif ()
        endforeach ()
        set(BUILTIN_ICON_THEMES_QRC_FILE "${CMAKE_CURRENT_BINARY_DIR}/icons/${META_PROJECT_NAME}_builtinicons.qrc")
        list(REMOVE_DUPLICATES ICON_THEME_FILES)
        string(CONCAT BUILTIN_ICON_THEMES_QRC_FILE_CONTENT "<RCC><qresource prefix=\"/icons\">" ${ICON_THEME_FILES}
                      "</qresource></RCC>")
        file(WRITE "${BUILTIN_ICON_THEMES_QRC_FILE}" "${BUILTIN_ICON_THEMES_QRC_FILE_CONTENT}")
        list(APPEND RES_FILES "${BUILTIN_ICON_THEMES_QRC_FILE}")
    endif ()
endif ()

# export Qt resources from specified RES_FILES
foreach (RES_FILE ${RES_FILES})
    get_filename_component(RES_EXT ${RES_FILE} EXT)
    if (RES_EXT STREQUAL ".qrc")
        get_filename_component(RES_NAME ${RES_FILE} NAME_WE)
        list(APPEND QT_RESOURCES "${RES_NAME}")
    endif ()
endforeach ()

# export Qt resources required by static libraries the static library depends on
if (STATIC_LIBRARIES_QT_RESOURCES)
    list(REMOVE_DUPLICATES STATIC_LIBRARIES_QT_RESOURCES)
    list(APPEND QT_RESOURCES ${STATIC_LIBRARIES_QT_RESOURCES})
endif ()

# enable Qt resources required by libraries the application depends on
if (QT_RESOURCES)
    list(REMOVE_DUPLICATES QT_RESOURCES)

    # make enabling resources of static dependencies available via qtconfig.h
    unset(ENABLE_QT_RESOURCES_OF_STATIC_DEPENDENCIES)
    foreach (QT_RESOURCE ${STATIC_LIBRARIES_QT_RESOURCES})
        set(ENABLE_QT_RESOURCES_OF_STATIC_DEPENDENCIES
            "${ENABLE_QT_RESOURCES_OF_STATIC_DEPENDENCIES} \\\n    struct initializer_${QT_RESOURCE} { \\\n        initializer_${QT_RESOURCE}() { Q_INIT_RESOURCE(${QT_RESOURCE}); } \\\n        ~initializer_${QT_RESOURCE}() { Q_CLEANUP_RESOURCE(${QT_RESOURCE}); } \\\n    } dummy_${QT_RESOURCE};"
        )
    endforeach ()
endif ()

# enable moc, uic and rcc by default for all targets
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
if (WIDGETS_UI_FILES AND WIDGETS_GUI)
    set(CMAKE_AUTOUIC ON)
    if (INSTALL_UI_HEADER)
        # also add install for header files generated by uic
        foreach (UI_FILE WIDGETS_UI_FILES)
            get_filename_component(UI_NAME "${UI_FILE}" NAME_WE)
            install(
                FILES "${CMAKE_CURRENT_BINARY_DIR}/ui_${UI_NAME}.h"
                DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${META_PROJECT_NAME}/ui"
                COMPONENT ui-header)
        endforeach ()
    endif ()
endif ()

# add configuration header for Qt-specific configuration
find_template_file("qtconfig.h" QT_UTILITIES QT_CONFIG_H_TEMPLATE_FILE)
configure_file("${QT_CONFIG_H_TEMPLATE_FILE}" "${CMAKE_CURRENT_BINARY_DIR}/resources/qtconfig.h")

set(QT_CONFIGURED YES)
