import network
from umqtt.simple import MQTTClient
import time
import machine

# Wi-Fi credentials
ssid = 'BarifloLabs'
password = 'Bfl_wifi0@1234'

# MQTT broker information
mqtt_server = '4.240.114.7'
mqtt_port = 1883
mqtt_username = 'BarifloLabs'
mqtt_password = 'Bfl@123'
client_id = 'ESP32A'
topic = 'test/topic'

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

def message_callback(topic, msg):
    print(f'Received message from topic {topic.decode()}: {msg.decode()}')
    # Add your message processing logic here

def connect_and_subscribe():
    client = MQTTClient(client_id, mqtt_server, user=mqtt_username, password=mqtt_password, port=mqtt_port)
    
    try:
        client.connect()
        print(f'Connected to {mqtt_server}')
    except OSError as e:
        print(f'Failed to connect to MQTT broker: {e}')
        return
    
    client.set_callback(message_callback)
    client.subscribe(topic)
    print(f'Subscribed to {topic}')
    
    try:
        while True:
            client.check_msg()  # Non-blocking method to check for new messages
            time.sleep(1)
    except OSError as e:
        print('Subscription failed:', e)
        client.disconnect()

def main():
    connect_to_wifi()
    
    while True:
        connect_and_subscribe()
        time.sleep(10)
        machine.reset()  # Reset and try again

if __name__ == '__main__':
    main()
