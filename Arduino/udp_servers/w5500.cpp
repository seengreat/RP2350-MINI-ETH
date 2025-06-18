#include "w5500.h"
#include <SPI.h>
#include <functional>

uint8_t socket_num = 0;

void W5500_Gpio_Init(void) 
{
    socket_num = 0;
    W5500_configureInterrupt(W5500_INTn);
    pinMode(W5500_CSn, OUTPUT);
    digitalWrite(W5500_CSn, HIGH);
    
    pinMode(W5500_RSTn, OUTPUT);
    W5500_reset();
}

void W5500_reset() {
     digitalWrite(W5500_RSTn, LOW);
     delay(10);
     digitalWrite(W5500_RSTn, HIGH);
     delay(500);
}

void W5500_getVer() {
    uint8_t ver = W5500_read1Byte(VERR);
    Serial.print("Chip version: 0x");
    Serial.println(ver, HEX);
}

void W5500_write1Byte(uint16_t reg, uint8_t dat) {
    digitalWrite(W5500_CSn, LOW);
    SPI.transfer16(reg);
    SPI.transfer(FDM1 | RWB_WRITE | COMMON_R);
    SPI.transfer(dat);
    digitalWrite(W5500_CSn, HIGH);
}

void W5500_write2Byte(uint16_t reg, uint16_t dat) {
    digitalWrite(W5500_CSn, LOW);
    SPI.transfer16(reg);
    SPI.transfer(FDM2 | RWB_WRITE | COMMON_R);
    SPI.transfer16(dat);
    digitalWrite(W5500_CSn, HIGH);
}

void W5500_writeBytes(uint16_t reg, uint8_t* dat, uint16_t Size) {
    digitalWrite(W5500_CSn, LOW);
    SPI.transfer16(reg);
    SPI.transfer(VDM | RWB_WRITE | COMMON_R);
    for (uint16_t i = 0; i < Size; i++) {
        SPI.transfer(dat[i]);
    }
    digitalWrite(W5500_CSn, HIGH);
}

uint8_t W5500_read1Byte(uint16_t reg) {
    digitalWrite(W5500_CSn, LOW);
    SPI.transfer16(reg);
    SPI.transfer(FDM1 | RWB_READ | COMMON_R);
    uint8_t dat = SPI.transfer(0);
    digitalWrite(W5500_CSn, HIGH);
    return dat;
}

void W5500_writeSOCK1Byte(uint8_t s, uint16_t reg, uint8_t dat) {
    digitalWrite(W5500_CSn, LOW);
    SPI.transfer16(reg);
    SPI.transfer(FDM1 | RWB_WRITE | (s * 0x20 + 0x08));
    SPI.transfer(dat);
    digitalWrite(W5500_CSn, HIGH);
}

void W5500_writeSOCK2Byte(uint8_t s, uint16_t reg, uint16_t dat) {
    digitalWrite(W5500_CSn, LOW);
    SPI.transfer16(reg);
    SPI.transfer(FDM2 | RWB_WRITE | (s * 0x20 + 0x08));
    SPI.transfer16(dat);
    digitalWrite(W5500_CSn, HIGH);
}

void W5500_writeSOCK4Byte(uint8_t s, uint16_t reg, uint8_t* dat) {
    digitalWrite(W5500_CSn, LOW);
    SPI.transfer16(reg);
    SPI.transfer(FDM4 | RWB_WRITE | (s * 0x20 + 0x08));
    for (uint8_t i = 0; i < 4; i++) {
        SPI.transfer(dat[i]);
    }
    digitalWrite(W5500_CSn, HIGH);
}

uint8_t W5500_readSOCK1Byte(uint8_t s, uint16_t reg) {
    digitalWrite(W5500_CSn, LOW);
    SPI.transfer16(reg);
    SPI.transfer(FDM1 | RWB_READ | (s * 0x20 + 0x08));
    uint8_t dat = SPI.transfer(0);
    digitalWrite(W5500_CSn, HIGH);
    return dat;
}

uint16_t W5500_readSOCK2Byte(uint8_t s, uint16_t reg) {
    digitalWrite(W5500_CSn, LOW);
    SPI.transfer16(reg);
    SPI.transfer(FDM2 | RWB_READ | (s * 0x20 + 0x08));
    uint16_t dat = SPI.transfer16(0);
    digitalWrite(W5500_CSn, HIGH);
    return dat;
}

void W5500_readSOCK4Byte(uint8_t s, uint16_t reg, uint8_t* dat) {
    digitalWrite(W5500_CSn, LOW);
    SPI.transfer16(reg);
    SPI.transfer(FDM4 | RWB_READ | (s * 0x20 + 0x08));
    for (uint8_t i = 0; i < 4; i++) {
        dat[i] = SPI.transfer(0);
    }
    digitalWrite(W5500_CSn, HIGH);
}

uint16_t W5500_readSOCKDataBuffer(uint8_t s, uint8_t* buf) {
    uint16_t rx_size = W5500_readSOCK2Byte(s, Sn_RX_RSR);
    if (rx_size == 0) {
        return 0;
    }
    if (rx_size > 1460) {
        rx_size = 1460;
    }
    
    uint16_t offset = W5500_readSOCK2Byte(s, Sn_RX_RD);
    uint16_t offset1 = offset;
    offset &= (S_RX_SIZE - 1);
    
    digitalWrite(W5500_CSn, LOW);
    SPI.transfer(offset >> 8);
    SPI.transfer(offset & 0xFF);
    SPI.transfer(VDM | RWB_READ | (s * 0x20 + 0x18));
    
    if ((offset + rx_size) < S_RX_SIZE) {
        for (uint16_t i = 0; i < rx_size; i++) {
            buf[i] = SPI.transfer(0);
        }
    } else {
        uint16_t first_part_size = S_RX_SIZE - offset;
        for (uint16_t i = 0; i < first_part_size; i++) {
            buf[i] = SPI.transfer(0);
        }
        
        digitalWrite(W5500_CSn, HIGH);
        digitalWrite(W5500_CSn, LOW);
        SPI.transfer(0);
        SPI.transfer(0);
        SPI.transfer(VDM | RWB_READ | (s * 0x20 + 0x18));
        
        for (uint16_t i = first_part_size; i < rx_size; i++) {
            buf[i] = SPI.transfer(0);
        }
    }
    digitalWrite(W5500_CSn, HIGH);
    
    offset1 += rx_size;
    W5500_writeSOCK2Byte(s, Sn_RX_RD, offset1);
    W5500_writeSOCK1Byte(s, Sn_CR, RECV);
    
    return rx_size;
}

void W5500_writeSOCKDataBuffer(uint8_t s, uint8_t* dat, uint16_t Size) {
    uint16_t offset = W5500_readSOCK2Byte(s, Sn_TX_WR);
    uint16_t offset1 = offset;
    offset &= (S_TX_SIZE - 1);
    
    digitalWrite(W5500_CSn, LOW);
    SPI.transfer(offset >> 8);
    SPI.transfer(offset & 0xFF);
    SPI.transfer(VDM | RWB_WRITE | (s * 0x20 + 0x10));
    
    if ((offset + Size) < S_TX_SIZE) {
        for (uint16_t i = 0; i < Size; i++) {
            SPI.transfer(dat[i]);
        }
    } else {
        uint16_t first_part_size = S_TX_SIZE - offset;
        for (uint16_t i = 0; i < first_part_size; i++) {
            SPI.transfer(dat[i]);
        }
        
        digitalWrite(W5500_CSn, HIGH);
        digitalWrite(W5500_CSn, LOW);
        SPI.transfer(0);
        SPI.transfer(0);
        SPI.transfer(VDM | RWB_WRITE | (s * 0x20 + 0x10));
        
        for (uint16_t i = first_part_size; i < Size; i++) {
            SPI.transfer(dat[i]);
        }
    }
    digitalWrite(W5500_CSn, HIGH);
    
    offset1 += Size;
    W5500_writeSOCK2Byte(s, Sn_TX_WR, offset1);
    W5500_writeSOCK1Byte(s, Sn_CR, SEND);
}

uint8_t W5500_socketConnect(uint8_t s) {
    W5500_writeSOCK1Byte(s, Sn_MR, MR_TCP);
    W5500_writeSOCK1Byte(s, Sn_CR, OPEN);
    
    delay(5);
    if (W5500_readSOCK1Byte(s, Sn_SR) != SOCK_INIT) {
        W5500_writeSOCK1Byte(s, Sn_CR, CLOSE);
        return FALSE;
    }
    
    W5500_writeSOCK1Byte(s, Sn_CR, CONNECT);
    return TRUE;
}

uint8_t W5500_socketListen(uint8_t s) {
    W5500_writeSOCK1Byte(s, Sn_MR, MR_TCP);
    W5500_writeSOCK1Byte(s, Sn_CR, OPEN);
    
    delay(5);
    if (W5500_readSOCK1Byte(s, Sn_SR) != SOCK_INIT) {
        W5500_writeSOCK1Byte(s, Sn_CR, CLOSE);
        return FALSE;
    }
    
    W5500_writeSOCK1Byte(s, Sn_CR, LISTEN);
    delay(5);
    if (W5500_readSOCK1Byte(s, Sn_SR) != SOCK_LISTEN) {
        W5500_writeSOCK1Byte(s, Sn_CR, CLOSE);
        return FALSE;
    }
    
    return TRUE;
}

uint8_t W5500_socketUDP(uint8_t s) {
    W5500_writeSOCK1Byte(s, Sn_MR, MR_UDP);
    W5500_writeSOCK1Byte(s, Sn_CR, OPEN);
    delay(5);
    if (W5500_readSOCK1Byte(s, Sn_SR) != SOCK_UDP) {
        W5500_writeSOCK1Byte(s, Sn_CR, CLOSE);
        return FALSE;
    }
    return TRUE;
}

void W5500_intCallback() {
    uint8_t i = W5500_readSOCK1Byte(socket_num, Sn_IR);
    if (i != 0) {
        W5500_writeSOCK1Byte(socket_num, Sn_IR, i);
        
        if (i & IR_CON) {
            Serial.println("connected");
            uint8_t client_ip[4];
            W5500_readSOCK4Byte(socket_num, Sn_DIPR, client_ip);
            Serial.print("Destination IP: ");
            Serial.print(client_ip[0]);
            Serial.print(".");
            Serial.print(client_ip[1]);
            Serial.print(".");
            Serial.print(client_ip[2]);
            Serial.print(".");
            Serial.println(client_ip[3]);
        }
        if (i & IR_RECV) {
            Serial.println("Data received");
        }
        if (i & IR_DISCON) {
            W5500_writeSOCK1Byte(socket_num, Sn_CR, CLOSE);
        }
        if (i & IR_SEND_OK) {
            Serial.println("Data send ok");
        }
        if (i & IR_TIMEOUT) {
            Serial.println("Timeout occurred");
            W5500_writeSOCK1Byte(socket_num, Sn_CR, CLOSE);
        }
    }
}

void W5500_configureInterrupt(uint8_t intPin) {
    pinMode(intPin, INPUT_PULLUP);
    W5500_write1Byte(IMR, 0xFF);
    W5500_write1Byte(SIMR, 0x01);
    W5500_writeSOCK1Byte(socket_num, Sn_IMR, IMR_SENDOK | IMR_TIMEOUT | IMR_RECV | IMR_DISCON | IMR_CON);
    attachInterrupt(digitalPinToInterrupt(W5500_INTn), &W5500_intCallback, FALLING);
}
