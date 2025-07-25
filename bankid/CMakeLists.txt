# BankID Library sources
set(BANKID_LIB_SOURCES
  ${CMAKE_CURRENT_LIST_DIR}/bankid.cpp)

# Include directories
set(BANKID_INCLUDE_DIR
  ${CMAKE_CURRENT_LIST_DIR}/includes)

# Find OpenSSL package
find_package(OpenSSL REQUIRED)

# Find the nolohmann_json package
find_package(nlohmann_json REQUIRED)

# Create the BankID library (shared/static based on BUILD_SHARED_LIBS)
add_library(bankid_lib ${BANKID_LIB_SOURCES})

# Set library properties
set_target_properties(bankid_lib PROPERTIES
    OUTPUT_NAME "bankid"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Set compile definitions for Windows DLL export/import
if(WIN32)
    if(BUILD_SHARED_LIBS)
        target_compile_definitions(bankid_lib PRIVATE BANKID_EXPORTS)
    else()
        # For static libraries, we don't need the export/import macros
        target_compile_definitions(bankid_lib PUBLIC BANKID_STATIC)
    endif()
endif()

# Include directories for the library
target_include_directories(bankid_lib PUBLIC 
    $<BUILD_INTERFACE:${BANKID_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(bankid_lib PUBLIC OpenSSL::SSL OpenSSL::Crypto nlohmann_json::nlohmann_json)

# Installation rules
include(GNUInstallDirs)

# Install the library
install(TARGETS bankid_lib
    EXPORT BankIDTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install headers
install(DIRECTORY ${BANKID_INCLUDE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Export targets for other projects to use
install(EXPORT BankIDTargets
    FILE BankIDTargets.cmake
    NAMESPACE BankID::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/BankID
)

# Create and install config file
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/BankIDConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/BankIDConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/BankID
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/BankIDConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/BankIDConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/BankIDConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/BankID
)
