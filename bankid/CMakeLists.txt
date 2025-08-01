# BankID Library sources
set(BANKID_LIB_SOURCES
  ${CMAKE_CURRENT_LIST_DIR}/bankid.cpp)

# Include directories
set(BANKID_INCLUDE_DIR
  ${CMAKE_CURRENT_LIST_DIR}/includes)

# Determine whether we're building as a standalone project
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(BUILDING_BANKID_PROJECT ON)
else()
    set(BUILDING_BANKID_PROJECT OFF)
endif()

# Find OpenSSL package
find_package(OpenSSL REQUIRED)

# Find the nlohmann_json package
if(NOT TARGET nlohmann_json::nlohmann_json)
    find_package(nlohmann_json REQUIRED)
endif()

# Create the BankID library (shared/static based on BUILD_SHARED_LIBS)  
if(BUILD_SHARED_LIBS)
    add_library(bankid_lib SHARED ${BANKID_LIB_SOURCES})
    set_target_properties(bankid_lib PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
    )
else()
    add_library(bankid_lib STATIC ${BANKID_LIB_SOURCES})
endif()

add_library(BankID::bankid ALIAS bankid_lib)

# Set library properties
set_target_properties(bankid_lib PROPERTIES
    OUTPUT_NAME "bankid"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Set compile definitions for symbol export/import
if(BUILD_SHARED_LIBS)
    target_compile_definitions(bankid_lib PRIVATE BANKID_EXPORTS)
    target_compile_definitions(bankid_lib INTERFACE BANKID_SHARED)
else()
    target_compile_definitions(bankid_lib PUBLIC BANKID_STATIC)
endif()

# Include directories for the library
target_include_directories(bankid_lib PUBLIC 
    $<BUILD_INTERFACE:${BANKID_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(bankid_lib PUBLIC cxx_std_23)
target_link_libraries(bankid_lib PUBLIC OpenSSL::SSL OpenSSL::Crypto nlohmann_json::nlohmann_json)

# Only export and install if building as a standalone project
if(BUILDING_BANKID_PROJECT)
    include(GNUInstallDirs)

    install(TARGETS bankid_lib
        EXPORT BankIDTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(DIRECTORY ${BANKID_INCLUDE_DIR}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )

    install(EXPORT BankIDTargets
        FILE BankIDTargets.cmake
        NAMESPACE BankID::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/BankID
    )

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
endif()
