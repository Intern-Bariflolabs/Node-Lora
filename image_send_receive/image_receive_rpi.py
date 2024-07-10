import socket

# Port to listen on (should be the same as on the sender)
PORT = 12345

# Create a socket
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Bind the socket to a specific address and port
server_socket.bind(('0.0.0.0', PORT))

# Listen for incoming connections (1 connection at a time)
server_socket.listen(1)

print("Waiting for incoming connection...")

# Accept a connection from the sender
client_socket, client_address = server_socket.accept()

try:
    print(f"Connection established from {client_address}")
    
    # Receive the image data
    image_data = client_socket.recv(4096)  # Adjust buffer size as necessary
    
    # Write the received image data to a file
    with open('received_image.jpg', 'wb') as f:
        f.write(image_data)
    
    print("Image received and saved as 'received_image.jpg'.")
    
finally:
    # Close the server socket
    server_socket.close()
