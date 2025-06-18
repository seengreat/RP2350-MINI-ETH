import array, time
import rp2
from rp2 import PIO, StateMachine, asm_pio
import machine
from machine import Pin, SPI
import w5500

gp0 = machine.Pin(0, machine.Pin.OUT)
gp1 = machine.Pin(1, machine.Pin.OUT)
gp2 = machine.Pin(2, machine.Pin.OUT)
gp3 = machine.Pin(3, machine.Pin.OUT)
gp4 = machine.Pin(4, machine.Pin.OUT)
gp5 = machine.Pin(5, machine.Pin.OUT)
gp6 = machine.Pin(6, machine.Pin.OUT)
gp7 = machine.Pin(7, machine.Pin.OUT)
gp8 = machine.Pin(8, machine.Pin.OUT)
gp9 = machine.Pin(9, machine.Pin.OUT)
gp10 = machine.Pin(10, machine.Pin.OUT)
gp11 = machine.Pin(11, machine.Pin.OUT)
gp22 = machine.Pin(22, machine.Pin.OUT)
gp26 = machine.Pin(26, machine.Pin.OUT)
gp27 = machine.Pin(27, machine.Pin.OUT)
gp28 = machine.Pin(28, machine.Pin.OUT)

gpio = [gp0, gp1, gp2, gp3, gp4, gp5, gp6, gp7, gp8, gp9, gp10, gp11, gp22, gp26, gp27, gp28]

gpio_value = 1

# Configure the number of WS2812 LEDs.
NUM_LEDS = 1

@asm_pio(sideset_init=PIO.OUT_LOW, out_shiftdir=PIO.SHIFT_LEFT, autopull=True, pull_thresh=24)
def ws2812():
    T1 = 2
    T2 = 5
    T3 = 3
    wrap_target()
    label("bitloop")
    out(x, 1)               .side(0)    [T3 - 1] 
    jmp(not_x, "do_zero")   .side(1)    [T1 - 1] 
    jmp("bitloop")          .side(1)    [T2 - 1] 
    label("do_zero")
    nop()                   .side(0)    [T2 - 1]
    wrap()


# W5500 pin definition
W5500_RSTn = Pin(20, Pin.OUT)
W5500_INTn = Pin(21, Pin.IN, Pin.PULL_UP)
W5500_CSn = Pin(17, Pin.OUT)
W5500_MOSI = Pin(19)
W5500_MISO = Pin(16)
W5500_SCLK = Pin(18)
 
# config W5500
def W5500_Config(eth):
    # config gateway IP to 192.168.0.1
    dat = bytearray([192, 168, 1, 1])
    eth.Write_Bytes(w5500.GAR, dat, 4)
    gateway = f"{dat[0]}.{dat[1]}.{dat[2]}.{dat[3]}"
    print("Gateway:",gateway)
    # Set the subnet mask to 255.255.255.0
    dat = bytearray([255, 255, 255, 0])
    eth.Write_Bytes(w5500.SUBR, dat, 4)
    sub_addr = f"{dat[0]}.{dat[1]}.{dat[2]}.{dat[3]}"
    print("Sub mask:",sub_addr)

    # set MAC address to 0x48,0x53,0x00,0x57,0x55,0x00
    dat = bytearray([0x48, 0x53, 0x00, 0x57, 0x55, 0x00])
    eth.Write_Bytes(w5500.SHAR, dat, 6)
    mac_address = ':'.join(map(lambda x: '{:02x}'.format(x), dat))
    print("MAC: ",mac_address)

    # set W5500 IP to 192.168.0.20
    dat = bytearray([192, 168, 1, 101])
    eth.Write_Bytes(w5500.SIPR, dat, 4)
    ip_address = f"{dat[0]}.{dat[1]}.{dat[2]}.{dat[3]}"
    print("Server IP:",ip_address)

# detect gateway
def Detect_Gateway(eth):
    # Set a target in a different subnet IP
    dat = bytearray([192 + 1, 168 + 1, 0 + 1, 20 + 1])
    eth.Write_SOCK_4_Byte(eth.socket_num, w5500.Sn_DIPR, dat)

    # set Socket to TCP mode
    eth.Write_SOCK_1_Byte(eth.socket_num, w5500.Sn_MR, w5500.MR_TCP)
    # open Socket 
    eth.Write_SOCK_1_Byte(eth.socket_num, w5500.Sn_CR, w5500.OPEN)

    # wait
    time.sleep_ms(5)
    # Check if the Socket status is SOCK_INIT.
    if eth.Read_SOCK_1_Byte(eth.socket_num, w5500.Sn_SR) != w5500.SOCK_INIT:
        # if not，close Socket
        eth.Write_SOCK_1_Byte(eth.socket_num, w5500.Sn_CR, w5500.CLOSE)
        return w5500.FALSE

    # 发起连接
    eth.Write_SOCK_1_Byte(eth.socket_num, w5500.Sn_CR, w5500.CONNECT)

    while True:
        # Check if it has timed out.
        if eth.Read_SOCK_1_Byte(eth.socket_num, w5500.Sn_IR) & w5500.IR_TIMEOUT:
            # if time out，close Socket
            eth.Write_SOCK_1_Byte(eth.socket_num, w5500.Sn_CR, w5500.CLOSE)
            return w5500.FALSE
        # Check if the gateway is detected.
        elif eth.Read_SOCK_1_Byte(eth.socket_num, w5500.Sn_DHAR) != 0xff:
            # If the gateway is detected ，close Socket
            eth.Write_SOCK_1_Byte(eth.socket_num, w5500.Sn_CR, w5500.CLOSE)
            break

    return w5500.TRUE

# config Socket
def Socket_Config(eth):
    # set Socket 0 port to 5000
    eth.Write_SOCK_2_Byte(eth.socket_num, w5500.Sn_PORT, 5000)
    # Set the maximum segment size to 1460
    eth.Write_SOCK_2_Byte(eth.socket_num, w5500.Sn_MSSR, 1460)
    # Set the size of the RX buffer to 2K
    eth.Write_SOCK_1_Byte(eth.socket_num, w5500.Sn_RXBUF_SIZE, 0x02)
    # Set the size of the TX buffer to 2K
    eth.Write_SOCK_1_Byte(eth.socket_num, w5500.Sn_TXBUF_SIZE, 0x02)

# Process the received data.
def Loop_Back(eth):
    S_Buffer = eth.Read_SOCK_Data_Buffer(eth.socket_num)
    if S_Buffer == 0:
        return
    print("len:",len(S_Buffer),S_Buffer)
    eth.Write_SOCK_Data_Buffer(eth.socket_num, S_Buffer, len(S_Buffer))
    while True:
        i = eth.Read_SOCK_1_Byte(eth.socket_num, w5500.Sn_IR)
        if (i & w5500.IR_TIMEOUT) or (i & w5500.IR_SEND_OK):
            break
                
if __name__ == "__main__":
    # Create the StateMachine with the ws2812 program, outputting on Pin(16).
    sm = StateMachine(0, ws2812, freq=8000000, sideset_base=Pin(25))

    # Start the StateMachine, it will wait for data on its FIFO.
    sm.active(1)

    # Display a pattern on the LEDs via an array of LED RGB values.
    ar = array.array("I", [0 for _ in range(NUM_LEDS)]) 
      
    print("red")
    for j in range(128): 
        ar = j<<8
        sm.put(ar,8) 
        time.sleep_ms(20)
        
    print("green")
    for j in range(128): 
        ar = j<<16
        sm.put(ar,8) 
        time.sleep_ms(20)
        
    print("blue")
    for j in range(128): 
        ar = j
        sm.put(ar,8) 
        time.sleep_ms(20)
        
    print("white")
    for j in range(256):
        ar = (j<<16) + (j<<8) + j
        sm.put(ar,8) 
        time.sleep_ms(20)
        
    print("turn off")    
    ar = 0x000000    
    sm.put(ar,8)
    print("TCP Server mode test")
    # Initialize the SPI
    spi = SPI(0, baudrate=1000000, polarity=0, phase=0, sck=W5500_SCLK, mosi=W5500_MOSI, miso=W5500_MISO)
    # Initialize W5500
    eth = w5500.W5500(spi, W5500_CSn, W5500_RSTn, W5500_INTn)
    eth.get_ver()
    # delay 100ms
    time.sleep_ms(100)
    # Detect the Ethernet connection
    while True:
        time.sleep_ms(100)
        i = eth.Read_1_Byte(w5500.PHYCFGR)
        if i & w5500.LINK:
            print("linked")
            break

    # config W5500
    W5500_Config(eth)
    # detect gateway
    if Detect_Gateway(eth):
        print("detect gateway ok")

    while True:
        # Detect the Ethernet connection
        if not (eth.Read_1_Byte(w5500.PHYCFGR) & w5500.LINK):
            print("not link")
            # close Socket 0
            eth.Write_SOCK_1_Byte(eth.socket_num, w5500.Sn_CR, w5500.CLOSE)
            # Wait for the Ethernet connection to be restored.
            while not (eth.Read_1_Byte(w5500.PHYCFGR) & w5500.LINK):
                time.sleep_ms(100) 
            print("linked!")
        # read Socket 0 state
        i = eth.Read_SOCK_1_Byte(eth.socket_num, w5500.Sn_SR)
        if i == 0:
            Socket_Config(eth)
            while not eth.Socket_Listen(eth.socket_num):
                time.sleep(1)
        elif i == w5500.SOCK_ESTABLISHED:
            Loop_Back(eth)
        time.sleep(1)  # Wait before sending the next data
        if gpio_value == 1:
            gpio_value = 0
            print("GPIO LOW")
        else:
            gpio_value = 1
            print("GPIO HIGH")
        for j,pio in enumerate(gpio):    
            pio.value(gpio_value) 
