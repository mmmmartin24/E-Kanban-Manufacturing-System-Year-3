// MQTT MESSAGE: PRODUCT_STATION1

#include <time.h>
#include <TM1637Display.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <ArduinoJson.h>
#define RST_PIN         2           // Configurable, see typical pin layout above
#define SS_PIN          5           // Configurable, see typical pin layout above
#define NTP_OFFSET  0      // In seconds
#define NTP_INTERVAL  25200    // In miliseconds
#define NTP_ADDRESS  "pool.ntp.org"  


MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;

// Wi-Fi
const char *ssid = "AndroidAPCA5E";
const char *password = "Apahayo.";
WiFiClient wifiClient;

// MQTT
const char *mqtt_server = ""192.168.132.190"";
const char *data_topic = "rfid";
const char *clientID = "RFID_Station_1";
PubSubClient client(mqtt_server, 1889, wifiClient);

String id;
String date;
String productType;

int array[] = { };
void setup() {
  Serial.begin(9600);  
  setup_wifi(); // Initialize serial communications with the PC
  SPI.begin();  // Init SPI bus
  mfrc522.PCD_Init();  
}

void publishMQTT(String update_id_string, String id_prog_string) {
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
  doc["progress"] = "Assembly Station 2";
  doc["percentage"] = "75%";
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

String removeNullChars(String str) {
  String cleanedStr = "";
  for (int i = 0; i < str.length(); i++) {
    if (str[i] != '\0') {
      cleanedStr += str[i];
    }
  }
  return cleanedStr;
}

String* scan() {
  static String scannedData[3]; // Array to hold scanned data: id, date, productType

  // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
  for (byte i = 0; i < 6; i++) 
    key.keyByte[i] = 0xFF;
  //-------------------------------------------
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if (!mfrc522.PICC_IsNewCardPresent()) {
    static String scannedData[3] = {"", "", ""};
    return scannedData;
  }
  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    return scannedData;
  }
  
  Serial.println(F("**Card Detected:**"));

  byte buffer[18];
  byte len = 18;
  for (byte block = 4; block <= 6; block++) {
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
    status = mfrc522.MIFARE_Read(block, buffer, &len);
    if (status != MFRC522::STATUS_OK) {
      return scannedData;
    }
  
    String value = "";
    for (uint8_t i = 0; i < 16; i++) {
      value += (char)buffer[i];
    }
    value.trim();
    
    // Assign values to variables based on block
    if (block == 4) {
      scannedData[0] = value; // id
      connect_mqtt();
      if (client.publish("ID", value.c_str())) {
        Serial.println("Data sent!");
      }
      else {
        Serial.println("Data failed to send. Reconnecting...");
        client.connect(clientID, mqtt_server, data_topic);
        delay(10); // This delay ensures that client.publish doesn’t clash with the client.connect call
        client.publish("ID", value.c_str());
      }

    } else if (block == 5) {
      scannedData[1] = value; // date
        connect_mqtt();
        if (client.publish("Date", value.c_str())) {
          Serial.println("Data sent!");
        }
        else {
          Serial.println("Data failed to send. Reconnecting...");
          client.connect(clientID, mqtt_server, data_topic);
          delay(10); // This delay ensures that client.publish doesn’t clash with the client.connect call
          client.publish("Date", value.c_str());
        }
      
    } else if (block == 6) {
      scannedData[2] = value; // productType
        connect_mqtt();
        if (client.publish("PRODUCT_STATION2", value.c_str())) {
          Serial.println("Data sent!");
        }
        else {
          Serial.println("Data failed to send. Reconnecting...");
          client.connect(clientID, mqtt_server, data_topic);
          delay(10); // This delay ensures that client.publish doesn’t clash with the client.connect call
          client.publish("PRODUCT_STATION2", value.c_str());
        }
      }
    
    Serial.print("Block ");
    Serial.print(block);
    Serial.print(": ");
    Serial.println(value);
  }
  String station_id = "1";
  publishMQTT(scannedData[0],scannedData[2]);
    connect_mqtt();
      if (client.publish("Station", station_id.c_str())) {
          Serial.println("Data sent!");
        }
        else {
          Serial.println("Data failed to send. Reconnecting...");
          client.connect(clientID, mqtt_server, data_topic);
          delay(10); // This delay ensures that client.publish doesn’t clash with the client.connect call
          client.publish("Station", station_id.c_str());
        }
  Serial.println("Station ID: 1");
  
  Serial.println(F("**End Reading**\n"));
  delay(1000);
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  return scannedData;
}

void loop() {
  String* scannedData = scan();
  String String_id = String(scannedData[0]); // Convert char to String
  String String_date = String(scannedData[1]); // Convert char to String
  String productType = String(scannedData[2]);
}

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
  while (! client.connect(clientID, mqtt_server, data_topic)) Serial.println("Connection to MQTT Broker failed…");
  Serial.println("Connected to MQTT Broker!");
}
