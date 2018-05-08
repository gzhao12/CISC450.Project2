/* udp_server.c */
/* Originally Programmed by Adarsh Sethi */
/* February 21, 2018 */
/* Updated by Grant Zhao and Konark Christian */
/* May 8, 2018 */

#include <ctype.h>          /* for toupper */
#include <stdio.h>          /* for standard I/O functions */
#include <stdlib.h>         /* for exit */
#include <string.h>         /* for memset */
#include <sys/socket.h>     /* for socket, sendto, and recvfrom */
#include <netinet/in.h>     /* for sockaddr_in */
#include <unistd.h>         /* for close */
#include <time.h>           /* for timing/timeout */
#include <stdbool.h>
#include <math.h>
#include <assert.h>

#define STRING_SIZE 1024

/* SERV_UDP_PORT is the port number on which the server listens for
 incoming messages from clients. You should change this to a different
 number to prevent conflicts with others in the class. */

#define SERV_UDP_PORT 46792

#define MAX_CHAR_PER_SEND 80
#define PACKET_PRINT_NUM 1

struct packet{
   short unsigned int sequence_number, count;
   char payLoadLine[STRING_SIZE];
};

struct ACK {
   short unsigned int sequence_number;
};

struct time_t {
   
};

char * nextLine(FILE * file, char * copiedLine) {
   memset(copiedLine, '\n', MAX_CHAR_PER_SEND);
   return fgets(copiedLine, MAX_CHAR_PER_SEND + 1, file);
}

int main(int argc, char * argv[]) {
   int TIMEOUT;
   if (argc == 2) {
      TIMEOUT = atoi(argv[1]);
      assert(TIMEOUT >= 1);
      assert(TIMEOUT <= 10);
   } else {
      exit(1);
   }
   
   int sock_server;  /* Socket on which server listens to clients */
   
   struct sockaddr_in server_addr;  /* Internet address structure that
                                     stores server address */
   unsigned short server_port;  /* Port number used by server (local port) */
   
   struct sockaddr_in client_addr;  /* Internet address structure that
                                     stores client address */
   unsigned int client_addr_len;  /* Length of client address structure */
   
   struct packet packetReceived;
   struct ACK ACKreceived;
   
   struct timeval timeout;
   
   char fileName[STRING_SIZE];  /* receive message */
   char sendLine[MAX_CHAR_PER_SEND]; /* send message */
   
   int counter = 1;                 /* number of packets transmitted */
   int totalBytes = 0;              /* total number of data bytes transmitted */
   int retransCounter = 0;          /* total number of retransmissions */
   int totalTransCounter = 0;       /* total number of transmissions */
   int timeoutCounter = 0;          /* number of timeouts */
   int ACKcounter = 0;              /* number of ACKs received */

   
   unsigned int msg_len;  /* length of message */
   int bytes_sent, bytes_recd; /* number of bytes sent or received */
   
   unsigned int i;  /* temporary loop variable */
   
   /* sets the timeout time */
   
   if(TIMEOUT >= 6) {
      timeout.tv_sec = pow(10, TIMEOUT - 6);
      timeout.tv_usec = 0;
   } else {
      timeout.tv_sec = 0;
      timeout.tv_usec = pow(10, TIMEOUT);
   }
   
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
      while(nextLine(requestedFile, sendLine) != NULL ) {
         totalBytes += strlen(sendLine);
         
         struct packet packetSent = {htons(counter%2), htons(strlen(sendLine))};
         strcpy(packetSent.payLoadLine, sendLine);
         
         /* send message */
         bytes_sent = sendto(sock_server, &packetSent, strlen(packetSent.payLoadLine) + 4, 0, (struct sockaddr*) &client_addr, client_addr_len);
         
         if(counter % 2 == PACKET_PRINT_NUM)
            printf("Packet %d transmitted with %lu data bytes \n", counter % 2, strlen(sendLine));
         
         
         while(ntohs(ACKreceived.sequence_number) != counter % 2) {
            setsockopt(sock_server, SOL_SOCKET, SO_RCVTIMEO, (const void *) &timeout, sizeof(timeout));
            
            bytes_recd = recvfrom(sock_server, &ACKreceived, 2, 0, (struct sockaddr *) &client_addr, &client_addr_len);
            
            if (bytes_recd <= 0) {
               if(counter % 2 == PACKET_PRINT_NUM)
                  printf("Timeout expired for packet numbered %d \n", counter % 2);
               
                  bytes_sent = sendto(sock_server, &packetSent, strlen(packetSent.payLoadLine) + 4, 0, (struct sockaddr*) &client_addr, client_addr_len);
               
               if(counter % 2 == PACKET_PRINT_NUM)
                  printf("Packet %d retransmitted with %lu data bytes \n", counter % 2, strlen(sendLine));
               
               timeoutCounter++;
               totalTransCounter++;
               retransCounter++;
            } else {
               ACKcounter++;
               if(ntohs(ACKreceived.sequence_number) == PACKET_PRINT_NUM)
                  printf("ACK %d received \n", ntohs(ACKreceived.sequence_number));
               
               if (ntohs(ACKreceived.sequence_number) != counter % 2) {
                  continue;
               } else {
                  counter++;
                  totalTransCounter++;
                  break;
               }
            }
         }
      }
      
      /* End of Transmission Packet */
      struct packet packetSent = {htons(counter%2), htons(0)};
      bytes_sent = sendto(sock_server, &packetSent, 4, 0,
                          (struct sockaddr*) &client_addr, client_addr_len);
      printf("End of Transmission Packet with sequence number %d transmitted with %d data bytes\n\n", counter % 2, 0);
      
      printf("Number of data packets transmitted: %d\n", counter - 1);
      printf("Total number of data bytes transmitted: %d\n", totalBytes);
      printf("Total number of retransmissions: %d\n", retransCounter);
      printf("Total number of transmissions: %d\n", totalTransCounter);
      printf("Total number of ACKs received: %d\n", ACKcounter);
      printf("Total number of timeouts: %d\n", timeoutCounter);

      counter = 1;
      totalBytes = 0;
      retransCounter = 0;
      totalTransCounter = 0;
      ACKcounter = 0;
      timeoutCounter = 0;
      
      fclose(requestedFile);
      break;
   }
}
