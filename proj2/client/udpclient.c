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

#define STRING_SIZE 1024
#define MAX_CHAR_PER_SEND 80  /* constant for the max characters allowed to be copied from a line of text */
#define MAX_PACKET_RECEIVED_SIZE 84
#define PACKET_PRINT_NUM 5    /* constant for what packet to print */

struct packet {
   short unsigned int sequence_number, count;
   char payloadLine[STRING_SIZE];
};

struct ACK {
   short unsigned int sequence_number;
};

int main(void) {

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
   char lineReceived[STRING_SIZE];
   
   int expectedSeqNum = 1;
   int totalBytes = 0;
   
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
      bytes_recd = recvfrom(sock_client, &packetReceived, MAX_PACKET_RECEIVED_SIZE, 0,
                            (struct sockaddr *) 0, (unsigned int *) 0);
      if (ntohs(packetReceived.count) > 0) {
         if (ntohs(packetReceived.sequence_number) == expectedSeqNum%2) {
            totalBytes += ntohs(packetReceived.count);
            
            strcpy(lineReceived, packetReceived.payloadLine);
            printf("Payload: %s\n", lineReceived);
            
            fputs(lineReceived, receivedFile);
            
            memset(lineReceived, '\0', MAX_CHAR_PER_SEND);
            expectedSeqNum++;
         } else {
            continue;
         }
      } else {
         printf("End of Transmission Packet with sequence number %d received with %d data bytes\n\n", ntohs(packetReceived.sequence_number), ntohs(packetReceived.count));
         break;
      }
   }
   /* close the socket */

   close (sock_client);
}
