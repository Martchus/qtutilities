cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
find_package(Corrosion 0.6 QUIET)
if (NOT Corrosion_FOUND)
    message(STATUS "Skipping translator, unable to find Corrosion")
    return ()
endif ()

corrosion_import_crate(MANIFEST_PATH Cargo.toml IMPORTED_CRATES TRANSLATOR_CRATES)
message(STATUS "Building the following crates for translator: ${TRANSLATOR_CRATES}")

corrosion_set_hostbuild(translator)
corrosion_install(TARGETS translator DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}/${META_TARGET_NAME}")

add_test(
    NAME translator_tests
    COMMAND cargo test
        --manifest-path "${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml"
        --target-dir "${CMAKE_CURRENT_BINARY_DIR}/cargo"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
set_tests_properties(translator_tests PROPERTIES
    FIXTURES_REQUIRED translator_built
)
