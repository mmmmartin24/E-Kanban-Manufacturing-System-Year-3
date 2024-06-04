#include <TM1637Display.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <time.h>
#define NTP_OFFSET  0      // In seconds
#define NTP_INTERVAL  25200    // In miliseconds
#define NTP_ADDRESS  "pool.ntp.org" 


#define RST_PIN         4          // Configurable, see typical pin layout above
#define SS_1_PIN        12         // Configurable, take a unused pin, only HIGH/LOW required, must be different to SS 2
#define SS_2_PIN        13         // Configurable, take a unused pin, only HIGH/LOW required, must be different to SS 1

#define NR_OF_READERS   2

byte ssPins[] = {SS_1_PIN, SS_2_PIN};

int storage_x = 50;
int storage_y = 40;
int storage_z = 30;

#define CLK_x  32 // The ESP32 pin GPIO22 connected to CLK 26
#define DIO_x  21 // The ESP32 pin GPIO23 connected to DIO 25

#define CLK_y  33 // The ESP32 pin GPIO22 connected to CLK 26
#define DIO_y  17 // The ESP32 pin GPIO23 connected to DIO 25

#define CLK_z  25 // The ESP32 pin GPIO22 connected to CLK 26
#define DIO_z  16 // The ESP32 pin GPIO23 connected to DIO 25

TM1637Display display_x = TM1637Display(CLK_x, DIO_x);
TM1637Display display_y = TM1637Display(CLK_y, DIO_y);
TM1637Display display_z = TM1637Display(CLK_z, DIO_z);

MFRC522 mfrc522[NR_OF_READERS];   // Create MFRC522 instance.

// Wi-Fi
const char *ssid = "AndroidAPCA5E";
const char *password = "Apahayo.";
WiFiClient wifiClient;

// MQTT
const char *mqtt_server = "192.168.132.190";
const char *data_topic = "data";
const char* mqtt_username = "siaiti"; 
const char* mqtt_password = "wololo"; 
const char *clientID = "RFID_Station_1";
PubSubClient client(mqtt_server, 1889, wifiClient);

// Default key for authentication
MFRC522::MIFARE_Key key;

void setup_wifi() {
  configTime(NTP_OFFSET * 3600, NTP_INTERVAL, NTP_ADDRESS);
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void connect_mqtt() {
  while (! client.connect(clientID)) Serial.println("Connection to MQTT Broker failed…");
  Serial.println("Connected to MQTT Broker!");
}

void setup() {
  Serial.begin(9600); // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  setup_wifi();
  client.setServer(mqtt_server, 1889);
  SPI.begin();        // Init SPI bus

  for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) {
    mfrc522[reader].PCD_Init(ssPins[reader], RST_PIN); // Init each MFRC522 card
    Serial.print(F("Reader "));
    Serial.print(reader);
    Serial.print(F(": "));
    mfrc522[reader].PCD_DumpVersionToSerial();
  }

  // Initialize the key with all 0xFF values (default key)
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  display_x.clear();
  display_y.clear();
  display_z.clear();

  display_x.setBrightness(7);
  display_y.setBrightness(7);
  display_z.setBrightness(7);
  
}
String removeNullChars(String str) {
  String cleanedStr = "";
  for (int i = 0; i < str.length(); i++) {
    if (str[i] != '\0') {
      cleanedStr += str[i];
    }
  }
  return cleanedStr;
}
void publishMQTTin(String update_id_string, String id_prog_string) {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  char timestamp[50];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);
  // Remove null characters from the strings
  String cleanedUpdateId = removeNullChars(update_id_string);
  String cleanedIdProg = removeNullChars(id_prog_string);
  // Create a JSON document
  StaticJsonDocument<200> doc;

  // Populate the JSON document with data
  doc["update_id"] = cleanedUpdateId;
  doc["id_product_type"] = cleanedIdProg;
  doc["progress"] = "Storage In Station";
  doc["percentage"] = "100%";
  doc["timestamp"] = timestamp;

  // Convert the JSON document to a string
  String payload;
  serializeJson(doc, payload);  

  // Connect to MQTT and publish the message
  connect_mqtt();
  if (client.publish("progress", payload.c_str())) {
    Serial.println("Data sent!");
  } else {
    Serial.println("Data failed to send. Reconnecting...");
    client.connect(clientID, mqtt_server, data_topic);
    delay(10); // This delay ensures that client.publish doesn’t clash with the client.connect call
    client.publish("progress", payload.c_str());
  }
}

void publishMQTTout(String update_id_string, String id_prog_string) {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  char timestamp[50];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);
  // Remove null characters from the strings
  String cleanedUpdateId = removeNullChars(update_id_string);
  String cleanedIdProg = removeNullChars(id_prog_string);
  // Create a JSON document
  StaticJsonDocument<200> doc;

  // Populate the JSON document with data
  doc["update_id"] = cleanedUpdateId;
  doc["id_product_type"] = cleanedIdProg;
  doc["progress"] = "Storage Out Station";
  doc["percentage"] = "25%";
  doc["timestamp"] = timestamp;

  // Convert the JSON document to a string
  String payload;
  serializeJson(doc, payload);  

  // Connect to MQTT and publish the message
  connect_mqtt();
  if (client.publish("progress", payload.c_str())) {
    Serial.println("Data sent!");
  } else {
    Serial.println("Data failed to send. Reconnecting...");
    client.connect(clientID, mqtt_server, data_topic);
    delay(10); // This delay ensures that client.publish doesn’t clash with the client.connect call
    client.publish("progress", payload.c_str());
  }
}

void publishMQTT(String p_type, String p_amount) {
  // Create a JSON document
  StaticJsonDocument<200> doc;

  // Populate the JSON document with data
  doc["Product_type"] = p_type;
  doc["Amount"] = p_amount;

  // Convert the JSON document to a string
  String payload;
  serializeJson(doc, payload);

  connect_mqtt();
  
//  client.publish("storage/amount", payload.c_str());
  if (client.publish("storage/amount", payload.c_str())) {
    Serial.println("Data sent!");
  } else {
    Serial.println("Data failed to send. Reconnecting...");
    client.connect(clientID, mqtt_server, "storage/amount");
    delay(10); // This delay ensures that client.publish doesn’t clash with the client.connect call
    client.publish("data_topic", payload.c_str());
  }
}

void loop() {
  display_x.showNumberDec(storage_x);
  display_y.showNumberDec(storage_y);
  display_z.showNumberDec(storage_z);

  for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) {
      String block4Data;  // Variable to store block 4 data
      String blockData;  // Variable to store block 6 data
      if (mfrc522[reader].PICC_IsNewCardPresent() && mfrc522[reader].PICC_ReadCardSerial()) {
        // Card detected, read UID and block 4 and 6 data
        byte blocks[] = {4, 6};  // Blocks to read
        MFRC522::StatusCode status;

        for (int i = 0; i < 2; i++) {
          byte len = 18;
          byte buffer[18];
          String data = "";

          status = (MFRC522::StatusCode)mfrc522[reader].PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blocks[i], &key, &(mfrc522[reader].uid));
          if (status != MFRC522::STATUS_OK) {
            Serial.println(F("Authentication failed"));
            return;
          }

          status = (MFRC522::StatusCode)mfrc522[reader].MIFARE_Read(blocks[i], buffer, &len);
          if (status != MFRC522::STATUS_OK) {
            Serial.println(F("Read failed"));
            return;
          }

          for (byte j = 0; j < len; j++) {
            if (buffer[j] != 0) {
              data += (char)buffer[j];
            } else {
              break;
            }
          }

          // Assign the data to the correct variable
          if (blocks[i] == 4) {
            block4Data = data;
          } else if (blocks[i] == 6) {
            blockData = data;
          }
        }
        // Now block4Data contains the data from block 4 and blockData contains the data from block 6
        Serial.println("Block 4 data: " + block4Data);
        Serial.println("Block 6 data: " + blockData);
      
      
      Serial.print(F("Reader "));
      Serial.print(reader);
      Serial.print(F(": Card UID:"));
      dump_byte_array(mfrc522[reader].uid.uidByte, mfrc522[reader].uid.size);
      Serial.println();
      Serial.print(F("PICC type: "));
      MFRC522::PICC_Type piccType = mfrc522[reader].PICC_GetType(mfrc522[reader].uid.sak);
      Serial.println(mfrc522[reader].PICC_GetTypeName(piccType));
      Serial.print(F("Data in block 6: "));
      Serial.println(blockData);

      // Handle product type and update storage
      if (reader == 0) {
        client.publish("Product_type", blockData.c_str());
        publishMQTTout(block4Data, blockData);  // Added missing semicolon
        if (blockData == "X") {
          storage_x--;
          publishMQTT("Product X", String(storage_x).c_str());
        } else if (blockData == "Y") {
          storage_y--;
          publishMQTT("Product Y", String(storage_y).c_str());
        } else if (blockData == "Z") {
          storage_z--;
          publishMQTT("Product Z", String(storage_z).c_str());
        }
      } else if (reader == 1) {
        client.publish("Product_type", blockData.c_str());
        publishMQTTin(block4Data,blockData);
        if (blockData == "X") {
          storage_x++;
          publishMQTT("Product X", String(storage_x).c_str());
        } else if (blockData == "Y") {
          storage_y++;
          publishMQTT("Product Y", String(storage_y).c_str());
        } else if (blockData == "Z") {
          storage_z++;
          publishMQTT("Product Z", String(storage_z).c_str());
        }
      }
      }

      mfrc522[reader].PICC_HaltA();
      mfrc522[reader].PCD_StopCrypto1();
  }
}



/**
 * Helper routine to dump a byte array as hex values to Serial.
 */
void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}



void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(clientID)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
