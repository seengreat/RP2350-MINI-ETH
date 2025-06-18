#include "w5500.h"

//// W5500 pin definitions
//#define W5500_RSTn 20
//#define W5500_INTn 21
//#define W5500_CSn 17
//#define W5500_MOSI 19
//#define W5500_MISO 16
//#define W5500_SCLK 18

void W5500_Config() {
    // config gateway IP to 192.168.1.1
    uint8_t dat[4] = {192, 168, 0, 1};
    W5500_writeBytes(GAR, dat, 4);
    Serial.print("Gateway: ");
    Serial.print(dat[0]); Serial.print(".");
    Serial.print(dat[1]); Serial.print(".");
    Serial.print(dat[2]); Serial.print(".");
    Serial.println(dat[3]);
    
    // Set the subnet mask to 255.255.255.0
    uint8_t sub_mask[4] = {255, 255, 255, 0};
    W5500_writeBytes(SUBR, sub_mask, 4);
    Serial.print("Sub mask: ");
    Serial.print(sub_mask[0]); Serial.print(".");
    Serial.print(sub_mask[1]); Serial.print(".");
    Serial.print(sub_mask[2]); Serial.print(".");
    Serial.println(sub_mask[3]);

    // set MAC address to 0x48,0x53,0x00,0x57,0x55,0x00
    uint8_t mac[6] = {0x48, 0x53, 0x00, 0x57, 0x55, 0x00};
    W5500_writeBytes(SHAR, mac, 6);
    Serial.print("MAC: ");
    for (int i = 0; i < 6; i++) {
        if (i > 0) Serial.print(":");
        if (mac[i] < 0x10) Serial.print("0");
        Serial.print(mac[i], HEX);
    }
    Serial.println();

    // set W5500 IP to 192.168.1.101
    uint8_t ip[4] = {192, 168, 0, 101};
    W5500_writeBytes(SIPR, ip, 4);
    Serial.print("Client IP: ");
    Serial.print(ip[0]); Serial.print(".");
    Serial.print(ip[1]); Serial.print(".");
    Serial.print(ip[2]); Serial.print(".");
    Serial.println(ip[3]);
}

uint8_t Detect_Gateway() {
    // Set a target in a different subnet IP
    uint8_t dat[4] = {192 + 1, 168 + 1, 0 + 1, 20 + 1};
    W5500_writeSOCK4Byte(socket_num, Sn_DIPR, dat);

    // set Socket to TCP mode
    W5500_writeSOCK1Byte(socket_num, Sn_MR, MR_TCP);
    // open Socket 
    W5500_writeSOCK1Byte(socket_num, Sn_CR, OPEN);

    // wait
    delay(5);
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

void Socket_Config() {
    // set Socket port to 5000
    W5500_writeSOCK2Byte(socket_num, Sn_PORT, 5000);
    // Set the maximum segment size to 1460
    W5500_writeSOCK2Byte(socket_num, Sn_MSSR, 1460);
    // Set the size of the RX buffer to 2K
    W5500_writeSOCK1Byte(socket_num, Sn_RXBUF_SIZE, 0x02);
    // Set the size of the TX buffer to 2K
    W5500_writeSOCK1Byte(socket_num, Sn_TXBUF_SIZE, 0x02);
    // Set the destination IP address (example: 192.168.1.18)
    uint8_t server_ip[4] = {192, 168, 0, 113}; // your TCP server IP
    W5500_writeSOCK4Byte(socket_num, Sn_DIPR, server_ip);
    // Set the destination port (example: 5000)
    W5500_writeSOCK2Byte(socket_num, Sn_DPORTR, 5000);
}

void setup() {
    Serial.begin(115200);
    while (!Serial);
    
    Serial.println("TCP Client mode test");
    W5500_Gpio_Init(); 
    // Initialize the SPI
    SPI.setRX(W5500_MISO);
    SPI.setTX(W5500_MOSI);
    SPI.setSCK(W5500_SCLK);
    SPI.begin();
    SPISettings settings(1000000, MSBFIRST, SPI_MODE0);
    SPI.beginTransaction(settings);
    
    // Initialize W5500
    socket_num = 0; // Using socket 0 for client
    W5500_getVer();
    delay(100);
    
    // Detect the Ethernet connection
    while (true) {
        delay(100);
        uint8_t i = W5500_read1Byte(PHYCFGR);
        if (i & LINK) {
            Serial.println("linked");
            break;
        }
    }

    // config W5500
    W5500_Config();
    // detect gateway
    if (Detect_Gateway()) {
        Serial.println("detect gateway ok");
    }
}

void loop() {
   while(1)
   {
      // Detect the Ethernet connection
      if (!(W5500_read1Byte(PHYCFGR) & LINK)) {
          Serial.println("not link");
          // close Socket
          W5500_writeSOCK1Byte(socket_num, Sn_CR, CLOSE);
          // Wait for the Ethernet connection to be restored.
          while (!(W5500_read1Byte(PHYCFGR) & LINK)) {
              delay(100); 
          }
          Serial.println("linked!");
      }
    
      // read Socket state
      uint8_t i = W5500_readSOCK1Byte(socket_num, Sn_SR);
      if (i == SOCK_CLOSED || i == SOCK_CLOSE_WAIT) {
          Serial.println("Socket closed, reconnecting...");
          W5500_writeSOCK1Byte(socket_num, Sn_CR, CLOSE);
          delay(1000);
          Socket_Config(); 
          if (!W5500_socketConnect(socket_num)) {
              Serial.println("Reconnect failed, retrying...");
              delay(1000);
              continue;
          }
      }
      else if (i == SOCK_ESTABLISHED) {
          // Send data to server
          const char* send_data = "Hello Server!";
          uint8_t data_buffer[strlen(send_data)];
          memcpy(data_buffer, send_data, strlen(send_data));
          
          W5500_writeSOCKDataBuffer(socket_num, data_buffer, strlen(send_data));
          Serial.print("Data sent to server: ");
          Serial.println(send_data);
          
          // Receive data from server
          uint8_t recv_buffer[1460];
          uint16_t recv_len = W5500_readSOCKDataBuffer(socket_num, recv_buffer);
          if (recv_len != 0) {
              Serial.print("Data received from server: ");
              for (uint16_t i = 0; i < recv_len; i++) {
                  Serial.print((char)recv_buffer[i]);
              }
              Serial.println();
          }
          
          delay(1000); // Wait before sending the next data
      }
   }
}
