import time
from machine import Pin, SPI
from SX127x import SX127x  # Ensure SX127x class is adapted for MicroPython

# Begin LoRa radio and set NSS, reset, busy, IRQ, txen, and rxen pin with connected ESP32 gpio pins
busId = 1  # SPI bus
csId = 5  # Chip select pin
resetPin = 14  # Updated reset pin to D14
irqPin = -1
txenPin = -1
rxenPin = -1

# Configure SPI and CS pin
spi = SPI(busId, baudrate=10000000, polarity=0, phase=0, sck=Pin(18), mosi=Pin(23), miso=Pin(19))
cs = Pin(csId, Pin.OUT)
reset = Pin(resetPin, Pin.OUT)

# Initialize LoRa
LoRa = SX127x(spi=spi, cs=cs, reset=reset)
print("Begin LoRa radio")

# Debugging: Check if the LoRa object is created successfully
if not LoRa:
    raise Exception("LoRa object creation failed")

# Debugging: Print SPI and pin configurations
print("SPI and pin configurations:")
print(f"SCK: {18}, MOSI: {23}, MISO: {19}, CS: {csId}, RESET: {resetPin}")

# Attempt to begin LoRa radio
LoRa.begin()
if  LoRa.begin():
    raise Exception("Something wrong, can't begin LoRa radio")

# Set frequency to 433 MHz
print("Set frequency to 433 MHz")
LoRa.setFrequency(433000000)

# Set TX power, this function will set PA config with optimal setting for requested TX power
print("Set TX power to +17 dBm")
LoRa.setTxPower(17, LoRa.TX_POWER_PA_BOOST)  # TX power +17 dBm using PA boost pin

# Configure modulation parameter including spreading factor (SF), bandwidth (BW), and coding rate (CR)
# Receiver must have same SF and BW setting with transmitter to be able to receive LoRa packet
print("Set modulation parameters:\n\tSpreading factor = 7\n\tBandwidth = 125 kHz\n\tCoding rate = 4/5")
LoRa.setSpreadingFactor(7)  # LoRa spreading factor: 7
LoRa.setBandwidth(125000)  # Bandwidth: 125 kHz
LoRa.setCodeRate(4)  # Coding rate: 4/5

# Configure packet parameter including header type, preamble length, payload length, and CRC type
# The explicit packet includes header containing CR, number of byte, and CRC type
# Receiver can receive packet with different CR and packet parameters in explicit header mode
print("Set packet parameters:\n\tExplicit header type\n\tPreamble length = 12\n\tPayload Length = 15\n\tCRC on")
LoRa.setHeaderType(LoRa.HEADER_EXPLICIT)  # Explicit header mode
LoRa.setPreambleLength(12)  # Set preamble length to 12
LoRa.setPayloadLength(15)  # Initialize payloadLength to 15
LoRa.setCrcEnable(True)  # Set CRC enable

# Set synchronize word for public network (0x34)
print("Set synchronize word to 0x34")
LoRa.setSyncWord(0x34)

print("\n-- LoRa Transmitter --\n")

# Message to transmit
message = b"Hello Bariflo"
messageList = list(message)
#for i in range(len(messageList)):
 #   messageList[i] = ord(messageList[i])
counter = 0

# Transmit message continuously
while True:
    # Transmit message and counter
    # write() method must be placed between beginPacket() and endPacket()
    message_with_counter = messageList + [counter]
    LoRa.beginPacket()
    LoRa.write(message_with_counter)
    #LoRa.write([counter])
    LoRa.endPacket()

    # Print message and counter
    print(f"{message}  {counter}")
    
    # Wait until modulation process for transmitting packet finish
    LoRa.wait()

    # Print transmit time and data rate
    #print("Transmit time: {0:0.2f} ms | Data rate: {1:0.2f} byte/s".format(LoRa.transmitTime(), LoRa.dataRate()))

    # Don't load RF module with continuous transmit
    time.sleep(2)
    counter = (counter + 1) % 256

