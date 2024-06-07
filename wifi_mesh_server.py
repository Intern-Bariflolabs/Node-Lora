import network
import socket
import time
import machine
import _thread

# Set up LED pin
led = machine.Pin(2, machine.Pin.OUT)  # GPIO2 is often used for the onboard LED on many ESP32 boards

# Connect to the Wi-Fi network
ssid = 'tapashnandi'
password = 'mypassword'

sta = network.WLAN(network.STA_IF)
sta.active(True)
sta.connect(ssid, password)

while not sta.isconnected():
    print('Connecting to network...')
    time.sleep(1)

print('Network connected')
print('IP:', sta.ifconfig()[0])

server_ip_1 = '192.168.4.1'
server_ip_2 = '192.168.4.2'  # Assuming the second server has this IP
server_port_1 = 12345
server_port_2 = 56789
addr_1 = (server_ip_1, server_port_1)
addr_2 = (server_ip_2, server_port_2)

# Variable to store the latest received data
received_data = []
data_lock = _thread.allocate_lock()

def receive_data(addr, name):
    global received_data
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    #s.setblocking(False)
    #time.sleep(5)  # Add a timeout to prevent blocking forever
    try:
        while True:
            try:
                s.sendto(b'REQUEST', addr)
                data, server_addr = s.recvfrom(1024)
                with data_lock:
                    received_data.append((name, data.decode()))
                print(f'Received from {name} ({addr}):', data.decode())
                
                # Blink the LED
                led.on()
                time.sleep(0.5)  # LED on for 0.5 seconds
                led.off()
                time.sleep(0.5)  # LED off for 0.5 seconds
                
                time.sleep(1)  # Adjust the sleep time as needed
            except socket.timeout:
                continue  # Retry on timeout
    except Exception as e:
        print(f'Error in receive_data from {name}:', e)
    finally:
        s.close()

def create_ap_and_forward_data():
    global received_data
    # Creating Access Point
    ap_ssid = 'tapashnandi'
    ap_password = 'mypassword'

    ap = network.WLAN(network.AP_IF)
    ap.active(True)
    ap.config(essid=ap_ssid, password=ap_password)
    while not ap.active():
        time.sleep(1)

    print('Access Point active')
    print('AP IP:', ap.ifconfig()[0])

    # Setting up UDP server on the AP
    ap_server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    ap_server_socket.bind(('0.0.0.0', server_port_1))

    connected_clients = []  # List to store connected clients

    try:
        while True:
            data, client_addr = ap_server_socket.recvfrom(1024)
            print('Received connection from:', client_addr)
            
            # Add new clients to the list
            if client_addr not in connected_clients:
                connected_clients.append(client_addr)
            
            # Forward all received data to all connected clients
            with data_lock:
                for name, message in received_data:
                    for client in connected_clients:
                        ap_server_socket.sendto(f'From {name}: {message}'.encode(), client)
                        print('Forwarded data to client:', client)
            received_data.clear()  # Clear the list after forwarding
            time.sleep(1)  # Adjust the sleep time as needed
    except Exception as e:
        print('Error in create_ap_and_forward_data:', e)
    finally:
        ap_server_socket.close()

# Start the threads
_thread.start_new_thread(receive_data, (addr_1, 'Server 1'))
#_thread.start_new_thread(receive_data, (addr_2, 'Server 2'))
#_thread.start_new_thread(create_ap_and_forward_data, ())

# Keep the main thread alive
while True:
    time.sleep(1)

