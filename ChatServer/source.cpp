#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <iostream>
#include <string>
#include <fstream>
#include <ctime>

std::string logFile = "ServerLog.txt";
#define PIPE_NAME R"(\\.\pipe\ChatPipe)"


std::string getCurrentTime() {
    time_t now = time(0);
    tm* localTime = localtime(&now);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", localTime);
    return std::string(buffer);
}
void saveToLog(const std::string& fileName, const std::string& message) {
    std::ofstream logFile(fileName, std::ios::app);
    if (logFile.is_open()) {
        logFile << getCurrentTime() << " " << message << "\n";
        logFile.close();
    }
    else {
        std::cerr << "Error: Unable to open log file " << fileName << "\n";
    }
}
//---------------------------------------------------------
void startNamedPipesChat(){
    std::ofstream file(logFile, std::ios::app);
    file << "\n";
    HANDLE hPipe;
    char buffer[1024];
    DWORD bytesRead;

    std::cout << "Chat server started. Waiting for client...\n";
    saveToLog(logFile, "Chat server started. Waiting for client...");

    hPipe = CreateNamedPipeA(
        PIPE_NAME,
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        1024,
        1024,
        0,
        nullptr);

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create named pipe. Error code: " << GetLastError() << "\n";
        saveToLog(logFile, "Failed to create named pipe. Error code: " + std::to_string(GetLastError()));
        return;
    }

    if (!ConnectNamedPipe(hPipe, nullptr)) {
        std::cerr << "Failed to connect client. Error code: " << GetLastError() << "\n";
        saveToLog(logFile, "Failed to connect client. Error code: " + std::to_string(GetLastError()));
        CloseHandle(hPipe);
        return;
    }

    std::cout << "Client connected via Named Pipes!\n";
    saveToLog(logFile, "Client connected via Named Pipes!");
    char serverMessage[1024];
    DWORD bytesWritten;
    while (true) {
        if (!ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr)) {
            std::cerr << "Error reading data. Error code: " << GetLastError() << "\n";
            saveToLog(logFile, "Error reading data. Error code: " + std::to_string(GetLastError()));
            break;
        }
        buffer[bytesRead] = '\0';

        std::cout << "Client: " << buffer << "\n";
        saveToLog(logFile, "Client: " + std::string(buffer));
        if (strcmp(buffer, "/menu") == 0) {
            system("cls");
            saveToLog(logFile, "The chat session has been ended by the client.");
            DisconnectNamedPipe(hPipe);
            CloseHandle(hPipe);
            return;
        }
        if (strcmp(buffer, "/exit") == 0) {
            std::cout << "The chat session has been ended by the client.\n";
            saveToLog(logFile, "The chat session has been ended by the client.");
            DisconnectNamedPipe(hPipe);
            CloseHandle(hPipe);
            exit(0);
        }

        std::cout << "You: ";
        std::cin.getline(serverMessage, sizeof(serverMessage));
        saveToLog(logFile, "You: " + std::string(serverMessage));
        if (strcmp(serverMessage, "/menu") == 0) {
            system("cls");
            WriteFile(hPipe, "/menu", 6, &bytesWritten, nullptr);
            saveToLog(logFile, "The chat session has been ended by the server.");
            DisconnectNamedPipe(hPipe);
            CloseHandle(hPipe);
            return;  
        }
        if (strcmp(serverMessage, "/exit") == 0) {
            std::cout << "The chat session has been ended.\n";
            WriteFile(hPipe, "/exit", 6, &bytesWritten, nullptr);
            saveToLog(logFile, "The chat session has been ended by the server.");
            DisconnectNamedPipe(hPipe);
            CloseHandle(hPipe);
            exit(0);
        }
        if (!WriteFile(hPipe, serverMessage, strlen(serverMessage), &bytesWritten, nullptr)) {
            std::cerr << "Error sending data. Error code: " << GetLastError() << "\n";
            saveToLog(logFile, "Error sending data. Error code: " + std::to_string(GetLastError()));
            break;
        }
    }
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);
    exit(0);
}
//---------------------------------------------------------
void startFileMappingChat() {
    std::ofstream file(logFile, std::ios::app);
    file << "\n";
    int bufferSize = 1024;
    const char* fileMappingName = "Local//ChatFile";
    char* sharedMemory;
    HANDLE hMapFile;

    std::cout << "Chat server started. Waiting for client...\n";
    saveToLog(logFile, "Chat server started. Waiting for client...");

    hMapFile = CreateFileMappingA(
        INVALID_HANDLE_VALUE,    
        NULL,                   
        PAGE_READWRITE,         
        0,                       
        bufferSize,             
        fileMappingName);          

    if (hMapFile == NULL) {
        std::cerr << "Could not create file mapping object. Error code: " << GetLastError() << "\n";
        saveToLog(logFile, "Failed to create file mapping object. Error code: " + std::to_string(GetLastError()));
        return;
    }

    sharedMemory = (char*)MapViewOfFile(hMapFile, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, bufferSize);
    if (sharedMemory == NULL) {
        std::cerr << "Could not map view of file. Error code: " << GetLastError() << "\n";
        saveToLog(logFile, "Failed to map view of file. Error code: " + std::to_string(GetLastError()));
        CloseHandle(hMapFile);
        return;
    }
    while (true) {
        if (sharedMemory[0] == '1') {
            std::cout << "Client connected via File Mapping!\n";
            saveToLog(logFile, "Client connected via File Mapping!");
            break;
        }
    }
    char serverMessage[1024];
    DWORD bytesWritten;
    while (true) {
        while (sharedMemory[0] == '1') {
            Sleep(100);
        }

        std::cout << "Client: " << (sharedMemory + 1) << "\n";
        saveToLog(logFile, "Client: " + std::string(sharedMemory + 1));

        if (strcmp(sharedMemory + 1, "/menu") == 0) {
            saveToLog(logFile, "The chat session has been ended by the client.");
            system("cls");
            UnmapViewOfFile(sharedMemory);
            CloseHandle(hMapFile);
            break;
        }
        if (strcmp(sharedMemory + 1, "/exit") == 0) {
            std::cout << "The chat session has been ended by the client.\n";
            saveToLog(logFile, "The chat session has been ended by the client.");
            UnmapViewOfFile(sharedMemory);
            CloseHandle(hMapFile);
            exit(0);
        }

        std::cout << "You: ";
        std::cin.getline(serverMessage, sizeof(serverMessage));
        saveToLog(logFile, "You: " + std::string(serverMessage));

        strcpy(sharedMemory + 1, serverMessage); 
        sharedMemory[0] = '1';                  

        if (strcmp(serverMessage, "/menu") == 0) {
            saveToLog(logFile, "The chat session has been ended by the server.");
            system("cls");
            UnmapViewOfFile(sharedMemory);
            CloseHandle(hMapFile);
            break;
        }
        if (strcmp(serverMessage, "/exit") == 0) {
            std::cout << "The chat session has been ended.\n";
            saveToLog(logFile, "The chat session has been ended by the server.");
            UnmapViewOfFile(sharedMemory);
            CloseHandle(hMapFile);
            exit(0);
        }
        while (sharedMemory[0] != '1') {
            Sleep(100);
        }
    }
    UnmapViewOfFile(sharedMemory);
    CloseHandle(hMapFile);
}
//---------------------------------------------------------
void showProgramInfo() {
    system("cls");
    int exit;
    std::cout << "Program Information:\n";
    std::cout << "This program allows communication between a client and a server using two methods:\n";
    std::cout << "1. Named Pipes\n";
    std::cout << "2. File Mapping\n\n";
    std::cout << "When the client sends a message, you will be able to send a response. You can then continue the conversation.\n";
    std::cout << "Choose the communication method when prompted at the beginning.\n\n";
    std::cout << "Available commands during the chat session:\n";
    std::cout << "/exit - Ends the chat session.\n";
    std::cout << "/menu - Ends the chat session and returns to the main menu.\n\n";
    std::cout << "Enjoy chatting!\n\n";
    std::cout << "Enter '0' to return to the menu\n";
    while (true) {
        std::cin >> exit;
        if (exit == 0) {
            system("cls");
            return;
        }
    }
}
//----------------------------------------------------------

int main() {
    while (true) {
        int choice;
        std::cout << "SERVER CHAT\n";
        std::cout << "Choose communication method:\n";
        std::cout << "1. Named Pipes\n";
        std::cout << "2. File Mapping\n";
        std::cout << "3. Exit\n";
        std::cout << "0. View Program Info\n";
        std::cout << "Enter your choice: ";
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice) {
        case 1:
            startNamedPipesChat();
            break;
        case 2:
            startFileMappingChat();
            break;
        case 3:
            std::cout << "Exiting the program.\n";
            return 0;
        case 0:
            showProgramInfo();
            break;
        default:
            system("cls");
            std::cout << "Invalid choice. Try again.\n";
            break;
        }
    }
    return 0;
}