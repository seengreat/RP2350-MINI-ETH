from machine import Pin, SPI
import time
import w5500 

'''
Name: demo codes for RP2350-MINI(ETH)
Version V1.0
For more details of the product, please refer to "https://www.seengreat.com".
'''
print("UDP Servers mode test")

# W5500 pin definitions
W5500_RSTn = Pin(20, Pin.OUT)
W5500_INTn = Pin(21, Pin.IN, Pin.PULL_UP)
W5500_CSn = Pin(17, Pin.OUT)
W5500_MOSI = Pin(19)
W5500_MISO = Pin(16)
W5500_SCLK = Pin(18)

# Define multiple socket numbers
NUM_SOCKETS = 4  # Can be adjusted as needed
SOCKET_PORTS = [5000, 5001, 5002, 5003]  # Different port numbers

def W5500_Config(eth):
    """Configure W5500 basic network settings"""
    # Configure gateway IP to 192.168.0.1
    dat = bytearray([192, 168, 1, 1])
    eth.Write_Bytes(w5500.GAR, dat, 4)
    gateway = f"{dat[0]}.{dat[1]}.{dat[2]}.{dat[3]}"
    print("Gateway:", gateway)
    
    # Set subnet mask to 255.255.255.0
    dat = bytearray([255, 255, 255, 0])
    eth.Write_Bytes(w5500.SUBR, dat, 4)
    sub_addr = f"{dat[0]}.{dat[1]}.{dat[2]}.{dat[3]}"
    print("Sub mask:", sub_addr)

    # Set MAC address to 0x48,0x53,0x00,0x57,0x55,0x00
    dat = bytearray([0x48, 0x53, 0x00, 0x57, 0x55, 0x00])
    eth.Write_Bytes(w5500.SHAR, dat, 6)
    mac_address = ':'.join(map(lambda x: '{:02x}'.format(x), dat))
    print("MAC: ", mac_address)

    # Set W5500 IP to 192.168.0.20
    dat = bytearray([192, 168, 1, 101])
    eth.Write_Bytes(w5500.SIPR, dat, 4)
    ip_address = f"{dat[0]}.{dat[1]}.{dat[2]}.{dat[3]}"
    print("Server IP:", ip_address)
    

def Socket_Config(eth):
    """Configure all sockets as UDP servers"""
    for i in range(NUM_SOCKETS):
        eth.socket_num = i
        # Set socket to UDP mode
        eth.Write_SOCK_1_Byte(eth.socket_num, w5500.Sn_MR, w5500.MR_UDP)
        # Set socket port
        eth.Write_SOCK_2_Byte(eth.socket_num, w5500.Sn_PORT, SOCKET_PORTS[i])
        # Set maximum segment size to 1460
        eth.Write_SOCK_2_Byte(eth.socket_num, w5500.Sn_MSSR, 1460)
        # Set RX buffer size to 2K
        eth.Write_SOCK_1_Byte(eth.socket_num, w5500.Sn_RXBUF_SIZE, 0x02)
        # Set TX buffer size to 2K
        eth.Write_SOCK_1_Byte(eth.socket_num, w5500.Sn_TXBUF_SIZE, 0x02)
        # Open socket
        eth.Write_SOCK_1_Byte(eth.socket_num, w5500.Sn_CR, w5500.OPEN)
        
        # Wait for socket to open
        time.sleep_ms(5)
        
        # Verify socket is in UDP mode
        if eth.Read_SOCK_1_Byte(eth.socket_num, w5500.Sn_SR) != w5500.SOCK_UDP:
            print(f"Failed to open UDP Socket {i}")
            return False
        
        print(f"UDP Server started on port {SOCKET_PORTS[i]}")
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
    # Initialize SPI interface
    spi = SPI(0, baudrate=1000000, polarity=0, phase=0, 
              sck=W5500_SCLK, mosi=W5500_MOSI, miso=W5500_MISO)
    
    # Initialize W5500
    eth = w5500.W5500(spi, W5500_CSn, W5500_RSTn, W5500_INTn)
    eth.get_ver()
    
    # Initial delay
    time.sleep_ms(100)
    
    # Wait for Ethernet connection
    while True:
        time.sleep_ms(100)
        i = eth.Read_1_Byte(w5500.PHYCFGR)
        if i & w5500.LINK:
            print("linked")
            break

    # Configure W5500
    W5500_Config(eth)
    
    # Configure sockets as UDP servers
    if not Socket_Config(eth):
        print("Failed to configure UDP Server")
        while True:
            pass
    
    # Set retry time and retry count
    eth.Write_2_Byte(w5500.RTR, 200)  # 200ms retry time
    eth.Write_1_Byte(w5500.RCR, 8)    # 8 retries
    
    # Main loop
    while True:
        try:
            # Check link status
            if not (eth.Read_1_Byte(w5500.PHYCFGR) & w5500.LINK):
                print("not link")
                # Close all sockets
                for i in range(NUM_SOCKETS):
                    eth.socket_num = i
                    eth.Write_SOCK_1_Byte(eth.socket_num, w5500.Sn_CR, w5500.CLOSE)
                # Wait for link to restore
                while not (eth.Read_1_Byte(w5500.PHYCFGR) & w5500.LINK):
                    time.sleep_ms(100) 
                print("linked!")
                # Reconfigure sockets
                if not Socket_Config(eth):
                    print("Failed to reconfigure UDP Server")
                    while True:
                        pass
        
            # Check each socket for received data
            for i in range(NUM_SOCKETS):
                eth.socket_num = i
                rx_size = eth.Read_SOCK_2_Byte(eth.socket_num, w5500.Sn_RX_RSR)
                if rx_size > 0:
                    print(f"Current port {SOCKET_PORTS[eth.socket_num]}")
                    # Read complete packet (including UDP header)
                    raw_data = eth.Read_SOCK_Data_Buffer(eth.socket_num)
                    
                    # Parse source IP and port
                    src_ip, src_port, payload = parse_udp_packet(raw_data)
                    if not src_ip:
                        print("Invalid UDP packet format")
                        continue
                    
                    print(f"Received {len(payload)} bytes from {'.'.join(map(str, src_ip))}:{src_port}:{payload}")
                    
                    # Set reply destination (now we configure Sn_DIPR/Sn_DPORTR)
                    eth.Write_SOCK_4_Byte(eth.socket_num, w5500.Sn_DIPR, src_ip)
                    eth.Write_SOCK_2_Byte(eth.socket_num, w5500.Sn_DPORTR, src_port)
                    
                    # Send reply (echo payload or custom data)
                    eth.Write_SOCK_Data_Buffer(eth.socket_num, payload, len(payload))
                    
            time.sleep_ms(10)  # Reduced sleep for more responsive handling
        
        except Exception as e:
            print("Error:", e)
            # Close all sockets on error
            for i in range(NUM_SOCKETS):
                eth.socket_num = i
                eth.Write_SOCK_1_Byte(eth.socket_num, w5500.Sn_CR, w5500.CLOSE)
            break