cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
find_package(Corrosion 0.6)
if (NOT Corrosion_FOUND)
    message(WARNING "Skipping icontool, unable to find Corrosion")
    return ()
endif ()

corrosion_import_crate(MANIFEST_PATH Cargo.toml IMPORTED_CRATES ICONTOOL_CRATES)
corrosion_set_hostbuild(icontool)
corrosion_install(TARGETS icontool DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}/${META_TARGET_NAME}")

message(STATUS "Building the following crates for icontool: ${ICONTOOL_CRATES}")
