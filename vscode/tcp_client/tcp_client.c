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
    printf("TCP Client IP: %d.%d.%d.%d\r\n",ip[0],ip[1],ip[2],ip[3]);
}

uint8_t Detect_Gateway(void) {
    // Set a target in a different subnet IP
    uint8_t dat[4] = {192 + 1, 168 + 1, 0 + 1, 20 + 1};
    W5500_writeSOCK4Byte(socket_num, Sn_DIPR, dat);

    // set Socket to TCP mode
    W5500_writeSOCK1Byte(socket_num, Sn_MR, MR_TCP);
    // open Socket 
    W5500_writeSOCK1Byte(socket_num, Sn_CR, OPEN);

    // wait
    sleep_ms(5);
    // Check if the Socket status is SOCK_INIT.
    if (W5500_readSOCK1Byte(socket_num, Sn_SR) != SOCK_INIT) {
        // if not，close Socket
        W5500_writeSOCK1Byte(socket_num, Sn_CR, CLOSE);
        return FALSE;
    }

    // Initiate connection
    W5500_writeSOCK1Byte(socket_num, Sn_CR, CONNECT);

    while (true) {
        // Check if it has timed out.
        if (W5500_readSOCK1Byte(socket_num, Sn_IR) & IR_TIMEOUT) {
            // if time out，close Socket
            W5500_writeSOCK1Byte(socket_num, Sn_CR, CLOSE);
            return FALSE;
        }
        // Check if the gateway is detected.
        else if (W5500_readSOCK1Byte(socket_num, Sn_DHAR) != 0xff) {
            // If the gateway is detected ，close Socket
            W5500_writeSOCK1Byte(socket_num, Sn_CR, CLOSE);
            break;
        }
    }

    return TRUE;
}

void Socket_Config(void) {
    // set Socket 0 port to 5000
    uint16_t port = 5000;
    W5500_writeSOCK2Byte(socket_num, Sn_PORT, port);
    // Set the maximum segment size to 1460
    W5500_writeSOCK2Byte(socket_num, Sn_MSSR, 1460);
    // Set the size of the RX buffer to 2K
    W5500_writeSOCK1Byte(socket_num, Sn_RXBUF_SIZE, 0x02);
    // Set the size of the TX buffer to 2K
    W5500_writeSOCK1Byte(socket_num, Sn_TXBUF_SIZE, 0x02);
    uint8_t server_ip[4] = {192, 168, 0, 113}; // your TCP server IP
    W5500_writeSOCK4Byte(socket_num, Sn_DIPR, server_ip);
    // Set the destination port (example: 5000)
    W5500_writeSOCK2Byte(socket_num, Sn_DPORTR, 5000);
}

void Loop_Back(void) {
    uint8_t buffer[1460];
    uint16_t len = W5500_readSOCKDataBuffer(socket_num, buffer);
    if (len == 0) {
        return;
    }
    
    printf("len: %d ",len);
    for (uint16_t i = 0; i < len; i++) {
        putchar_raw(buffer[i]); 
    }
    printf("\r\n");
    
    W5500_writeSOCKDataBuffer(socket_num, buffer, len);
}

void main() {
    stdio_init_all();
    irq_set_enabled(IO_IRQ_BANK0, true); 
    printf("TCP Client mode test\r\n");
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
    // detect gateway
    if (Detect_Gateway()) {
        printf("detect gateway ok\r\n");
    }
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
        
        // read Socket state
        uint8_t i = W5500_readSOCK1Byte(socket_num, Sn_SR);
        if (i == SOCK_CLOSED || i == SOCK_CLOSE_WAIT) {
            printf("Socket closed, reconnecting...\r\n");
            W5500_writeSOCK1Byte(socket_num, Sn_CR, CLOSE);
            sleep_ms(100);
            Socket_Config(); 
            if (!W5500_socketConnect(socket_num)) {
                printf("Reconnect failed, retrying...\r\n");
                sleep_ms(1000);
                continue;
            }
        }
        else if (i == SOCK_ESTABLISHED) {
            // Send data to server
            const char* send_data = "Hello Server!";
            uint16_t send_len = strlen(send_data);
            uint8_t data_buffer[send_len];
            memcpy(data_buffer, send_data, send_len);
            
            W5500_writeSOCKDataBuffer(socket_num, data_buffer, send_len);
            printf("Data sent to server: ");
            for(uint16_t i = 0; i < send_len; i++)
            {
                putchar_raw(send_data[i]);
            }
            printf("\r\n");
            // Receive data from server
            uint8_t recv_buffer[1460];
            uint16_t recv_len = W5500_readSOCKDataBuffer(socket_num, recv_buffer);
            if (recv_len != 0) {
                printf("Data received from server: ");
                for (uint16_t i = 0; i < recv_len; i++) {
                    putchar_raw(recv_buffer[i]);
                }
                printf("\r\n");
            }
            sleep_ms(1000); // Wait before sending the next data
        }
    }
}
