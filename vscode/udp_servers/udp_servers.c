/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"


#include "w5500.h"

// W5500 pin definition
//#define W5500_RSTn 20
//#define W5500_INTn 21
//#define W5500_CSn 17
//#define W5500_MOSI 19
//#define W5500_MISO 16
//#define W5500_SCLK 18

uint8_t NUM_SOCKETS = 4;  //Can be adjusted as needed
uint16_t SOCKET_PORTS[4] = {5000, 5001, 5002, 5003};  // Different port numbers

void W5500_Config(void) {
    // config gateway IP to 192.168.0.1
    uint8_t dat[4] = {192, 168, 0, 1};
    W5500_writeBytes(GAR, dat, 4);
    printf("Gateway: %d.%d.%d.%d\r\n",dat[0],dat[1],dat[2],dat[3]);
    
    // Set the subnet mask to 255.255.255.0
    uint8_t sub_mask[4] = {255, 255, 255, 0};
    W5500_writeBytes(SUBR, sub_mask, 4);
    printf("Sub mask: %d.%d.%d.%d\r\n",sub_mask[0],sub_mask[1],sub_mask[2],sub_mask[3]);

    // set MAC address to 0x48,0x53,0x00,0x57,0x55,0x00
    uint8_t mac[6] = {0x48, 0x53, 0x00, 0x57, 0x55, 0x00};
    W5500_writeBytes(SHAR, mac, 6);
    printf("MAC: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\r\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

    // set W5500 IP to 192.168.0.101
    uint8_t ip[4] = {192, 168, 0, 101};
    W5500_writeBytes(SIPR, ip, 4);
    printf("UDP Client IP: %d.%d.%d.%d\r\n",ip[0],ip[1],ip[2],ip[3]);

}

bool Socket_Config() {
  for(int i=0;i<4;i++)
  {
      socket_num = i;
      W5500_writeSOCK1Byte(socket_num, Sn_MR, MR_UDP);
      // set Socket port to 5000
      W5500_writeSOCK2Byte(socket_num, Sn_PORT, SOCKET_PORTS[i]);
      // Set the maximum segment size to 1460
      W5500_writeSOCK2Byte(socket_num, Sn_MSSR, 1460);
      // Set the size of the RX buffer to 2K
      W5500_writeSOCK1Byte(socket_num, Sn_RXBUF_SIZE, 0x02);
      // Set the size of the TX buffer to 2K
      W5500_writeSOCK1Byte(socket_num, Sn_TXBUF_SIZE, 0x02);
      W5500_writeSOCK1Byte(socket_num, Sn_CR, OPEN);
      sleep_ms(5);
      //Check if Socket is in UDP mode
      if(W5500_readSOCK1Byte(socket_num, Sn_SR) != SOCK_UDP){
          printf("Failed to open UDP Socket\r\n");
          return false;
      }
      printf("UDP Server started on port:%d\r\n",SOCKET_PORTS[i]);
  }
  return true;
}

uint8_t src_ip[4];
uint16_t src_port;
uint8_t payload[1024];

/**
 * Parse a UDP packet to extract source IP, port, and payload data
 * @param data Buffer containing the received UDP packet
 * @param length Length of the packet data
 * @param src_ip Output parameter: Source IP address (4-byte array)
 * @param src_port Output parameter: Source port number
 * @param payload Output parameter: Buffer for payload data
 * @param max_payload_len Maximum length of the payload buffer
 * @return Actual length of the parsed payload, 0 on failure
 */
uint16_t parse_udp_packet(const uint8_t* data, uint16_t length, 
                          uint8_t* src_ip, uint16_t* src_port, 
                          uint8_t* payload, uint16_t max_payload_len) {
    // Check if packet length is sufficient (UDP header is 8 bytes)
    if (length < 8) {
        return 0;
    }
    
    // Extract source IP address (first 4 bytes assumed to be IP)
    memcpy(src_ip, data, 4);
    
    // Extract source port number (big-endian byte order)
    *src_port = (data[4] << 8) | data[5];
    
    // Extract payload data
    uint16_t payload_length = length - 8;
    if (payload_length > max_payload_len) {
        payload_length = max_payload_len;
    }
    memcpy(payload, data + 8, payload_length);
    
    return payload_length;
}     
void main() {
    stdio_init_all();
    irq_set_enabled(IO_IRQ_BANK0, true); 
    printf("UDP Servers mode test\r\n");
    sleep_ms(2000);
    W5500_Gpio_Init(); // Initialize W5500
    sleep_ms(2000);
    W5500_getVer();
    // Detect the Ethernet connection
    while (true) {
        sleep_ms(100);
        uint8_t i = W5500_read1Byte(PHYCFGR);
        if (i & LINK) {
            printf("linked\r\n");
            break;
        }
    }
    // config W5500
    W5500_Config();
    /* Configure Sockets as UDP server */
    if (!Socket_Config()) {
        printf("Failed to configure UDP Servers\n");
        return ;
    }
    
    /* Set Retry Time and Retry Count */
    W5500_write2Byte(RTR, 200);  /* 200ms retry time */
    W5500_write1Byte(RCR, 8);    /* 8 retries */
    
    printf("UDP servers started\n");
    while(1){
        // Detect the Ethernet connection
        if (!(W5500_read1Byte(PHYCFGR) & LINK)) {
            printf("not link\r\n");
            // close Socket 0
            W5500_writeSOCK1Byte(socket_num, Sn_CR, CLOSE);
            // Wait for the Ethernet connection to be restored.
            while (!(W5500_read1Byte(PHYCFGR) & LINK)) {
                sleep_ms(100); 
            }
            printf("linked!");
        }
        
        //Check if data is received from PC
        for(int k =0;k<NUM_SOCKETS;k++)
        {
            socket_num = k;
            uint16_t rx_size = W5500_readSOCK2Byte(socket_num, Sn_RX_RSR);
            if(rx_size > 0){
                printf("Current port:%d\r\n",SOCKET_PORTS[socket_num]);
                //Read received data
                uint8_t buffer[1460];
                memset((uint8_t*)payload, 0, strlen((const char*)payload));
                uint16_t bytes_read = W5500_readSOCKDataBuffer(socket_num, buffer);
                uint16_t payload_length  = parse_udp_packet(buffer, bytes_read,src_ip,&src_port,payload,1460);
                printf("Data received from PC:%s\r\n",(const char*)payload);
                if(src_port == 0)
                {
                    printf("Invalid UDP packet format\r\n");
                    continue;
                }
                printf("Received %d bytes from ",payload_length);
                printf("%d.%d.%d.%d:%d\r\n",src_ip[0],src_ip[1],src_ip[2],src_ip[3],src_port); 
                // Set reply destination (now we configure Sn_DIPR/Sn_DPORTR)
                W5500_writeSOCK4Byte(socket_num, Sn_DIPR, src_ip);
                W5500_writeSOCK2Byte(socket_num, Sn_DPORTR, src_port);
                
                W5500_writeSOCKDataBuffer(socket_num, (uint8_t*)payload, strlen((const char*)payload));
            }
        }
        sleep_ms(1000);
    }
}
