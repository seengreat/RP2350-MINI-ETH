import array, time
from machine import Pin
import rp2
from rp2 import PIO, StateMachine, asm_pio

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
