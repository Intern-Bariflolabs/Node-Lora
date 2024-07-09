import socket

def receive_image(save_path, host, port):
    # Create a socket object
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        # Connect to the server
        s.connect((host, port))
        print(f'Connected to server at {host}:{port}')

        # Receive the image data
        image_data = b''
        while True:
            chunk = s.recv(4096)
            if not chunk:
                break
            image_data += chunk

        # Save the received image data to a file
        with open(save_path, 'wb') as f:
            f.write(image_data)
        print('Image received and saved successfully')

if __name__ == "__main__":
    save_path = 'C:/Users/TAPASHRANJAN/Documents/bfl _ bin testing/image1.jpg'
    host = '172.18.105.134'  # Replace with the local IP address of your Ubuntu machine
    port = 12345  # Should match the port number used by the server

    receive_image(save_path, host, port)
