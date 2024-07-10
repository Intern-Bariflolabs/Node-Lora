from azure.storage.blob import BlobServiceClient

# Replace with your SAS token and SAS URL
sas_token = "sv=2022-11-02&ss=bfqt&srt=sco&sp=rwdlacupiytfx&se=2024-07-10T12:19:52Z&st=2024-07-10T04:19:52Z&spr=https,http&sig=l4AA4QGhygsoovW7Q44uPaaJY7ms%2BgulZR4qBljzMzw%3D"
sas_url = "https://lorabit.blob.core.windows.net/?sv=2022-11-02&ss=bfqt&srt=sco&sp=rwdlacupiytfx&se=2024-07-10T12:19:52Z&st=2024-07-10T04:19:52Z&spr=https,http&sig=l4AA4QGhygsoovW7Q44uPaaJY7ms%2BgulZR4qBljzMzw%3D"

# Initialize the BlobServiceClient using SAS token and SAS URL
blob_service_client = BlobServiceClient(account_url=sas_url, credential=sas_token)

# Replace with the container (folder) name you want to upload to
container_name = "esp32"
 # Replace with the name you want to give to the blob
blob_name = "esp32" 
# Path to the local image file
local_file_path = "C:/Users/TAPASHRANJAN/Documents/bfl _ bin testing/image1.jpg"

# Read the image file as binary
with open(local_file_path, "rb") as f:
    image_data = f.read()

# Create a blob client using the container (folder) and blob name
blob_client = blob_service_client.get_blob_client(container=container_name, blob=blob_name)

# Upload the image data to Azure Blob Storage
blob_client.upload_blob(image_data, overwrite=True)

print(f"Image '{blob_name}' uploaded to Azure Blob Storage successfully in container '{container_name}'")
