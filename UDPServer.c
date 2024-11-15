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
    char hostBuf[NI_MAXHOST], serviceBuf[NI_MAXSERV];
    
    int rtnVal = getnameinfo((struct sockaddr *) &clntAddr, clntAddrLen, &hostBuf, sizeof(hostBuf), 
        &serviceBuf, sizeof(serviceBuf), NI_NUMERICHOST | NI_NUMERICSERV);
    if (rtnVal < 0)
      dieWithError("getnameinfo() failed");
    
    // Format message in order to display it
    int version = msgptr->flags >> 4;
    int flags = msgptr->flags & 0xf;
    int magic = ntohs(msgptr->magic); //convert byte order from message
    int length = ntohs(msgptr->length);
    buffer[numBytesRcvd] = '\0';
    int id_first, id_second, id_third, id_fourth;

    // Separate the four bytes of id
    id_first = (msgptr->xactionid & 0xff000000) >> 24;
    id_second = (msgptr->xactionid& 0xff0000) >> 16;
    id_third = (msgptr->xactionid & 0xff00) >> 8;
    id_fourth = (msgptr->xactionid& 0xff);

    /* Display values of the received message */
    printf("magic=%d\n", magic);
    printf("length=%d\n", length);
    printf("xid=%x %x %x %x\n", id_first, id_second, id_third, id_fourth); 
    printf("version=%d\n", version);
    printf("flags=%x\n", flags);
    printf("result=%d\n", msgptr->result);
    printf("port=%d\n", msgptr->port);
    printf("Address=%s\n", &hostBuf);
    printf("Port=%s\n", &serviceBuf);
    printf("variable part=%s\n", &buffer[offset]);
    
    /* Check for formatting errors */
    char err_buf[MAXMSGLEN];
    char err_msg[MAXMSGLEN];
    int errBufLen;
    int errMsgLen;

    if (magic != 270){
      msgptr->flags |=  0x1; //Set error bit in flag byte of message
      strcpy(err_msg, "bad magic number.");
      errMsgLen = strlen(err_msg);
      memcpy(&err_buf[0], err_msg, errMsgLen);
    }
    if ((length < 16) || (length > 512)){
      msgptr->flags |=  0x1;
      strcpy(err_msg, "length too small or large.");
      errBufLen = strlen(err_buf);
      errMsgLen = strlen(err_msg);
      memcpy(&err_buf[errBufLen], err_msg, errMsgLen);
    }
    if(length != numBytesRcvd){
      msgptr->flags |=  0x1;
      strcpy(err_msg, "wrong length value.");
      errBufLen = strlen(err_buf);
      errMsgLen = strlen(err_msg);
      memcpy(&err_buf[errBufLen], err_msg, errMsgLen);
    }
    if ((msgptr->flags & 0x2) == 0){
      msgptr->flags |=  0x1;
      strcpy(err_msg, "bad request/response bit.");
      errBufLen = strlen(err_buf);
      errMsgLen = strlen(err_msg);
      memcpy(&err_buf[errBufLen], err_msg, errMsgLen);
    }
    if (msgptr->result != 0){
      msgptr->flags |=  0x1;
      strcpy(err_msg, "incorrect result value.");
      errBufLen = strlen(err_buf);
      errMsgLen = strlen(err_msg);
      memcpy(&err_buf[errBufLen], err_msg, errMsgLen);
    }
    if (msgptr->port != 0){
      msgptr->flags |=  0x1;
      strcpy(err_msg, "incorrect port value.");
      errBufLen = strlen(err_buf);
      errMsgLen = strlen(err_msg);
      memcpy(&err_buf[errBufLen], err_msg, errMsgLen);
    }

    /* YOUR CODE HERE:  construct Response message in buffer, display it */
    memset(&buffer[offset], '\0', (numBytesRcvd - offset)); //Clear variable part of message
    char srvr_msg[MAXMSGLEN];
    int addrLen = strlen(hostBuf); //length not counting null character
    int portLen = strlen(serviceBuf);

    // Write error message to variable part of response message
    if ((msgptr->flags & 0x1) == 1){
      errBufLen = strlen(err_buf);
      memcpy(&srvr_msg[0], err_buf, errBufLen);
    }
    else {
      /* construct cookie using: IP Address, Port Number, Version, Length of request message */
      memcpy(&srvr_msg[0], hostBuf, addrLen);
      memcpy(&srvr_msg[addrLen], serviceBuf, portLen);
      sprintf(&srvr_msg[addrLen + portLen], "%d|%d", version, length); //used to convert the two integers to strings
    }

    int msgLen = strlen(srvr_msg) + sizeof(header_t); //total response message size
    memcpy(&buffer[offset], srvr_msg, strlen(srvr_msg));

    ssize_t numBytesSent = sendto(sock, buffer, msgLen, 0,
        (struct sockaddr *) &clntAddr, sizeof(clntAddr));
    if (numBytesSent < 0)
      dieWithSystemError("sendto() failed)");
  }
  // NOT REACHED
}
