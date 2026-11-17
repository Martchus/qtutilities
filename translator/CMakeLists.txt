cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
find_package(Corrosion 0.6 QUIET)
if (NOT Corrosion_FOUND)
    message(STATUS "Skipping translator, unable to find Corrosion")
    return ()
endif ()

corrosion_import_crate(MANIFEST_PATH Cargo.toml IMPORTED_CRATES TRANSLATOR_CRATES LOCKED)
message(STATUS "Building the following crates for translator: ${TRANSLATOR_CRATES}")

corrosion_set_hostbuild(translator)
corrosion_install(TARGETS translator DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}/${META_TARGET_NAME}")
