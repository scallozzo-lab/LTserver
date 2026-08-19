#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>       // for close()
#include <arpa/inet.h>    // for inet_pton(), sockaddr_in
#include <sys/socket.h>   // for socket functions
#include <fcntl.h>  // for fcntl()
#include <errno.h>
#include "loop2app.h"

#define BUFFER_SIZE 1024
#define LOCAL_PORT  12345  // Change as needed
#define REMOTE_PORT 12346  // Port to send to
#define LOOP_ADDRESS    "127.0.0.1"

static int loopsockfd = 0;
  
int init_loop2app(void)
{

    struct sockaddr_in local_addr, sender_addr;
    
    loopsockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (loopsockfd < 0) {
        perror("socket (rx)");
        exit(EXIT_FAILURE);
    }

    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(LOCAL_PORT);
    local_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(loopsockfd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        perror("bind");
        close(loopsockfd);
        exit(EXIT_FAILURE);
    }

    int flags = fcntl(loopsockfd, F_GETFL, 0);
    fcntl(loopsockfd, F_SETFL, flags | O_NONBLOCK);


    printf("Listening on UDP port %d...\n", LOCAL_PORT);

}

// Receive UDP packets on LOCAL_PORT
uint8_t *udp_receive(uint16_t *plen)
{
    
    static uint8_t buffer[BUFFER_SIZE];
    struct sockaddr_in sender_addr;
    socklen_t addr_len = sizeof(sender_addr);
   
  
    ssize_t len = recvfrom(loopsockfd, buffer, BUFFER_SIZE - 1, 0,
                            (struct sockaddr*)&sender_addr, &addr_len);
    if (len < 0) {
        
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            // No data available yet — not an error
            return NULL;
        else perror("recvfrom");
    }
    else if(len)
    {
        printf("Received from %s:%d: len (%d) [",
                inet_ntoa(sender_addr.sin_addr),
                ntohs(sender_addr.sin_port), len);
        for(int x=0;x<len;x++) printf("%02X",buffer[x]);
        printf("]\n");        
        if(plen) *plen = (uint16_t)len;  
        return buffer; 
  
    }
    return 0; 
    //close(loopsockfd);
}

// Send UDP packet to REMOTE_PORT
void udp_send(const char* dest_ip, const char* message, uint16_t len)
{
    //int sockfd;
    struct sockaddr_in dest_addr;

    //sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    //if (sockfd < 0) {
    //    perror("socket (tx)");
    //    exit(EXIT_FAILURE);
    //}

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(REMOTE_PORT);

    if (inet_pton(AF_INET, dest_ip, &dest_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(loopsockfd);
        exit(EXIT_FAILURE);
    }

    ssize_t sent = sendto(loopsockfd, message, len, 0,
                          (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    if (sent < 0) {
        perror("sendto");
    } else {
        printf("Sent %ld bytes to %s:%d\n", sent, dest_ip, REMOTE_PORT);
    }

    //close(loopsockfd);
}

int test_loop(void)
{
    uint16_t rxlen;
    stTxRxRequest TxRxRequest;
    
    TxRxRequest.msjtype = 0x5a;
    TxRxRequest.datalen = 28;
    memcpy(TxRxRequest.data, "Este es una mensaje de prueba", 28);
    // Send mode: ./program <dest_ip> <message>
    while(1)
    {
        uint8_t *rx = udp_receive(&rxlen);
        if(rx)
        {
            if(rxlen >= 3)
            {
                printf("tx cmd ok\n");
                udp_send(LOOP_ADDRESS, (uint8_t*)&TxRxRequest, 28);            
            }    
        }
        sleep(1);
    }
    return 0;
}
