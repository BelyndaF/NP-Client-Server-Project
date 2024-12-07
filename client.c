#include <arpa/inet.h>
#include <netinet/tcp.h> // Include for TCP_NODELAY
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h> // Include for setsockopt
#include <unistd.h>

#define PORT 8080

int main() {
  int sock = 0;
  struct sockaddr_in serv_addr;
  char buffer[1024] = {0};

  // Create socket
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("\n Socket creation error \n");
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);

  // Convert IP addresses from text to binary form
  if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
    printf("\nInvalid address/ Address not supported \n");
    return -1;
  }

  // Disable Nagle's algorithm by setting TCP_NODELAY option
  int flag = 1;
  if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int)) < 0) {
    perror("setsockopt TCP_NODELAY failed");
    return -1;
  }

  // Connect to server
  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    printf("\nConnection Failed \n");
    return -1;
  }

  printf("Connected to the server. Type messages below:\n");
  fflush(stdout); // Flush the stdout buffer immediately after printing

  while (1) {
    // Receive message from server
    memset(buffer, 0, sizeof(buffer));
    read(sock, buffer, sizeof(buffer)); // Receive prompt from server
    printf("%s", buffer);               // Print the received prompt
    fflush(
        stdout); // Flush the stdout buffer to ensure prompt appears immediately

    // Get user input and remove newline if present
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline character

    // Send input to server
    write(sock, buffer, strlen(buffer));

    // Exit condition check
    if (strcmp(buffer, "99") == 0) { // Exit if the user types "3" or "5"
      printf("Exiting...\n");
      fflush(stdout); // Ensure the exit message is flushed
      break;
    }
  }

  close(sock);
  return 0;
}
