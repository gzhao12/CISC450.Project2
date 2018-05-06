/* udp_server.c */
/* Programmed by Adarsh Sethi */
/* February 21, 2018 */

#include <ctype.h>          /* for toupper */
#include <stdio.h>          /* for standard I/O functions */
#include <stdlib.h>         /* for exit */
#include <string.h>         /* for memset */
#include <sys/socket.h>     /* for socket, sendto, and recvfrom */
#include <netinet/in.h>     /* for sockaddr_in */
#include <unistd.h>         /* for close */

#define STRING_SIZE 1024

/* SERV_UDP_PORT is the port number on which the server listens for
   incoming messages from clients. You should change this to a different
   number to prevent conflicts with others in the class. */

#define SERV_UDP_PORT 46792
#define MAX_CHAR_PER_SEND 80
#define PACKET_PRINT_NUM 5

struct packet{
   short unsigned int sequence_number, count;
   char payLoadLine[STRING_SIZE];
};

struct ACK {
   short unsigned int sequence_number;
};

char * nextLine(FILE * file, char * copiedLine) {
   return fgets(copiedLine, MAX_CHAR_PER_SEND + 1, file);
}

int main(void) {

   int sock_server;  /* Socket on which server listens to clients */

   struct sockaddr_in server_addr;  /* Internet address structure that
                                        stores server address */
   unsigned short server_port;  /* Port number used by server (local port) */

   struct sockaddr_in client_addr;  /* Internet address structure that
                                        stores client address */
   unsigned int client_addr_len;  /* Length of client address structure */
   
   struct packet packetReceived;
   
   char fileName[STRING_SIZE];  /* receive message */
   char sendLine[MAX_CHAR_PER_SEND]; /* send message */
   
   int counter = 1;
   int totalBytes = 0;
   
   unsigned int msg_len;  /* length of message */
   int bytes_sent, bytes_recd; /* number of bytes sent or received */
   
   unsigned int i;  /* temporary loop variable */

   /* open a socket */

   if ((sock_server = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
      perror("Server: can't open datagram socket\n");
      exit(1);
   }

   /* initialize server address information */

   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   server_addr.sin_addr.s_addr = htonl (INADDR_ANY);  /* This allows choice of
                                        any host interface, if more than one
                                        are present */
   server_port = SERV_UDP_PORT; /* Server will listen on this port */
   server_addr.sin_port = htons(server_port);

   /* bind the socket to the local server port */

   if (bind(sock_server, (struct sockaddr *) &server_addr,
                                    sizeof (server_addr)) < 0) {
      perror("Server: can't bind to local address\n");
      close(sock_server);
      exit(1);
   }

   /* wait for incoming messages in an indefinite loop */

   printf("Waiting for incoming messages on port %hu\n\n", 
                           server_port);

   client_addr_len = sizeof (client_addr);

   for (;;) {
      bytes_recd = recvfrom(sock_server, &packetReceived, STRING_SIZE + 4, 0,
                     (struct sockaddr *) &client_addr, &client_addr_len);
      
//      printf("Received file name is: %s\n     with length %d\n\n", packetReceived.payLoadLine, ntohs(packetReceived.count));
      
      /* File IO setup */
      
      FILE * requestedFile = fopen(packetReceived.payLoadLine, "r");
      
      /* no such file */
      if (requestedFile == NULL) {
         struct packet packetSent = {htons(counter), htons(0)};
         bytes_sent = sendto(sock_server, &packetSent, 4, 0,
                                         (struct sockaddr*) &client_addr, client_addr_len);
         perror("There is no file with this name");
         return(-1);
      }
      
      /* prepare the message to send */
      while(nextLine(requestedFile,sendLine) != NULL ) {
         totalBytes += strlen(sendLine);
         
         struct packet packetSent = {htons(counter%2), htons(strlen(sendLine))};
         strcpy(packetSent.payLoadLine, sendLine);
         
         /* send message */
         bytes_sent = sendto(sock_server, &packetSent, strlen(packetSent.payLoadLine) + 4, 0,
                             (struct sockaddr*) &client_addr, client_addr_len);
         
         /* Test for byte sent */
         printf("Data bytes sent: %d \n", bytes_sent);
         
         counter++;
      }
   }
}
