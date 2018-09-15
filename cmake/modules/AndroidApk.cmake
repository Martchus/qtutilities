cmake_minimum_required(VERSION 3.3.0 FATAL_ERROR)

# add a target to create an Android API with the help of androiddeployqt if target platform is Android

if(NOT ANDROID)
    return()
endif()
if(NOT BASIC_PROJECT_CONFIG_DONE OR NOT QT_CONFIGURED)
    message(FATAL_ERROR "Before including the ApkConfig module, the AppTarget module and QtConfig module must be included.")
endif()
if(ANDROID_APK_CONFIGURED)
    message(FATAL_ERROR "The AndroidApk module mustn't be included twice.")
endif()

# find "android" subdirectory in the source directory and check for AndroidManifest.xml
set(ANDROID_APK_SUBDIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
if(NOT IS_DIRECTORY "${ANDROID_APK_SUBDIR}")
    message(FATAL_ERROR "The directory containing Android-specific files is expected to be \"${ANDROID_APK_SUBDIR}\" but doesn't exist.")
endif()
set(ANDROID_APK_MANIFEST_PATH "${ANDROID_APK_SUBDIR}/AndroidManifest.xml")
if(NOT EXISTS "${ANDROID_APK_MANIFEST_PATH}")
    message(FATAL_ERROR "The Android manifest doesn't exist at \"${ANDROID_APK_SUBDIR}/AndroidManifest.xml\".")
endif()

# make subdirectory to store build artefacts for APK
set(ANDROID_APK_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/apk")
file(MAKE_DIRECTORY "${ANDROID_APK_BUILD_DIR}")

# find Qt
get_filename_component(ANDROID_APK_QT_CMAKE_DIR "${Qt5Core_DIR}" DIRECTORY)
get_filename_component(ANDROID_APK_QT_LIBRARY_DIR "${ANDROID_APK_QT_CMAKE_DIR}" DIRECTORY)
get_filename_component(ANDROID_APK_QT_INSTALL_PREFIX "${ANDROID_APK_QT_LIBRARY_DIR}" DIRECTORY)

# deduce Android toolchain prefix from "CMAKE_CXX_ANDROID_TOOLCHAIN_PREFIX"
if(CMAKE_CXX_ANDROID_TOOLCHAIN_PREFIX MATCHES ".*/(.+)-")
    set(ANDROID_APK_TOOL_PREFIX "${CMAKE_MATCH_1}")
else()
    set(ANDROID_APK_TOOL_PREFIX "${CMAKE_CXX_ANDROID_TOOLCHAIN_PREFIX}")
endif()

# determine Android build tools version
# note: Assuming the build tools are installed under "${CMAKE_ANDROID_SDK}/build-tools"
file(GLOB ANDROID_APK_BUILD_TOOLS_VERSIONS
    LIST_DIRECTORIES TRUE
    RELATIVE "${CMAKE_ANDROID_SDK}/build-tools"
    "${CMAKE_ANDROID_SDK}/build-tools/*"
)
if(NOT ANDROID_APK_BUILD_TOOLS_VERSIONS)
    message(FATAL_ERROR "No build-tools present under \"${CMAKE_ANDROID_SDK}/build-tools\".")
endif()
list(GET ANDROID_APK_BUILD_TOOLS_VERSIONS 0 ANDROID_APK_BUILD_TOOLS_VERSION)

# deduce path of C++ standard library from "CMAKE_CXX_STANDARD_LIBRARIES"
# note: Assuming CMAKE_CXX_STANDARD_LIBRARIES contains a paths or quotes paths with flags appended.
foreach(CMAKE_CXX_STANDARD_LIBRARY ${CMAKE_CXX_STANDARD_LIBRARIES})
    if(EXISTS "${CMAKE_CXX_STANDARD_LIBRARY}")
        set(ANDROID_APK_CXX_STANDARD_LIBRARY "${CMAKE_CXX_STANDARD_LIBRARY}")
        break()
    elseif(CMAKE_CXX_STANDARD_LIBRARY MATCHES "\"(.*)\".*")
        if(EXISTS "${CMAKE_MATCH_1}")
            set(ANDROID_APK_CXX_STANDARD_LIBRARY "${CMAKE_MATCH_1}")
            break()
        endif()
    endif()
    message(WARNING "${CMAKE_CXX_STANDARD_LIBRARY} from CMAKE_CXX_STANDARD_LIBRARIES does not exist.")
endforeach()
if(NOT ANDROID_APK_CXX_STANDARD_LIBRARY)
    message(FATAL_ERROR "CMAKE_CXX_STANDARD_LIBRARIES does not contain path to standard library.")
endif()

# determine extra prefix dirs
include(ListToString)
set(ANDROID_APK_BINARY_DIRS "${RUNTIME_LIBRARY_PATH}")
if(NOT CMAKE_CURRENT_BINARY_DIR IN_LIST ANDROID_APK_BINARY_DIRS)
    list(APPEND ANDROID_APK_BINARY_DIRS "${CMAKE_CURRENT_BINARY_DIR}")
endif()
list_to_string("" "\n        \"" "\"," "${ANDROID_APK_BINARY_DIRS}" ANDROID_APK_BINARY_DIRS)

# find dependencies
# note: androiddeployqt seems to find only Qt libraries and plugins but misses other target_link_libraries
set(ANDROID_APK_ADDITIONAL_LIBS "" CACHE STRING "additional libraries to be bundled into the Android APK")
set(ANDROID_APK_EXTRA_LIBS "${ANDROID_APK_ADDITIONAL_LIBS}")
function(add_android_apk_extra_libs TARGET_NAME)
    get_target_property(ANDROID_APK_EXTRA_LIBS_FOR_TARGET ${TARGET_NAME} LINK_LIBRARIES)
    if(NOT ANDROID_APK_EXTRA_LIBS_FOR_TARGET)
        return()
    endif()
    foreach(LIBRARY ${ANDROID_APK_EXTRA_LIBS_FOR_TARGET})
        if(TARGET "${LIBRARY}")
            list(APPEND ANDROID_APK_EXTRA_LIBS "\$<TARGET_FILE:${LIBRARY}>")
            add_android_apk_extra_libs(${LIBRARY})
        elseif(EXISTS "${LIBRARY}")
            list(APPEND ANDROID_APK_EXTRA_LIBS "${LIBRARY}")
        else()
            message(WARNING "Unable to find library \"${LIBRARY}\" required by target \"${TARGET_NAME}\". It won't be added to the APK.")
        endif()
    endforeach()
    set(ANDROID_APK_EXTRA_LIBS "${ANDROID_APK_EXTRA_LIBS}" PARENT_SCOPE)
endfunction()
add_android_apk_extra_libs("${TARGET_PREFIX}${META_PROJECT_NAME}${TARGET_SUFFIX}")
list_to_string("," "" "" "${ANDROID_APK_EXTRA_LIBS}" ANDROID_APK_EXTRA_LIBS)

# find template for deployment JSON
find_template_file("android-deployment.json" QT_UTILITIES ANDROID_DEPLOYMENT_JSON_TEMPLATE_FILE)
set(ANDROID_DEPLOYMENT_JSON_FILE "${CMAKE_CURRENT_BINARY_DIR}/android-deployment.json")
configure_file(
    "${ANDROID_DEPLOYMENT_JSON_TEMPLATE_FILE}"
    "${ANDROID_DEPLOYMENT_JSON_FILE}.configured"
)
file(GENERATE
     OUTPUT "${ANDROID_DEPLOYMENT_JSON_FILE}"
     INPUT "${ANDROID_DEPLOYMENT_JSON_FILE}.configured"
)

# pass make arguments
if (CMAKE_GENERATOR STREQUAL "Unix Makefiles")
    set(MAKE_ARGUMENTS "\\$(ARGS)")
endif()

# add rules to make APK
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(ANDROID_APK_FILE_PATH "${ANDROID_APK_BUILD_DIR}/build/outputs/apk/apk-debug.apk")
    set(ANDROID_APK_ADDITIONAL_ANDROIDDEPOYQT_OPTIONS)
else()
    set(ANDROID_APK_FILE_PATH "${ANDROID_APK_BUILD_DIR}/build/outputs/apk/apk-release-unsigned.apk")
    set(ANDROID_APK_ADDITIONAL_ANDROIDDEPOYQT_OPTIONS --release)
endif()
set(ANDROID_APK_BINARY_PATH "${ANDROID_APK_BUILD_DIR}/libs/${CMAKE_ANDROID_ARCH_ABI}/lib${TARGET_PREFIX}${META_PROJECT_NAME}${TARGET_SUFFIX}.so")
add_custom_command(OUTPUT "${ANDROID_APK_BINARY_PATH}"
    COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:${TARGET_PREFIX}${META_PROJECT_NAME}${TARGET_SUFFIX}>" "${ANDROID_APK_BINARY_PATH}"
    COMMENT "Preparing build dir for Android APK"
    DEPENDS "${TARGET_PREFIX}${META_PROJECT_NAME}${TARGET_SUFFIX}"
    COMMAND_EXPAND_LISTS
    VERBATIM
)
add_custom_command(OUTPUT "${ANDROID_APK_FILE_PATH}"
    COMMAND $<TARGET_FILE_DIR:Qt5::qmake>/androiddeployqt
        --gradle
        --input "${ANDROID_DEPLOYMENT_JSON_FILE}"
        --output "${ANDROID_APK_BUILD_DIR}"
        --deployment bundled ${MAKE_ARGUMENTS}
        ${ANDROID_APK_ADDITIONAL_ANDROIDDEPOYQT_OPTIONS}
    WORKING_DIRECTORY "${ANDROID_APK_BUILD_DIR}"
    COMMENT "Creating Android APK ${ANDROID_APK_FILE_PATH} using androiddeployqt"
    DEPENDS "${ANDROID_DEPLOYMENT_JSON_FILE};${ANDROID_APK_BINARY_PATH}"
    COMMAND_EXPAND_LISTS
    VERBATIM
)
add_custom_target("${TARGET_PREFIX}${META_PROJECT_NAME}${TARGET_SUFFIX}_apk"
    COMMENT "Android APK"
    DEPENDS "${ANDROID_APK_FILE_PATH}"
)
if(NOT TARGET apk)
    add_custom_target(apk)
endif()
add_dependencies(apk "${TARGET_PREFIX}${META_PROJECT_NAME}${TARGET_SUFFIX}_apk")

# add install target for APK
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(ANDROID_APK_FINAL_NAME "${META_ID}-debug-${META_APP_VERSION}.apk")
else()
    set(ANDROID_APK_FINAL_NAME "${META_ID}-${META_APP_VERSION}.apk")
endif()
install(
    FILES "${ANDROID_APK_FILE_PATH}"
    DESTINATION "share/apk"
    RENAME "${ANDROID_APK_FINAL_NAME}"
    COMPONENT apk
)
add_custom_target("${TARGET_PREFIX}${META_PROJECT_NAME}${TARGET_SUFFIX}_install_apk"
    COMMAND "${CMAKE_COMMAND}" -DCMAKE_INSTALL_COMPONENT=apk -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
)
add_dependencies("${TARGET_PREFIX}${META_PROJECT_NAME}${TARGET_SUFFIX}_install_apk" "${TARGET_PREFIX}${META_PROJECT_NAME}${TARGET_SUFFIX}_apk")
if(NOT TARGET install-apk)
    add_custom_target(install-apk)
endif()
add_dependencies(install-apk "${TARGET_PREFIX}${META_PROJECT_NAME}${TARGET_SUFFIX}_install_apk")

# add deploy target for APK
find_program(ADB_BIN adb)
add_custom_target("${TARGET_PREFIX}${META_PROJECT_NAME}${TARGET_SUFFIX}_deploy_apk"
    COMMAND "${ADB_BIN}" install -r "${ANDROID_APK_FILE_PATH}"
    COMMENT "Deploying Android APK"
    DEPENDS "${ANDROID_APK_FILE_PATH}"
)

set(ANDROID_APK_CONFIGURED YES)
