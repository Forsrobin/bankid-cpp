# Server application using BankID library and CrowCpp

cmake_minimum_required(VERSION 3.16)
project(BankIDServer)

# Enable SSL support for Crow
set(CROW_ENABLE_SSL OFF)

# Find required packages
if (CROW_ENABLE_SSL)
  message(STATUS "CROW_ENABLE_SSL is ON, finding OpenSSL")
  find_package(OpenSSL REQUIRED)
endif()

find_package(Crow REQUIRED)
find_package(nlohmann_json REQUIRED)

# Server executable
add_executable(bankid_server main.cpp)

# Enable SSL compilation flag
if (CROW_ENABLE_SSL)
    target_compile_definitions(bankid_server PRIVATE CROW_ENABLE_SSL)
else()
    target_compile_definitions(bankid_server PRIVATE CROW_DISABLE_SSL)
endif()

# Include directories for BankID headers
target_include_directories(bankid_server PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../bankid/includes
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

if (CROW_ENABLE_SSL)
    target_link_libraries(bankid_server PRIVATE 
    Crow::Crow
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    bankid_lib)
else()
    target_link_libraries(bankid_server PRIVATE 
    Crow::Crow
    bankid_lib)
endif()

# Link libraries


# Set executable properties
set_target_properties(bankid_server PROPERTIES
    OUTPUT_NAME "bankid_server"
)

# If using shared libraries on Windows, copy the DLL
if(WIN32 AND BUILD_SHARED_LIBS)
    add_custom_command(TARGET bankid_server POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:bankid_lib>
        $<TARGET_FILE_DIR:bankid_server>
        COMMENT "Copying BankID DLL to server output directory"
    )
endif()

# Copy certificates folder to output directory
add_custom_command(TARGET bankid_server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/certs
    $<TARGET_FILE_DIR:bankid_server>/certs
    COMMENT "Copying certificates folder to server output directory"
)
