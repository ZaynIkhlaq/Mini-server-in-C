#include <winsock2.h>
#include <ws2tcpip.h>    
#include <windows.h>
#include <stdio.h>       
#include <stdlib.h>
#include <string.h>

#define PORT 8080 
#define BUFFER_SIZE 1024


int main() {
    // setup
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }

    // create tcp socket - IPv4
    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd == INVALID_SOCKET) {
        fprintf(stderr, "socket failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // easy restarts
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));


    struct sockaddr_in addr = {0}; 
    addr.sin_family = AF_INET;     
    addr.sin_addr.s_addr = INADDR_ANY; 
    addr.sin_port = htons(PORT);   

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        fprintf(stderr, "bind failed: %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    // start listening
    if (listen(server_fd, 5) == SOCKET_ERROR) {
        fprintf(stderr, "listen failed: %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    // accept client
    for (;;) {
        struct sockaddr_in cli;
        int len = sizeof(cli);
        SOCKET client = accept(server_fd, (struct sockaddr*)&cli, &len);
        if (client == INVALID_SOCKET) {
            fprintf(stderr, "accept failed: %d\n", WSAGetLastError());
            continue;
        }

        send_ok_header(client);

        // stream index.html - gotta be in the same directory
        FILE *file = fopen("index.html", "rb");
        if (file) {
            printf("Opened index.html for reading.\n");
            // get file size
            fseek(file, 0, SEEK_END);
            long filesize = ftell(file);
            rewind(file);
            printf("index.html size: %ld bytes.\n", filesize);

            // Allocate buffer
            char *filebuf = (char*)malloc(filesize);
            if (filebuf && filesize > 0) {
                size_t readbytes = fread(filebuf, 1, filesize, file);
                printf("Read %zu bytes from index.html\n", readbytes);

                // Send HTTP header with Content-Length
                char header[256];
                snprintf(header, sizeof(header),
                    "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %ld\r\nConnection: close\r\n\r\n",
                    filesize);
                send(client, header, strlen(header), 0);
                printf("Sent header: %s\n", header);

                // Send file content
                int sent = send(client, filebuf, (int)readbytes, 0);
                printf("Sent %d bytes to client.\n", sent);
                free(filebuf);
            } else {
                printf("Failed to allocate buffer or file is empty.\n");
            }
            fclose(file);
            printf("Closed index.html\n");
        } else {
            printf("Failed to open index.html\n");
        }

        closesocket(client);
    }

    // cleanup
    closesocket(server_fd);
    WSACleanup();
    return 0;
}