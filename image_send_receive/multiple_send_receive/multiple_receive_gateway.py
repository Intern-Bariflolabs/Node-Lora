import socket
import os

port = 12345

# Dictionary to keep track of the number of images received from each node
image_counter = {}

# Create a server socket
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind(("0.0.0.0", port))
server_socket.listen(5)  # Listen for incoming connections

print("Waiting for connections...")

while True:
    client_socket, client_address = server_socket.accept()
    print(f"Connection established with {client_address}")

    try:
        # Receive node ID (assuming it's sent as a string and terminated with a newline)
        node_id = client_socket.recv(1024).decode().strip()
        if not node_id:
            print("No node ID received.")
            client_socket.close()
            continue

        # Initialize or update the image counter for the node ID
        if node_id not in image_counter:
            image_counter[node_id] = 1
        else:
            image_counter[node_id] += 1

        # Receive image data
        received_data = bytearray()
        while True:
            data = client_socket.recv(4096)  # Receive 4096 bytes at a time
            if not data:
                break
            received_data.extend(data)

        # Construct the filename using node ID and counter
        filename = f'received_image_{node_id}_{image_counter[node_id]}.jpg'

        # Write received image data to a file
        with open(filename, 'wb') as f:
            f.write(received_data)

        print(f"Image received and saved as {filename}.")

    except Exception as e:
        print(f"An error occurred: {e}")

    finally:
        client_socket.close()
        print("Client socket closed. Waiting for the next connection...")

server_socket.close()
print("Server socket closed.")
