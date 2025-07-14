#include <time.h>
#include <TM1637Display.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <ArduinoJson.h>
#define NTP_OFFSET  0      // In seconds
#define NTP_INTERVAL  25200    // In miliseconds
#define NTP_ADDRESS  "pool.ntp.org"


int a = 0; // 1: Bolt
int b = 0; // 2: Nut silver
int c = 0; // 3: Nut black
int d = 0; // 4: Washer silver
int e = 0; // 5: Washer black
int a_tot = 100;
int b_tot = 100;
int c_tot = 100;
int d_tot = 100;
int e_tot = 100;
#define RST_PIN         2           // Configurable, see typical pin layout above
#define SS_PIN          5           // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;

// Wi-Fi
const char *ssid = "AndroidAPCA5E";
const char *password = "Apahayo.";
WiFiClient wifiClient;

// MQTT
const char *mqtt_server = "192.168.132.190";
const char *data_topic = "rfid";
const char *clientID = "RFID_Station_1";
PubSubClient client(mqtt_server, 1889, wifiClient);

String id;
String date;
String productType;

// Define pin configurations for the first display
#define CLK1  13 // The ESP32 pin GPIO22 connected to CLK 26
#define DIO1  12 // The ESP32 pin GPIO23 connected to DIO 25

// Define pin configurations for the second display
#define CLK2  14 // Example: The ESP32 pin GPIO5 connected to CLK for the second display 26
#define DIO2  27 // Example: The ESP32 pin GPIO6 connected to DIO for the second display 25

#define CLK3  26 // Example: The ESP32 pin GPIO5 connected to CLK for the second display 14
#define DIO3  25 // Example: The ESP32 pin GPIO6 connected to DIO for the second display 27

#define CLK4  33 // Example: The ESP32 pin GPIO5 connected to CLK for the second display
#define DIO4  32 // Example: The ESP32 pin GPIO6 connected to DIO for the second display

#define CLK5  22 // Example: The ESP32 pin GPIO5 connected to CLK for the second display
#define DIO5  21 // Example: The ESP32 pin GPIO6 connected to DIO for the second display
// Create display objects for both TM1637 modules
TM1637Display display1 = TM1637Display(CLK1, DIO1);
TM1637Display display2 = TM1637Display(CLK2, DIO2);
TM1637Display display3 = TM1637Display(CLK3, DIO3);
TM1637Display display4 = TM1637Display(CLK4, DIO4);
TM1637Display display5 = TM1637Display(CLK5, DIO5);

int array[] = { };

String removeNullChars(String str) {
  String cleanedStr = "";
  for (int i = 0; i < str.length(); i++) {
    if (str[i] != '\0') {
      cleanedStr += str[i];
    }
  }
  return cleanedStr;
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
  doc["progress"] = "Warehouse Station";
  doc["percentage"] = "40%";
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

void setup() {
  Serial.begin(9600);  
  setup_wifi();                                         // Initialize serial communications with the PC
  SPI.begin();                                                  // Init SPI bus
  mfrc522.PCD_Init();                                           // Init MFRC522 card
  // Clear displays
  display1.clear();
  display2.clear();
  display3.clear();
  display4.clear();
  display5.clear();

  // Set brightness for both displays
  display1.setBrightness(7); // set the brightness to 7 (0:dimmest, 7:brightest)
  display2.setBrightness(7);
  display3.setBrightness(7);
  display4.setBrightness(7);
  display5.setBrightness(7);
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
      if (client.publish("warehouse_id", value.c_str())) {
        Serial.println("Data sent!");
      }
      else {
        Serial.println("Data failed to send. Reconnecting...");
        client.connect(clientID, mqtt_server, data_topic);
        delay(10); // This delay ensures that client.publish doesn’t clash with the client.connect call
        client.publish("warehouse_id", value.c_str());
      }

    } else if (block == 5) {
      scannedData[1] = value; // date
        connect_mqtt();
        if (client.publish("warehouse_date", value.c_str())) {
          Serial.println("Data sent!");
        }
        else {
          Serial.println("Data failed to send. Reconnecting...");
          client.connect(clientID, mqtt_server, data_topic);
          delay(10); // This delay ensures that client.publish doesn’t clash with the client.connect call
          client.publish("warehouse_date", value.c_str());
        }
      
    } else if (block == 6) {
      scannedData[2] = value; // productType
        connect_mqtt();
        if (client.publish("warehouse_product", value.c_str())) {
          Serial.println("Data sent!");
        }
        else {
          Serial.println("Data failed to send. Reconnecting...");
          client.connect(clientID, mqtt_server, data_topic);
          delay(10); // This delay ensures that client.publish doesn’t clash with the client.connect call
          client.publish("warehouse_product", value.c_str());
        }
      }
    
    Serial.print("Block ");
    Serial.print(block);
    Serial.print(": ");
    Serial.println(value);

    publishMQTT(scannedData[0], scannedData[2]);
  }
  String station_id = "1";
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
  if (productType == "X") {
      a = 1;
      a_tot -= a;
      b = 2;
      b_tot -= b;
      c = 3;
      c_tot -= c;
      d = 2;
      d_tot -= d;
      e = 1;
      e_tot -= e;
    Serial.println('x');
    send_inventory_data();
  }
  else if (productType == "Y") {
      a = 1;
      a_tot -= a;
      b = 2;
      b_tot -= b;
      c = 3;
      c_tot -= c;
      d = 2;
      d_tot -= d;
      e = 1;
      e_tot -= e;
    Serial.println('y');
    send_inventory_data();
  }
    else if (productType == "Z") {
      a = 1;
      a_tot -= a;
      b = 5;
      b_tot -= b;
      c = 0;
      c_tot -= c;
      d = 3;
      d_tot -= d;
      e = 0;
      e_tot -= e;
    Serial.println('z');
    send_inventory_data();
    
  }


  // int numberToShow1 = a;
  // int numberToShow2 = b;
  // int numberToShow3 = c;
  // int numberToShow4 = d;
  // int numberToShow5 = e;

  display1.showNumberDec(a);
  display2.showNumberDec(b);
  display3.showNumberDec(c);
  display4.showNumberDec(d);
  display5.showNumberDec(e);

  delay(4000);
  display1.clear();
  display2.clear();
  display3.clear();
  display4.clear();
  display5.clear();

  display1.showNumberDec(a_tot);
  display2.showNumberDec(b_tot);
  display3.showNumberDec(c_tot);
  display4.showNumberDec(d_tot);
  display5.showNumberDec(e_tot);

  delay(4000);
  display1.clear();
  display2.clear();
  display3.clear();
  display4.clear();
  display5.clear();
}

void send_inventory_data() {
  int size = sizeof(int)*8+1;
  char temp[size];
  client.publish("Bolt", String(a_tot).c_str());
  Serial.print("Bolt");
  Serial.println(String(a_tot).c_str());
  
  client.publish("Nut silver", String(b_tot).c_str());
  Serial.print("Nut silver");
  Serial.println(String(b_tot).c_str());
  
  client.publish("Nut black", String(c_tot).c_str());
  Serial.print("Nut black");
  Serial.println(String(c_tot).c_str());
  
  client.publish("Washer silver", String(d_tot).c_str());
  Serial.print("Washer silver");
  Serial.println(String(d_tot).c_str());
  
  client.publish("Washer black", String(e_tot).c_str());
  Serial.print("Washer black");
  Serial.println(String(e_tot).c_str());
  
  Serial.println("sent_run");

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
