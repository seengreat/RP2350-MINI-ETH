#ifndef W5500_H
#define W5500_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "pico/time.h"

#define W5500_RSTn 20
#define W5500_INTn 21
#define W5500_CSn  17
#define W5500_MOSI 19
#define W5500_MISO 16
#define W5500_SCLK 18  

// W5500 Register Addresses
#define MR          0x0000
#define GAR         0x0001
#define SUBR        0x0005
#define SHAR        0x0009
#define SIPR        0x000F
#define INTLEVEL    0x0013
#define IR          0x0015
#define IMR         0x0016
#define SIR         0x0017
#define SIMR        0x0018
#define RTR         0x0019
#define RCR         0x001B
#define PHYCFGR     0x002E
#define VERR        0x0039

// Socket Register Addresses
#define Sn_MR         0x0000
#define Sn_CR         0x0001
#define Sn_IR         0x0002
#define Sn_SR         0x0003
#define Sn_PORT       0x0004
#define Sn_DHAR       0x0006
#define Sn_DIPR       0x000C
#define Sn_DPORTR     0x0010
#define Sn_MSSR       0x0012
#define Sn_TOS        0x0015
#define Sn_TTL        0x0016
#define Sn_RXBUF_SIZE 0x001E
#define Sn_TXBUF_SIZE 0x001F
#define Sn_TX_FSR     0x0020
#define Sn_TX_RD      0x0022
#define Sn_TX_WR      0x0024
#define Sn_RX_RSR     0x0026
#define Sn_RX_RD      0x0028
#define Sn_RX_WR      0x002A
#define Sn_IMR        0x002C
#define Sn_FRAG       0x002D
#define Sn_KPALVTR    0x002F

// Mode Register Values
#define MR_CLOSE     0x00
#define MR_TCP       0x01
#define MR_UDP       0x02
#define MR_MACRAW    0x04

// Command Values
#define OPEN         0x01
#define LISTEN       0x02
#define CONNECT      0x04
#define DISCON       0x08
#define CLOSE        0x10
#define SEND         0x20
#define SEND_MAC     0x21
#define SEND_KEEP    0x22
#define RECV         0x40

// Interrupt Register Values
#define IR_SEND_OK   0x10
#define IR_TIMEOUT   0x08
#define IR_RECV      0x04
#define IR_DISCON    0x02
#define IR_CON       0x01

// Socket Interrupt Mask Values
#define IMR_SENDOK   0x10
#define IMR_TIMEOUT  0x08
#define IMR_RECV     0x04
#define IMR_DISCON   0x02
#define IMR_CON      0x01

// Socket Status Values
#define SOCK_CLOSED      0x00
#define SOCK_INIT        0x13
#define SOCK_LISTEN      0x14
#define SOCK_ESTABLISHED 0x17
#define SOCK_CLOSE_WAIT  0x1C
#define SOCK_UDP         0x22
#define SOCK_MACRAW      0x02

// PHYCFGR Values
#define LINK        0x01

// SPI Control Byte
#define VDM         0x00
#define FDM1        0x01
#define FDM2        0x02
#define FDM4        0x03
#define RWB_READ    0x00
#define RWB_WRITE   0x04
#define COMMON_R    0x00

// Socket Base Addresses
#define S0_REG      0x08
#define S0_TX_BUF   0x10
#define S0_RX_BUF   0x18

// Buffer Sizes
#define S_RX_SIZE   2048
#define S_TX_SIZE   2048

// Constants
#define TRUE        0xFF
#define FALSE       0x00


void W5500_Gpio_Init(void); 
void W5500_reset();
void W5500_getVer();
    
// Common Register Operations
void W5500_write1Byte(uint16_t reg, uint8_t dat);
void W5500_write2Byte(uint16_t reg, uint16_t dat);
void W5500_writeBytes(uint16_t reg, uint8_t* dat, uint16_t Size);
uint8_t W5500_read1Byte(uint16_t reg);
    
// Socket Register Operations
void W5500_writeSOCK1Byte(uint8_t s, uint16_t reg, uint8_t dat);
void W5500_writeSOCK2Byte(uint8_t s, uint16_t reg, uint16_t dat);
void W5500_writeSOCK4Byte(uint8_t s, uint16_t reg, uint8_t* dat);
uint8_t W5500_readSOCK1Byte(uint8_t s, uint16_t reg);
uint16_t W5500_readSOCK2Byte(uint8_t s, uint16_t reg);
void W5500_readSOCK4Byte(uint8_t s, uint16_t reg, uint8_t* dat);
    
// Data Buffer Operations
uint16_t W5500_readSOCKDataBuffer(uint8_t s, uint8_t* buf);
void W5500_writeSOCKDataBuffer(uint8_t s, uint8_t* dat, uint16_t Size);
    
// Socket Operations
uint8_t W5500_socketConnect(uint8_t s);
uint8_t W5500_socketListen(uint8_t s);
uint8_t W5500_socketUDP(uint8_t s);
    
// Interrupt Handling
void W5500_intCallback(uint gpio, uint32_t events);
void W5500_configureInterrupt(uint8_t intPin);
    
extern uint8_t socket_num;

#endif
