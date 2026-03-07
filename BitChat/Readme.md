# Task 1 — ChatService

A simple, well-structured C++ implementation of a chat service demonstrating object-oriented design using classes for Users, Messages, Chats (Direct and Group), and a ChatService manager.

This project is intended as a small exercise / sample showcasing clean interfaces, ownership using smart pointers, and basic runtime checks.

Features
- Create users with unique IDs
- Create direct chats between two distinct users
- Create group chats with multiple users
- Send and store messages in chats
- Retrieve chat message history
- Basic validation and logging for invalid operations

Repository structure (relevant files)

- Task-1/
  - Header_Files/
    - Chat.h         — Abstract base class for chats
    - DirectChat.h   — Direct (two-user) chat implementation
    - GroupChat.h    — Group chat implementation
    - Message.h      — Message model
    - User.h         — User model
  - Source_Files/
    - Chat.cpp
    - DirectChat.cpp
    - GroupChat.cpp
    - Message.cpp
    - User.cpp
    - ChatService.cpp — Core service that manages users and chats
    - Main.cpp        — Example usage and basic tests
  - readMe.md        — This file

Build and run

Requirements
- A C++17-capable compiler (g++ or clang++)
- Standard C++ library

Quick compile and run (Linux / macOS / WSL)

1. From the repository root run:

   mkdir -p bin
   g++ -std=c++17 Task-1/Source_Files/*.cpp -I Task-1/Header_Files -o bin/chat_service

2. Run the program:

   ./bin/chat_service

The example Main.cpp exercises user creation, direct/group chats, message sending, and edge cases. The program prints logs and message histories to stdout.

Notes on design

- Ownership: ChatService stores users and chats using std::unique_ptr to manage lifetime and avoid memory leaks.
- Extensibility: Chat is an abstract class; new chat types can be added by inheriting and implementing the isUserInChat method.
- Validation: ChatService performs validation for users and chats and logs error messages to help debugging.

Testing

The included Main.cpp contains assertions that act as lightweight tests. To perform the basic checks, compile and run the binary; assertion failures will abort the program.


Author

Rudr-1705
