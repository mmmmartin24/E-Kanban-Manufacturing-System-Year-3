from paho.mqtt import client as mqtt_client
import time
import sqlite3
import os
from PIL import Image, ImageTk
import tkinter as tk

path = "/home/pi/Desktop/IEE21/"

broker = '10.252.233.186'
port = 1889
data = {'ID':None, 'Date':None, 'Product':None, 'Station':None}
window = tk.Tk()
window.title("Product Instruction")

# Global variable to store the reference of the image
photo = None

def connect_mqtt() -> mqtt_client:
    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            print("Connected to MQTT Broker!")
        else:
            print("Failed to connect, return code %d\n", rc)
    client = mqtt_client.Client('Python1')
    client.on_connect = on_connect
    client.connect(broker, port)
    return client

def subscribe(client: mqtt_client):
    def on_message(client, userdata, msg):
        global photo  # Declare photo as global to modify it inside the function
        print(f"Received {msg.payload.decode()} from {msg.topic} topic")
        if msg.topic == 'QUALITY_CONTROL':
            data['QUALITY_CONTROL'] = msg.payload.decode()
            image_path = ""
            if data['QUALITY_CONTROL'] == 'Product type: X':
                image_path = path + "X.png"
            elif data['QUALITY_CONTROL'] == 'Product type: Y':
                image_path = path + "Y.png"
            elif data['QUALITY_CONTROL'] == 'Product type: Z':
                image_path = path + "Z.png"

            if image_path:
                image = Image.open(image_path)
                photo = ImageTk.PhotoImage(image)
                label.config(image=photo)
                label.image = photo

    client.subscribe('ID')
    client.subscribe('Date')
    client.subscribe('Station')
    client.subscribe('QUALITY_CONTROL')
    client.on_message = on_message

# Create a label to display images
label = tk.Label(window)
label.pack()

# MQTT
client = connect_mqtt()
subscribe(client)
client.loop_start()

# Run the Tkinter main loop
window.mainloop()
