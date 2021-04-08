#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include "UDPCookie.h"

#define TIMEOUT_PERIOD 3
#define LIMIT 5

sig_atomic_t sigalarm_flag;

void sigalarm_handler(){

  sigalarm_flag = 1;
  printf("In signal handler\n"); fflush(stdout);

}

int main(int argc, char *argv[]) {
  char msgBuf[MAXMSGLEN];
  int msgLen = 0; // quiet the compiler
  struct sigaction sigalarm_action;
  sigset_t mask, prev;

  if (argc != 3) // Test for correct number of arguments
    dieWithError("Parameter(s): <Server Address/Name> <Server Port/Service>");

  char *server = argv[1];     // First arg: server address/name
  char *servPort = argv[2];

  /* Install SIGALRM signal handler */
  sigalarm_action.sa_handler = sigalarm_handler;
  if(sigemptyset(&sigalarm_action.sa_mask) < 0) dieWithError("\nsig empty set error");
  if (sigaction(SIGALRM, &sigalarm_action, NULL) < 0) dieWithError("\nsig action error");

  /* Block all signals */
  if (sigaddset(&mask, SIGALRM) < 0) dieWithError("\nsig add set error");
  if (sigemptyset(&prev) < 0) dieWithError("\nsig empty set error");
  if (sigprocmask(SIG_BLOCK, &mask, &prev) < 0) dieWithError("\nsig proc mask error");

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
 char *userMsg = "wtja222";
  
  header_t *msgptr = (header_t *) msgBuf;
  msgptr->magic = htons(270);
  msgptr->length = htons(12 + strlen(userMsg));
  msgptr->xactionid = 0xdeadbeef;
  msgptr->flags = 0x2A;
  msgptr->result = 0;
  msgptr->port = 0;  

  msgLen = strlen(userMsg) + sizeof(header_t);
  int offset = sizeof(header_t);
  memcpy(&msgBuf[offset], userMsg, strlen(userMsg));

ssize_t numBytes = sendto(sock, msgBuf, msgLen, 0, servAddr->ai_addr,
			    servAddr->ai_addrlen);
  if (numBytes < 0)
    dieWithSystemError("sendto() failed");
  else if (numBytes != msgLen)
    dieWithError("sendto() returned unexpected number of bytes");

  /* YOUR CODE HERE - receive, parse and display response message */
  struct sockaddr_storage srvrAddr;
  socklen_t srvrAddrLen = sizeof(struct sockaddr_storage);
  int numBytesRcvd = 0;

  sigalarm_flag = 0;       
  int count = 0;
  
  do {
    
    alarm(TIMEOUT_PERIOD);
    while ((sigalarm_flag == 0)){
      
      if ((numBytesRcvd = recvfrom(sock, msgBuf, MAXMSGLEN, 0, &srvrAddr, &srvrAddrLen)) < 0) {
        if (errno = EINTR){
          numBytes = sendto(sock, msgBuf, msgLen, 0, servAddr->ai_addr,
            servAddr->ai_addrlen);
          if (numBytes < 0)
            dieWithSystemError("sendto() failed");
          else if (numBytes != msgLen)
            dieWithError("sendto() returned unexpected number of bytes");   

          count += 1;
        }
        else{
          dieWithSystemError("recvfrom() failed");
        }
      }
      sigsuspend(&prev);
    }

    sigalarm_flag = 0;

} while ((count != LIMIT) && (numBytesRcvd < 0));
  
  printf("Num Bytes received = %d\n", numBytesRcvd);
  msgBuf[numBytesRcvd] = '\0';
 
  printf("Result = %d\n", msgBuf[9]);
  printf("Response message: %s\n", &msgBuf[offset]);

  freeaddrinfo(servAddr);

  close(sock);
  exit( 0);
}