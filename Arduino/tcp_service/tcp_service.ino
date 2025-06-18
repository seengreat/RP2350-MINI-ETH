#include "w5500.h"

// W5500 pin definition
//#define W5500_RSTn 20
//#define W5500_INTn 21
//#define W5500_CSn 17
//#define W5500_MOSI 19
//#define W5500_MISO 16
//#define W5500_SCLK 18


void W5500_Config(void) {
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
    Serial.print("TCP Server IP: ");
    Serial.print(ip[0]); Serial.print(".");
    Serial.print(ip[1]); Serial.print(".");
    Serial.print(ip[2]); Serial.print(".");
    Serial.println(ip[3]);
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

void Socket_Config(void) {
    // set Socket 0 port to 5000
    uint16_t port = 5000;
    Serial.print("The port is: ");
    Serial.println(port);
    W5500_writeSOCK2Byte(socket_num, Sn_PORT, port);
    // Set the maximum segment size to 1460
    W5500_writeSOCK2Byte(socket_num, Sn_MSSR, 1460);
    // Set the size of the RX buffer to 2K
    W5500_writeSOCK1Byte(socket_num, Sn_RXBUF_SIZE, 0x02);
    // Set the size of the TX buffer to 2K
    W5500_writeSOCK1Byte(socket_num, Sn_TXBUF_SIZE, 0x02);
}

void Loop_Back(void) {
    uint8_t buffer[1460];
    uint16_t len = W5500_readSOCKDataBuffer(socket_num, buffer);
    if (len == 0) {
        return;
    }
    
    Serial.print("len: ");
    Serial.print(len);
    Serial.print(" ");
    for (uint16_t i = 0; i < len; i++) {
        Serial.print((char)buffer[i]);
    }
    Serial.println();
    
    W5500_writeSOCKDataBuffer(socket_num, buffer, len);
    
//    while (true) {
//        uint8_t i = W5500_readSOCK1Byte(socket_num, Sn_IR);
//        if ((i & IR_TIMEOUT) || (i & IR_SEND_OK)) {
//            break;
//        }
//    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial);
    
    Serial.println("TCP Server mode test");
    W5500_Gpio_Init(); 
    // Initialize the SPI
    SPI.setRX(W5500_MISO);
    SPI.setTX(W5500_MOSI);
    SPI.setSCK(W5500_SCLK);
    SPI.begin();
    SPISettings settings(1000000, MSBFIRST, SPI_MODE0);
    SPI.beginTransaction(settings);
    
    // Initialize W5500
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
    // Detect the Ethernet connection
    if (!(W5500_read1Byte(PHYCFGR) & LINK)) {
        Serial.println("not link");
        // close Socket 0
        W5500_writeSOCK1Byte(socket_num, Sn_CR, CLOSE);
        // Wait for the Ethernet connection to be restored.
        while (!(W5500_read1Byte(PHYCFGR) & LINK)) {
            delay(100); 
        }
        Serial.println("linked!");
    }
    
    // read Socket 0 state
    uint8_t i = W5500_readSOCK1Byte(socket_num, Sn_SR);
    if (i == 0) {
        Socket_Config();
        while (!W5500_socketListen(socket_num)) {
            delay(1000);
        }
    }
    else if (i == SOCK_ESTABLISHED) {
        Loop_Back();
    }
    delay(100);
}
