import socket
import os
from azure.storage.blob import BlobServiceClient

port = 12345

# Dictionary to keep track of the number of images received from each node
image_counter = {}

def receive_image(client_socket):
    try:
        # Receive node ID (assuming it's sent as a string and terminated with a newline)
        node_id = client_socket.recv(1024).decode().strip()
        if not node_id:
            print("No node ID received.")
            client_socket.close()
            return None

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
        return filename

    except Exception as e:
        print(f"An error occurred: {e}")
        return None

    finally:
        client_socket.close()
        print("Client socket closed.")

def upload_to_azure(file_path, blob_name):
    # Replace with your SAS token and SAS URL
    sas_token = "sp=racwl&st=2024-07-13T08:12:11Z&se=2024-07-13T16:12:11Z&sv=2022-11-02&sr=c&sig=X1mX9lGAh7WPtgzHex9dHb7V8pYpR6syFTm2tpVm0wY%3D"
    sas_url = "https://lorabit.blob.core.windows.net/esp32?sp=racwl&st=2024-07-13T08:12:11Z&se=2024-07-13T16:12:11Z&sv=2022-11-02&sr=c&sig=X1mX9lGAh7WPtgzHex9dHb7V8pYpR6syFTm2tpVm0wY%3D"

    # Initialize the BlobServiceClient using SAS token and SAS URL
    blob_service_client = BlobServiceClient(account_url=sas_url, credential=sas_token)

    # Replace with the container (folder) name you want to upload to
    container_name = "esp32"

    # Read the image file as binary
    with open(file_path, "rb") as f:
        image_data = f.read()

    # Create a blob client using the container (folder) and blob name
    blob_client = blob_service_client.get_blob_client(container=container_name, blob=blob_name)

    # Upload the image data to Azure Blob Storage
    blob_client.upload_blob(image_data, overwrite=True)

    print(f"Image '{blob_name}' uploaded to Azure Blob Storage successfully in container '{container_name}'")

def main():
    # Create a server socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind(("0.0.0.0", port))
    server_socket.listen(5)  # Listen for incoming connections

    print("Waiting for connections...")

    while True:
        client_socket, client_address = server_socket.accept()
        print(f"Connection established with {client_address}")

        filename = receive_image(client_socket)
        if filename:
            blob_name = os.path.basename(filename)  # Use the file name as the blob name
            upload_to_azure(filename, blob_name)

    server_socket.close()
    print("Server socket closed.")

if __name__ == "__main__":
    main()
