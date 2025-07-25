cmake_minimum_required(VERSION 3.16)

# Find required packages
find_package(GTest REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(OpenSSL REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/bankid/includes)

# Test executable
add_executable(bankid_tests
    test_main.cpp
    test_auth.cpp
    # test_sign.cpp
    # test_payment.cpp
    # test_other_payment.cpp
    # test_phone_auth.cpp
    # test_phone_sign.cpp
    # test_collect.cpp
    # test_cancel.cpp
)

# Link libraries
target_link_libraries(bankid_tests
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    bankid_lib
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Set target properties
set_target_properties(bankid_tests PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Enable testing
enable_testing()

# Add test discovery - temporarily disabled to avoid DLL issues
# include(GoogleTest)
# gtest_discover_tests(bankid_tests)

# Manual test registration as fallback
add_test(NAME AuthTests COMMAND bankid_tests --gtest_filter=AuthTest.*)

# Add custom test target
add_custom_target(run_tests
    COMMAND bankid_tests
    DEPENDS bankid_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# If using shared libraries on Windows, copy the DLL and dependencies
if(WIN32 AND BUILD_SHARED_LIBS)
    # Create a script to handle DLL copying
    set(COPY_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/copy_dlls.cmake")
    file(WRITE ${COPY_SCRIPT} "
        message(STATUS \"Copying DLL from: \${SRC_FILE}\")
        message(STATUS \"Copying DLL to: \${DST_DIR}\")
        
        if(EXISTS \"\${SRC_FILE}\")
            file(COPY \"\${SRC_FILE}\" DESTINATION \"\${DST_DIR}\")
            message(STATUS \"Successfully copied DLL\")
        else()
            message(WARNING \"Source DLL not found: \${SRC_FILE}\")
        endif()
        
        # List files in destination directory
        file(GLOB DST_FILES \"\${DST_DIR}/*\")
        message(STATUS \"Files in destination directory:\")
        foreach(FILE \${DST_FILES})
            message(STATUS \"  \${FILE}\")
        endforeach()
    ")
    
    # Copy the BankID DLL using the script
    add_custom_command(TARGET bankid_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} 
            -DSRC_FILE=$<TARGET_FILE:bankid_lib>
            -DDST_DIR=$<TARGET_FILE_DIR:bankid_tests>
            -P ${COPY_SCRIPT}
        COMMENT "Copying BankID DLL to tests output directory with detailed logging"
        VERBATIM
    )
endif()

# Copy certificates folder to output directory for tests
add_custom_command(TARGET bankid_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/../server/certs
    $<TARGET_FILE_DIR:bankid_tests>/certs
    COMMENT "Copying certificates folder to tests output directory"
    VERBATIM
)
