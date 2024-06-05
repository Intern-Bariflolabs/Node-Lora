import time
import socket
import network
import ujson
#CONFIG_FILE = 'wifi_config.json'

# Function to save Wi-Fi configuration
#def save_config(ssid, password):
    #config = {'ssid': ssid, 'password': password}
    #with open(CONFIG_FILE, 'w') as f:
   #     ujson.dump(config, f)

# Function to load Wi-Fi configuration
#def load_config():
    #try:
       # with open(CONFIG_FILE, 'r') as f:
      #      config = ujson.load(f)
     #       return config['ssid'], config['password']
    #except (OSError, ValueError):
         #return 'tapashnandi', 'mypassword'
# Setup Access Point
ap = network.WLAN(network.AP_IF)
ap.active(True)
ssid = 'tapashnandi'
password = 'mypassword'
ap.config(essid=ssid, password=password)
ap.config(authmode=network.AUTH_WPA_WPA2_PSK)
time.sleep(2)
print('Access Point Config:', ap.ifconfig())

addr = socket.getaddrinfo('0.0.0.0', 80)[0][-1]
s = socket.socket()
s.bind(addr)
s.listen(1)
print('Listening on:', addr)

def create_frontend(current_ssid):
    # Define the HTML response
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

    # Check if the request is a POST request
    if 'POST' in request:
        # Extract the form data
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
            ap.config(essid=ssid, password=password)

    response = create_frontend(ssid)
    client_socket.send(response)
    client_socket.close()
def reconfigure_ap(new_ssid, new_password):
    # Deactivate the current AP
    ap.active(False)
    time.sleep(1)
    # Reconfigure the AP with the new settings
    ap.config(essid=new_ssid, password=new_password)
    ap.active(True)
    time.sleep(2)
    print('Reconfigured Access Point to:', new_ssid)
    print('Access Point Config:', ap.ifconfig())
while True:
    cl, addr = s.accept()
    print('Client connected from:', addr)
    handle_request(cl)

