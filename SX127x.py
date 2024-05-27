from machine import Pin, SPI
import time

class SX127x:
    """Class for SX1276/77/78/79 LoRa chipsets from Semtech"""

    # Define registers and constants (same as original)
    REG_FIFO                               = 0x00
    REG_OP_MODE                            = 0x01
    REG_FRF_MSB                            = 0x06
    REG_FRF_MID                            = 0x07
    REG_FRF_LSB                            = 0x08
    REG_PA_CONFIG                          = 0x09
    REG_PA_RAMP                            = 0x0A
    REG_OCP                                = 0x0B
    REG_LNA                                = 0x0C
    REG_FIFO_ADDR_PTR                      = 0x0D
    REG_FIFO_TX_BASE_ADDR                  = 0x0E
    REG_FIFO_RX_BASE_ADDR                  = 0x0F
    REG_FIFO_RX_CURRENT_ADDR               = 0x10
    REG_IRQ_FLAGS_MASK                     = 0x11
    REG_IRQ_FLAGS                          = 0x12
    REG_RX_NB_BYTES                        = 0x13
    REG_RX_HEADR_CNT_VALUE_MSB             = 0x14
    REG_RX_HEADR_CNT_VALUE_LSB             = 0x15
    REG_RX_PKT_CNT_VALUE_MSB               = 0x16
    REG_RX_PKT_CNT_VALUE_LSB               = 0x17
    REG_MODEB_STAT                         = 0x18
    REG_PKT_SNR_VALUE                      = 0x19
    REG_PKT_RSSI_VALUE                     = 0x1A
    REG_RSSI_VALUE                         = 0x1B
    REG_HOP_CHANNEL                        = 0x1C
    REG_MODEM_CONFIG_1                     = 0x1D
    REG_MODEM_CONFIG_2                     = 0x1E
    REG_SYMB_TIMEOUT_LSB                   = 0x1F
    REG_PREAMBLE_MSB                       = 0x20
    REG_PREAMBLE_LSB                       = 0x21
    REG_PAYLOAD_LENGTH                     = 0x22
    REG_MAX_PAYLOAD_LENGTH                 = 0x23
    REG_HOP_PERIOD                         = 0x24
    REG_FIFO_RX_BYTE_ADDR                  = 0x25
    REG_MODEM_CONFIG_3                     = 0x26
    REG_FREQ_ERROR_MSB                     = 0x28
    REG_FREQ_ERROR_MID                     = 0x29
    REG_FREQ_ERROR_LSB                     = 0x2A
    REG_RSSI_WIDEBAND                      = 0x2C
    REG_FREQ1                              = 0x2F
    REG_FREQ2                              = 0x30
    REG_DETECTION_OPTIMIZE                 = 0x31
    REG_INVERTIQ                           = 0x33
    REG_HIGH_BW_OPTIMIZE_1                 = 0x36
    REG_DETECTION_THRESHOLD                = 0x37
    REG_SYNC_WORD                          = 0x39
    REG_HIGH_BW_OPTIMIZE_2                 = 0x3A
    REG_INVERTIQ2                          = 0x3B
    REG_DIO_MAPPING_1                      = 0x40
    REG_DIO_MAPPING_2                      = 0x41
    REG_VERSION                            = 0x42
    REG_TCXO                               = 0x4B
    REG_PA_DAC                             = 0x4D
    REG_FORMER_TEMP                        = 0x5B
    REG_AGC_REF                            = 0x61
    REG_AGC_THRESH_1                       = 0x62
    REG_AGC_THRESH_2                       = 0x63
    REG_AGC_THRESH_3                       = 0x64
    REG_PLL                                = 0x70
    REG_SYNC_WORD                          = 0x39

    # Modem options
    FSK_MODEM                              = 0x00 # GFSK packet type
    LORA_MODEM                             = 0x01 # LoRa packet type
    OOK_MODEM                              = 0x02 # OOK packet type

    # Long range mode and modulation type
    LONG_RANGE_MODE                        = 0x80 # GFSK packet type
    MODULATION_OOK                         = 0x20 # OOK packet type
    MODULATION_FSK                         = 0x00 # LoRa packet type

    # Devices modes
    MODE_SLEEP                             = 0x00 # sleep
    MODE_STDBY                             = 0x01 # standby
    MODE_TX                                = 0x03 # transmit
    MODE_RX_CONTINUOUS                     = 0x05 # continuous receive
    MODE_RX_SINGLE                         = 0x06 # single receive
    MODE_CAD                               = 0x07 # channel activity detection (CAD)

    # Rx operation mode
    RX_SINGLE                              = 0x000000    # Rx timeout duration: no timeout (Rx single mode)
    RX_CONTINUOUS                          = 0xFFFFFF    #                      infinite (Rx continuous mode)

    # TX power options
    TX_POWER_RFO                           = 0x00        # output power is limited to +14 dBm
    TX_POWER_PA_BOOST                      = 0x80        # output power is limited to +20 dBm

    # RX gain options
    RX_GAIN_POWER_SAVING                   = 0x00        # gain used in Rx mode: power saving gain (default)
    RX_GAIN_BOOSTED                        = 0x01        #                       boosted gain
    RX_GAIN_AUTO                           = 0x00        # option enable auto gain controller (AGC)

    # Header type
    HEADER_EXPLICIT                        = 0x00        # explicit header mode
    HEADER_IMPLICIT                        = 0x01        # implicit header mode

    # LoRa syncword
    SYNCWORD_LORAWAN                       = 0x34        # reserved LoRaWAN syncword

    # Oscillator options
    OSC_CRYSTAL                            = 0x00        # crystal oscillator with external crystal
    OSC_TCXO                               = 0x10        # external clipped sine TCXO AC-connected to XTA pin

    # DIO mapping
    DIO0_RX_DONE                           = 0x00        # set DIO0 interrupt for: RX done
    DIO0_TX_DONE                           = 0x40        #                         TX done
    DIO0_CAD_DONE                          = 0x80        #                         CAD done

    # IRQ flags
    IRQ_CAD_DETECTED                       = 0x01        # Valid Lora signal detected during CAD operation
    IRQ_FHSS_CHANGE                        = 0x02        # FHSS change channel interrupt
    IRQ_CAD_DONE                           = 0x04        # channel activity detection finished
    IRQ_TX_DONE                            = 0x08        # packet transmission completed
    IRQ_HEADER_VALID                       = 0x10        # valid LoRa header received
    IRQ_CRC_ERR                            = 0x20        # wrong CRC received
    IRQ_RX_DONE                            = 0x40        # packet received
    IRQ_RX_TIMEOUT                         = 0x80        # waiting packet received timeout

    # Rssi offset
    RSSI_OFFSET_LF                         = 164         # low band frequency RSSI offset
    RSSI_OFFSET_HF                         = 157         # high band frequency RSSI offset
    RSSI_OFFSET                            = 139         # frequency RSSI offset for SX1272
    BAND_THRESHOLD                         = 525E6       # threshold between low and high band frequency

    # TX and RX operation status
    STATUS_DEFAULT                         = 0           # default status (false)
    STATUS_TX_WAIT                         = 1
    STATUS_TX_TIMEOUT                      = 2
    STATUS_TX_DONE                         = 3
    STATUS_RX_WAIT                         = 4
    STATUS_RX_CONTINUOUS                   = 5
    STATUS_RX_TIMEOUT                      = 6
    STATUS_RX_DONE                         = 7
    STATUS_HEADER_ERR                      = 8
    STATUS_CRC_ERR                         = 9
    STATUS_CAD_WAIT                        = 10
    STATUS_CAD_DETECTED                    = 11
    STATUS_CAD_DONE                        = 12
    # SPI and GPIO pin setting
    _spi = None
    _cs = None
    _reset = None
    _irq = None
    _txen = None
    _rxen = None
    _spiSpeed = 7800000
    _txState = 0
    _rxState = 0

    # LoRa setting (same as original)
    _dio = 1
    _modem = LORA_MODEM
    _frequency = 915000000
    _sf = 7
    _bw = 125000
    _cr = 5
    _ldro = False
    _headerType = HEADER_EXPLICIT
    _preambleLength = 12
    _payloadLength = 32
    _crcType = False
    _invertIq = False

    # Operation properties (same as original)
    _payloadTxRx = 32
    _statusWait = STATUS_DEFAULT
    _statusIrq = STATUS_DEFAULT
    _transmitTime = 0.0

    # callback functions (same as original)
     # callback functions
    _onTransmit = None
    _onReceive = None
### COMMON OPERATIONAL METHODS ###

    def begin(self, spi_bus: int = 1, cs_pin: int = 5, reset_pin: int = 22, irq_pin: int = -1, txen_pin: int = -1, rxen_pin: int = -1) -> bool:
        # Set SPI and GPIO pins
        self.setSpi(spi_bus, cs_pin)
        self.setPins(reset_pin, irq_pin, txen_pin, rxen_pin)

        # Perform device reset
        if not self.reset():
            return False

        # Set modem to LoRa
        self.setModem(self.LORA_MODEM)
        # Set TX power and RX gain
        self.setTxPower(17, self.TX_POWER_PA_BOOST)
        self.setRxGain(self.RX_GAIN_BOOSTED, self.RX_GAIN_AUTO)
        return True

    def end(self):
        self.sleep()

    def reset(self):
        # Put reset pin to low then wait 5 ms
        self._reset.value(0)
        time.sleep(0.001)
        self._reset.value(1)
        time.sleep(0.005)
        # Wait until device connected, return false when device too long to respond
        t = time.time()
        version = 0x00
        while version != 0x12 and version != 0x22:
            version = self.readRegister(self.REG_VERSION)
            if time.time() - t > 1:
                return False
        return True

    def sleep(self):
        # Put device in sleep mode
        self.writeRegister(self.REG_OP_MODE, self._modem | self.MODE_SLEEP)

    def wake(self):
        # Wake device by putting it in standby mode
        self.writeRegister(self.REG_OP_MODE, self._modem | self.MODE_STDBY)

    def standby(self):
        self.writeRegister(self.REG_OP_MODE, self._modem | self.MODE_STDBY)

### HARDWARE CONFIGURATION METHODS ###

    def setSpi(self, bus: int, cs: int, speed: int = _spiSpeed):
        self._spi = SPI(bus, baudrate=speed, polarity=0, phase=0)
        self._cs = Pin(cs, Pin.OUT)

    def setPins(self, reset: int, irq: int = -1, txen: int = -1, rxen: int = -1):
        self._reset = Pin(reset, Pin.OUT)
        self._irq = Pin(irq, Pin.IN) if irq != -1 else None
        self._txen = Pin(txen, Pin.OUT) if txen != -1 else None
        self._rxen = Pin(rxen, Pin.OUT) if rxen != -1 else None

    def setCurrentProtection(self, current: int):
        # Calculate ocp trim
        ocpTrim = 27
        if current <= 120:
            ocpTrim = int((current - 45) / 5)
        elif current <= 240:
            ocpTrim = int((current + 30) / 10)
        # Set over current protection config
        self.writeRegister(self.REG_OCP, 0x20 | ocpTrim)

    def setOscillator(self, option: int):
        cfg = self.OSC_CRYSTAL
        if option == self.OSC_TCXO:
            cfg = self.OSC_TCXO
        self.writeRegister(self.REG_TCXO, cfg)

### MODEM, MODULATION PARAMETER, AND PACKET PARAMETER SETUP METHODS ###

    def setModem(self, modem: int):
        if modem == self.LORA_MODEM:
            self._modem = self.LONG_RANGE_MODE
        elif modem == self.FSK_MODEM:
            self._modem = self.MODULATION_FSK
        else:
            self._modem = self.MODULATION_OOK
        self.sleep()
        self.writeRegister(self.REG_OP_MODE, self._modem | self.MODE_STDBY)

    def setFrequency(self, frequency: int):
        self._frequency = frequency
        # Calculate frequency
        frf = int((frequency << 19) / 32000000)
        self.writeRegister(self.REG_FRF_MSB, (frf >> 16) & 0xFF)
        self.writeRegister(self.REG_FRF_MID, (frf >> 8) & 0xFF)
        self.writeRegister(self.REG_FRF_LSB, frf & 0xFF)

    def setTxPower(self, txPower: int, paPin: int):
        # Maximum TX power is 20 dBm and 14 dBm for RFO pin
        if txPower > 20:
            txPower = 20
        elif txPower > 14 and paPin == self.TX_POWER_RFO:
            txPower = 14

        paConfig = 0x00
        outputPower = 0x00
        if paPin == self.TX_POWER_RFO:
            # txPower = Pmax - (15 - outputPower)
            if txPower == 14:
                # Max power (Pmax) 14.4 dBm
                paConfig = 0x60
                outputPower = txPower + 1
            else:
                # Max power (Pmax) 13.2 dBm
                paConfig = 0x40
                outputPower = txPower + 2
        else:
            paConfig = 0xC0
            paDac = 0x04
            # txPower = 17 - (15 - outputPower)
            if txPower > 17:
                outputPower = 15
                paDac = 0x07
                self.setCurrentProtection(100)  # Max current 100 mA
            else:
                if txPower < 2:
                    txPower = 2
                outputPower = txPower - 2
                self.setCurrentProtection(140)  # Max current 140 mA
            # Enable or disable +20 dBm option on PA_BOOST pin
            self.writeRegister(self.REG_PA_DAC, paDac)

        # Set PA config
        self.writeRegister(self.REG_PA_CONFIG, paConfig | outputPower)

    def setRxGain(self, boost: int, level: int):
        # Valid RX gain level 0 - 6 (0 -> AGC on)
        if level > 6:
            level = 6
        # Boost LNA and automatic gain controller config
        LnaBoostHf = 0x00
        if boost:
            LnaBoostHf = 0x03
        AgcOn = 0x00
        if level == self.RX_GAIN_AUTO:
            AgcOn = 0x01

        # Set gain and boost LNA config
        self.writeRegister(self.REG_LNA, LnaBoostHf | (level << 5))
        # Enable or disable AGC
        self.writeBits(self.REG_MODEM_CONFIG_3, AgcOn, 2, 1)

    def setLoRaModulation(self, sf: int, bw: int, cr: int, ldro: bool = False):
        self.setSpreadingFactor(sf)
        self.setBandwidth(bw)
        self.setCodeRate(cr)
        self.setLdroEnable(ldro)

    def setLoRaPacket(self, headerType: int, preambleLength: int, payloadLength: int, crcType: bool = False, invertIq: bool = False):
        self.setHeaderType(headerType)
        self.setPreambleLength(preambleLength)
        self.setPayloadLength(payloadLength)
        self.setCrcEnable(crcType)
        # self.setInvertIq(invertIq)

    def setSpreadingFactor(self, sf: int):
        self._sf = sf
        # Valid spreading factor is 6 - 12
        if sf < 6:
            sf = 6
        elif sf > 12:
            sf = 12
        # Set appropriate signal detection optimize and... (continued)
        if sf == 11 or sf == 12:
           self.setLdroEnable(True)
        else:
           self.setLdroEnable(False)
        # Set spreading factor config
           self.writeBits(self.REG_MODEM_CONFIG_2, sf, 4, 4)

    def setBandwidth(self, bw: int):
        self._bw = bw
        bwMap = {7800: 0, 10400: 1, 15600: 2, 20800: 3, 31250: 4, 41700: 5, 62500: 6, 125000: 7, 250000: 8, 500000: 9}
        if bw not in bwMap:
            bw = 125000
        self.writeBits(self.REG_MODEM_CONFIG_1, bwMap[bw], 4, 4)

    def setCodeRate(self, cr: int):
        self._cr = cr
    # Valid code rate is 5 - 8
        if cr < 5:
           cr = 5
        elif cr > 8:
           cr = 8
    
        #shift_count = cr - 4 if cr >= 4 else 0  # Ensure shift count is non-negative
        shift_count = max(0, cr - 4)
        self.writeBits(self.REG_MODEM_CONFIG_1,0b001,4,3 )
        #self.writeBits(self.REG_MODEM_CONFIG_1, max(0, shift_count), 1, 3)
    def setSyncWord(self , sync_word_value):
        self.writeRegister(self.REG_SYNC_WORD, sync_word_value & 0xFF)  # LSB
        self.writeRegister(self.REG_SYNC_WORD + 1, (sync_word_value >> 8) & 0xFF)
    def beginPacket(self):
        self.writeRegister(self.REG_FIFO_ADDR_PTR, 0x80)
        self.writeRegister(self.REG_PAYLOAD_LENGTH, 0)
    def write(self, payload):
        for byte in payload:
            self.writeRegister(self.REG_FIFO, byte)
    def wait(self):
        while (self.readRegister(self.REG_IRQ_FLAGS) & 0x08) == 0:
            pass
        self.writeRegister(self.REG_IRQ_FLAGS, 0x08)
    def request(self, mode):
        """Simulate requesting data or packets based on the mode."""
        if mode == "send":
            # Simulate sending request
            print("Sending request...")
        elif mode == "receive":
            # Simulate receiving request
            print("Receiving request...")
        else:
            raise ValueError("Invalid mode specified")
    def available(self):
        return True
    def read(self):
        return b""
    def transmitTime(self):
        """Calculate the transmit time for the current packet."""
        # Assuming you have a way to determine the packet size in bytes
        packetSizeBytes = len(self._currentPacket)
        # Calculate the transmit time based on packet size and data rate
        transmitTimeMs = (packetSizeBytes * 8) / self._dataRate  # Assuming _dataRate is in bits per second
        return transmitTimeMs

    def dataRate(self):
        """Calculate the data rate for the current transmission."""
        # Assuming you have a way to measure the time taken for transmission
        # and the size of the transmitted packet in bytes
        transmissionTimeSec = self._getTransmissionTime()  # Function to get the transmission time in seconds
        packetSizeBytes = len(self._currentPacket)
        # Calculate data rate in bytes per second (Bps)
        dataRateBps = packetSizeBytes / transmissionTimeSec
        return dataRateBps
    def endPacket(self):
        self.writeRegister(self.REG_OP_MODE, self.MODE_TX)
        while (self.readRegister(self.REG_IRQ_FLAGS) & 0x08) == 0:
            time.sleep(0.01)
        self.writeRegister(self.REG_IRQ_FLAGS, 0x08)
    def setLdroEnable(self, enable: bool):
        self._ldro = enable
        self.writeBits(self.REG_MODEM_CONFIG_3, 1 if enable else 0, 3, 1)
    
    def setHeaderType(self, headerType: int):
        self._headerType = headerType
        self.writeBits(self.REG_MODEM_CONFIG_1, headerType, 0, 1)
     
    def setPreambleLength(self, length: int):
        self._preambleLength = length
        self.writeRegister(self.REG_PREAMBLE_MSB, (length >> 8) & 0xFF)
        self.writeRegister(self.REG_PREAMBLE_LSB, length & 0xFF)

    def setPayloadLength(self, length: int):
        self._payloadLength = length
        self.writeRegister(self.REG_PAYLOAD_LENGTH, length & 0xFF)

    def setCrcEnable(self, enable: bool):
        self._crcType = enable
        self.writeBits(self.REG_MODEM_CONFIG_2, 1 if enable else 0, 2, 1)

    def setInvertIq(self, invert: bool):
        self._invertIq = invert
        if invert:
            self.writeRegister(self.REG_INVERTIQ, 0x66)
            self.writeRegister(self.REG_INVERTIQ2, 0x19)
        else:
            self.writeRegister(self.REG_INVERTIQ, 0x27)
            self.writeRegister(self.REG_INVERTIQ2, 0x1D)

### IRQ AND CALLBACK CONFIGURATION METHODS ###

    def setDioMapping(self, dio: int, map: int):
        # dio: 0 - 5
        if dio > 5:
            dio = 5
        # map: 0 - 3
        if map > 3:
            map = 3
        # Set DIO mapping
        dioReg = self.REG_DIO_MAPPING_1 if dio < 4 else self.REG_DIO_MAPPING_2
        dioShift = (dio % 4) * 2
        self.writeBits(dioReg, map, dioShift, 2)

    def setIrqFlagsMask(self, mask: int):
        self.writeRegister(self.REG_IRQ_FLAGS_MASK, mask)

    def clearIrqFlags(self, flags: int):
        self.writeRegister(self.REG_IRQ_FLAGS, flags)

    def setCallback(self, event: int, callback):
        if event == self.STATUS_TX_DONE:
            self._onTransmit = callback
        elif event == self.STATUS_RX_DONE:
            self._onReceive = callback

### TRANSMIT AND RECEIVE METHODS ###

    def transmit(self, payload: bytes):
        # Ensure device is in standby mode
        self.standby()
        # Set payload length
        self.writeRegister(self.REG_PAYLOAD_LENGTH, len(payload))
        # Set FIFO address pointer
        self.writeRegister(self.REG_FIFO_ADDR_PTR, 0x00)
        # Write payload to FIFO
        self.writePayload(payload)
        # Clear TX done IRQ
        self.clearIrqFlags(self.IRQ_TX_DONE)
        # Set DIO0 mapping for TX done
        self.setDioMapping(0, self.DIO0_TX_DONE)
        # Set device to TX mode
        self.writeRegister(self.REG_OP_MODE, self._modem | self.MODE_TX)
        # Wait for TX done
        while not self._irq.value():
            time.sleep(0.001)
        # Clear TX done IRQ
        self.clearIrqFlags(self.IRQ_TX_DONE)
        # Return to standby mode
        self.standby()

    def receive(self):
        # Ensure device is in standby mode
        self.standby()
        # Clear RX done IRQ
        self.clearIrqFlags(self.IRQ_RX_DONE)
        # Set DIO0 mapping for RX done
        self.setDioMapping(0, self.DIO0_RX_DONE)
        # Set device to continuous RX mode
        self.writeRegister(self.REG_OP_MODE, self._modem | self.MODE_RX_CONTINUOUS)
        # Wait for RX done
        while not self._irq.value():
            time.sleep(0.001)
        # Read received payload from FIFO
        payload = self.readPayload()
        # Clear RX done IRQ
        self.clearIrqFlags(self.IRQ_RX_DONE)
        # Return to standby mode
        self.standby()
        return payload

### LOW-LEVEL SPI READ/WRITE METHODS ###

    def writeRegister(self, address: int, value: int):
        self._cs.value(0)
        self._spi.write(bytes([address | 0x80, value]))
        self._cs.value(1)

    def readRegister(self, address: int) -> int:
        self._cs.value(0)
        self._spi.write(bytes([address & 0x7F]))
        value = self._spi.read(1)
        self._cs.value(1)
        return value[0]

    def writeBits(self, address: int, value: int, position: int, length: int):
        currentValue = self.readRegister(address)
        mask = (1 << length) - 1
        value = (currentValue & ~(mask << position)) | ((value & mask) << position)
        self.writeRegister(address, value)

    def writePayload(self, payload: bytes):
        self._cs.value(0)
        self._spi.write(bytes([self.REG_FIFO | 0x80]) + payload)
        self._cs.value(1)

    def readPayload(self) -> bytes:
        self._cs.value(0)
        self._spi.write(bytes([self.REG_FIFO & 0x7F]))
        payload = self._spi.read(self._payloadLength)
        self._cs.value(1)
        return payload

### READ/WRITE REGISTER METHODS ###

    def writeRegister(self, address: int, value: int):
        self._cs.value(0)
        self._spi.write(bytearray([address | 0x80, value]))
        self._cs.value(1)

    def readRegister(self, address: int) -> int:
        self._cs.value(0)
        self._spi.write(bytearray([address & 0x7F]))
        result = self._spi.read(1)
        self._cs.value(1)
        return result[0]

    def writeBits(self, address: int, value: int, msb: int, lsb: int):
        data = self.readRegister(address)
        length = msb - lsb + 1
        mask = ((1 << length) - 1) << lsb
        value = (value << lsb) & mask
        data = (data & ~mask) | value
        self.writeRegister(address, data)
    def readBits(self, address: int, msb: int, lsb: int) -> int:
        data = self.readRegister(address)
        length = msb - lsb + 1
        mask = (1 << length) - 1
        data >>= lsb
        return data & mask

    def writeBytes(self, address: int, data: bytearray, length: int):
        self._cs.value(0)
        self._spi.write(bytearray([address | 0x80]) + data[:length])
        self._cs.value(1)

    def readBytes(self, address: int, length: int) -> bytearray:
        self._cs.value(0)
        self._spi.write(bytearray([address & 0x7F]))
        result = self._spi.read(length)
        self._cs.value(1)
        return result

    def readInterrupt(self) -> int:
        if self._irq is not None:
            return self._irq.value()
        return -1

    def txEnable(self):
        if self._txen is not None and not self._txState:
            self._txen.value(1)
            self._txState = 1
        if self._rxen is not None and self._rxState:
            self._rxen.value(0)
            self._rxState = 0

    def rxEnable(self):
        if self._txen is not None and self._txState:
            self._txen.value(0)
            self._txState = 0
        if self._rxen is not None and not self._rxState:
            self._rxen.value(1)
            self._rxState = 1

### DEVICE STATUS METHODS ###

    def getStatus(self) -> int:
        return self.readRegister(self.REG_IRQ_FLAGS)

    def getPacketStatus(self) -> int:
        return self.readRegister(self.REG_PKT_RSSI_VALUE)

    def getPacketSnr(self) -> float:
        return self.readRegister(self.REG_PKT_SNR_VALUE) * 0.25

    def getRssi(self) -> int:
        return self.readRegister(self.REG_RSSI_VALUE) - 137

    def getErrors(self) -> int:
        return self.readRegister(self.REG_IRQ_FLAGS_MASK) & 0x1C

    def getFrequencyError(self) -> int:
        return self.readBits(self.REG_FEI_MSB, 3, 0)

### FIFO OPERATION METHODS ###

    def fifoClear(self):
        self.writeRegister(self.REG_FIFO_ADDR_PTR, self._fifoRxBaseAddr)
        self.writeRegister(self.REG_FIFO_ADDR_PTR, self._fifoTxBaseAddr)

    def fifoWrite(self, data: bytearray):
        self.writeBytes(self.REG_FIFO, data, len(data))

    def fifoRead(self, length: int) -> bytearray:
        return self.readBytes(self.REG_FIFO, length)

    def sendPacket(self, data: bytearray):
        self.wake()
        self.standby()
        self.fifoClear()
        self.fifoWrite(data)
        self.txEnable()
        self.writeRegister(self.REG_OP_MODE, self._modem | self.MODE_TX)

    def receivePacket(self, length: int) -> bytearray:
        self.wake()
        self.standby()
        self.fifoClear()
        self.rxEnable()
        self.writeRegister(self.REG_OP_MODE, self._modem | self.MODE_RXCONT)
        return self.fifoRead(length)

