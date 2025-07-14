import sqlite3
from datetime import datetime
import paho.mqtt.client as mqtt
import time
import json

# MQTT broker settings
broker_address = "10.252.225.135"
broker_port = 1889
publish_id = "scan_id"
publish_product = "scan_product"
publish_json = "order_json"
subscribe_topic = "rfid_status"

# Callback when a new message is received
def on_message(client, userdata, message):
    if int(message.payload.decode()) == 1:
        print("Received message 1, stopping loop...")
        global loop_flag
        loop_flag = False

order_id = 1

while True:
    qr_scanned = input("Scan barcode (type 'stop' to exit): ")
    if qr_scanned.lower() == "stop":
        break
    elif qr_scanned[:2] != "ID" :
        continue
    client = mqtt.Client()
    client.on_message = on_message
    client.connect(broker_address, broker_port)
    client.loop_start()
    client.subscribe(subscribe_topic)
    loop_flag = True

    kanban_data = qr_scanned.split(',')
    print(kanban_data)
    # Extract the relevant parts
    order_id = kanban_data[0].split(':')[1]
    product_type = kanban_data[1].split(':')[1]
    timestamp = time.strftime("%Y-%m-%d %H:%M:%S", time.gmtime())

    json_data = str({
        "id_order": str(order_id),
        "Product_type_order": str(product_type),
        "timestamp": str(timestamp)
    })

    print(f"Order ID: {order_id}\nProduct type: {product_type}")
    print(f"JSON data:\n{json_data}")
    while loop_flag:

        client.publish(publish_id, order_id)
        print(f"Published id: '{order_id}', topic: {publish_id}")

        client.publish(publish_product, product_type)
        print(f"Published product: '{product_type}', topic: {publish_product}")

        client.publish(publish_json, json_data)
        print(f"Published json: '{json_data}', topic: {publish_json}")

        time.sleep(1)
    print("Publish successful")
    order_id += 1
    client.loop_stop()
    client.disconnect()
