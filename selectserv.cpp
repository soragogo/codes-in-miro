#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

int PORT;
#define MAX_CLIENTS 30

int main(int ac, char*av[]) {
    PORT = atoi(av[1]);
    int server_fd, new_socket, client_socket[MAX_CLIENTS], activity, i, valread, sd;
    int max_sd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1025];  // 1KB buffer for incoming messages

    // Set of socket descriptors
    fd_set readfds;

    // Initialize all client_socket[] to 0 so not checked
    for (i = 0; i < MAX_CLIENTS; i++) {
        client_socket[i] = 0;
    }

    // Create a master socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // // Set master socket to allow multiple connections
    // if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
    //     perror("setsockopt");
    //     exit(EXIT_FAILURE);
    // }

    // Type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to localhost port 8080
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listener on port %d \n", PORT);

    // Try to specify maximum of 10 pending connections for the master socket
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Accept incoming connections
    puts("Waiting for connections ...");

    while (true) {
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add master socket to set
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        // Add child sockets to set
        for (i = 0; i < MAX_CLIENTS; i++) {
            // If valid socket descriptor then add to read list
            if (client_socket[i] > 0)
                FD_SET(client_socket[i], &readfds);

            // Highest file descriptor number, need it for the select function
            if (client_socket[i] > max_sd)
                max_sd = client_socket[i];
        }

        // Wait for an activity on one of the sockets, timeout is NULL, so wait indefinitely
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            printf("select error");
        }

        // If something happened on the master socket, then it's an incoming connection
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            // Inform user of socket number - used in send and receive commands
            printf("New connection, socket fd is %d, ip is : %s, port : %d \n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            // Add new socket to array of sockets
            for (i = 0; i < MAX_CLIENTS; i++) {
                // If position is empty
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    printf("Adding to list of sockets as %d\n", i);
                    break;
                }
            }
        }

        // Else it's some IO operation on some other socket
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (FD_ISSET(client_socket[i], &readfds)) {
                // Check if it was for closing, and also read the incoming message
                if ((valread = read(client_socket[i], buffer, 1024)) == 0) {
                    // Somebody disconnected, get his details and print
                    getpeername(client_socket[i], (struct sockaddr*)&address, (socklen_t*)&addrlen);
                    printf("Host disconnected, ip %s, port %d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                    // Close the socket and mark as 0 in list for reuse
                    close(client_socket[i]);
                    client_socket[i] = 0;
                } else {
                    // Echo back the message that came in
                    buffer[valread] = 0;
                    const char* response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Hello, WebBrouser!</h1></body></html>";
                    printf("sent message\n");

                    // // ↓もし接続を維持するならこの二行はいらない
                    // close(client_socket[i]);
                    // client_socket[i] = 0;
                }
            }
        }
    }

    return 0;
}
