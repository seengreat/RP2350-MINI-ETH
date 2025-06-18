from machine import Pin, SPI
import time
import w5500 

'''
Name:demo codes for RP2350-ETH
Version V1.0
For more details of the product, please refer to "https://www.seengreat.com".
'''
print("UDP Server mode test")
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

    # set W5500 IP to 192.168.1.101
    dat = bytearray([192, 168, 1, 101])
    eth.Write_Bytes(w5500.SIPR, dat, 4)
    ip_address = f"{dat[0]}.{dat[1]}.{dat[2]}.{dat[3]}"
    print("Server IP:",ip_address)

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
    
    print("UDP Server started on port 5000")
    return True

def parse_udp_packet(data):
    """Parse UDP packet to extract source IP, port and payload"""
    if len(data) < 8:  # UDP header is 8 bytes
        return None, None, None
    
    # Extract source IP and port (first 8 bytes are UDP header)
    src_ip = data[0:4]    # First 4 bytes are source IP
    src_port = int.from_bytes(data[4:6], 'big')  # Bytes 5-6 are source port
    payload = data[8:]    # Actual data starts from byte 9
    
    return src_ip, src_port, payload

                
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
    # Configure Socket 0 as UDP Server
    if not Socket_Config(eth):
        print("Failed to configure UDP Server")
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
        
            # Check if data is received
            rx_size = eth.Read_SOCK_2_Byte(eth.socket_num, w5500.Sn_RX_RSR)
            if rx_size > 0:
                # Read received data
                raw_data = eth.Read_SOCK_Data_Buffer(eth.socket_num)
                # Parse source IP and port
                src_ip, src_port, payload = parse_udp_packet(raw_data)
                if not src_ip:
                    print("Invalid UDP packet format")
                print(f"Received {len(payload)} bytes from {'.'.join(map(str, src_ip))}:{src_port}:{payload}")
                    
                # Set reply destination (now we configure Sn_DIPR/Sn_DPORTR)
                eth.Write_SOCK_4_Byte(eth.socket_num, w5500.Sn_DIPR, src_ip)
                eth.Write_SOCK_2_Byte(eth.socket_num, w5500.Sn_DPORTR, src_port)
                    
                    # Send reply (echo payload or custom data)
                eth.Write_SOCK_Data_Buffer(eth.socket_num, payload, len(payload))
#             time.sleep_ms(500)
        except Exception as e:
            print("An error occurred:",e)
            print("Program terminated by user. Closing Socket...")
            eth.Write_SOCK_1_Byte(eth.socket_num, w5500.Sn_CR, w5500.CLOSE)
            break
