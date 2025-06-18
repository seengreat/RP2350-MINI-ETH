from machine import Pin, SPI
import time
import w5500 

'''
Name:demo codes for RP2350-ETH
Version V1.0
For more details of the product, please refer to "https://www.seengreat.com".
'''
print("UDP Client mode test")

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
    print("Client IP:",ip_address)

    # Set the target IP address (the IP address of the PC)
    dat = bytearray([192, 168, 1, 17])  # Replace it with the actual IP address of the PC.
    eth.Write_SOCK_4_Byte(eth.socket_num, w5500.Sn_DIPR, dat)

    # Set the target port (the UDP port of the PC)
    eth.Write_SOCK_2_Byte(eth.socket_num, w5500.Sn_DPORTR, 8080) # Replace it with the actual Port of the PC.

# config Socket
def Socket_Config(eth):
    eth.Write_SOCK_1_Byte(eth.socket_num, w5500.Sn_MR, w5500.MR_UDP)
    # set Socket 0 port to 5000
    eth.Write_SOCK_2_Byte(eth.socket_num, w5500.Sn_PORT, 5000)
    # Set the maximum segment size to 1460
    eth.Write_SOCK_2_Byte(eth.socket_num, w5500.Sn_MSSR, 1460)
    # Set the size of the RX buffer to 2K
    eth.Write_SOCK_1_Byte(eth.socket_num, w5500.Sn_RXBUF_SIZE, 0x02)
    # Set the size of the TX buffer to 2K
    eth.Write_SOCK_1_Byte(eth.socket_num, w5500.Sn_TXBUF_SIZE, 0x02)
    # Open Socket 0
    eth.Write_SOCK_1_Byte(eth.socket_num, w5500.Sn_CR, w5500.OPEN)
    
    # Wait for Socket to open
    time.sleep_ms(5)
    
    # Check if Socket is in UDP mode
    if eth.Read_SOCK_1_Byte(eth.socket_num, w5500.Sn_SR) != w5500.SOCK_UDP:
        print("Failed to open UDP Socket")
        return False
    
    print("UDP Client started on port 5000")
    return True

# Parse UDP packet
def parse_udp_packet(data):
    if len(data) < 8:  # UDP header（8 bytes）
        print(len(data),data)
        return "Invalid UDP packet"
    
    # get the data length（bytes 7-8 of the UDP header）
    len_bytes = data[7:8]
    length = int.from_bytes(len_bytes, 'big')
    
    # Ensure the data length does not exceed the available data range
    dl_bytes = data[8:8+length]
    
    return dl_bytes

if __name__ == "__main__":
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
    # Configure Socket 0 as UDP Client
    if not Socket_Config(eth):
        print("Failed to configure UDP Client")
        while True:
            pass
    # Set retry time and retry count
    eth.Write_2_Byte(w5500.RTR, 200)  # 200ms retry time
    eth.Write_1_Byte(w5500.RCR, 8)    # 8 retries
    
    while True:
        try:
            # Detect the Ethernet connection
            if not (eth.Read_1_Byte(w5500.PHYCFGR) & w5500.LINK):
                print("not link")
                # close Socket 0
                eth.Write_SOCK_1_Byte(eth.socket_num, w5500.Sn_CR, w5500.CLOSE)
                # Wait for the Ethernet connection to be restored.
                while not (eth.Read_1_Byte(w5500.PHYCFGR) & w5500.LINK):
                    time.sleep_ms(100) 
                print("linked!")
        
            # Send data to PC
            send_data = b"Hello SEENGREAT, this is W5500 UDP Client demo\r\n"
            eth.Write_SOCK_Data_Buffer(eth.socket_num, send_data, len(send_data))
            print("Data sent to PC:", send_data)

            # Check if data was sent successfully
            i = eth.Read_SOCK_1_Byte(eth.socket_num, w5500.Sn_IR)
            if i & w5500.IR_SEND_OK:
                eth.Write_SOCK_1_Byte(eth.socket_num, w5500.Sn_IR, w5500.IR_SEND_OK)  # Clear flag
            else:
                print("Failed to send data")

            # Check if data is received from PC
            rx_size = eth.Read_SOCK_2_Byte(eth.socket_num, w5500.Sn_RX_RSR)
            if rx_size > 0:
                # Read received data
                recv_data = eth.Read_SOCK_Data_Buffer(eth.socket_num)
                if recv_data != 0:
                    parsed_data = parse_udp_packet(recv_data)
                    print("Data received from PC:", parsed_data)
                else:
                    print("No data received from PC")

            time.sleep_ms(1500)  # Send data every 1.5 second
        except Exception as e:
            print("An error occurred:",e)
            print("Program terminated by user. Closing Socket...")
            eth.Write_SOCK_1_Byte(eth.socket_num, w5500.Sn_CR, w5500.CLOSE)
            break