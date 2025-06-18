from machine import Pin, SPI
import time

PHYCFGR = 0x002e
MR = 0x0000
RST = 0x80
WOL = 0x20
PB = 0x10
PPP = 0x08
FARP = 0x02

GAR = 0x0001
SUBR = 0x0005
SHAR = 0x0009
SIPR = 0x000f

INTLEVEL = 0x0013
IR = 0x0015
CONFLICT = 0x80
UNREACH = 0x40
PPPOE = 0x20
MP = 0x10

IMR = 0x0016
IM_IR7 = 0x80
IM_IR6 = 0x40
IM_IR5 = 0x20
IM_IR4 = 0x10

SIR = 0x0017
S7_INT = 0x80
S6_INT = 0x40
S5_INT = 0x20
S4_INT = 0x10
S3_INT = 0x08
S2_INT = 0x04
S1_INT = 0x02
S0_INT = 0x01

SIMR = 0x0018
S7_IMR = 0x80
S6_IMR = 0x40
S5_IMR = 0x20
S4_IMR = 0x10
S3_IMR = 0x08
S2_IMR = 0x04
S1_IMR = 0x02
S0_IMR = 0x01

RTR = 0x0019
RCR = 0x001b

PTIMER = 0x001c
PMAGIC = 0x001d
PHA = 0x001e
PSID = 0x0024
PMRU = 0x0026

UIPR = 0x0028
UPORT = 0x002c

PHYCFGR = 0x002e
RST_PHY = 0x80
OPMODE = 0x40
DPX = 0x04
SPD = 0x02
LINK = 0x01

VERR = 0x0039
# ********************* Socket Register ******************
Sn_MR = 0x0000
MULTI_MFEN = 0x80
BCASTB = 0x40
ND_MC_MMB = 0x20
UCASTB_MIP6B = 0x10
MR_CLOSE = 0x00
MR_TCP = 0x01
MR_UDP = 0x02
MR_MACRAW = 0x04

Sn_CR = 0x0001
OPEN = 0x01
LISTEN = 0x02
CONNECT = 0x04
DISCON = 0x08
CLOSE = 0x10
SEND = 0x20
SEND_MAC = 0x21
SEND_KEEP = 0x22
RECV = 0x40

Sn_IR = 0x0002
IR_SEND_OK = 0x10
IR_TIMEOUT = 0x08
IR_RECV = 0x04
IR_DISCON = 0x02
IR_CON = 0x01

Sn_SR = 0x0003
SOCK_CLOSED = 0x00
SOCK_INIT = 0x13
SOCK_LISTEN = 0x14
SOCK_ESTABLISHED = 0x17
SOCK_CLOSE_WAIT = 0x1c
SOCK_UDP = 0x22
SOCK_MACRAW = 0x02
SOCK_SYNSEND = 0x15
SOCK_SYNRECV = 0x16
SOCK_FIN_WAI = 0x18
SOCK_CLOSING = 0x1a
SOCK_TIME_WAIT = 0x1b
SOCK_LAST_ACK = 0x1d

Sn_PORT = 0x0004
Sn_DHAR = 0x0006
Sn_DIPR = 0x000c
Sn_DPORTR = 0x0010

Sn_MSSR = 0x0012
Sn_TOS = 0x0015
Sn_TTL = 0x0016

Sn_RXBUF_SIZE = 0x001e
Sn_TXBUF_SIZE = 0x001f
Sn_TX_FSR = 0x0020
Sn_TX_RD = 0x0022
Sn_TX_WR = 0x0024
Sn_RX_RSR = 0x0026
Sn_RX_RD = 0x0028
Sn_RX_WR = 0x002a

Sn_IMR = 0x002c
IMR_SENDOK = 0x10
IMR_TIMEOUT = 0x08
IMR_RECV = 0x04
IMR_DISCON = 0x02
IMR_CON = 0x01

Sn_FRAG = 0x002d
Sn_KPALVTR = 0x002f

# ******************************************************************
# ************************ SPI Control Byte ************************
# ******************************************************************
# Operation mode bits
VDM = 0x00
FDM1 = 0x01
FDM2 = 0x02
FDM4 = 0x03

# Read_Write control bit
RWB_READ = 0x00
RWB_WRITE = 0x04

# Block select bits
COMMON_R = 0x00

# Socket 0
S0_REG = 0x08
S0_TX_BUF = 0x10
S0_RX_BUF = 0x18

# Socket 1
S1_REG = 0x28
S1_TX_BUF = 0x30
S1_RX_BUF = 0x38

# Socket 2
S2_REG = 0x48
S2_TX_BUF = 0x50
S2_RX_BUF = 0x58

# Socket 3
S3_REG = 0x68
S3_TX_BUF = 0x70
S3_RX_BUF = 0x78

# Socket 4
S4_REG = 0x88
S4_TX_BUF = 0x90
S4_RX_BUF = 0x98
# Socket 5
S5_REG = 0xa8
S5_TX_BUF = 0xb0
S5_RX_BUF = 0xb8
# Socket 6
S6_REG = 0xc8
S6_TX_BUF = 0xd0
S6_RX_BUF = 0xd8
# Socket 7
S7_REG = 0xe8
S7_TX_BUF = 0xf0
S7_RX_BUF = 0xf8
TRUE = 0xff
FALSE = 0x00
S_RX_SIZE = 2048
S_TX_SIZE = 2048

        
class W5500:
    def __init__(self, spi, cs_pin, rst_pin=None,int_pin=None):
        self.socket_num = 0
        self.spi = spi
        self.cs_pin = cs_pin
        self.rst_pin = rst_pin
        if self.rst_pin:
            self.rst_pin.init(Pin.OUT)
            self.reset()
        self.cs_pin.init(Pin.OUT)
        self.cs_pin.value(1)
        self.S_Buffer = [0] * 1470
        self.int_pin = int_pin
        if self.int_pin :
            self.configure_interrupt()

    def reset(self):
        if self.rst_pin:
            self.rst_pin.value(0)
            time.sleep_ms(10)
            self.rst_pin.value(1)
            time.sleep_ms(500)
            
    def get_ver(self):
        ver = self.Read_1_Byte(0x0039)
        print("chip version:",hex(ver))
        
    #Write W5500 Common Register a byte */
    def Write_1_Byte(self,reg, data):
        self.cs_pin.value(0)
        self.spi.write(bytearray([(reg >> 8) & 0xFF, reg & 0xFF, FDM1|RWB_WRITE|COMMON_R]))
        self.spi.write(bytearray([data]))
        self.cs_pin.value(1)
        
    def Write_2_Byte(self,reg, data):
        self.cs_pin.value(0)
        self.spi.write(bytearray([(reg >> 8) & 0xFF, reg & 0xFF, FDM2|RWB_WRITE|COMMON_R]))
        self.spi.write(bytearray([(data>>8) & 0xFF,data&0xff]))
        self.cs_pin.value(1)
        
    def Write_Bytes(self,reg, data, size):
        self.cs_pin.value(0)
        self.spi.write(bytearray([(reg >> 8) & 0xFF, reg & 0xFF, VDM|RWB_WRITE|COMMON_R]))
        for i in range(size):
            self.spi.write(bytearray([data[i]]))
        self.cs_pin.value(1)

    def Write_SOCK_1_Byte(self, s, reg, data):
        self.cs_pin.value(0)        
        self.spi.write(bytearray([(reg >> 8) & 0xFF, reg & 0xFF, FDM1|RWB_WRITE|(s*0x20+0x08)]))
        self.spi.write(bytearray([data]))
        self.cs_pin.value(1)
        
    def Write_SOCK_2_Byte(self, s, reg, data):
        self.cs_pin.value(0)        
        self.spi.write(bytearray([(reg >> 8) & 0xFF, reg & 0xFF, FDM2|RWB_WRITE|(s*0x20+0x08)]))
        self.spi.write(bytearray([(data>>8) & 0xFF,data&0xff]))
        self.cs_pin.value(1)
        
    def Write_SOCK_4_Byte(self, s, reg, data):
        self.cs_pin.value(0)        
        self.spi.write(bytearray([(reg >> 8) & 0xFF, reg & 0xFF, FDM4|RWB_WRITE|(s*0x20+0x08)]))
        self.spi.write(bytearray([data[0], data[1], data[2], data[3]]))
        self.cs_pin.value(1)
        
    def Read_1_Byte(self, reg):
        self.cs_pin.value(0)
        self.spi.write(bytearray([(reg >> 8) & 0xFF, reg & 0xFF, FDM1|RWB_READ|COMMON_R]))
        data = self.spi.read(1)
        self.cs_pin.value(1)
        return int.from_bytes(data)
    # Read W5500 Socket register 1 Byte */
    def Read_SOCK_1_Byte(self, s, reg):
        self.cs_pin.value(0)
        self.spi.write(bytearray([(reg >> 8) & 0xFF, reg & 0xFF, FDM1|RWB_READ|(s*0x20+0x08)]))
        data = self.spi.read(1)
        self.cs_pin.value(1)
        return int.from_bytes(data)

    def Read_SOCK_2_Byte(self, s, reg):
        self.cs_pin.value(0)
        self.spi.write(bytearray([(reg >> 8) & 0xFF, reg & 0xFF, FDM2|RWB_READ|(s*0x20+0x08)]))
        data = self.spi.read(2)
        self.cs_pin.value(1)
        return int.from_bytes(data, 'big')
    
    def Read_SOCK_4_Byte(self, s, reg):
        self.cs_pin.value(0)
        self.spi.write(bytearray([(reg >> 8) & 0xFF, reg & 0xFF, FDM4|RWB_READ|(s*0x20+0x08)]))
        data = self.spi.read(4)
        self.cs_pin.value(1)
        return data    
    # Define a function to read the data reception buffer of the W5500 Socket.
    def Read_SOCK_Data_Buffer(self, s):
        rx_size = self.Read_SOCK_2_Byte(s, Sn_RX_RSR)
        if rx_size == 0:  # If no data is received，return 0
            return 0
        if rx_size > 1460:
            rx_size = 1460
        offset = self.Read_SOCK_2_Byte(s, Sn_RX_RD)
        offset1 = offset
        offset &= (S_RX_SIZE - 1)  # Calculate the actual physical address.
        dat_ptr = bytearray(rx_size)  # Create a byte array to store data
        self.cs_pin.value(0)
        # 写入地址
        self.spi.write(bytes([offset>>8]))
        self.spi.write(bytes([offset&0Xff]))
        # write control byte
        control_byte = VDM | RWB_READ | (s * 0x20 + 0x18)
        self.spi.write(bytes([control_byte]))
        if (offset + rx_size) < S_RX_SIZE:
            # read data
            for i in range(rx_size):
                dummy_byte = bytes([0x00])
                read_buffer = bytearray(1)
                self.spi.write_readinto(dummy_byte, read_buffer)
                dat_ptr[i] = read_buffer[0]
        else:
            first_part_size = S_RX_SIZE - offset
            for i in range(first_part_size):
                dummy_byte = bytes([0x00])
                read_buffer = bytearray(1)
                self.spi.write_readinto(dummy_byte, read_buffer)
                dat_ptr[i] = read_buffer[0]

            self.cs_pin.value(1)
            self.cs_pin.value(0)
            self.spi.write(bytes([0x00]))
            self.spi.write(bytes([0x00]))
            # write control byte
            control_byte = VDM | RWB_READ | (s * 0x20 + 0x18)
            self.spi.write(bytes([control_byte]))

            for i in range(first_part_size, rx_size):
                dummy_byte = bytes([0x00])
                read_buffer = bytearray(1)
#                 received_byte = self.spi.write_readinto(dummy_byte)[0]
                self.spi.write_readinto(dummy_byte, read_buffer)
                dat_ptr[i] = read_buffer[0]
        self.cs_pin.value(1)
        # Update the offset.
        offset1 += rx_size
        self.Write_SOCK_2_Byte(s, Sn_RX_RD, offset1)
        self.Write_SOCK_1_Byte(s, Sn_CR, RECV)  # write RECV command
        return dat_ptr

    def Write_SOCK_Data_Buffer(self, s, dat_ptr, size):
        # Read the offset of the TX write pointer.
        offset = self.Read_SOCK_2_Byte(s, Sn_TX_WR)
        offset1 = offset
        offset &= (S_TX_SIZE - 1)  # Calculate the actual physical address.
        self.cs_pin.value(0)
        print("offset:",offset)
        self.spi.write(bytes([offset>>8]))
        self.spi.write(bytes([offset&0xff]))

        control_byte = VDM | RWB_WRITE | (s * 0x20 + 0x10)
        self.spi.write(bytes([control_byte]))

        if (offset + size) < S_TX_SIZE:
            for i in range(size):
                self.spi.write(bytes([dat_ptr[i]]))
        else:
            first_part_size = S_TX_SIZE - offset
            for i in range(first_part_size):
                self.spi.write(bytes([dat_ptr[i]]))
            self.cs_pin.value(1)
            self.cs_pin.value(0)
            self.spi.write(bytes([0x00]))
            self.spi.write(bytes([0x00]))
            self.spi.write(bytes([control_byte]))
            for i in range(first_part_size, size):
                self.spi.write(bytes([dat_ptr[i]]))
        self.cs_pin.value(1)

        offset1 += size
        self.Write_SOCK_2_Byte(s, Sn_TX_WR, offset1)
        # write SEND command
        self.Write_SOCK_1_Byte(s, Sn_CR, SEND)

    def Socket_Connect(self, s):
        # set Socket n to TCP mode
        self.Write_SOCK_1_Byte(s, Sn_MR, MR_TCP)
        # open Socket n
        self.Write_SOCK_1_Byte(s, Sn_CR, OPEN)

        # wait
        time.sleep_ms(5)
        # Check whether the Socket status is SOCK_INIT.
        if self.Read_SOCK_1_Byte(s, Sn_SR) != SOCK_INIT:
            # if not close Socket n
            self.Write_SOCK_1_Byte(s, Sn_CR, CLOSE)
            return FALSE

        # Initiate a connection.
        self.Write_SOCK_1_Byte(s, Sn_CR, CONNECT)
        return TRUE

    def Socket_Listen(self, s):
        # set Socket n to TCP mode
        self.Write_SOCK_1_Byte(s, Sn_MR, MR_TCP)
        # 打开 Socket n
        self.Write_SOCK_1_Byte(s, Sn_CR, OPEN)

        # wait
        time.sleep_ms(5)
        # Check whether the Socket status is SOCK_INIT.
        if self.Read_SOCK_1_Byte(s, Sn_SR) != SOCK_INIT:
            # if not, close Socket 
            self.Write_SOCK_1_Byte(s, Sn_CR, CLOSE)
            return FALSE

        # set Socket n to server mode
        self.Write_SOCK_1_Byte(s, Sn_CR, LISTEN)
        time.sleep_ms(5)
        # Check whether the Socket status is SOCK_LISTEN
        if self.Read_SOCK_1_Byte(s, Sn_SR) != SOCK_LISTEN:
            # if not, close Socket n
            self.Write_SOCK_1_Byte(s, Sn_CR, CLOSE)
            return FALSE

        return TRUE

    def Socket_UDP(self, s):
        # set Socket n to UDP mode
        self.Write_SOCK_1_Byte(s, Sn_MR, MR_UDP)
        # open Socket n
        self.Write_SOCK_1_Byte(s, Sn_CR, OPEN)
        time.sleep_ms(5)
        # Check whether the Socket status is SOCK_UDP
        if self.Read_SOCK_1_Byte(s, Sn_SR) != SOCK_UDP:
            # if not, close Socket n
            self.Write_SOCK_1_Byte(s, Sn_CR, CLOSE)
            return FALSE

        return TRUE
    def int_callback(self):
        # Read the interrupt flag.
        i = self.Read_SOCK_1_Byte(self.socket_num, Sn_IR)
        if i != 0:
            self.Write_SOCK_1_Byte(self.socket_num, Sn_IR, i)
            # Handle different interrupt events.
        if i & IR_CON:
            print("Client connected")
            # read client IP
            client_ip_bytes = self.Read_SOCK_4_Byte(self.socket_num,Sn_DIPR)
            client_ip = ".".join(map(str, client_ip_bytes))
            print(f"Client IP: {client_ip}")
        if i & IR_RECV:
            print("Data received")
        if i & IR_DISCON:
#             print("disconnected")
            self.Write_SOCK_1_Byte(self.socket_num,Sn_CR, CLOSE)
        if i & IR_SEND_OK:
            print("Data send ok")
        if i & IR_TIMEOUT:
            print("Timeout occurred")
            self.Write_SOCK_1_Byte(self.socket_num, Sn_CR, CLOSE)
            
    def configure_interrupt(self): 
        self.int_pin.init(Pin.IN, Pin.PULL_UP)
        self.Write_1_Byte(IMR, 0xFF)  # Enable all global interrupts.
        self.Write_1_Byte(SIMR, 0x01)  # Enable socket0 interrupts.
        # Configure the interrupt enable register of the Socket.
        register_address = Sn_IMR + self.socket_num * 0x0020
        self.Write_SOCK_1_Byte(self.socket_num, register_address, IMR_SENDOK | IMR_TIMEOUT | IMR_RECV | IMR_DISCON | IMR_CON)
        self.int_pin.irq(trigger=Pin.IRQ_FALLING, handler=lambda p: self.int_callback())

