from picamera2 import Picamera2
from time import sleep

# Initialize the camera
picam2 = Picamera2()

# Configure the camera resolution
picam2.configure(picam2.create_still_configuration(main={"size": (1200, 900)}))

# Start the camera
picam2.start()

# Wait for 2 seconds to allow the camera to adjust
sleep(2)

# Capture an image and save it as 'cam.png'
picam2.capture_file("cam.png")

print("Done.")

