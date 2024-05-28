import network
from umqtt.simple import MQTTClient
import time
import machine
import json
import random
from lib2.blink import blink


# Wi-Fi credentials
ssid = 'BarifloLabs'
password = 'Bfl_wifi0@1234'

# MQTT broker information
mqtt_server = '4.240.114.7'
mqtt_port = 1883
mqtt_username = 'BarifloLabs'
mqtt_password = 'Bfl@123'
client_id = 'ESP32A'
topic = '307452396536950/data'

def connect_to_wifi():
    station = network.WLAN(network.STA_IF)
    station.active(True)
    station.connect(ssid, password)
    print('Connecting to Wi-Fi...')
    
    while not station.isconnected():
        print('...')
        time.sleep(1)
    
    print('Connected to Wi-Fi')
    print('IP Address:', station.ifconfig()[0])

def connect_and_publish():
    client = MQTTClient(client_id, mqtt_server, user=mqtt_username, password=mqtt_password, port=mqtt_port)
    
    try:
        client.connect()
        print(f'Connected to {mqtt_server}')
    except OSError as e:
        print(f'Failed to connect to MQTT broker: {e}')
        return
    
    message_count = 0
    
    while True:
        try:
            #basicConfig(level=20, filename='read.txt', filemode='w')
            number = random.randint(19, 21)
            current_time = time.localtime()
            formatted_time = '{}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}'.format(
            current_time[0], current_time[1], current_time[2],
            current_time[3], current_time[4], current_time[5])
            print(formatted_time[0])
            message = {'dataPoint': formatted_time, 'paramType': "cpu_temp", 'paramValue': number, 'deviceId': "307452396536950" }
            message_bytes = json.dumps(message).encode('utf-8')  # Convert dict to JSON string and encode to bytes
            client.publish(topic, message_bytes)
            print('Published:', message)
            message_count += 1
            blink()
            time.sleep(4)  # Publish a message every 4 seconds
        except OSError as e:
            print('Publish failed:', e)
            break

def pub_main():
    connect_to_wifi()
    
    while True:
        connect_and_publish()
        time.sleep(10)
        machine.reset()  # Reset and try again

if __name__ == '__main__':
    pub_main()

