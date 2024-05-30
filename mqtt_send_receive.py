import network
import time
from umqtt.simple import MQTTClient
import random
import json
from machine import Pin

# Replace with your network credentials
ssid = 'BarifloLabs'
password = 'Bfl_wifi0@1234'

# MQTT Broker details
mqtt_server = '4.240.114.7'  # Public broker
mqtt_port = 1883
mqtt_user = 'BarifloLabs'  # Leave empty if no authentication
mqtt_password = 'Bfl@123'  # Leave empty if no authentication
client_id = 'ESP32Client'
topic_pub = '307452396536950/data'
topic_sub = b'307452396536950'  # Ensure topic is bytes type for subscribing

# Initialize LED Pin
led = Pin(2, Pin.OUT)

def connect_wifi():
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    wlan.connect(ssid, password)
    
    while not wlan.isconnected():
        print('Connecting to WiFi...')
        time.sleep(1)
    
    print('WiFi connected')
    print('IP address:', wlan.ifconfig()[0])

def blink_led():
    led.on()   # Turn the LED on
    time.sleep(0.5)
    led.off()  # Turn the LED off
    time.sleep(0.5)

def connect_mqtt():
    client = MQTTClient(client_id, mqtt_server, mqtt_port, mqtt_user, mqtt_password)
    client.connect()
    print('Connected to MQTT broker')
    client.set_callback(on_message)
    client.subscribe(topic_sub)
    print(f"Subscribed to topic {topic_sub.decode('utf-8')}")
    return client

def on_message(topic, msg):
    try:
        data = msg.decode('utf-8')
        print(f"Received message on topic {topic.decode('utf-8')}: {data}")
        # Add your message processing logic here
        
        # For example, blink the LED when a message is received
        blink_led()
        
    except Exception as e:
        print("Error converting received message:", str(e))

def main():
    connect_wifi()
    client = connect_mqtt()
    
    while True:
        try:
            number = random.randint(19, 21)
            current_time = time.localtime()
            formatted_time = '{}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}'.format(
                current_time[0], current_time[1], current_time[2],
                current_time[3], current_time[4], current_time[5])
            message = {'dataPoint': formatted_time, 'paramType': "cpu_temp", 'paramValue': number, 'deviceId': "307452396536950" }
            message_bytes = json.dumps(message).encode('utf-8')  # Convert dict to JSON string and encode to bytes
            client.publish(topic_pub, message_bytes)
            print('Published:', message)
            time.sleep(4)  # Publish a message every 4 seconds
            
            # Check for incoming messages
            client.check_msg()
            
        except OSError as e:
            print('Publish failed:', e)
            break

if __name__ == '__main__':
    main()

