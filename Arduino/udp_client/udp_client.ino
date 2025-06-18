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
    Serial.print("Client IP: ");
    Serial.print(ip[0]); Serial.print(".");
    Serial.print(ip[1]); Serial.print(".");
    Serial.print(ip[2]); Serial.print(".");
    Serial.println(ip[3]);

    //Set the target IP address (the IP address of the PC)
    uint8_t pc_ip[4] = {192, 168, 1, 17};  //Replace it with the actual IP address of the PC.
    W5500_writeSOCK4Byte(socket_num, Sn_DIPR, pc_ip);

    //Set the target port (the UDP port of the PC)
    W5500_writeSOCK2Byte(socket_num, Sn_DPORTR, 8080);  //Replace it with the actual Port of the PC.
}

bool Socket_Config() {
    W5500_writeSOCK1Byte(socket_num, Sn_MR, MR_UDP);
    // set Socket port to 5000
    W5500_writeSOCK2Byte(socket_num, Sn_PORT, 5000);
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
    Serial.print("UDP Client started on port 5000\r\n");
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
 
void setup() {
    Serial.begin(115200);
    while (!Serial);
    
    Serial.println("UDP Client mode test");
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
        Serial.println("Failed to configure UDP Client");
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
    // Send data to PC
    const uint8_t send_data[] = "Hello SEENGREAT, this is W5500 UDP Client demo\r\n";
    W5500_writeSOCKDataBuffer(socket_num, (uint8_t*)send_data, strlen((const char*)send_data));
    Serial.print("Data sent to PC:");
    Serial.println((const char*)send_data);
    // read Socket state
    uint8_t i = W5500_readSOCK1Byte(socket_num, Sn_IR);
    if (i&IR_SEND_OK) {
        W5500_writeSOCK1Byte(socket_num, Sn_IR, IR_SEND_OK);
    }
    else{
      Serial.print("Failed to send data\r\n");  
    }
    //Check if data is received from PC
    uint16_t rx_size = W5500_readSOCK2Byte(socket_num, Sn_RX_RSR);
    if(rx_size > 0){
        //Read received data
        uint8_t buffer[1024];
        uint16_t bytes_read = W5500_readSOCKDataBuffer(socket_num, buffer);
        if (bytes_read > 0) {
            char *parsed_data = parse_udp_packet(buffer);
            Serial.print("Data received from PC:");
            Serial.println(parsed_data);
        }
        else{
            Serial.print("No data received from PC");
        }
    }
    delay(1000);
}
