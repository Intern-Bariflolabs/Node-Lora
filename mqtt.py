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
            message = f'Message {message_count}'
            client.publish(topic, message)
            print('Published:', message)
            message_count += 1
            time.sleep(5)  # Publish a message every 5 seconds
        except OSError as e:
            print('Publish failed:', e)
            break

def main():
    connect_to_wifi()
    
    while True:
        connect_and_publish()
        time.sleep(10)
        machine.reset()  # Reset and try again

if __name__ == '__main__':
    main()
