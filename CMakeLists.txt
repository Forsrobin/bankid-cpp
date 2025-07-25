cmake_minimum_required(VERSION 3.16)
project(BANKID LANGUAGES CXX
    VERSION 0.1.0
    DESCRIPTION "BankID - A Modern & Safe Authentication Method Implementation in C++"
    HOMEPAGE_URL "https://github.com/Forsrobin/bankID-auth-examples")

# Must compile with C++23, no
# extensions
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN On)

if(NOT WIN32 AND CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    if (NOT DEFINED CMAKE_INSTALL_PREFIX)
        set(CMAKE_INSTALL_PREFIX "/opt/${PROJECT_NAME}")
    endif()
endif()

option(BUILD_SHARED_LIBS "Build shared library" Off)

option(BUILD_TESTS "Build tests" On)

add_subdirectory(bankid)
add_subdirectory(server)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

