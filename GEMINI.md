# GameMan Station - Project Context

## Project Overview

**GameMan Station** is a modular system designed to manage and launch games on a Linux-based arcade or station. It transitions from a monolithic architecture to a multi-process system comprising three main components:

1.  **Process Manager (`gameman-pm`)**: The core service responsible for monitoring and managing game processes. It uses `epoll` and `signalfd` for asynchronous process handling.
2.  **Log Server (`gameman-log`)**: A dedicated daemon that aggregates logs from other components via Unix Domain Sockets.
3.  **User Interface (`ui`)**: A graphical frontend built with **SDL2** for browsing the game library and launching titles.

## Directory Structure

*   `process-manager/`: Source code for the Process Manager service.
    *   `src/`: Implementation files (`ProcessManager.cpp`, `main.cpp`).
    *   `include/`: Header files.
    *   `docs/`: Architecture documentation (`ARCHITECTURE.md`).
    *   `Makefile`: Build script for the services.
*   `logs/`: Source code for the centralized Logging Service.
    *   `src/`: Implementation of the Log Server and client libraries.
    *   `include/`: Shared headers (`ILogger.hpp`, `SocketLogger.hpp`).
    *   `docs/`: Logging specific documentation.
*   `ui/`: Source code for the Graphical Interface.
    *   `src/`: Implementation files (`main.cpp`, `JanelaJogo.cpp`, etc.).
    *   `include/`: Headers for UI components.
    *   `assets/`: Images, fonts, and game data files.
    *   `README`: Specific instructions for the UI.

## Build and Run Instructions

### Prerequisites

*   **Compiler**: C++17 compliant compiler (e.g., `g++`).
*   **Libraries**:
    *   SDL2 (`libsdl2-dev`)
    *   SDL2 Image (`libsdl2-image-dev`)
    *   SDL2 TTF (`libsdl2-ttf-dev`)
    *   SDL2 GFX (`libsdl2-gfx-dev`)

### 1. Process Manager & Log Server

The `process-manager` directory contains a `Makefile` intended to build both the manager and the log server.

> **⚠️ Note:** The current `process-manager/Makefile` appears to have inconsistencies (potential missing variable definitions like `LOG_DIR` and `TARGET_LOG_SERVER`). It may require manual adjustment.

**Attempt to build:**
```bash
cd process-manager
make
```

**Run Process Manager:**
```bash
./bin/process-manager
```

### 2. User Interface (UI)

The UI component currently requires a manual compilation command (based on `ui/README`).

**Build:**
```bash
cd ui
g++ src/*.cpp -I include -o gerenciador -lSDL2 -lSDL2_image -lSDL2_gfx -lSDL2_ttf
```

**Run:**
```bash
./gerenciador
```

## Architecture Highlights

*   **Inter-Process Communication (IPC)**: Components communicate via Unix Domain Sockets. The Logger acts as a server, and the Process Manager acts as a client.
*   **Resilience**: The logging system implements a "Composite Logger" pattern. If the Log Server is unreachable, the system automatically falls back to local file logging (`FallbackLogger`), ensuring no data is lost and the application doesn't crash.
*   **UI Design**: The interface features a carousel for game selection, search functionality, and detailed game views, optimized for keyboard/mouse navigation.

## Development Conventions

*   **Language**: C++17.
*   **Style**:
    *   Headers in `include/`, Sources in `src/`.
    *   Interfaces start with `I` (e.g., `ILogger`).
    *   Dependency Injection is used for testability (e.g., injecting `ILogger` into `ProcessManager`).
*   **Logging**: All components should use the abstract `ILogger` interface rather than concrete logging classes.
