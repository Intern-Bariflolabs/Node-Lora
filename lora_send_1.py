from machine import Pin, SPI
from SX127x1 import SX127x
import utime

# LoRa module pins
NSS = Pin(5, Pin.OUT)
MOSI = Pin(23)
MISO = Pin(19)
SCK = Pin(18)
RST = Pin(14, Pin.OUT)

# SPI initialization
spi = SPI(1, baudrate=5000000, polarity=0, phase=0, sck=SCK, mosi=MOSI, miso=MISO)

# LoRa initialization
lora = SX127x(spi, NSS, RST, frequency=433e6)

# Sending function
def send_message(message):
    lora.write_payload(message)
    lora.set_mode_tx()
    while lora.busy():
        utime.sleep_ms(10)
    print("Message sent:", message)

while True:
    send_message("Hello, LoRa!")
    utime.sleep(2)  # Send a message every 10 seconds

