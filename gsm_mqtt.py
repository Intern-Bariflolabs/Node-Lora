import time
from machine import UART, Pin
from umqtt.simple import MQTTClient
import json
import random

# Setup UART for GSM module
uart = UART(1, baudrate=115200, tx=Pin(17), rx=Pin(16))
# tx = 17 , rx = 16
# Initialize GSM module
def send_at_command(command, timeout=1000):
    uart.write(command + '\r')
    time.sleep_ms(timeout)
    response = uart.read()
    if response:
        print(response.decode('utf-8'))
        print(command)
    return response

def connect_to_internet():
   # if send_at_command('AT+CGDCONT=1 , "IP" , "jionet","0.0.0.0",0,0') is None :
    ##   return false
    topic =  b'875481275432279' 
    if send_at_command('AT+QMTCFG= "SSL", 0,1,2') is None:
        print('failed')
    #if send_at_command('AT+CIFSR') is None :
     #   print('failed')
    
    #if send_at_command('AT+QMTOPEN=0,"4.240.114.7",1881,"BarifloLabs","Bfl@123"') is None:
     #   print('failed')
    #send_at_command('AT+QMTOPEN?')
    #if send_at_command('AT+QMTCONN=0,3') is None :
       # print('Failed')
    #if send_at_command(f'AR+QMTSUB= 0,1,"{topic}",1'):
     #   print(f"Subscribed to {topic.decode('utf-8')} ")
    #if send_at_command('AT+QMTOPEN= 0 , ""')
    if send_at_command('AT+COPS?') is None:
        print('failed')
    if send_at_command('AT') is None:
        print('Failed to communicate with the GSM module')
        return False
    if send_at_command('AT+CSQ') is None:
        print('Failed to get signal quality')
        return False
    if send_at_command('AT+CREG?') is None:
        print('Failed to check network registration')
        return False
    if send_at_command('AT+CGATT=1') is None:
        print('Failed to attach to GPRS')
        return False
    
#    if send_at_command('AT+CSTT="jionet","",""') is None:
 #       print('Failed to set APN')
  #      return False
   # if send_at_command('AT+CIICR') is None:
    #    print('Failed to bring up wireless connection')
     #   return False
    if send_at_command('AT+QIACT?') is None:
        print('Failed to get IP address')
        return False
    #if send_at_command('AT+QMTCFG="VERSION",0,4,1') is None:
     #   print("Failed")
    if send_at_command('AT+QMTCFG="KEEPALIVE",0,60') is None:
        print("Failed")
    if send_at_command('AT+QMTOPEN=0,"4.240.114.7",1883,"BarifloLabs","Bfl@123"') is None:
        print("Failed")
    if send_at_command('AT+QMTCONN=1,"client123","BarifloLabs","Bfl@123"') is None:
        print("Failed")
    send_at_command('AT+QMTCONN?')
    #if send_at_command('AT+SMCONF= "URL","4.240.114.7",1883') is None:
     #   print("Failed")
    if send_at_command('AT+QMTPUB= 0,0,0,0,"307452396536950/data"'):
        uart.write('Hello\r')
    if send_at_command('AT+QICSGP=1,1,"jionet","","",1') is None:
         print("Failed")
    return True


# Main code
def main():
    
    if not connect_to_internet():
        print('Failed to connect to the internet')
        return
    else:
        print('connected')
    #client = connect_to_mqtt()
    #if client is None:
     #   print('Failed to connect to MQTT broker')
      #  return
    #publish_data(client)

if __name__ == '__main__':
    main()


