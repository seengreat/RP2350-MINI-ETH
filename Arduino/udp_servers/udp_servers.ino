#include <stdint.h>
#include <string.h>
#include "w5500.h"

//// W5500 pin definitions
//#define W5500_RSTn 20
//#define W5500_INTn 21
//#define W5500_CSn 17
//#define W5500_MOSI 19
//#define W5500_MISO 16
//#define W5500_SCLK 18
// Define multiple socket numbers
uint8_t NUM_SOCKETS = 4;  //Can be adjusted as needed
uint16_t SOCKET_PORTS[4] = {5000, 5001, 5002, 5003};  // Different port numbers

void W5500_Config() {
    // config gateway IP to 192.168.1.1
    uint8_t dat[4] = {192, 168, 1, 1};
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
    uint8_t ip[4] = {192, 168, 1, 101};
    W5500_writeBytes(SIPR, ip, 4);
    Serial.print("Server IP: ");
    Serial.print(ip[0]); Serial.print(".");
    Serial.print(ip[1]); Serial.print(".");
    Serial.print(ip[2]); Serial.print(".");
    Serial.println(ip[3]);
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
      delay(5);
      //Check if Socket is in UDP mode
      if(W5500_readSOCK1Byte(socket_num, Sn_SR) != SOCK_UDP){
          Serial.print("Failed to open UDP Socket\r\n");
          return false;
      }
      Serial.print("UDP Server started on port:");
      Serial.println(SOCKET_PORTS[i]);
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
 
void setup() {
    Serial.begin(115200);
    while (!Serial);
    
    Serial.println("UDP Servers mode test");
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
    //Configure Socket 0 as UDP Client
    if(Socket_Config()==false){
        Serial.println("Failed to configure UDP Server");
        while(1);
    }
    //Set retry time and retry count
    W5500_write2Byte(RTR, 200);  //200ms retry time
    W5500_write1Byte(RCR, 8);    //8 retries
}

void loop() {
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

    //Check if data is received from PC
    for(int k =0;k<NUM_SOCKETS;k++)
    {
        socket_num = k;
        uint16_t rx_size = W5500_readSOCK2Byte(socket_num, Sn_RX_RSR);
        if(rx_size > 0){
            Serial.print("Current port:");Serial.println(SOCKET_PORTS[socket_num]);
            //Read received data
            uint8_t buffer[1460];
            memset((uint8_t*)payload, 0, strlen((const char*)payload));
            uint16_t bytes_read = W5500_readSOCKDataBuffer(socket_num, buffer);
            uint16_t payload_length  = parse_udp_packet(buffer, bytes_read,src_ip,&src_port,payload,1460);
            Serial.print("Data received from PC:");
            Serial.println((const char*)payload);
            if(src_port == 0)
            {
                Serial.print("Invalid UDP packet format");
                continue;
            }
            Serial.print("Received ");Serial.print(payload_length);Serial.print(" bytes from ");
            Serial.print(src_ip[0]); Serial.print(".");Serial.print(src_ip[1]); Serial.print(".");
            Serial.print(src_ip[2]); Serial.print(".");Serial.print(src_ip[3]);
            Serial.print(":");Serial.println(src_port);
            // Set reply destination (now we configure Sn_DIPR/Sn_DPORTR)
            W5500_writeSOCK4Byte(socket_num, Sn_DIPR, src_ip);
            W5500_writeSOCK2Byte(socket_num, Sn_DPORTR, src_port);
              
            W5500_writeSOCKDataBuffer(socket_num, (uint8_t*)payload, strlen((const char*)payload));
        }
    }
    delay(1000);
}
