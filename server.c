#define _GNU_SOURCE

#include "net.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define PORT 28281
#define BUFFER_SIZE 1024
#define METHOD_SIZE 16
#define FILE_NAME_SIZE 256

// Collect all child processes that have finished
void handle_sigchld(int signal_number)
{
   // Mark the signal parameter as intentionally unused
   (void)signal_number;

   // Collect each terminated child without blocking the server
   while (waitpid(-1, NULL, WNOHANG) > 0)
   {
   }
}

// Send an error message to the connected client
void send_error(int nfd, const char *message)
{
   // Send the complete error message through the socket
   if (write(nfd, message, strlen(message)) == -1)
   {
      perror("write");
   }
}

// Send the requested file to the connected client
void send_file(int nfd, const char *filename)
{
   // Store the file descriptor for the requested file
   int file_fd;

   // Store each block read from the requested file
   char buffer[BUFFER_SIZE];

   // Store the number of bytes read from the file
   ssize_t bytes_read;

   // Open the requested file for reading
   file_fd = open(filename, O_RDONLY);

   // Send an error when the requested file cannot be opened
   if (file_fd == -1)
   {
      send_error(nfd, "Error: file not found\n");
      return;
   }

   // Read and send the requested file one block at a time
   while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0)
   {
      // Send the current block to the client
      if (write(nfd, buffer, bytes_read) == -1)
      {
         perror("write");
         close(file_fd);
         return;
      }
   }

   // Report a file reading error
   if (bytes_read == -1)
   {
      perror("read");
   }

   // Close the requested file
   close(file_fd);
}

// Process one GET request from a connected client
void handle_request(int nfd)
{
   // Store the request sent by the client
   char request[BUFFER_SIZE];

   // Store the request method
   char method[METHOD_SIZE];

   // Store the requested file name
   char filename[FILE_NAME_SIZE];

   // Store the number of bytes received
   ssize_t bytes_read;

   // Read one request from the client
   bytes_read = read(nfd, request, sizeof(request) - 1);

   // Check whether reading from the client failed
   if (bytes_read == -1)
   {
      perror("read");
      return;
   }

   // Stop when the client closes the connection
   if (bytes_read == 0)
   {
      return;
   }

   // Add the null terminator so the request can be treated as text
   request[bytes_read] = '\0';

   // Separate the request method and file name
   if (sscanf(request, "%15s %255s", method, filename) != 2)
   {
      send_error(nfd, "Error: invalid request\n");
      return;
   }

   // Accept only the GET request method
   if (strcmp(method, "GET") != 0)
   {
      send_error(nfd, "Error: only GET is supported\n");
      return;
   }

   // Send the requested file to the client
   send_file(nfd, filename);
}

// Continue accepting and processing client connections
void run_service(int fd)
{
   // Accept clients until the server is stopped
   while (1)
   {
      // Wait for the next client connection
      int nfd = accept_connection(fd);

      // Continue if accepting the client was interrupted
      if (nfd == -1)
      {
         continue;
      }

      // Create a child process for this client
      pid_t pid = fork();

      // Handle a failed fork call
      if (pid < 0)
      {
         perror("fork");
         close(nfd);
      }

      // Let the child process handle the client request
      else if (pid == 0)
      {
         // The child does not accept new client connections
         close(fd);

         // Process the client's request
         handle_request(nfd);

         // Close the connected client socket
         close(nfd);

         // End the child process
         exit(0);
      }

      // Let the parent continue accepting clients
      else
      {
         // The parent does not communicate with this client
         close(nfd);
      }
   }
}

int main(void)
{
   // Register the handler used when child processes terminate
   signal(SIGCHLD, handle_sigchld);

   // Create the listening server socket
   int fd = create_service(PORT);

   // Check whether the server socket was created successfully
   if (fd == -1)
   {
      perror("create_service");
      exit(1);
   }

   // Display the selected server port
   printf("Listening on port: %d\n", PORT);

   // Accept and process client connections
   run_service(fd);

   // Close the listening server socket
   close(fd);

   return 0;
}
