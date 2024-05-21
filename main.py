from mqtt_update import MqttConnect
import time
from datetime import datetime
from random import uniform
from paho.mqtt.client import Client
import threading
from recive import recive
import redis
from SendMail import SendMail

mail = SendMail()
mq = MqttConnect()
mq.topic = ["578689832956829"]
redis_server = redis.Redis(host="localhost",port=6379,db=0)

def post_data_to_publish():
    mq.connect_to_broker()
    while True:
        try:
            now = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
            for i in mq.topic:
                current_b = float(redis_server.get("current_b").decode('utf-8'))
                current_y = float(redis_server.get("current_y").decode('utf-8'))
                current_r = float(redis_server.get("current_r").decode('utf-8'))
                counter = int(redis_server.get("counter").decode('utf-8'))
                
                mq.data_publish({"dataPoint": now, "paramType": 'current', "paramValue": current_b , "deviceId": i})
                mq.data_publish({"dataPoint": now, "paramType": 'current', "paramValue": current_y , "deviceId": i})
                mq.data_publish({"dataPoint": now, "paramType": 'current', "paramValue": current_r , "deviceId": i})

                if counter%250 == 0:
                    args = (
                        'satyajit.bariflo@outlook.com', 
                        'New Current Status', 
                        'email_template.html',  
                        {'current_b': current_b, 'current_y': current_y, 'current_r': current_r} 
                    )
                    mail.send_email(*args)

            time.sleep(10)
        except Exception as e:
            print(str(e))
            time.sleep(10)
            continue



def data_subscribe():
    mqtt_client = Client()
    mqtt_client.on_connect = mq.on_connect
    mqtt_client.on_message = mq.on_sub_message
    mqtt_client.username_pw_set(mq._username, mq._password)
    mqtt_client.connect(mq._mqttBroker, port=mq._port)
    mqtt_client.loop_start()
    try:
        while True:
            pass
    except KeyboardInterrupt:
        mqtt_client.loop_start()



if __name__ == '__main__':
    threading.Thread(target=post_data_to_publish).start()
    threading.Thread(target=data_subscribe).start()
    threading.Thread(target=recive).start()