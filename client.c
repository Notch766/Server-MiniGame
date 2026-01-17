
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>

#define PORT 4001

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <server_ip>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server;

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    inet_pton(AF_INET, argv[1], &server.sin_addr);

    if (connect(sock,(struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("Connection failed");
        return EXIT_FAILURE;
    }

    printf("Connected to server\n");

    char buffer[1024];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int n = read(sock, buffer, sizeof(buffer)-1);

        if (n <= 0) {
            printf("Server disconnected\n");
            break;
        }

        printf("%s", buffer);

        if (strstr(buffer, "YOUR_TURN") != NULL)
        {
            printf("> ");
            fflush(stdout);

            char input[100];
            fgets(input, sizeof(input), stdin);
            write(sock, input, strlen(input));
        }

    }

    close(sock);
    return EXIT_SUCCESS;
}
