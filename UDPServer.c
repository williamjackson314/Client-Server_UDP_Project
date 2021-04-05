#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "UDPCookie.h"

#define ERRLEN 128

int main(int argc, char *argv[]) {

  if (argc != 2) // Test for correct number of arguments
    dieWithError("Parameter(s): <Server Port #>");

  char *service = argv[1]; // First arg:  local port/service

  // Construct the server address structure
  struct addrinfo addrCriteria;                   // Criteria for address
  memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zero out structure
  addrCriteria.ai_family = AF_INET;               // We want IPv4 only
  addrCriteria.ai_flags = AI_PASSIVE;             // Accept on any address/port
  addrCriteria.ai_socktype = SOCK_DGRAM;          // Only datagram socket
  addrCriteria.ai_protocol = IPPROTO_UDP;         // UDP socket

  struct addrinfo *servAddr; // List of server addresses
  int rtnVal = getaddrinfo(NULL, service, &addrCriteria, &servAddr);
  if (rtnVal != 0) {
    char error[ERRLEN];
    if (snprintf(error,ERRLEN,"getaddrinfo() failed: %s",
		 gai_strerror(rtnVal)) < 0) // recursive?!
      dieWithSystemError("snprintf() failed");
    dieWithError(error);
  }

  // Create socket for incoming connections
  int sock = socket(servAddr->ai_family, servAddr->ai_socktype,
      servAddr->ai_protocol);
  if (sock < 0)
    dieWithSystemError("socket() failed");

  // Bind to the local address/port
  if (bind(sock, servAddr->ai_addr, servAddr->ai_addrlen) < 0)
    dieWithSystemError("bind() failed");

  // Free address list allocated by getaddrinfo()
  freeaddrinfo(servAddr);

  for (;;) { // Run forever
    struct sockaddr_storage clntAddr; // Client address
    // Set Length of client address structure (in-out parameter)
    socklen_t clntAddrLen = sizeof(clntAddr);

    // Block until receive message from a client
    char buffer[MAXMSGLEN]; // I/O buffer
    // Size of received message
    ssize_t numBytesRcvd = recvfrom(sock, buffer, MAXMSGLEN, 0,
        (struct sockaddr *) &clntAddr, &clntAddrLen);
    if (numBytesRcvd < 0)
      dieWithSystemError("recvfrom() failed");

    /* YOUR CODE HERE:  parse & display incoming request message */
    header_t *msgptr = (header_t *) buffer;
    int offset = sizeof(header_t);
    
    /* Format message in order to display it */
    int version = msgptr->flags >> 4;
    int flags = msgptr->flags & 0xff;
    buffer[MAXMSGLEN] = '\0';

    /* Display values of the received message */
    printf("magic=%d\n", msgptr->magic);
    printf("length=%d\n", msgptr->length);
    printf("xid=%x\n", msgptr->xactionid); //Seperate into 4 bytes with spaces in between
    printf("version=%d\n", version);
    printf("flags=%x\n", flags);
    printf("result=%d\n", msgptr->result);
    printf("port=%d\n", msgptr->port);
    printf("variable part=%s\n", &buffer[offset]);


    /* YOUR CODE HERE:  construct Response message in buffer, display it */
    char *srvr_msg;

    if ((msgptr->flags & 0x1) == 1){
      srvr_msg = 'Errors Detected';
      buffer[offset] = srvr_msg;
    }


    ssize_t numBytesSent = sendto(sock, buffer, numBytesRcvd, 0,
        (struct sockaddr *) &clntAddr, sizeof(clntAddr));
    if (numBytesSent < 0)
      dieWithSystemError("sendto() failed)");
  }
  // NOT REACHED
}
