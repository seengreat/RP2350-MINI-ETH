import machine
import time

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

# gpio = [gp0, gp1, gp2, gp3, gp4, gp5, gp6, gp7, gp8, gp9, gp10, gp11, gp22, gp26, gp27, gp28]
gpio = [gp6, gp8, gp10,  gp22, gp26, gp28]
while True:
    print("GPIO HIGH")
    for i,pio in enumerate(gpio):    
        pio.value(1)
    time.sleep_ms(1000)
    print("GPIO LOW")
    for i,pio in enumerate(gpio):    
        pio.value(0)
    time.sleep_ms(1000)
