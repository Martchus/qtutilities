cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
find_package(Corrosion 0.6 QUIET)
if (NOT Corrosion_FOUND)
    message(STATUS "Skipping icontool, unable to find Corrosion")
    return ()
endif ()

corrosion_import_crate(MANIFEST_PATH Cargo.toml IMPORTED_CRATES ICONTOOL_CRATES)
message(STATUS "Building the following crates for icontool: ${ICONTOOL_CRATES}")

corrosion_set_hostbuild(icontool)
corrosion_install(TARGETS icontool DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}/${META_TARGET_NAME}")

add_test(
    NAME icontool_tests
    COMMAND cargo test
        --manifest-path "${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml"
        --target-dir "${CMAKE_CURRENT_BINARY_DIR}/cargo"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
set_tests_properties(icontool_tests PROPERTIES
    FIXTURES_REQUIRED icontool_built
)
