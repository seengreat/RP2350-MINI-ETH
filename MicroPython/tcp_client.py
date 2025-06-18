from machine import Pin, SPI
import time
import w5500 

'''
Name:demo codes for RP2350-ETH
Version V1.0
For more details of the product, please refer to "https://www.seengreat.com".
'''
print("TCP Client mode test")
# W5500 pin definition
W5500_RSTn = Pin(20, Pin.OUT)
W5500_INTn = Pin(21, Pin.IN, Pin.PULL_UP)
W5500_CSn = Pin(17, Pin.OUT)
W5500_MOSI = Pin(19)
W5500_MISO = Pin(16)
W5500_SCLK = Pin(18)
 
# config W5500
def W5500_Config(eth):
    # config gateway IP to 192.168.1.1
    dat = bytearray([192, 168, 0, 1])
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

    # set W5500 IP to 192.168.1.101
    dat = bytearray([192, 168, 0, 101])
    eth.Write_Bytes(w5500.SIPR, dat, 4)
    ip_address = f"{dat[0]}.{dat[1]}.{dat[2]}.{dat[3]}"
    print("Client IP:",ip_address)

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
    # set Socket port to 5000
    eth.Write_SOCK_2_Byte(eth.socket_num, w5500.Sn_PORT, 5000)
    # Set the maximum segment size to 1460
    eth.Write_SOCK_2_Byte(eth.socket_num, w5500.Sn_MSSR, 1460)
    # Set the size of the RX buffer to 2K
    eth.Write_SOCK_1_Byte(eth.socket_num, w5500.Sn_RXBUF_SIZE, 0x02)
    # Set the size of the TX buffer to 2K
    eth.Write_SOCK_1_Byte(eth.socket_num, w5500.Sn_TXBUF_SIZE, 0x02)
    # Set the destination IP address 
    dat = bytearray([192, 168, 0, 113]) #your tcp server IP
    eth.Write_SOCK_4_Byte(eth.socket_num, w5500.Sn_DIPR, dat)
    # Set the destination port (example: 5000)
    eth.Write_SOCK_2_Byte(eth.socket_num, w5500.Sn_DPORTR, 5000)
                
if __name__ == "__main__":
    # Initialize the SPI
    spi = SPI(0, baudrate=1000000, polarity=0, phase=0, sck=W5500_SCLK, mosi=W5500_MOSI, miso=W5500_MISO)
    # Initialize W5500
    eth = w5500.W5500(spi, W5500_CSn, W5500_RSTn, W5500_INTn)
    eth.socket_num = 0
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
        if i == w5500.SOCK_CLOSED or i == w5500.SOCK_CLOSE_WAIT:
             print("Socket closed, reconnecting...\r\n")
             eth.Write_SOCK_1_Byte(eth.socket_num, w5500.Sn_CR, w5500.CLOSE)
             time.sleep_ms(1000)
             Socket_Config(eth) 
             if not eth.Socket_Connect(eth.socket_num):
                print("Reconnect failed, retrying...\r\n")
                time.sleep_ms(1000)
                continue
        elif i == w5500.SOCK_ESTABLISHED:
            # Send data to server
            send_data = b"Hello Server!"
            eth.Write_SOCK_Data_Buffer(eth.socket_num, send_data, len(send_data))
            print("Data sent to server:", send_data)
            
            # Receive data from server
            recv_data = eth.Read_SOCK_Data_Buffer(eth.socket_num)
            if recv_data != 0:
                print("Data received from server:", recv_data)
            
            time.sleep(1)  # Wait before sending the next data