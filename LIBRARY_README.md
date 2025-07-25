# BankID C++ Library and Application

This project provides both a BankID library (that can be built as a static library or DLL) and a sample application that demonstrates its usage.

## Project Structure

- `bankid/` - Contains the BankID library source code and main application
  - `bankid.cpp` - BankID library implementation
  - `main.cpp` - Sample application using the library
  - `includes/bankid/bankid.h` - Public API header
- `example_usage/` - Example of how to use the BankID library in another project

## Building

### Build as Static Library (Default)
```bash
# Configure (using Visual Studio 2022 Debug preset)
cmake --preset vs2022-deb

# Build
cmake --build build/vs2022-deb --config Debug
```

### Build as Shared Library/DLL
```bash
# Configure with shared libraries enabled
cmake --preset vs2022-deb-shared

# Build
cmake --build build/vs2022-deb-shared --config Debug
```

### Install the Library
```bash
# Install to the configured prefix
cmake --install build/vs2022-deb --config Debug
```

## Outputs

After building, you'll get:

1. **Library**: `bankid.lib` (static) or `bankid.dll` + `bankid.lib` (shared)
2. **Executable**: `bankid.exe` - Sample application demonstrating library usage

## Using the Library in Another Project

After installing the library, you can use it in another CMake project:

```cmake
find_package(BankID REQUIRED)
target_link_libraries(your_target PRIVATE BankID::bankid_lib)
```

See the `example_usage/` directory for a complete example.

## API Overview

The BankID library provides the following main functions:

- `BankID::Initialize()` - Initialize the authentication system
- `BankID::StartAuthentication(personalNumber)` - Start authentication process
- `BankID::CheckAuthenticationStatus(token)` - Check authentication status
- `BankID::Shutdown()` - Cleanup and shutdown

## Cross-Platform Notes

The library uses proper export/import macros for Windows DLL support while remaining compatible with other platforms. The `BANKID_API` macro handles the necessary `__declspec` attributes on Windows automatically.
