#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/select.h>


#ifdef __cplusplus
extern "C" {
#endif

int server_socket;
int client_socket = -1;
static bool running = false;
static pthread_t listener_thread;
static void (*message_callback)(char*, int) = NULL;

// create socket and bind to port
int socket_create(int port) {
  struct sockaddr_in address;

  server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  // Creating socket file descriptor
  if (server_socket == -1) {
    return -1;
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  // Forcefully attaching socket to the port 8080
  if (bind(server_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
    return -1;
  }
  if (listen(server_socket, 3) < 0) {
    return -1;
  }

  return server_socket;
}

int accept_connection() {
  struct sockaddr_in client_address;
  socklen_t client_address_len = sizeof(client_address);

  // Wait for incoming connection with a short timeout so we can react to shutdown
  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(server_socket, &rfds);

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 500000; // 500 ms

  int sel = select(server_socket + 1, &rfds, NULL, NULL, &tv);
  if (sel == 0) {
    // Timeout; let caller loop/check stop flag
    return -1;
  } else if (sel < 0) {
    // Error
    return -2;
  }

  struct timeval timeout;
  timeout.tv_sec = 5;
  timeout.tv_usec = 0;

  int client = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
  if (client < 0) {
    return -2;
  }

  if (setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
    close(client);
    return -1;
  }

  if (setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
    close(client);
    return -1;
  }
  
  // Store the client socket for later use
  client_socket = client;
  printf("Client connected\n");

  return client;
}

// Function to receive messages from the client
void* receive_messages(void* arg) {
  char buffer[1024];
  int bytes_read;
  
  while (running) {
    if (client_socket < 0) {
      // No client connected, sleep and try again
      usleep(100000); // 100ms
      continue;
    }
    
    memset(buffer, 0, sizeof(buffer));
    ssize_t recv_result = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    bytes_read = (int)recv_result;
    
    if (bytes_read > 0) {
      // Call the message callback if it's set
      if (message_callback != NULL) {
        message_callback(buffer, bytes_read);
      }
    } else if (bytes_read == 0) {
      // Client disconnected
      close(client_socket);
      client_socket = -1;
      printf("Client disconnected\n");
    } else {
      // Error or timeout occurred
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        close(client_socket);
        client_socket = -1;
        printf("Socket error: %d\n", errno);
      }
    }
  }
  
  return NULL;
}

// Set a callback function to be called when a message is received
int set_message_listener(void (*listener)(char*, int)) {
  if (listener == NULL) {
    return -1;
  }
  
  message_callback = listener;
  
  // Start the listener thread if it's not already running
  if (!running) {
    running = true;
    if (pthread_create(&listener_thread, NULL, receive_messages, NULL) != 0) {
      running = false;
      return -1;
    }
  }
  
  return 0;
}

// Send a message to the connected client
int send_message(const char* message, int length) {
  if (client_socket < 0) {
    return -1; // No client connected
  }
  
  ssize_t send_result = send(client_socket, message, length, 0);
  return (int)send_result;
}

// Close the socket connection
void socket_close() {
  running = false;
  
  if (listener_thread) {
    pthread_join(listener_thread, NULL);
  }
  
  if (client_socket >= 0) {
    close(client_socket);
    client_socket = -1;
  }
  
  if (server_socket >= 0) {
    // Gracefully shutdown to interrupt any blocking accept()
    shutdown(server_socket, SHUT_RDWR);
    close(server_socket);
    server_socket = -1;
  }
}

#ifdef __cplusplus
} // extern "C"
#endif