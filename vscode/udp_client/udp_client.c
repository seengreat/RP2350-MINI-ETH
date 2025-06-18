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

    //Set the target IP address (the IP address of the PC)
    uint8_t dip[4] = {192, 168, 0, 113};//Replace it with the actual IP address of the PC.
    W5500_writeSOCK4Byte(socket_num, Sn_DIPR, dip);

    //Set the target port (the UDP port of the PC)
    W5500_writeSOCK2Byte(socket_num, Sn_DPORTR, 8080);//Replace it with the actual Port of the PC.
}

uint8_t Socket_Config(void) {
    W5500_writeSOCK1Byte(socket_num, Sn_MR, MR_UDP);
    // set Socket 0 port to 5000
    uint16_t port = 5000;
    W5500_writeSOCK2Byte(socket_num, Sn_PORT, port);
    // Set the maximum segment size to 1460
    W5500_writeSOCK2Byte(socket_num, Sn_MSSR, 1460);
    // Set the size of the RX buffer to 2K
    W5500_writeSOCK1Byte(socket_num, Sn_RXBUF_SIZE, 0x02);
    // Set the size of the TX buffer to 2K
    W5500_writeSOCK1Byte(socket_num, Sn_TXBUF_SIZE, 0x02);
    //Open Socket 0
    W5500_writeSOCK1Byte(socket_num, Sn_CR, OPEN);
    
    //Wait for Socket to open
    sleep_ms(5);
    
    //Check if Socket is in UDP mode
    if (W5500_readSOCK1Byte(socket_num, Sn_SR) != SOCK_UDP){
        printf("Failed to open UDP Socket");
        return false;
    }
    printf("UDP Client started on port 5000");
    return true;

}

// Parse UDP packet
char* parse_udp_packet(const uint8_t* dat) {
    static char buffer[1024];
    size_t data_len = strlen((const char*)dat);
    if (data_len < 8) {
        // Output data length and data
        printf("%zu ", data_len);
        for (size_t i = 0; i < data_len; i++){
            printf("%02x ", dat[i]);
        }
        printf("\n");
        strcpy(buffer, "Invalid UDP packet");
        return buffer;
    }

    // Extract data length (bytes 7-8 of the UDP header)
    uint16_t length = (dat[6] << 8) | dat[7];

    // Ensure the data length does not exceed the available data range
    if (8 + length > data_len) {
        strcpy(buffer, "Invalid UDP packet");
        return buffer;
    }

    // Extract the data payload (the part after the UDP header)
    memcpy(buffer, dat + 8, length);
    buffer[length] = '\0';

    return buffer;
}  
void main() {
    stdio_init_all();
    irq_set_enabled(IO_IRQ_BANK0, true); 
    printf("UDP Client mode test\r\n");
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
    /* Configure Socket 0 as UDP Client */
    if (!Socket_Config()) {
        printf("Failed to configure UDP Client\n");
        return ;
    }
    
    /* Set Retry Time and Retry Count */
    W5500_write2Byte(RTR, 200);  /* 200ms retry time */
    W5500_write1Byte(RCR, 8);    /* 8 retries */
    
    printf("UDP client started\n");
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
        
        // Send data to PC
        const uint8_t send_data[] = "Hello SEENGREAT, this is W5500 UDP Client demo";
        W5500_writeSOCKDataBuffer(socket_num, (uint8_t*)send_data, strlen((const char*)send_data));
        printf("Data sent to PC:%s \r\n",send_data);
        // read Socket state
        uint8_t i = W5500_readSOCK1Byte(socket_num, Sn_IR);
        if (i&IR_SEND_OK) {
            W5500_writeSOCK1Byte(socket_num, Sn_IR, IR_SEND_OK);
        }
        else{
            printf("Failed to send data\r\n");  
        }
        //Check if data is received from PC
        uint16_t rx_size = W5500_readSOCK2Byte(socket_num, Sn_RX_RSR);
        if(rx_size > 0){
            //Read received data
            uint8_t buffer[1024];
            uint16_t bytes_read = W5500_readSOCKDataBuffer(socket_num, buffer);
            if (bytes_read > 0) {
                char *parsed_data = parse_udp_packet(buffer);
                printf("Data received from PC:%s \r\n",parsed_data);
            }
            else{
                printf("No data received from PC");
            }
        }
        sleep_ms(1000);
    }
}
