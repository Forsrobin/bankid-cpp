# BankID REST API Server

A simple REST API server built with CrowCpp that provides BankID authentication endpoints using the BankID library.

## Getting Started

### Building and Running

1. **Build the server**:
   ```bash
   # Use VS Code task: Ctrl+Shift+P -> "Tasks: Run Task" -> "Build Static Library (Debug)"
   # Or use command line:
   cmake --preset vs2022-deb
   cmake --build build/vs2022-deb --config Debug
   ```

2. **Run the server**:
   ```bash
   # Use VS Code task: Ctrl+Shift+P -> "Tasks: Run Task" -> "Run Server (Static)"
   # Or use command line:
   build/vs2022-deb/server/Debug/bankid_server.exe
   ```

The server will start on `http://localhost:8080`

## API Endpoints

### GET /init

Initiates a new BankID authentication session.

**Request:**
```
GET http://localhost:8080/init
```

**Response:**
```json
{
  "status": "success",
  "token": "auth_token_1753129021044",
  "message": "Authentication initiated"
}
```

**Status Codes:**
- `200` - Authentication initiated successfully
- `500` - Failed to initialize BankID or start authentication

### GET /poll

Checks the status of the current authentication session.

**Request:**
```
GET http://localhost:8080/poll
```

**Response:**
```json
{
  "status": "success", 
  "token": "auth_token_1753129021044",
  "auth_status": "COMPLETED"
}
```

**Status Codes:**
- `200` - Status retrieved successfully
- `400` - No active authentication session
- `500` - BankID library not initialized

## Example Usage

```bash
# Start authentication
curl -X GET http://localhost:8080/init

# Check authentication status
curl -X GET http://localhost:8080/poll
```

## Architecture

- **Server Framework**: CrowCpp for REST API
- **Authentication**: BankID library (linked as static library or DLL)
- **JSON Responses**: Simple string concatenation (no external JSON library dependency)
- **Threading**: Multi-threaded server using Crow's built-in threading

## Development

The server is designed to be simple and focuses on demonstrating the integration between:
1. **CrowCpp** - Handling HTTP requests and responses
2. **BankID Library** - Authentication logic
3. **CMake** - Build system integration

### VS Code Tasks Available:
- `Run Server (Static)` - Run server with static library linkage
- `Run Server (Shared)` - Run server with DLL linkage
- `Build Static Library (Debug)` - Build with static linkage
- `Build Shared Library (Debug)` - Build with DLL linkage
