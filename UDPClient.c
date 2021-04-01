#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include "UDPCookie.h"

#define TIMEOUT 3
#define LIMIT 5

int main(int argc, char *argv[]) {
  char msgBuf[MAXMSGLEN];
  int msgLen = 0; // quiet the compiler

  if (argc != 3) // Test for correct number of arguments
    dieWithError("Parameter(s): <Server Address/Name> <Server Port/Service>");

  char *server = argv[1];     // First arg: server address/name
  char *servPort = argv[2];

  // Tell the system what kind(s) of address info we want
  struct addrinfo addrCriteria;                   // Criteria for address match
  memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zero out structure
  addrCriteria.ai_family = AF_INET;               // IPv4 only
  // For the following fields, a zero value means "don't care"
  addrCriteria.ai_socktype = SOCK_DGRAM;          // Only datagram sockets
  addrCriteria.ai_protocol = IPPROTO_UDP;         // Only UDP protocol

  // Get address(es)
  struct addrinfo *servAddr;      // List of server addresses
  int rtnVal = getaddrinfo(server, servPort, &addrCriteria, &servAddr);
  if (rtnVal != 0) {
    char error[MAXMSGLEN];
    snprintf(error,MAXMSGLEN,"getaddrinfo() failed: %s",gai_strerror(rtnVal));
    dieWithError(error);
  }
  /* Create a datagram socket */
  int sock = socket(servAddr->ai_family, servAddr->ai_socktype,
		    servAddr->ai_protocol); // Socket descriptor for client
  if (sock < 0)
    dieWithSystemError("socket() failed");

  /* YOUR CODE HERE - construct Request message in msgBuf               */
  /* msgLen must contain the size (in bytes) of the Request msg         */
  char *userMsg = "wtja222@wtja222.cs.uky.edu";
  
  header_t client;
  client.magic = htons(270);
  client.length = htons(12 + sizeof(userMsg));
  client.xactionid = 0xdeadbeef;
  client.flags = 0x28;
  client.result = 0;
  client.port = 0;

  int offset = sizeof(client);
  memcpy(&userMsg, &msgBuf[offset], sizeof(userMsg));
  memcpy(&client, &msgBuf, sizeof(client));

  msgLen = sizeof(userMsg);

  ssize_t numBytes = sendto(sock, msgBuf, msgLen, 0, servAddr->ai_addr,
			    servAddr->ai_addrlen);
  if (numBytes < 0)
    dieWithSystemError("sendto() failed");
  else if (numBytes != msgLen)
    dieWithError("sendto() returned unexpected number of bytes");

  /* YOUR CODE HERE - receive, parse and display response message */
  struct sockaddr_storage srvrAddr;
  socklen_t srvrAddrLen = sizeof(srvrAddr);

  ssize_t numBytesRcvd = recvfrom(sock, msgBuf, MAXMSGLEN, 0, &srvrAddr, &srvrAddrLen); //not receiving anything from server

  /* 
  Need to implement retransmission logic 
  
  if (numBytesRcvd == 0){
    numBytes = sendto(sock, msgBuf, msgLen, 0, servAddr->ai_addr,
			    servAddr->ai_addrlen);
    if (numBytes < 0)
      dieWithSystemError("sendto() failed");
    else if (numBytes != msgLen)
      dieWithError("sendto() returned unexpected number of bytes");
  }
  */
  if (numBytesRcvd <0) dieWithError("recvfrom() failed");

  msgBuf[numBytesRcvd] = '0';
 
  printf("Response message: %c", msgBuf[offset]);

  freeaddrinfo(servAddr);

  close(sock);
  exit(0);
}
