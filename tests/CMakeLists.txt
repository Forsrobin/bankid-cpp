cmake_minimum_required(VERSION 3.16)

# Enable testing
enable_testing()

# Find required packages
find_package(GTest REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(OpenSSL REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/bankid/includes)

# Test executable
add_executable(bankid_tests
    test_main.cpp
    test_ssl.cpp
    test_auth.cpp
    test_sign.cpp
    test_payment.cpp
    test_other_payment.cpp
    # test_phone_auth.cpp
    # test_phone_sign.cpp
    # test_collect.cpp
    test_cancel.cpp
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

# Handle shared library runtime path (RPATH) on Unix systems
if(BUILD_SHARED_LIBS AND UNIX)
    # Set RPATH so the executable can find the shared library
    set_target_properties(bankid_tests PROPERTIES
        BUILD_RPATH "${CMAKE_BINARY_DIR}/bankid"
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib"
        BUILD_WITH_INSTALL_RPATH FALSE
        SKIP_BUILD_RPATH FALSE
    )
endif()

# Manual test registration as fallback
add_custom_target(run_tests
    COMMAND bankid_tests
    DEPENDS bankid_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Handle DLL copying for Windows shared builds
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

# For Unix shared builds, ensure the library is available at test runtime
if(BUILD_SHARED_LIBS AND UNIX)
    # Add a custom command to create a symlink or copy the shared library
    add_custom_command(TARGET bankid_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            $<TARGET_FILE:bankid_lib>
            $<TARGET_FILE_DIR:bankid_tests>/lib$<TARGET_FILE_NAME:bankid_lib>
        COMMENT "Creating symlink to shared library in test directory"
        VERBATIM
    )
endif()

# Copy certificates folder to output directory for tests
add_custom_command(TARGET bankid_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/certs
    $<TARGET_FILE_DIR:bankid_tests>/certs
    COMMENT "Copying certificates folder to tests output directory"
    VERBATIM
)

add_test(NAME SSLConfigTest COMMAND bankid_tests --gtest_filter=SSLConfigTest.*)
set_tests_properties(SSLConfigTest PROPERTIES WORKING_DIRECTORY $<TARGET_FILE_DIR:bankid_tests>)

add_test(NAME AuthTests COMMAND bankid_tests --gtest_filter=AuthTest.*)
set_tests_properties(AuthTests PROPERTIES WORKING_DIRECTORY $<TARGET_FILE_DIR:bankid_tests>)

add_test(NAME SignTests COMMAND bankid_tests --gtest_filter=SignTest.*)
set_tests_properties(SignTests PROPERTIES WORKING_DIRECTORY $<TARGET_FILE_DIR:bankid_tests>)

add_test(NAME PaymentTests COMMAND bankid_tests --gtest_filter=PaymentTest.*)
set_tests_properties(PaymentTests PROPERTIES WORKING_DIRECTORY $<TARGET_FILE_DIR:bankid_tests>)

add_test(NAME OtherPaymentTests COMMAND bankid_tests --gtest_filter=OtherPaymentTest.*)
set_tests_properties(OtherPaymentTests PROPERTIES WORKING_DIRECTORY $<TARGET_FILE_DIR:bankid_tests>)

add_test(NAME CancelTests COMMAND bankid_tests --gtest_filter=CancelTest.*)
set_tests_properties(CancelTests PROPERTIES WORKING_DIRECTORY $<TARGET_FILE_DIR:bankid_tests>)

# Set environment variables for tests to find shared libraries
if(BUILD_SHARED_LIBS AND UNIX)
    set_tests_properties(SSLConfigTest AuthTests SignTests PROPERTIES
        ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/bankid:$ENV{LD_LIBRARY_PATH}"
    )
endif()