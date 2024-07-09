import socket

def send_image(image_path, host, port):

    with open(image_path, 'rb') as f:
        image_data = f.read()  # read the file in binary

    # Then we have to create a socket object
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        # Bind the socket to the host and port
        s.bind((host, port))
        # Listen for incoming connections
        s.listen(1)
        print(f'Server listening on {host}:{port}')

        # Accept a connection
        conn, addr = s.accept()
        with conn:
            print('Connected by', addr)
            # Send the image data
            conn.sendall(image_data)
            print('Image sent successfully')

if __name__ == "__main__":
    image_path = '/home/tapash/image/image1.jpg'
    host = '0.0.0.0'
    port = 12345

    send_image(image_path, host, port)
