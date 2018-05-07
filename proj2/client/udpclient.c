/* udp_client.c */ 
/* Originally Programmed by Adarsh Sethi */
/* February 21, 2018 */
/* Updated by Grant Zhao and Konark Christian */
/* May 8, 2018 */

#include <stdio.h>          /* for standard I/O functions */
#include <stdlib.h>         /* for exit */
#include <string.h>         /* for memset, memcpy, and strlen */
#include <netdb.h>          /* for struct hostent and gethostbyname */
#include <sys/socket.h>     /* for socket, sendto, and recvfrom */
#include <netinet/in.h>     /* for sockaddr_in */
#include <unistd.h>         /* for close */
#include <time.h>
#include <assert.h>

#define STRING_SIZE 1024
#define MAX_CHAR_PER_SEND 80  /* constant for the max characters allowed to be copied from a line of text */
#define MAX_PACKET_RECEIVED_SIZE 84
#define PACKET_PRINT_NUM 1


struct packet {
   short unsigned int sequence_number, count;
   char payloadLine[STRING_SIZE];
};

struct ACK {
   short unsigned int sequence_number;
};

int SimulateLoss(double PACKET_LOSS_RATE) {
   double r = (double)rand()/(double)RAND_MAX;
   if(r < PACKET_LOSS_RATE) {
      return 1;
   } else {
      return 0;
   }
}

int SimulateACKLoss(double ACK_LOSS_RATE) {
   double r = (double)rand()/(double)RAND_MAX;
   if(r < ACK_LOSS_RATE) {
      return 1;
   } else {
      return 0;
   }
}

int main(int argc, char * argv[]) {
   double PACKET_LOSS_RATE;
   double ACK_LOSS_RATE;
   if (argc == 3) {
      PACKET_LOSS_RATE = atof(argv[1]);
      ACK_LOSS_RATE = atof(argv[2]);
      
      assert(PACKET_LOSS_RATE >= 0);
      assert(ACK_LOSS_RATE >= 0);
      assert(PACKET_LOSS_RATE <= 1);
      assert(ACK_LOSS_RATE <= 1);
   } else {
      exit(1);
   }
   
   int sock_client;  /* Socket used by client */ 

   struct sockaddr_in client_addr;  /* Internet address structure that
                                        stores client address */
   unsigned short client_port;  /* Port number used by client (local port) */

   struct sockaddr_in server_addr;  /* Internet address structure that
                                        stores server address */
   struct hostent * server_hp;      /* Structure to store server's IP
                                        address */
   char server_hostname[STRING_SIZE]; /* Server's hostname */
   unsigned short server_port;  /* Port number used by server (remote port) */

   char fileName[STRING_SIZE];  /* send message */
   
   struct packet packetReceived;  /* received packet */
   
   int ACKcounter = 1;
   int expectedSeqNum = 1;          /* number of data packets received successfully */
   int totalBytes = 0;              /* total number of data bytes received which are delivered to user */
   int duplicateCounter = 0;        /* total number of duplicate data packets received */
   int lossCounter = 0;             /* number of data packets received but dropped due to loss */
   int totalCounter = 0;            /* total number of data packets received */
   int noLossACKCounter = 0;        /* number of ACKs transmitted without loss */
   int droppedACKCounter = 0;       /* number of ACKs generated but dropped due to loss */
   int totalACKCounter = 0;         /* total number of ACKs generated */
   
   srand(time(NULL));
   
   unsigned int msg_len;  /* length of message */
   int bytes_sent, bytes_recd; /* number of bytes sent or received */
  
   /* open a socket */

   if ((sock_client = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
      perror("Client: can't open datagram socket\n");
      exit(1);
   }

   /* Note: there is no need to initialize local client address information
            unless you want to specify a specific local port.
            The local address initialization and binding is done automatically
            when the sendto function is called later, if the socket has not
            already been bound. 
            The code below illustrates how to initialize and bind to a
            specific local port, if that is desired. */

   /* initialize client address information */

   client_port = 0;   /* This allows choice of any available local port */

   /* Uncomment the lines below if you want to specify a particular 
             local port: */
   /*
   printf("Enter port number for client: ");
   scanf("%hu", &client_port);
   */

   /* clear client address structure and initialize with client address */
   memset(&client_addr, 0, sizeof(client_addr));
   client_addr.sin_family = AF_INET;
   client_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* This allows choice of
                                        any host interface, if more than one 
                                        are present */
   client_addr.sin_port = htons(client_port);

   /* bind the socket to the local client port */

   if (bind(sock_client, (struct sockaddr *) &client_addr,
                                    sizeof (client_addr)) < 0) {
      perror("Client: can't bind to local address\n");
      close(sock_client);
      exit(1);
   }

   /* end of local address initialization and binding */

   /* initialize server address information */

   printf("Enter hostname of server: ");
   scanf("%s", server_hostname);
   if ((server_hp = gethostbyname(server_hostname)) == NULL) {
      perror("Client: invalid server hostname\n");
      close(sock_client);
      exit(1);
   }
   printf("Enter port number for server: ");
   scanf("%hu", &server_port);

   /* Clear server address structure and initialize with server address */
   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   memcpy((char *)&server_addr.sin_addr, server_hp->h_addr,
                                    server_hp->h_length);
   server_addr.sin_port = htons(server_port);

   /* user interface */

   printf("Please input a file name:\n");
   scanf("%s", fileName);
   msg_len = strlen(fileName) + 1;
   
   /* create packet to send to server */
   struct packet packetSent = {htons(0), htons(msg_len)};
   strcpy(packetSent.payloadLine, fileName);
   
   bytes_sent = sendto(sock_client, &packetSent, msg_len + 4, 0,
                       (struct sockaddr *) &server_addr, sizeof (server_addr));
   
   /* Tests for packet byte_sent */
//   printf("\nSize of packet: %lu\n", sizeof(packetSent));
//   printf("Packet bytes sent: %d\n\n", bytes_sent);
   
   /* local file access */
   FILE * receivedFile = fopen(fileName, "w");

   /* get response from server */
   while(1) {
      bytes_recd = recvfrom(sock_client, &packetReceived, MAX_PACKET_RECEIVED_SIZE, 0, (struct sockaddr *) 0, (unsigned int *) 0);
      if (ntohs(packetReceived.count) > 0) {
         if (SimulateLoss(PACKET_LOSS_RATE) == 1) {
            if(ntohs(packetReceived.sequence_number) == PACKET_PRINT_NUM)
               printf("Packet %d lost \n", ntohs(packetReceived.sequence_number));
            
            totalCounter++;
            lossCounter++;
         } else {
            if(ntohs(packetReceived.sequence_number) != expectedSeqNum % 2) {
               if(ntohs(packetReceived.sequence_number) == PACKET_PRINT_NUM)
                  printf("Duplicate packet %d received with %d data bytes \n", ntohs(packetReceived.sequence_number), ntohs(packetReceived.count));
               
               totalCounter++;
               duplicateCounter++;
               
               memset(packetReceived.payloadLine, '\0', MAX_CHAR_PER_SEND);
               
               struct ACK ACKsent = {htons(ACKcounter % 2)};
               
               if (SimulateACKLoss(ACK_LOSS_RATE) == 1) {
                  if(ACKcounter % 2 == PACKET_PRINT_NUM)
                     printf("ACK %d lost \n", ACKcounter % 2);
                  droppedACKCounter++;
               } else {
                  bytes_sent = sendto(sock_client, &ACKsent, 2, 0, (struct sockaddr *) &server_addr, sizeof (server_addr));
                  if (ACKcounter % 2 == PACKET_PRINT_NUM)
                     printf("ACK %d transmitted \n", ACKcounter % 2);
                  noLossACKCounter++;
                  ACKcounter++;
               }
            } else {
               if (ntohs(packetReceived.sequence_number) == PACKET_PRINT_NUM)
                  printf("Packet %d received with %d data bytes \n", ntohs(packetReceived.sequence_number), ntohs(packetReceived.count));
               
               totalBytes += ntohs(packetReceived.count);
               totalCounter++;
               
               fputs(packetReceived.payloadLine, receivedFile);
               memset(packetReceived.payloadLine, '\0', MAX_CHAR_PER_SEND);
               
               struct ACK ACKsent = {htons(ACKcounter % 2)};
               
               if (SimulateACKLoss(ACK_LOSS_RATE) == 1) {
                  if(ACKcounter % 2 == PACKET_PRINT_NUM)
                     printf("ACK %d lost \n", ACKcounter % 2);
                  expectedSeqNum++;
                  droppedACKCounter++;
               } else {
                  bytes_sent = sendto(sock_client, &ACKsent, 2, 0, (struct sockaddr *) &server_addr, sizeof (server_addr));
                  
                  if(ACKcounter % 2 == PACKET_PRINT_NUM)
                     printf("ACK %d transmitted \n", ACKcounter % 2);
                  expectedSeqNum++;
                  noLossACKCounter++;
                  ACKcounter++;
               }
            }
         }
      } else {
         totalACKCounter = noLossACKCounter + droppedACKCounter;
         
         printf("End of Transmission Packet with sequence number %d received with %d data bytes\n\n", ntohs(packetReceived.sequence_number), ntohs(packetReceived.count));
         
         printf("Number of data packets received successfully: %d\n", expectedSeqNum - 1);
         printf("Total number of data bytes received which are delivered to user: %d\n", totalBytes);
         printf("Total number of duplicate data packets received: %d\n", duplicateCounter);
         printf("Number of data packets received but dropped due to loss: %d\n", lossCounter);
         printf("Total number of data packets received: %d\n", totalCounter);
         printf("Number of ACKs transmitted without loss: %d\n", noLossACKCounter);
         printf("Number of ACKs generated but dropped due to loss: %d\n", droppedACKCounter);
         printf("Total number of ACKs generated: %d\n", totalACKCounter);
         
         break;
      }
   }
   
   fclose(receivedFile);
   /* close the socket */

   close (sock_client);
}
