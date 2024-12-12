#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <iostream>
#include <string>
#include <fstream>
#include <ctime>

std::string logFile = "ClientLog.txt";
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
//----------------------------------------------------------
void startNamedPipesChat()
{
    std::ofstream file(logFile, std::ios::app);
    file << "\n";
    HANDLE hPipe;
    char buffer[1024];
    DWORD bytesRead;

    std::cout << "Waiting for server to start..";
    saveToLog(logFile, "Waiting for server to start...");

    while (true) {
        if (WaitNamedPipeA(PIPE_NAME, NMPWAIT_WAIT_FOREVER)) {
            break;
        }
        std::cout << ".";
        Sleep(1000);
    }

    std::cout << "\nConnecting to the server...\n";
    saveToLog(logFile, "Connecting to the server...");

    hPipe = CreateFileA(
        PIPE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr);

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to connect to the server. Error code: " << GetLastError() << "\n";
        saveToLog(logFile, "Failed to connect to the server. Error code: " + std::to_string(GetLastError()));
        return;
    }

    std::cout << "Connected to the server via Named Pipes!\n";
    saveToLog(logFile, "Connected to the server via Named Pipes!");
    char clientMessage[1024];
    DWORD bytesWritten;
    while (true) {
        std::cout << "You: ";
        std::cin.getline(clientMessage, sizeof(clientMessage));
        saveToLog(logFile, "You: " + std::string(clientMessage));
        if (strcmp(clientMessage, "/menu") == 0) {
            system("cls");
            WriteFile(hPipe, "/menu", 6, &bytesWritten, nullptr);
            saveToLog(logFile, "The chat session has been ended by the client.");
            CloseHandle(hPipe);
            return;
        }
        if (strcmp(clientMessage, "/exit") == 0) {
            std::cout << "The chat session has been ended.\n";
            WriteFile(hPipe, "/exit", 6, &bytesWritten, nullptr);
            saveToLog(logFile, "The chat session has been ended by the client.");
            CloseHandle(hPipe);
            exit(0);
        }
        if (!WriteFile(hPipe, clientMessage, strlen(clientMessage), &bytesWritten, nullptr)) {
            std::cerr << "Error sending data. Error code: " << GetLastError() << "\n";
            saveToLog(logFile, "Error sending data. Error code: " + std::to_string(GetLastError()));
            break;
        }

        if (!ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr)) {
            std::cerr << "Error reading data. Error code: " << GetLastError() << "\n";
            saveToLog(logFile, "Error reading data. Error code: " + std::to_string(GetLastError()));
            break;
        }
        buffer[bytesRead] = '\0';

        std::cout << "Server: " << buffer << "\n";
        saveToLog(logFile, "Server: " + std::string(buffer));
        if (strcmp(buffer, "/menu") == 0) {
            system("cls");
            saveToLog(logFile, "The chat session has been ended by the server.");
            CloseHandle(hPipe);
            return;
        }
        if (strcmp(buffer, "/exit") == 0) {
            std::cout << "The chat session has been ended by the server.\n";
            saveToLog(logFile, "The chat session has been ended by the server.");
            CloseHandle(hPipe);
            exit(0);
        }
    }
    CloseHandle(hPipe);
    exit(0);
}
//----------------------------------------------------------
void startFileMappingChat() {
    std::ofstream file(logFile, std::ios::app);
    file << "\n";
    const char* fileMappingName = "Local//ChatFile";
    const int bufferSize = 1024;
    char* sharedMemory;
    HANDLE hMapFile;

    std::cout << "Waiting for server to start..";
    saveToLog(logFile, "Waiting for server to start...");

    while (true) {
        hMapFile = OpenFileMappingA(
            FILE_MAP_ALL_ACCESS,         
            FALSE,                  
            fileMappingName);          
        if (hMapFile != NULL) {
            break;
        }
        std::cout << ".";
        Sleep(1000);
    }
    sharedMemory = (char*)MapViewOfFile(hMapFile, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, bufferSize);
    if (sharedMemory == NULL) {
        std::cerr << "Could not map view of file. Error code: " << GetLastError() << "\n";
        saveToLog(logFile, "Failed to map view of file. Error code: " + std::to_string(GetLastError()));
        CloseHandle(hMapFile);
        return;
    }
    sharedMemory[0] = '1';
    std::cout << "\nConnected to the server via File Mapping!\n";
    saveToLog(logFile, "Connected to the server via File Mapping!");
    char clientMessage[1024];
    while (true) {
        std::cout << "You: ";
        std::cin.getline(clientMessage, sizeof(clientMessage));
        saveToLog(logFile, "You: " + std::string(clientMessage));

        strcpy(sharedMemory + 1, clientMessage); 
        sharedMemory[0] = '0';                  
        if (strcmp(clientMessage, "/menu") == 0) {
            saveToLog(logFile, "The chat session has been ended by the client.");
            system("cls");
            UnmapViewOfFile(sharedMemory);
            CloseHandle(hMapFile);
            break;
        }
        if (strcmp(clientMessage, "/exit") == 0) {
            std::cout << "The chat session has been ended.\n";
            saveToLog(logFile, "The chat session has been ended by the client.");
            UnmapViewOfFile(sharedMemory);
            CloseHandle(hMapFile);
            exit(0);
        }

        while (sharedMemory[0] == '0') {
            Sleep(100);
        }

        std::cout << "Server: " << (sharedMemory + 1) << "\n";
        saveToLog(logFile, "Server: " + std::string(sharedMemory + 1));
        if (strcmp(sharedMemory + 1, "/menu") == 0) {
            saveToLog(logFile, "The chat session has been ended by the server.");
            system("cls");
            UnmapViewOfFile(sharedMemory);
            CloseHandle(hMapFile);
            break;
        }
        if (strcmp(sharedMemory + 1, "/exit") == 0) {
            std::cout << "The chat session has been ended by the server.\n";
            saveToLog(logFile, "The chat session has been ended by the server.");
            UnmapViewOfFile(sharedMemory);
            CloseHandle(hMapFile);
            exit(0);
        }
        sharedMemory[0] = '1';
    }
    UnmapViewOfFile(sharedMemory);
    CloseHandle(hMapFile);
}
//----------------------------------------------------------
void showProgramInfo() {
    system("cls");
    int exit;
    std::cout << "Program Information:\n";
    std::cout << "This program allows communication between a client and a server using two methods:\n";
    std::cout << "1. Named Pipes\n";
    std::cout << "2. File Mapping\n\n";
    std::cout << "When you send a message, the server will reply, and you can continue the conversation.\n";
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
        std::cout << "CLIENT CHAT\n";
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
            std::cout << "Invalid choice. Try again.\n";
            break;
        }
    }
    return 0;
}