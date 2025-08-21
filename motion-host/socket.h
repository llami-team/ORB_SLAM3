#ifndef SOCKET_H
#define SOCKET_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a socket server and binds it to the specified port
 * @param port The port number to bind to
 * @return The socket file descriptor on success, -1 on failure
 */
int socket_create(int port);

/**
 * Accepts a connection from a client
 * @return The client socket file descriptor on success, -1 on failure
 */
int accept_connection();

/**
 * Sets a callback function to be called when a message is received
 * @param listener The callback function that takes a buffer and its length
 * @return 0 on success, -1 on failure
 */
int set_message_listener(void (*listener)(char*, int));

/**
 * Sends a message to the connected client
 * @param message The message buffer to send
 * @param length The length of the message
 * @return The number of bytes sent on success, -1 on failure
 */
int send_message(const char* message, int length);

/**
 * Closes the socket connection and cleans up resources
 */
void socket_close();

#ifdef __cplusplus
}
#endif

#endif // SOCKET_H
