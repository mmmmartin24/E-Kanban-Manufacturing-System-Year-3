📄 Overview
This project implements a Smart Digital Kanban System using IoT technologies for a manufacturing environment. The system automates order tracking, material handling, storage, assembly, and quality control using QR Codes, RFID, ESP32 microcontrollers, Raspberry Pi, MQTT protocol, and a Node-RED dashboard with a MySQL database. This system has also passed the live demo test by the class lecturer.

🚀 Features
- Order Management: Orders are initiated by scanning QR Codes; data is sent via MQTT.
- Digital Kanban Cards: Each order is linked to an RFID card carrying product and order info.
- Automated Material Handling: Material levels update in real time based on production needs.
- Multi-Station Workflow: Includes Order Station, Material Handling, Storage, Warehouse, Assembly Stations, and Quality Control.
- Live Dashboard: Node-RED flows display current order status, material stock, and production progress.
- Real-Time Data Sync: All stations communicate via MQTT and update a centralized database.

📚 System Architecture
Stations Implemented:
- Order Station: QR code scanner publishes order data to MQTT.
- Material Handling Station: Writes order info to RFID, updates progress.
- Storage Station: Reads RFID for in/out stock, shows unit count via TM1637 displays.
- Warehouse Station: Manages materials for each product type, displays needed vs. available materials.
- Assembly Stations (1 & 2): Guides workers with visual instructions based on product type.
- Quality Control Station: Verifies final product assembly meets standards.

🗂️ Tech Stack
Technology	Description
- ESP32	Main microcontroller for IoT nodes
- Raspberry Pi	Host for QR Scanner & dashboard
- MQTT	Message protocol for real-time updates
- RFID	Digital kanban cards for tracking orders
- Node-RED	Visualization and data flow management
- MySQL	Database for orders, progress, storage

🛠️ How It Works
1. Order Placed: QR code is scanned → Order info is published to MQTT.
2. Kanban Created: RFID card written with order info.
3. Production: Each station reads RFID, updates status, and provides instructions.
4. Dashboard: Progress and stock levels are visualized via Node-RED.

⚡ Challenges
- Complex integration of MQTT topics and JSON formatting.
- Troubleshooting hardware issues (RFID, cables, breadboards).
- Network limitations (unstable campus Wi-Fi, fallback to hotspot).
- Synchronizing multiple microcontrollers with Node-RED and MySQL.

👨‍💻 Contributors
Name	Contributions
Martin - Arduino codes for all stations, Node-RED flows, database setup, hardware assembly, debugging, report writing
Wesley - Arduino & Python codes for storage, warehouse, and order stations, hardware assembly, MQTT config
Moses - System integration ideas, JSON structuring, Tkinter visualizations, debugging, report writing

📌 Getting Started
- Create the hardware setup and props for the simulation
- Flash ESP32 boards with Arduino codes for each station.
- Run Python scripts on Raspberry Pi for QR scanning and dashboard.
- Set up MQTT broker and MySQL database.
- Import Node-RED flows and start the dashboard.

⚙️ Requirements
- ESP32 boards
- RFID readers (MFRC522)
- GM66 QR scanner
- Raspberry Pi with Python and Tkinter
- Node-RED installed with MySQL integration
- Wi-Fi or stable hotspot for MQTT
