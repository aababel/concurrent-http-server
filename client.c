#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>

#define PORT 28281
#define BUFFER_SIZE 1024

#define MIN_ARGS 2
#define MAX_ARGS 2
#define SERVER_ARG_IDX 1

#define USAGE_STRING "usage: %s <server address>\n"

// Validate the command-line arguments
void validate_arguments(int argc, char *argv[])
{
   // Check whether the program name is available
   if (argc == 0)
   {
      fprintf(stderr, USAGE_STRING, "client");
      exit(EXIT_FAILURE);
   }

   // Require exactly one server address
   if (argc < MIN_ARGS || argc > MAX_ARGS)
   {
      fprintf(stderr, USAGE_STRING, argv[0]);
      exit(EXIT_FAILURE);
   }
}

// Send one request and print the server response
void send_request(int fd)
{
   // Store the request entered by the user
   char *line = NULL;

   // Store the size of the getline buffer
   size_t size = 0;

   // Store the number of characters in the request
   ssize_t num;

   // Store blocks received from the server
   char buffer[BUFFER_SIZE];

   // Store the number of bytes received from the server
   ssize_t bytes_read;

   // Read one request from standard input
   num = getline(&line, &size, stdin);

   // Stop if no request was entered
   if (num < 0)
   {
      free(line);
      return;
   }

   // Send the request to the server
   if (write(fd, line, num) == -1)
   {
      perror("write");
      free(line);
      return;
   }

   // Read and print the complete response from the server
   while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
   {
      // Write the response block to standard output
      if (write(STDOUT_FILENO, buffer, bytes_read) == -1)
      {
         perror("write");
         break;
      }
   }

   // Report an error while reading the response
   if (bytes_read == -1)
   {
      perror("read");
   }

   // Release the buffer allocated by getline
   free(line);
}

// Connect to the specified server
int connect_to_server(struct hostent *host_entry)
{
   // Store the connected socket descriptor
   int fd;

   // Store the server address information
   struct sockaddr_in their_addr;

   // Create a TCP socket
   if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
   {
      return -1;
   }

   // Set the socket address family
   their_addr.sin_family = AF_INET;

   // Set the server port
   their_addr.sin_port = htons(PORT);

   // Set the server IP address
   their_addr.sin_addr =
      *((struct in_addr *)host_entry->h_addr);

   // Connect the socket to the server
   if (connect(fd,
      (struct sockaddr *)&their_addr,
      sizeof(struct sockaddr)) == -1)
   {
      close(fd);
      perror("connect");
      return -1;
   }

   return fd;
}

// Look up the server using its hostname
struct hostent *gethost(char *hostname)
{
   // Store the server host information
   struct hostent *host_entry;

   // Look up the specified server
   host_entry = gethostbyname(hostname);

   // Report a failed host lookup
   if (host_entry == NULL)
   {
      herror(hostname);
   }

   return host_entry;
}

int main(int argc, char *argv[])
{
   // Validate the supplied server address
   validate_arguments(argc, argv);

   // Look up the specified server
   struct hostent *host_entry =
      gethost(argv[SERVER_ARG_IDX]);

   // Continue only when the server was found
   if (host_entry != NULL)
   {
      // Connect to the server
      int fd = connect_to_server(host_entry);

      // Send one request after connecting successfully
      if (fd != -1)
      {
         send_request(fd);
         close(fd);
      }
   }

   return 0;
}
