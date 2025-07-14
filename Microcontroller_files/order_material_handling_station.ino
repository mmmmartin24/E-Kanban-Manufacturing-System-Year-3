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
#define NTP_ADDRESS  "pool.ntp.org"  // change this to whatever pool is closest (see ntp.org)

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;
bool newData = false;

byte data_id[50];
byte data_date[50];
byte data_product[50];

unsigned long lastMillis = 0;
unsigned long interval = 1000;

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

String id;
String date;
String productType;

int blockNum = 0;  

int array[] = { };

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

void callback(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, "scan_id") == 0){
    for (int i = 0; i < length; i++) {
      data_id[i] = payload[i];
    }
    data_id[length] = '\0'; // Null-terminate the received data
    Serial.println((char*)data_id);
  }
  if (strcmp(topic, "scan_date") == 0){
    for (int i = 0; i < length; i++) {
      data_date[i] = payload[i];
    }
    data_date[length] = '\0'; // Null-terminate the received data  
    Serial.println((char*)data_date);
  }
  if (strcmp(topic, "scan_product") == 0){
    for (int i = 0; i < length; i++) {
      data_product[i] = payload[i];
    }
    data_product[length] = '\0'; // Null-terminate the received data
    Serial.println((char*)data_product);
  }

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  publishMQTTjson((char*)data_id,(char*)data_product);
  client.publish("rfid_status","1");
  newData = true;
}

void setup() {
  Serial.begin(9600);  
  setup_wifi();    
  client.setCallback(callback);                                     // Initialize serial communications with the PC
  SPI.begin();                                                   // Init SPI bus
  mfrc522.PCD_Init();  
}

void connect_mqtt() {
  while (! client.connect(clientID)) Serial.println("Connection to MQTT Broker failed…");
  Serial.println("Connected to MQTT Broker!");
}

void WriteDataToBlock(int blockNum, byte blockData[]) {
  /* Authenticating the desired data block for write access using Key A */
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print("Authentication failed for Write: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  else
  {
    Serial.println("Authentication success");
  }

  
  /* Write data to the block */
  status = mfrc522.MIFARE_Write(blockNum, blockData, 16);
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print("Writing to Block failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  else
  {
    Serial.println("Data was written into Block successfully");
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
  delay(500);  
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
      value = (char*)data_id;
      Serial.print("\n");
      Serial.println("Writing to Data Block...");
      byte blockData[16];
      memcpy(blockData, data_id, sizeof(blockData)); // Copy 'data' to 'blockData'
      int blockNum = 4;  
      WriteDataToBlock(blockNum, blockData);
      Serial.println("ID Write Successful");
  }

    if (block == 5) {
      value = (char*)data_date;
      // connect_mqtt();

      Serial.print("\n");
      Serial.println("Writing to Data Block...");
      byte blockData[16];
      memcpy(blockData, data_date, sizeof(blockData)); // Copy 'data' to 'blockData'
      int blockNum = 5;  
      WriteDataToBlock(blockNum, blockData);
      Serial.println("ID Write Successful");
  }
      
    if (block == 6) {
      value = (char*)data_product;
      // connect_mqtt();
      Serial.print("\n");
      Serial.println("Writing to Data Block...");
      byte blockData[16];
      memcpy(blockData, data_product, sizeof(blockData)); // Copy 'data' to 'blockData'
      int blockNum = 6;  
      WriteDataToBlock(blockNum, blockData);
      Serial.println("ID Write Successful");
  }
    
    Serial.print("Block ");
    Serial.print(block);
    Serial.print(": ");
    Serial.println(value);

  }


  publishMQTT((char*)data_id,(char*)data_product);

  
  Serial.println(F("**End Reading**\n"));
  delay(1000);
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  return scannedData;
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
  doc["progress"] = "Material Handling Station";
  doc["percentage"] = "10%";
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

void publishMQTTjson(String update_id_string, String id_prog_string) {
  Serial.println("masukjson");
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
  doc["id_order"] = cleanedUpdateId;
  doc["product_type_order"] = cleanedIdProg;
  doc["timestamp"] = timestamp;

  // Convert the JSON document to a string
  String payload;
  serializeJson(doc, payload);  

  // Connect to MQTT and publish the message
  connect_mqtt();
  if (client.publish("order_json", payload.c_str())) {
    Serial.println("Data sent!");
  } else {
    Serial.println("Data failed to send. Reconnecting...");
    client.connect(clientID, mqtt_server, data_topic);
    delay(10); // This delay ensures that client.publish doesn’t clash with the client.connect call
    client.publish("order_json", payload.c_str());
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    }
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
    client.subscribe("scan_product");    
    client.subscribe("scan_date");    
    client.subscribe("scan_id");
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  // Check if interval has passed since last call to client.loop()
  if (millis() - lastMillis > interval) {
    lastMillis = millis(); // Update lastMillis
    client.loop();
  }

  String* scannedData = scan();

}


