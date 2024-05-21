import os
import time
import Adafruit_ADS1x15


class Current:

    def __init__(self):
        # Initialize ADC
        self.__adc = Adafruit_ADS1x15.ADS1115()
        self.__GAIN = 2/3  # Set gain to 2/3 (acceptable for 0-5V range)
        # ACS712 parameters
        self.__sensitivity = 0.185  # mV per Amp
        self.__zero_current_voltage = 2500  # mV


    def __current(self,adc_value):
        # Convert ADC value to voltage
        voltage = adc_value / 32767.0 * 4.096  # 4.096V is the full scale range for ADS1115 with GAIN = 2/3
        # Calculate AC current
        ac_current = (voltage - self.__zero_current_voltage) / self.__sensitivity
        ac_current = ac_current/1000
        return ac_current


    def get_current(self):
        # Read ADC value
        adc_value0 = self.__adc.read_adc(0, gain=self.__GAIN)
        adc_value1 = self.__adc.read_adc(1, gain=self.__GAIN)
        adc_value2 = self.__adc.read_adc(2, gain=self.__GAIN)

        current_r = self.__current(adc_value0)
        current_y = self.__current(adc_value1)
        current_b = self.__current(adc_value2)
        #print("AC Current: {:.2f} mA".format(ac_current))
        print("AC Current0: {:.2f} A".format(current_r))
        print("AC Current1: {:.2f} A".format(current_y))
        print("AC Current2: {:.2f} A".format(current_b))

        return f"{round(current_r,2)}/{round(current_y,2)}/{round(current_b,2)}"


c = Current()
c.get_current()