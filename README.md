# 📂 FTP Server/Client System

## 📝 Overview

This project is a **terminal-based FTP (File Transfer Protocol) system** built in **C++ using socket programming**. It includes two main components:

- 🖥️ **Server**: Manages file operations, user authentication, and data transfers
- 💻 **Client**: Provides a command-line interface for users to interact with the server

---

## ✨ Key Features

### ⚙️ Core Functionality

- 📤 File upload/download (ASCII & Binary modes)
- 📁 Directory listing & navigation
- 🗂️ File/Directory management (create, delete, rename)
- 🔑 User authentication with quotas
- 🔄 Two transfer modes: **Passive** & **Active**

### 🔐 Security Features

- 👤 User authentication (username & password)
- 📊 Download quotas per user
- 🚫 Directory access restrictions
- 🛡️ Admin vs. Normal user privileges

---

## 🏗️ System Architecture

### 🖥️ Server Components

1. **Control Connection** (Port `2121` by default)

   - 📝 Handles commands & responses
   - 🔑 Manages authentication
   - 📡 Coordinates data transfers

2. **Data Connection**

   - 📂 Transfers files & directory listings
   - 🔄 Opened separately for each transfer

3. **User Management**

   - 📑 Reads from `private/users.txt`
   - 👥 Supports admin & normal users
   - 📊 Enforces quotas

---

### 💻 Client Components

1. **Command Interface**

   - ⌨️ Interactive FTP prompt
   - 🔍 Command parsing & validation
   - 📡 Communicates with server

2. **Transfer Management**

   - 🔄 Handles passive & active modes
   - 🔌 Manages data connections

---

## 🔄 How It Works

### 🔗 Connection Flow

1. 🖥️ Server listens on control port
2. 💻 Client connects to server
3. 🔑 User authenticates (`USER`/`PASS`)
4. 🔄 Client sets transfer mode (`PASV`/`ACTIVE`)
5. 📂 File operations over data connection
6. ❌ Disconnect with `QUIT`

### 📡 Data Transfer Modes

1. **Active Mode**

   - 🔁 Server connects back to client’s port (`ACTIVE [port]`)

2. **Passive Mode** (default)

   - 🖥️ Server opens random port
   - 💻 Client connects to it (`PASV`)

---

## 🚀 Usage Guide

### ▶️ Starting the System

1. Compile both server & client:

   ```bash
   g++ server.cpp -o server
   g++ client.cpp -o client
   ```

2. Run server:

   ```bash
   ./server
   ```

3. Run client:

   ```bash
   ./client
   ```

---

### ⌨️ Client Commands

#### 🔑 Connection

- `User [username]` → Login
- `Pass [password]` → Password
- `quit` → Disconnect

#### 🔄 Transfer Modes

- `PASV` → Passive mode
- `ACTIVE [port]` → Active mode
- `ASCII` → ASCII mode
- `BINARY` → Binary mode

#### 📂 File Operations

- `LS` → List directory
- `PWD` → Show working directory
- `CWD [path]` → Change directory
- `RETR [filename]` → Download file
- `MKD [dirname]` → Create directory
- `DELE -f [file]` → Delete file
- `DELE -d [dir]` → Delete directory
- `RENAME [old] [new]` → Rename file/dir
- `HELP` → Show commands

---

## 🔧 Implementation Details

### 🛠️ Technical Aspects

1. **Socket Programming**

   - TCP sockets for reliable comms
   - Separate control & data channels
   - Non-blocking ops with timeouts

2. **File Transfer**

   - ASCII & Binary support
   - `<FILE_END>` marker
   - Chunked transfer with buffers

3. **User Management**

   - Plaintext authentication (demo)
   - Per-user quotas
   - Admin-only operations

4. **Error Handling**

   - FTP response codes
   - Clear error messages
   - State validation

---

## 💻 Example Session

### 🖥️ Server

```bash
$ ./server
Server listening on control port 2121...
Client connected.
Client connected to data port 54321
```

### 💻 Client

```bash
$ ./client
Connected to server control port.
ftp> User testuser
331 Password required for username.
ftp> Pass password123
230 User testuser logged in.
ftp> PASV
Connected to data port 54321
ftp> LS
file1.txt
file2.txt
directory1
200 PORT command successful.
150 Opening ASCII mode data connection for file list.
ftp> RETR file1.txt
Received and saved as received_file1.txt
226 Transfer complete.
ftp> quit
221 Goodbye!
```

---

## ⚙️ Configuration

📂 Server uses two config files:

1. `private/config.txt` → Server port + quotas
2. `private/users.txt` → User credentials & roles

---

## ⚠️ Limitations

1. 🔓 Plaintext login
2. 🛑 Limited error recovery
3. 👥 Few concurrent users
4. 📤 No file upload yet

---

## 🌱 Future Enhancements

1. 🔒 Add SSL/TLS encryption
2. 📤 Implement file upload (`STOR`)
3. 👥 Support many concurrent clients
4. 📝 Logging & monitoring
5. 🔄 Hot-reload config

---

✨ This project demonstrates **core FTP protocol operations and socket programming**. Its modular design makes it easy to extend into a more advanced FTP system.

