import socket

# Replace with your Raspberry Pi's IP address and the port you want to use
RPI_IP = '192.168.4.1'
PORT = 12345

# Path to the image file you want to send
IMAGE_FILE = 'C:/Users/TAPASHRANJAN/Documents/bfl _ bin testing/image1.jpg'

# Open the image file and read its content
with open(IMAGE_FILE, 'rb') as f:
    image_data = f.read()

# Create a socket
client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

try:
    # Connect to the server (Raspberry Pi)
    client_socket.connect((RPI_IP, PORT))
    
    # Send the image data
    client_socket.sendall(image_data)
    
    print(f"Image '{IMAGE_FILE}' sent successfully.")
    
finally:
    # Close the socket
    client_socket.close()
