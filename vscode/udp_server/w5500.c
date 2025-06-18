#include <string.h>
#include <stdlib.h>	
#include <stdio.h>
#include <stdint.h>
#include "w5500.h"


uint8_t socket_num = 0;

void W5500_Gpio_Init(void) 
{
    spi_init(spi0, 1000 * 1000);
    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(W5500_SCLK, GPIO_FUNC_SPI);
    gpio_set_function(W5500_MISO, GPIO_FUNC_SPI);
    gpio_set_function(W5500_MOSI, GPIO_FUNC_SPI);
    gpio_init(W5500_RSTn);
    gpio_set_dir(W5500_RSTn, GPIO_OUT);

    gpio_init(W5500_CSn);
    gpio_set_dir(W5500_CSn, GPIO_OUT);
    gpio_put(W5500_CSn, 1); 
    W5500_configureInterrupt(W5500_INTn);
    W5500_reset();

}

void W5500_reset() {
     gpio_put(W5500_RSTn, 0); 
     sleep_ms(10);
     gpio_put(W5500_RSTn, 1); 
     sleep_ms(500);
}

void W5500_getVer() {
    uint8_t ver = W5500_read1Byte(VERR);
    printf("Chip version: %02x\r\n",ver);
}

void W5500_write1Byte(uint16_t reg, uint8_t dat) {
    gpio_put(W5500_CSn, 0); 
    uint8_t buffer[4] = {
        (reg >> 8) & 0xFF,
        reg & 0xFF, 
        FDM1 | RWB_WRITE | COMMON_R, 
        dat};

    spi_write_blocking(spi0, buffer, 4);  
    gpio_put(W5500_CSn, 1); 
}

void W5500_write2Byte(uint16_t reg, uint16_t dat) {
    gpio_put(W5500_CSn, 0); 
    uint8_t buffer[5] = {
        reg >> 8, 
        reg & 0xFF,
        FDM2 | RWB_WRITE | COMMON_R,
        dat >> 8,
        dat & 0xFF
    }; //Split into high and low bytes
    spi_write_blocking(spi0, buffer, 5);
    gpio_put(W5500_CSn, 1); 
}

void W5500_writeBytes(uint16_t reg, uint8_t* dat, uint16_t Size) {
    gpio_put(W5500_CSn, 0); 
    uint8_t buffer[3] = {
        reg >> 8, 
        reg & 0xFF,
        VDM | RWB_WRITE | COMMON_R
    }; //Split into high and low bytes
    spi_write_blocking(spi0, buffer, 3);

    spi_write_blocking(spi0, dat, Size);

    gpio_put(W5500_CSn, 1); 
}

uint8_t W5500_read1Byte(uint16_t reg) {
    gpio_put(W5500_CSn, 0); 
    uint8_t buffer[3] = {
        reg >> 8, 
        reg & 0xFF,
        FDM1 | RWB_READ | COMMON_R
    }; //Split into high and low bytes
    spi_write_blocking(spi0, buffer, 3);

    spi_read_blocking(spi0, 0xFF, buffer, 1);
    uint8_t dat = buffer[0];
    gpio_put(W5500_CSn, 1); 
    return dat;
}

void W5500_writeSOCK1Byte(uint8_t s, uint16_t reg, uint8_t dat) {
    gpio_put(W5500_CSn, 0); 
    uint8_t buffer[4] = {
        reg >> 8, 
        reg & 0xFF,
        FDM1 | RWB_WRITE | (s * 0x20 + 0x08),
        dat
    }; //Split into high and low bytes
    spi_write_blocking(spi0, buffer, 4);

    gpio_put(W5500_CSn, 1); 
}

void W5500_writeSOCK2Byte(uint8_t s, uint16_t reg, uint16_t dat) {
    gpio_put(W5500_CSn, 0); 
    uint8_t buffer[5] = {
        reg >> 8, 
        reg & 0xFF,
        FDM2 | RWB_WRITE | (s * 0x20 + 0x08),
        dat >> 8,
        dat & 0xFF
    }; //Split into high and low bytes
    int result = spi_write_blocking(spi0, buffer, 5);

    gpio_put(W5500_CSn, 1); 
}

void W5500_writeSOCK4Byte(uint8_t s, uint16_t reg, uint8_t* dat) {
    gpio_put(W5500_CSn, 0); 
    uint8_t buffer[3] = {reg >> 8, reg & 0xFF,FDM4 | RWB_WRITE | (s * 0x20 + 0x08)}; //Split into high and low bytes
    int result = spi_write_blocking(spi0, buffer, 3);
    spi_write_blocking(spi0, dat, 4);   
    gpio_put(W5500_CSn, 1); 
}

uint8_t W5500_readSOCK1Byte(uint8_t s, uint16_t reg) {
    gpio_put(W5500_CSn, 0); 
    uint8_t buffer[3] = {
        reg >> 8, 
        reg & 0xFF,
        FDM1 | RWB_READ | (s * 0x20 + 0x08)
    }; //Split into high and low bytes
    spi_write_blocking(spi0, buffer, 3);

    spi_read_blocking(spi0, 0, buffer, 1);
    uint8_t dat = buffer[0];
    gpio_put(W5500_CSn, 1); 
    return dat;
}

uint16_t W5500_readSOCK2Byte(uint8_t s, uint16_t reg) {
    gpio_put(W5500_CSn, 0); 
    uint8_t buffer[3] = {
        reg >> 8, 
        reg & 0xFF,
        FDM2 | RWB_READ | (s * 0x20 + 0x08)
    }; //Split into high and low bytes
    spi_write_blocking(spi0, buffer, 3);

    spi_read_blocking(spi0, 0, buffer, 2);
    uint16_t dat = (buffer[0]<<8)|buffer[1];
    gpio_put(W5500_CSn, 1); 
    return dat;
}

void W5500_readSOCK4Byte(uint8_t s, uint16_t reg, uint8_t* dat) {
    gpio_put(W5500_CSn, 0); 
    uint8_t buffer[3] = {
        reg >> 8, 
        reg & 0xFF,
        FDM4 | RWB_READ | (s * 0x20 + 0x08)
    }; //Split into high and low bytes
    spi_write_blocking(spi0, buffer, 3);

    spi_read_blocking(spi0, 0, dat, 4);

    gpio_put(W5500_CSn, 1); 
}

uint16_t W5500_readSOCKDataBuffer(uint8_t s, uint8_t* buf) {
    uint8_t temp[4];
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
    
    gpio_put(W5500_CSn, 0); 
    temp[0] = offset >> 8;
    temp[1] = offset & 0xFF;
    temp[2] = VDM | RWB_READ | (s * 0x20 + 0x18); 
    spi_write_blocking(spi0, temp, 3); 
    
    if ((offset + rx_size) < S_RX_SIZE) {
        spi_read_blocking(spi0, 0, buf, rx_size);

    } else {
        uint16_t first_part_size = S_RX_SIZE - offset;
        spi_read_blocking(spi0, 0, buf, first_part_size);

        
        gpio_put(W5500_CSn, 1); 
        gpio_put(W5500_CSn, 0);
        temp[0] = 0;
        temp[1] = 0;
        temp[2] = VDM | RWB_READ | (s * 0x20 + 0x18); 
        spi_write_blocking(spi0, temp, 3);         


        spi_read_blocking(spi0, 0, buf+first_part_size, rx_size - first_part_size);

    }
    gpio_put(W5500_CSn, 1); 
    
    offset1 += rx_size;
    W5500_writeSOCK2Byte(s, Sn_RX_RD, offset1);
    W5500_writeSOCK1Byte(s, Sn_CR, RECV);
    
    return rx_size;
}

void W5500_writeSOCKDataBuffer(uint8_t s, uint8_t* dat, uint16_t Size) {
    uint8_t temp[4];
    uint16_t offset = W5500_readSOCK2Byte(s, Sn_TX_WR);
    uint16_t offset1 = offset;
    offset &= (S_TX_SIZE - 1);
    
    gpio_put(W5500_CSn, 0); 
    temp[0] = offset >> 8;
    temp[1] = offset & 0xFF;
    temp[2] = VDM | RWB_WRITE | (s * 0x20 + 0x10); 
    spi_write_blocking(spi0, temp, 3);     
    
    if ((offset + Size) < S_TX_SIZE) {
        spi_write_blocking(spi0, dat, Size);    

    } else {
        uint16_t first_part_size = S_TX_SIZE - offset;
        spi_write_blocking(spi0, dat, first_part_size); 
        
        gpio_put(W5500_CSn, 1); 
        gpio_put(W5500_CSn, 0); 
        temp[0] = 0;
        temp[1] = 0;
        temp[2] = VDM | RWB_WRITE | (s * 0x20 + 0x10); 
        spi_write_blocking(spi0, temp, 3);

        spi_write_blocking(spi0, dat+first_part_size, Size-first_part_size);

    }
    gpio_put(W5500_CSn, 1); 
    
    offset1 += Size;
    W5500_writeSOCK2Byte(s, Sn_TX_WR, offset1);
    W5500_writeSOCK1Byte(s, Sn_CR, SEND);
}

// uint8_t W5500_socketConnect(uint8_t s) {
//     W5500_writeSOCK1Byte(s, Sn_MR, MR_TCP);
//     W5500_writeSOCK1Byte(s, Sn_CR, OPEN);
    
//     sleep_ms(5);
//     if (W5500_readSOCK1Byte(s, Sn_SR) != SOCK_INIT) {
//         W5500_writeSOCK1Byte(s, Sn_CR, CLOSE);
//         return FALSE;
//     }
    
//     W5500_writeSOCK1Byte(s, Sn_CR, CONNECT);
//     return TRUE;
// }
uint8_t W5500_socketConnect(uint8_t s) {
    uint8_t retry_count = 5; //
    while (retry_count > 0) {
        W5500_writeSOCK1Byte(s, Sn_MR, MR_TCP);
        W5500_writeSOCK1Byte(s, Sn_CR, OPEN);
        
        sleep_ms(5);
        if (W5500_readSOCK1Byte(s, Sn_SR) != SOCK_INIT) {
            W5500_writeSOCK1Byte(s, Sn_CR, CLOSE);
            retry_count--;
            continue;
        }
        
        W5500_writeSOCK1Byte(s, Sn_CR, CONNECT);
        sleep_ms(100); //
        if (W5500_readSOCK1Byte(s, Sn_SR) == SOCK_ESTABLISHED) {
            return TRUE;
        }
        W5500_writeSOCK1Byte(s, Sn_CR, CLOSE);
        retry_count--;
    }
    return FALSE;
}

uint8_t W5500_socketListen(uint8_t s) {
    W5500_writeSOCK1Byte(s, Sn_MR, MR_TCP);
    W5500_writeSOCK1Byte(s, Sn_CR, OPEN);
    
    sleep_ms(5);
    if (W5500_readSOCK1Byte(s, Sn_SR) != SOCK_INIT) {
        W5500_writeSOCK1Byte(s, Sn_CR, CLOSE);
        return FALSE;
    }
    
    W5500_writeSOCK1Byte(s, Sn_CR, LISTEN);
    sleep_ms(5);
    if (W5500_readSOCK1Byte(s, Sn_SR) != SOCK_LISTEN) {
        W5500_writeSOCK1Byte(s, Sn_CR, CLOSE);
        return FALSE;
    }
    
    return TRUE;
}

uint8_t W5500_socketUDP(uint8_t s) {
    W5500_writeSOCK1Byte(s, Sn_MR, MR_UDP);
    W5500_writeSOCK1Byte(s, Sn_CR, OPEN);
    sleep_ms(5);
    if (W5500_readSOCK1Byte(s, Sn_SR) != SOCK_UDP) {
        W5500_writeSOCK1Byte(s, Sn_CR, CLOSE);
        return FALSE;
    }
    return TRUE;
}

void W5500_intCallback(uint gpio, uint32_t events) {

    if (gpio == W5500_INTn && (events & GPIO_IRQ_EDGE_FALL)) {
        uint8_t i = W5500_readSOCK1Byte(socket_num, Sn_IR);
        if (i != 0) {
            W5500_writeSOCK1Byte(socket_num, Sn_IR, i);
            if (i & IR_CON) {
                uint8_t client_ip[4];
                printf("connected\r\n");
                W5500_readSOCK4Byte(socket_num, Sn_DIPR, client_ip);
                printf("Destination IP:%d.%d.%d.%d\r\n",client_ip[0],client_ip[1],client_ip[2],client_ip[3]);
            }
            if (i & IR_RECV) {
                printf("Data received\r\n");
            }
            if (i & IR_DISCON) {
                // printf("Disconnected\r\n");
                // W5500_writeSOCK1Byte(socket_num, Sn_CR, CLOSE);
                printf("Disconnected\r\n");
                W5500_writeSOCK1Byte(socket_num, Sn_CR, CLOSE);
                
                W5500_writeSOCK1Byte(socket_num, Sn_SR, SOCK_CLOSED); 

                uint16_t tx_offset = W5500_readSOCK2Byte(socket_num, Sn_TX_WR);
                W5500_writeSOCK2Byte(socket_num, Sn_TX_WR, tx_offset); 

                uint16_t rx_offset = W5500_readSOCK2Byte(socket_num, Sn_RX_RD);
                W5500_writeSOCK2Byte(socket_num, Sn_RX_RD, rx_offset); 
            }
            if (i & IR_SEND_OK) {
                printf("Data send ok\r\n");
            }
            if (i & IR_TIMEOUT) {
                printf("Timeout occurred\r\n");
                W5500_writeSOCK1Byte(socket_num, Sn_CR, CLOSE);
            }

        }
    }

}

void W5500_configureInterrupt(uint8_t intPin) {
    gpio_init(intPin);
    gpio_set_dir(intPin, GPIO_IN);
    gpio_pull_up(intPin);  

    W5500_write1Byte(IMR, 0xFF);
    uint8_t imr = W5500_read1Byte(IMR);
    W5500_write1Byte(SIMR, 0x01);
    imr = W5500_read1Byte(SIMR);

    W5500_writeSOCK1Byte(socket_num, Sn_IMR, IMR_SENDOK | IMR_TIMEOUT | IMR_RECV | IMR_DISCON | IMR_CON);

    gpio_set_irq_enabled_with_callback(intPin, GPIO_IRQ_EDGE_FALL, true, &W5500_intCallback);
}
