cmake_minimum_required(VERSION 3.3.0 FATAL_ERROR)

# adds a target to create an Android APK with the help of androiddeployqt if target platform is Android

if (NOT ANDROID)
    return()
endif ()
if (NOT BASIC_PROJECT_CONFIG_DONE OR NOT QT_CONFIGURED)
    message(FATAL_ERROR "Before including the ApkConfig module, the AppTarget module and QtConfig module must be included.")
endif ()
if (ANDROID_APK_CONFIGURED)
    message(FATAL_ERROR "The AndroidApk module mustn't be included twice.")
endif ()

if (NOT EXISTS "${CMAKE_ANDROID_SDK}")
    message(FATAL_ERROR "CMAKE_ANDROID_SDK must contain the path of the Android SDK.")
endif ()
if (NOT EXISTS "${CMAKE_ANDROID_NDK}")
    message(FATAL_ERROR "CMAKE_ANDROID_NDK must contain the path of the Android NDK.")
endif ()

# determine some variables
if (NOT META_ANDROID_PACKAGE_NAME)
    message(FATAL_ERROR "Attempt to load AndroidApk.cmake without having set ANDROID_PACKAGE_NAME.")
endif ()
set(ANDROID_APK_APPLICATION_ID_SUFFIX
    ""
    CACHE STRING "suffix for Android APK ID, use e.g. \".debug\" to produce a co-installable debug version")
set(ANDROID_APK_APPLICATION_LABEL
    "${META_APP_NAME}"
    CACHE STRING "user visible name for the APK")

# find "android" subdirectory in the source directory and check for AndroidManifest.xml
set(ANDROID_APK_SUBDIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
if (NOT IS_DIRECTORY "${ANDROID_APK_SUBDIR}")
    message(
        FATAL_ERROR
            "The directory containing Android-specific files is expected to be \"${ANDROID_APK_SUBDIR}\" but doesn't exist.")
endif ()
set(ANDROID_APK_MANIFEST_PATH "${ANDROID_APK_SUBDIR}/AndroidManifest.xml")
if (NOT EXISTS "${ANDROID_APK_MANIFEST_PATH}" AND NOT EXISTS "${ANDROID_APK_MANIFEST_PATH}.in")
    message(FATAL_ERROR "The Android manifest doesn't exist at \"${ANDROID_APK_SUBDIR}/AndroidManifest.xml\".")
endif ()

# link Android package directory to binary directory evaluating templates on top-level
set(ANDROID_PACKAGE_SOURCE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/android-package-source-directory")
file(MAKE_DIRECTORY "${ANDROID_PACKAGE_SOURCE_DIRECTORY}")
file(
    GLOB_RECURSE ANDROID_APK_FILES
    LIST_DIRECTORIES false
    RELATIVE "${ANDROID_APK_SUBDIR}"
    "${ANDROID_APK_SUBDIR}/*")
set(ANDROID_SOURCE_DIRECTORY_FILES)
foreach (ANDROID_APK_FILE ${ANDROID_APK_FILES})
    if (IS_DIRECTORY "${ANDROID_APK_FILE}")
        continue()
    endif ()
    get_filename_component(ANDROID_APK_FILE_DIR ${ANDROID_APK_FILE} DIRECTORY)
    if (ANDROID_APK_FILE_DIR)
        set(ANDROID_APK_FILE_DESTINATION "${ANDROID_PACKAGE_SOURCE_DIRECTORY}/${ANDROID_APK_FILE_DIR}")
        file(MAKE_DIRECTORY "${ANDROID_APK_FILE_DESTINATION}")
    else ()
        set(ANDROID_APK_FILE_DESTINATION "${ANDROID_PACKAGE_SOURCE_DIRECTORY}")
    endif ()
    get_filename_component(ANDROID_APK_FILE_EXT ${ANDROID_APK_FILE} LAST_EXT)
    if (ANDROID_APK_FILE_EXT STREQUAL ".in")
        string(LENGTH "${ANDROID_APK_FILE}" ANDROID_APK_FILE_LENGTH)
        math(EXPR ANDROID_APK_FILE_LENGTH "${ANDROID_APK_FILE_LENGTH} - 3")
        string(SUBSTRING "${ANDROID_APK_FILE}" 0 ${ANDROID_APK_FILE_LENGTH} ANDROID_APK_FILE_NAME)
        configure_file("${ANDROID_APK_SUBDIR}/${ANDROID_APK_FILE}"
                       "${ANDROID_PACKAGE_SOURCE_DIRECTORY}/${ANDROID_APK_FILE_NAME}")
        set(ANDROID_APK_FILE "${ANDROID_APK_FILE_NAME}")
    else ()
        file(COPY "${ANDROID_APK_SUBDIR}/${ANDROID_APK_FILE}" DESTINATION "${ANDROID_APK_FILE_DESTINATION}")
    endif ()
    # FIXME: tracking these deps doesn't work
endforeach ()

# make subdirectory to store build artefacts for APK
set(ANDROID_APK_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/apk")
file(MAKE_DIRECTORY "${ANDROID_APK_BUILD_DIR}")

# find Qt
get_filename_component(ANDROID_APK_QT_CMAKE_DIR "${Qt5Core_DIR}" DIRECTORY)
get_filename_component(ANDROID_APK_QT_LIBRARY_DIR "${ANDROID_APK_QT_CMAKE_DIR}" DIRECTORY)
get_filename_component(ANDROID_APK_QT_INSTALL_PREFIX "${ANDROID_APK_QT_LIBRARY_DIR}" DIRECTORY)

# deduce Android toolchain prefix and version from "CMAKE_CXX_ANDROID_TOOLCHAIN_PREFIX"
message(STATUS "Android toolchain prefix: ${CMAKE_CXX_ANDROID_TOOLCHAIN_PREFIX}")
if (CMAKE_CXX_ANDROID_TOOLCHAIN_PREFIX MATCHES ".*/(.+)-")
    set(ANDROID_APK_TOOL_PREFIX "${CMAKE_MATCH_1}")
else ()
    set(ANDROID_APK_TOOL_PREFIX "${CMAKE_CXX_ANDROID_TOOLCHAIN_PREFIX}")
endif ()

set(ANDROID_APK_TOOLCHAIN_VERSION
    ""
    CACHE STRING "toolchain version for making APK file")
if (NOT ANDROID_APK_TOOLCHAIN_VERSION)
    if (CMAKE_CXX_ANDROID_TOOLCHAIN_PREFIX MATCHES ".*/.+-linux-android-([^/]+)/.*")
        set(ANDROID_APK_TOOLCHAIN_VERSION "${CMAKE_MATCH_1}")
    elseif (CMAKE_CXX_ANDROID_TOOLCHAIN_PREFIX MATCHES ".*/.+-linux-androideabi-([^/]+)/.*")
        set(ANDROID_APK_TOOLCHAIN_VERSION "${CMAKE_MATCH_1}")
    elseif (NOT CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION MATCHES "clang.*")
        set(ANDROID_APK_TOOLCHAIN_VERSION "${CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION}")
    else ()
        message(FATAL_ERROR "Unable to detect the toolchain version for making the APK.")
    endif ()
endif ()

# determine Android build tools version note: Assuming the build tools are installed under "${CMAKE_ANDROID_SDK}/build-tools"
file(
    GLOB ANDROID_APK_BUILD_TOOLS_VERSIONS
    LIST_DIRECTORIES TRUE
    RELATIVE "${CMAKE_ANDROID_SDK}/build-tools"
    "${CMAKE_ANDROID_SDK}/build-tools/*")
if (NOT ANDROID_APK_BUILD_TOOLS_VERSIONS)
    message(FATAL_ERROR "No build-tools present under \"${CMAKE_ANDROID_SDK}/build-tools\".")
endif ()
list(GET ANDROID_APK_BUILD_TOOLS_VERSIONS 0 ANDROID_APK_BUILD_TOOLS_VERSION)

# deduce path of C++ standard library from "CMAKE_CXX_STANDARD_LIBRARIES" note: Assuming CMAKE_CXX_STANDARD_LIBRARIES
# contains a paths or quotes paths with flags appended.
set(ANDROID_APK_CXX_STANDARD_LIBRARY
    ""
    CACHE STRING "path to standard library for making APK file")
if (NOT ANDROID_APK_CXX_STANDARD_LIBRARY)
    foreach (CMAKE_CXX_STANDARD_LIBRARY ${CMAKE_CXX_STANDARD_LIBRARIES})
        if (EXISTS "${CMAKE_CXX_STANDARD_LIBRARY}")
            set(ANDROID_APK_CXX_STANDARD_LIBRARY "${CMAKE_CXX_STANDARD_LIBRARY}")
            break()
        elseif (CMAKE_CXX_STANDARD_LIBRARY MATCHES "\"(.*)\".*")
            if (EXISTS "${CMAKE_MATCH_1}")
                set(ANDROID_APK_CXX_STANDARD_LIBRARY "${CMAKE_MATCH_1}")
                break()
            endif ()
        endif ()
        message(WARNING "${CMAKE_CXX_STANDARD_LIBRARY} from CMAKE_CXX_STANDARD_LIBRARIES does not exist.")
    endforeach ()
endif ()
if (NOT ANDROID_APK_CXX_STANDARD_LIBRARY)
    message(FATAL_ERROR "CMAKE_CXX_STANDARD_LIBRARIES does not contain path to standard library.")
endif ()

# determine extra prefix dirs
set(ANDROID_APK_BINARY_DIRS "${RUNTIME_LIBRARY_PATH}")
if (NOT CMAKE_CURRENT_BINARY_DIR IN_LIST ANDROID_APK_BINARY_DIRS)
    list(APPEND ANDROID_APK_BINARY_DIRS "${CMAKE_CURRENT_BINARY_DIR}")
endif ()
set(ANDROID_APK_BINARY_DIRS_DEPENDS "")
foreach (PATH ${ANDROID_APK_BINARY_DIRS})
    # symlink "lib" subdirectory so androiddeployqt finds the library in the runtime path when specified via
    # "extraPrefixDirs"
    list(APPEND ANDROID_APK_BINARY_DIRS_DEPENDS "${PATH}/lib")
    add_custom_command(OUTPUT "${PATH}/lib" COMMAND "${CMAKE_COMMAND}" -E create_symlink "${PATH}" "${PATH}/lib")
endforeach ()
include(ListToString)
list_to_string("" "\n        \"" "\"," "${ANDROID_APK_BINARY_DIRS}" ANDROID_APK_BINARY_DIRS)

# find dependencies note: androiddeployqt seems to find only Qt libraries and plugins but misses other target_link_libraries
set(ANDROID_APK_ADDITIONAL_LIBS
    ""
    CACHE STRING "additional libraries to be bundled into the Android APK")
set(ANDROID_APK_EXTRA_LIBS "${ANDROID_APK_ADDITIONAL_LIBS}")
function (add_android_apk_extra_libs TARGET_NAME)
    get_target_property(ANDROID_APK_EXTRA_LIBS_FOR_TARGET ${TARGET_NAME} LINK_LIBRARIES)
    if (NOT ANDROID_APK_EXTRA_LIBS_FOR_TARGET)
        return()
    endif ()
    foreach (LIBRARY ${ANDROID_APK_EXTRA_LIBS_FOR_TARGET})
        if (TARGET "${LIBRARY}")
            list(APPEND ANDROID_APK_EXTRA_LIBS "\$<TARGET_FILE:${LIBRARY}>")
            add_android_apk_extra_libs(${LIBRARY})
        elseif (EXISTS "${LIBRARY}")
            list(APPEND ANDROID_APK_EXTRA_LIBS "${LIBRARY}")
        else ()
            message(
                WARNING
                    "Unable to find library \"${LIBRARY}\" required by target \"${TARGET_NAME}\". It won't be added to the APK."
            )
        endif ()
    endforeach ()
    set(ANDROID_APK_EXTRA_LIBS
        "${ANDROID_APK_EXTRA_LIBS}"
        PARENT_SCOPE)
endfunction ()
add_android_apk_extra_libs("${META_TARGET_NAME}")
list_to_string("," "" "" "${ANDROID_APK_EXTRA_LIBS}" ANDROID_APK_EXTRA_LIBS)

# query certain qmake variables
foreach (QMAKE_VARIABLE QT_INSTALL_QML QT_INSTALL_PLUGINS QT_INSTALL_IMPORTS)
    query_qmake_variable(${QMAKE_VARIABLE})
endforeach ()

# define function to get a list of (existing) paths
function (compose_dirs_for_android_apk)
    # parse arguments
    set(OPTIONAL_ARGS)
    set(ONE_VALUE_ARGS OUTPUT_VARIABLE)
    set(MULTI_VALUE_ARGS POSSIBLE_DIRS)
    cmake_parse_arguments(ARGS "${OPTIONAL_ARGS}" "${ONE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN})

    list(REMOVE_DUPLICATES ARGS_POSSIBLE_DIRS)
    unset(DIRS)
    foreach (POSSIBLE_DIR ${ARGS_POSSIBLE_DIRS})
        if (IS_DIRECTORY "${POSSIBLE_DIR}")
            list(APPEND DIRS "${POSSIBLE_DIR}")
        endif ()
    endforeach ()

    list_to_string("," "" "" "${DIRS}" DIRS)
    set("${ARGS_OUTPUT_VARIABLE}"
        "${DIRS}"
        PARENT_SCOPE)
endfunction ()

# pick QML import paths from install prefix
compose_dirs_for_android_apk(
    OUTPUT_VARIABLE
    ANDROID_APK_QML_IMPORT_DIRS
    POSSIBLE_DIRS
    "${QT_INSTALL_IMPORTS}"
    "${QT_INSTALL_QML}"
    "${CMAKE_INSTALL_FULL_LIBDIR}${SELECTED_LIB_SUFFIX}/qt/imports"
    "${CMAKE_INSTALL_FULL_LIBDIR}${SELECTED_LIB_SUFFIX}/imports"
    "${CMAKE_INSTALL_FULL_LIBDIR}${SELECTED_LIB_SUFFIX}/qt/qml"
    "${CMAKE_INSTALL_FULL_LIBDIR}${SELECTED_LIB_SUFFIX}/qml")
if (NOT ANDROID_APK_QML_IMPORT_DIRS)
    message(WARNING "Unable to find QML import directories for making the APK.")
endif ()

# pick extra plugins from install prefix
compose_dirs_for_android_apk(
    OUTPUT_VARIABLE ANDROID_APK_EXTRA_PLUGIN_DIRS POSSIBLE_DIRS "${QT_INSTALL_PLUGINS}"
    "${CMAKE_INSTALL_FULL_LIBDIR}${SELECTED_LIB_SUFFIX}/qt/plugins"
    "${CMAKE_INSTALL_FULL_LIBDIR}${SELECTED_LIB_SUFFIX}/plugins")
if (NOT ANDROID_APK_EXTRA_PLUGIN_DIRS)
    message(WARNING "Unable to find extra plugin directories for making the APK.")
endif ()

# find template for deployment JSON
find_template_file("android-deployment.json" QT_UTILITIES ANDROID_DEPLOYMENT_JSON_TEMPLATE_FILE)
set(ANDROID_DEPLOYMENT_JSON_FILE "${CMAKE_CURRENT_BINARY_DIR}/android-deployment.json")
configure_file("${ANDROID_DEPLOYMENT_JSON_TEMPLATE_FILE}" "${ANDROID_DEPLOYMENT_JSON_FILE}.configured")
file(
    GENERATE
    OUTPUT "${ANDROID_DEPLOYMENT_JSON_FILE}"
    INPUT "${ANDROID_DEPLOYMENT_JSON_FILE}.configured")

# pass make arguments
if (CMAKE_GENERATOR STREQUAL "Unix Makefiles")
    set(MAKE_ARGUMENTS "\\$(ARGS)")
endif ()

# add rules to make APK
option(ANDROID_APK_FORCE_DEBUG "specifies whether a debug APK should be created even when building in release mode" OFF)
if (Qt5Core_VERSION VERSION_LESS 5.12.0)
    set(ANDROID_APK_FILE_DIRECTORY "")
else ()
    if (ANDROID_APK_FORCE_DEBUG OR CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(ANDROID_APK_FILE_DIRECTORY "debug/")
    else ()
        set(ANDROID_APK_FILE_DIRECTORY "release/")
    endif ()
endif ()
if (ANDROID_APK_FORCE_DEBUG OR CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(ANDROID_APK_FILE_PATH "${ANDROID_APK_BUILD_DIR}/build/outputs/apk/${ANDROID_APK_FILE_DIRECTORY}apk-debug.apk")
    set(ANDROID_APK_ADDITIONAL_ANDROIDDEPOYQT_OPTIONS)
else ()
    set(ANDROID_APK_ADDITIONAL_ANDROIDDEPOYQT_OPTIONS --release)
    set(ANDROID_APK_KEYSTORE_URL
        ""
        CACHE STRING "keystore URL for signing the Android APK")
    set(ANDROID_APK_KEYSTORE_ALIAS
        ""
        CACHE STRING "keystore alias for signing the Android APK")
    set(ANDROID_APK_KEYSTORE_PASSWORD
        ""
        CACHE STRING "keystore password for signing the Android APK")
    set(ANDROID_APK_KEYSTORE_KEY_PASSWORD
        ""
        CACHE STRING "keystore key password for signing the Android APK")

    if (ANDROID_APK_KEYSTORE_URL AND ANDROID_APK_KEYSTORE_ALIAS)
        set(ANDROID_APK_FILE_PATH
            "${ANDROID_APK_BUILD_DIR}/build/outputs/apk/${ANDROID_APK_FILE_DIRECTORY}apk-release-signed.apk")
        list(APPEND ANDROID_APK_ADDITIONAL_ANDROIDDEPOYQT_OPTIONS --sign "${ANDROID_APK_KEYSTORE_URL}"
             "${ANDROID_APK_KEYSTORE_ALIAS}")
        if (ANDROID_APK_KEYSTORE_PASSWORD)
            list(APPEND ANDROID_APK_ADDITIONAL_ANDROIDDEPOYQT_OPTIONS --storepass "${ANDROID_APK_KEYSTORE_PASSWORD}")
        endif ()
        if (ANDROID_APK_KEYSTORE_KEY_PASSWORD)
            list(APPEND ANDROID_APK_ADDITIONAL_ANDROIDDEPOYQT_OPTIONS --keypass "${ANDROID_APK_KEYSTORE_KEY_PASSWORD}")
        endif ()
    else ()
        set(ANDROID_APK_FILE_PATH
            "${ANDROID_APK_BUILD_DIR}/build/outputs/apk/${ANDROID_APK_FILE_DIRECTORY}apk-release-unsigned.apk")
        message(WARNING "Set ANDROID_APK_KEYSTORE_URL/ANDROID_APK_KEYSTORE_ALIAS to sign Android APK release.")
    endif ()

endif ()
set(ANDROID_APK_BINARY_PATH "${ANDROID_APK_BUILD_DIR}/libs/${CMAKE_ANDROID_ARCH_ABI}/lib${META_TARGET_NAME}.so")
add_custom_command(
    OUTPUT "${ANDROID_APK_BINARY_PATH}"
    COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:${META_TARGET_NAME}>" "${ANDROID_APK_BINARY_PATH}"
    COMMENT "Preparing build dir for Android APK"
    DEPENDS "${META_TARGET_NAME}"
    COMMAND_EXPAND_LISTS VERBATIM)
add_custom_command(
    OUTPUT "${ANDROID_APK_FILE_PATH}"
    COMMAND rm -r "${ANDROID_APK_BUILD_DIR}/assets/--Added-by-androiddeployqt--/lib" || true
    COMMAND
        $<TARGET_FILE_DIR:Qt5::qmake>/androiddeployqt --gradle --input "${ANDROID_DEPLOYMENT_JSON_FILE}" --output
        "${ANDROID_APK_BUILD_DIR}" --deployment bundled ${MAKE_ARGUMENTS} --verbose
        ${ANDROID_APK_ADDITIONAL_ANDROIDDEPOYQT_OPTIONS}
    WORKING_DIRECTORY "${ANDROID_APK_BUILD_DIR}"
    COMMENT "Creating Android APK ${ANDROID_APK_FILE_PATH} using androiddeployqt"
    DEPENDS
        "${ANDROID_DEPLOYMENT_JSON_FILE};${ANDROID_APK_BINARY_PATH};${ANDROID_SOURCE_DIRECTORY_FILES};${ANDROID_APK_BINARY_DIRS_DEPENDS}"
    COMMAND_EXPAND_LISTS VERBATIM)
add_custom_target(
    "${META_TARGET_NAME}_apk"
    COMMENT "Android APK"
    DEPENDS "${ANDROID_APK_FILE_PATH}")
if (NOT TARGET apk)
    add_custom_target(apk)
endif ()
add_dependencies(apk "${META_TARGET_NAME}_apk")

# add install target for APK
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(ANDROID_APK_FINAL_NAME "${META_ID}-debug-${META_APP_VERSION}.apk")
else ()
    set(ANDROID_APK_FINAL_NAME "${META_ID}-${META_APP_VERSION}.apk")
endif ()
install(
    FILES "${ANDROID_APK_FILE_PATH}"
    DESTINATION "share/apk"
    RENAME "${ANDROID_APK_FINAL_NAME}"
    COMPONENT apk)
add_custom_target("${META_TARGET_NAME}_install_apk" COMMAND "${CMAKE_COMMAND}" -DCMAKE_INSTALL_COMPONENT=apk -P
                                                            "${CMAKE_BINARY_DIR}/cmake_install.cmake")
add_dependencies("${META_TARGET_NAME}_install_apk" "${META_TARGET_NAME}_apk")
if (NOT TARGET install-apk)
    add_custom_target(install-apk)
endif ()
add_dependencies(install-apk "${META_TARGET_NAME}_install_apk")

# add deploy target for APK
find_program(ADB_BIN adb)
add_custom_target(
    "${META_TARGET_NAME}_deploy_apk"
    COMMAND "${ADB_BIN}" install -r "${ANDROID_APK_FILE_PATH}"
    COMMENT "Deploying Android APK"
    DEPENDS "${ANDROID_APK_FILE_PATH}")

set(ANDROID_APK_CONFIGURED YES)
