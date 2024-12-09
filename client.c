#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <stdbool.h>
#include <unistd.h> // read(), write(), close()
#define MAX 80
#define PORT 8080
#define SA struct sockaddr
void func(int sockfd)
{
    char buff[MAX];
    int n;
    for (;;) {
        bzero(buff, sizeof(buff));
        printf("\nPlease, choose an option to continue:\n");
        printf("1. Login\n");
        printf("2. Create Account\n");
        printf("3. Exit\n");
        printf("Option: ");
        n = 0;
        while ((buff[n++] = getchar()) != '\n')
            ;

        write(sockfd, buff, sizeof(buff));
        bzero(buff, sizeof(buff));
        read(sockfd, buff, sizeof(buff));
        // printf("\n%s\n", buff);
        if ((strncmp(buff, "exit", 4)) == 0) {
            printf("Client Exit...\n");
            break;
        } else if (strncmp(buff, "login", 5) == 0) { // If server asks for login, prompt user for credentials
            printf("Enter username: ");
            bzero(buff, sizeof(buff)); // Clear the buffer
            fgets(buff, sizeof(buff), stdin); // Read the username
            write(sockfd, buff, sizeof(buff)); // Send username to server

            bzero(buff, sizeof(buff)); // Clear the buffer
            printf("Enter password: ");
            fgets(buff, sizeof(buff), stdin); // Read the password
            write(sockfd, buff, sizeof(buff)); // Send password to server

            bzero(buff, sizeof(buff)); // Clear the buffer
            read(sockfd, buff, sizeof(buff)); // Read server's response to login attempt
            printf("%s\n\n", buff); // Print the server's response (success or failure)

            if (strncmp(buff, "You have successfully logged in!", sizeof(buff)) == 0) {
                printf("Team D's Library Main Menu:\n");
                printf("----------------------------------------------------------\n");
                for (;;) {
                    printf("\nPlease choose an option:\n1. Review Account Information\n2. View Books\n3. Rent Book\n4. Return Book\n5. Exit\nOption: ");
                    bzero(buff, sizeof(buff));
                    n = 0;
                    while ((buff[n++] = getchar()) != '\n')
                        ;

                    write(sockfd, buff, sizeof(buff));

                    if (strncmp(buff, "5", 1) == 0) {
                        printf("Client Exit...\n");
                        break;
                    } else if (strncmp(buff, "1", 1) == 0) {
                        char buffer2[1020];
                        bzero(buffer2, sizeof(buffer2));
                        read(sockfd, buffer2, sizeof(buffer2));
                        printf("\n%s\n", buffer2);
                    } else if (strncmp(buff, "2", 1) == 0) {
                        char buffer2[2048];
                        bzero(buff, sizeof(buff));
                        bzero(buffer2, sizeof(buffer2));
                        read(sockfd, buffer2, sizeof(buffer2));
                        printf("\n%s\n", buffer2);
                    } else if (strncmp(buff, "3", 1) == 0) {
                        bzero(buff, sizeof(buff)); // Clear the buffer
                        printf("Enter the book ID you want to rent: ");
                        fgets(buff, sizeof(buff), stdin); // Read the ID
                        write(sockfd, buff, sizeof(buff)); 

                        bzero(buff, sizeof(buff));
                        read(sockfd, buff, sizeof(buff));
                        printf("\n%s\n", buff);
                    } else if (strncmp(buff, "4", 1) == 0) {
                        bzero(buff, sizeof(buff)); // Clear the buffer
                        printf("Enter the book ID you want to return: ");
                        fgets(buff, sizeof(buff), stdin); // Read the ID
                        write(sockfd, buff, sizeof(buff)); 

                        bzero(buff, sizeof(buff));
                        read(sockfd, buff, sizeof(buff));
                        printf("\n%s\n", buff);
                    }
                }
                
                break;
            }

        } else if (strncmp(buff, "create", 6) == 0) { // If server prompts for account creation
            printf("Enter a new username: ");
            bzero(buff, sizeof(buff)); // Clear the buffer
            fgets(buff, sizeof(buff), stdin); // Read the new username
            write(sockfd, buff, sizeof(buff)); // Send the username to the server

            bzero(buff, sizeof(buff)); // Clear the buffer
            printf("Enter a new password: ");
            fgets(buff, sizeof(buff), stdin); // Read the new password
            write(sockfd, buff, sizeof(buff)); // Send the password to the server

            // bzero(buff, sizeof(buff)); // Clear the buffer
            // printf("Enter your ZIP code: ");
            // fgets(buff, sizeof(buff), stdin); // Read the new zip
            // write(sockfd, buff, sizeof(buff)); // Send the zip to the server

            bzero(buff, sizeof(buff)); // Clear the buffer
            read(sockfd, buff, sizeof(buff)); // Read server's response to account creation attempt
            printf("%s\n", buff); // Print the server's response (success or failure)
        } 
    }
}

int main()
{
    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n\n");
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);

    // connect the client socket to server socket
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr))
        != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    }
    else{
        printf("You have successfully connected to the Library Server!\n");
        printf("----------------------------------------------------------\n");
        printf("Welcome to Team D's Library Database!\n");
        printf("----------------------------------------------------------\n");
    }

    // function for chat
    func(sockfd);

    // close the socket
    close(sockfd);
}
