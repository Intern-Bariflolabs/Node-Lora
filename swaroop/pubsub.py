import network
from umqtt.simple import MQTTClient
import time
import machine
import json
import random
import _thread  # Import the threading library
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
topic = b'307452396536950'  # Ensure topic is bytes type for subscribing
publish_topic = '307452396536950/data'  # Topic for publishing

def connect_to_wifi():
    station = network.WLAN(network.STA_IF)
    station.active(True)
    print('Connecting to Wi-Fi...')
    station.connect(ssid, password)
    
    retry_count = 0
    max_retries = 10
    
    while not station.isconnected():
        if retry_count >= max_retries:
            print('Failed to connect to Wi-Fi')
            return False
        print('Retrying Wi-Fi connection...')
        retry_count += 1
        time.sleep(2)
    
    print('Connected to Wi-Fi')
    print('IP Address:', station.ifconfig()[0])
    return True

def on_message(topic, msg):
    try:
        data = msg.decode('utf-8')
        print(f"Received message on topic {topic.decode('utf-8')}: {data}")
    except Exception as e:
        print("Error converting received message:", str(e))

def connect_and_subscribe():
    client = MQTTClient(client_id, mqtt_server, user=mqtt_username, password=mqtt_password, port=mqtt_port)
    
    try:
        client.connect()
        print(f'Connected to MQTT broker {mqtt_server}')
        client.set_callback(on_message)
        client.subscribe(topic)
        print(f"Subscribed to topic {topic.decode('utf-8')}")
        return client
    except OSError as e:
        print(f'Failed to connect to MQTT broker: {e}')
        return None

def subscribe():
    while True:
        mqtt_client = connect_and_subscribe()
        if mqtt_client:
            try:
                print("Listening for messages...")
                while True:
                    mqtt_client.wait_msg()
            except OSError as e:
                print(f"An error occurred: {e}")
            finally:
                mqtt_client.disconnect()
                print("Disconnected from MQTT broker")
        time.sleep(10)  # Wait before trying to reconnect

def connect_and_publish():
    while True:
        client = MQTTClient(client_id, mqtt_server, user=mqtt_username, password=mqtt_password, port=mqtt_port)
        try:
            client.connect()
            print(f'Connected to {mqtt_server}')
            while True:
                try:
                    number = random.randint(30, 50)
                    current_time = time.localtime()
                    formatted_time = '{}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}'.format(
                        current_time[0], current_time[1], current_time[2],
                        current_time[3], current_time[4], current_time[5])
                    message = {'dataPoint': formatted_time, 'paramType': "cpu_temp", 'paramValue': number, 'deviceId': "307452396536950"}
                    message_bytes = json.dumps(message).encode('utf-8')  # Convert dict to JSON string and encode to bytes
                    client.publish(publish_topic, message_bytes)
                    print('Published:', message)
                    blink()
                    time.sleep(4)  # Publish a message every 4 seconds
                except OSError as e:
                    print('Publish failed:', e)
                    break
        except OSError as e:
            print(f'Failed to connect to MQTT broker: {e}')
        finally:
            try:
                client.disconnect()
                print("Disconnected from MQTT broker")
            except OSError as e:
                print(f'Error disconnecting from MQTT broker: {e}')
        time.sleep(10)  # Wait before trying to reconnect

def pub_main():
    while True:
        if connect_to_wifi():
            connect_and_publish()
        time.sleep(10)
        machine.reset()  # Reset and try again

def sub_main():
    while True:
        if connect_to_wifi():
            subscribe()
        time.sleep(10)
        machine.reset()  # Reset and try again

if __name__ == '__main__':
    _thread.start_new_thread(sub_main, ())  # Start the subscribe function in a new thread
    time.sleep(10)
    _thread.start_new_thread(pub_main, ())  # Start the publish function in a new thread
    time.sleep(10)
    # Keep the main thread alive
    while True:
        time.sleep(1)
