import time
import socket
import network
import machine

# Setup Access Point
led = machine.Pin(2, machine.Pin.OUT)
ap = network.WLAN(network.AP_IF)
ap.active(True)
ssid = 'tapashnandi'
password = 'mypassword'
ap.config(essid=ssid, password=password)
ap.config(authmode=network.AUTH_WPA_WPA2_PSK)
time.sleep(2)
print('Access Point Config:', ap.ifconfig())

addr = ('0.0.0.0', 12345)
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.bind(addr)
print('Listening on:', addr)

def create_frontend(current_ssid):
    html = """\
HTTP/1.1 200 OK

<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Web Server</title>
</head>
<body>
<form action="/" method="post">
    <label for="id">ID:</label>
    <input type="text" id="id" name="id" required>
    <br>
    <label for="password">Password:</label>
    <input type="password" id="password" name="password" required>
    <br>
    <input type="submit" value="Submit">
</form>
</body>
</html>
""".format(current_ssid)
    return html

def handle_request(client_socket):
    request = client_socket.recv(1024).decode('utf-8')
    print('Request received:', request)

    if 'POST' in request:
        form_data = request.split('\r\n\r\n')[1]
        params = dict(param.split('=') for param in form_data.split('&'))
        new_id = params.get('id')
        new_password = params.get('password')
        
        if new_id and new_password:
            global ssid, password
            ssid = new_id
            password = new_password
            print('Updating SSID to:', ssid)
            print('Updating Password to:', password)
            reconfigure_ap(ssid, password)

    response = create_frontend(ssid)
    client_socket.send(response)
    client_socket.close()

def reconfigure_ap(new_ssid, new_password):
    ap.active(False)
    time.sleep(1)
    ap.config(essid=new_ssid, password=new_password)
    ap.active(True)
    time.sleep(2)
    print('Reconfigured Access Point to:', new_ssid)
    print('Access Point Config:', ap.ifconfig())

try:
    while True:
        data, addr = s.recvfrom(1024)
        print('Client connected from:', addr)
        if data.decode() == 'REQUEST':
            response = "Hello World"
            s.sendto(response.encode(), addr)
            print('Response sent to', addr)
            time.sleep(2)
            # Wait for acknowledgment
            
except KeyboardInterrupt:
    print('Server stopped')
    s.close()

