import socket

# Replace with your Raspberry Pi's IP address and the port you want to use
RPI_IP = '192.168.4.1'
PORT = 12345

# Node ID to send
NODE_ID = '101'

# Path to the image file you want to send
IMAGE_FILE = 'cam_test.png'

# Open the image file and read its content
with open(IMAGE_FILE, 'rb') as f:
    image_data = f.read()

# Create a socket
client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

try:
    # Connect to the server (Raspberry Pi)
    client_socket.connect((RPI_IP, PORT))

    # Send the node ID first
    client_socket.send(f"{NODE_ID}\n".encode())

    # Send the image data
    client_socket.send(image_data)

    print(f"Image '{IMAGE_FILE}' with node ID '{NODE_ID}' sent successfully.")

finally:
    # Close the socket
    client_socket.close()

