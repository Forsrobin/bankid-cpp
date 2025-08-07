# BankID REST API Server

A lightweight REST API server built with **CrowCpp**, providing simple BankID authentication endpoints using the **BankID C++ library**.

---

## 🚀 Getting Started

### 🛠 Build & Run

#### 1. **Build the server**

You can build the server using either **VS Code tasks** or the **command line**:

**Via VS Code:**
- Press `Ctrl + Shift + P`
- Select: `Tasks: Run Task` → `Build Static Library (Debug)`

**Via Command Line:**
```bash
cmake --preset vs2022-deb
cmake --build build/vs2022-deb --config Debug
```

#### 2. **Run the server**

**Via VS Code:**
- Press `Ctrl + Shift + P`
- Select: `Tasks: Run Task` → `Run Server (Static)`

**Via Command Line:**
```bash
build/vs2022-deb/server/Debug/bankid_server.exe
```

Server starts at: [http://localhost:8080](http://localhost:8080)

---

## 📡 API Endpoints

### 🔹 `GET /init` – Start Authentication

Initiates a new BankID authentication session.

#### Request:
```
GET http://localhost:8080/init
```

#### Example Response:
```json
{
  "status": "success",
  "token": "auth_token_1753129021044",
  "message": "Authentication initiated"
}
```

#### Status Codes:
- `200 OK` – Authentication started
- `500 Internal Server Error` – Failed to initialize BankID or start session

---

### 🔹 `GET /poll` – Poll Authentication Status

Polls the current status of the ongoing BankID session.

#### Request:
```
GET http://localhost:8080/poll
```

#### Example Response:
```json
{
  "status": "success",
  "token": "auth_token_1753129021044",
  "auth_status": "COMPLETED"
}
```

#### Status Codes:
- `200 OK` – Status retrieved
- `400 Bad Request` – No active session
- `500 Internal Server Error` – BankID not initialized

---

## 🧪 Example Usage (cURL)

```bash
# Start a BankID authentication session
curl -X GET http://localhost:8080/init

# Poll the session status
curl -X GET http://localhost:8080/poll
```

---

## ⚙️ Architecture Overview

- **Framework**: [CrowCpp](https://github.com/CrowCpp/crow) for HTTP routing
- **Auth Layer**: BankID C++ library (linked as static or shared)
- **JSON Handling**: Simple string formatting (no external JSON dependency)
- **Concurrency**: Multi-threaded using Crow’s thread pool

---

## 👨‍💻 Development Notes

This server is structured to demonstrate clean separation between:

- **CrowCpp** – REST API request handling  
- **BankID Integration** – Secure authentication logic  
- **CMake** – Cross-platform build system

---

## 📋 VS Code Tasks

The following tasks are available in `.vscode/tasks.json`:

| Task Name                | Description                       |
|--------------------------|-----------------------------------|
| `Build Static Library`   | Build using static linking        |
| `Build Shared Library`   | Build using DLL/shared linking    |
| `Run Server (Static)`    | Run server with static linkage    |
| `Run Server (Shared)`    | Run server with DLL linkage       |

---

## 📄 License

This project is for demonstration and educational purposes. Licensing details for the BankID library must be obtained from the official provider.