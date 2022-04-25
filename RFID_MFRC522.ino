// include the library code:
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>
#include <SoftwareSerial.h>
#include <StreamUtils.h>
#include <ArduinoJson.h>

//Hardware Pin
#define SS_PIN 53 //Slave Select pin
#define RST_PIN 49  //Reset Pin
int LED_G = 7;  //Green LED
int LED_R = 6;  // Red LED
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

//Instantiate MFRC522 object Class
MFRC522 rfidReader(SS_PIN, RST_PIN); // Instance of the class

//Instantiate ESP8266 object Class
//SoftwareSerial wifiModule(0, 23); // 2 to TX Pin, 3 to RX Pin to the esp8266

bool isRead = false;
bool isNewCard = false;
String tagContent = "";
String currentUID = "";
String email = "";

// Interval before we process the same RFID
int INTERVAL = 2000;
unsigned long previousTime = 0;
unsigned long startTime = 0;

void setup() {
  // Initiating:
  Serial.begin(9600);
  //wifiModule.begin(4800);
  Serial1.begin(4800);
  //wifiModule.setTimeout(5000);
  SPI.begin();
  rfidReader.PCD_Init(); 
  
  lcd.begin(16, 2);
  //LCD start up sequence
  lcd.setCursor(3,0);
  lcd.print("Welcome To ");
  lcd.setCursor(3,1);
  lcd.print("BnB Secure");
  delay(3000);
  //Print Firmware Version
  rfidReader.PCD_DumpVersionToSerial();
  delay(1500);
  printWaitingModeMessage();
}

void loop() {
  // put your main code here, to run repeatedly:
  // Look for new tag
  if (rfidReader.PICC_IsNewCardPresent()) {
    // Select tag
    if(rfidReader.PICC_ReadCardSerial()) {
      isRead = true;
      byte letter;
      for (byte i = 0; i < rfidReader.uid.size; i++){
        Serial.print(rfidReader.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(rfidReader.uid.uidByte[i], HEX);

        tagContent.concat(String(rfidReader.uid.uidByte[i] < 0x10 ? " 0" : " "));
        tagContent.concat(String(rfidReader.uid.uidByte[i], HEX));
      }
      Serial.println();
      tagContent.toUpperCase();
    }
    if(isRead) {
      startTime = millis();
      // If new card is present (different from the previous card), then validate if it has access
      if(currentUID != tagContent) {
        currentUID = tagContent;
        isNewCard = true;
      } else {  // If it is the same RFID UID then we wait for interval to pass before we process the same RFID
        if(startTime - previousTime >= INTERVAL) {
          isNewCard = true;
        } else {
          isNewCard = false;
        } 
      } 
      if(isNewCard) {
        if(tagContent != "") {
          previousTime = startTime;
          //Send the RFID UID code to the ESP8266 for validation
          Serial.print("Sending data to module: ");
          //Serial.println(tagContent);
          //wifiModule.println(tagContent);
          Serial1.println(tagContent);
          // Checking wifi module
          Serial.println("Waiting for response from wifi module...");
          
          int iCtr = 0;
          
          while(!Serial1.available()) {
            iCtr++;
            if(iCtr >= 100)
              break;
            delay(100); 
            }
            
          if(Serial1.available()>0) {
            Serial.println("Wifi module is available");
            bool isAuthorized = isUserAuthorized(tagContent);
            while(Serial1.available()>0){
              Serial1.read();
            }
            //if user is OR not authorized display the message
            if(!isAuthorized) {
              printNotAuthorized();
              delay(1500);
              printWaitingModeMessage();
              
            } else {
              printAuthorized();
              delay(1500);              
              printWaitingModeMessage();
            }
          }
            Serial.println("Finished processing response from Wifi Module");
          }
        }
    } else {
      Serial.println("No card is presented");
    }
    tagContent = "";
    isNewCard = false;
  }
}

bool isUserAuthorized(String tagContent) {
  //Process if message is ready
  const size_t capacity = JSON_OBJECT_SIZE(1) + 256;
  DynamicJsonDocument doc(capacity);
  ReadLoggingStream loggingStream(Serial1, Serial);
  DeserializationError error = deserializeJson(doc, loggingStream);
  //DeserializationError error = deserializeJson(doc, Serial1);
  if(error) {
    Serial.print("DeserializeJson() failed: ");
    Serial.println(error.c_str());
    return error.c_str();
  }
  String auth = doc["is_authorized"];
  Serial.println(auth);
  bool is_authorized = doc["is_authorized"] == "true";
  email = doc["email"].as<String>();
  Serial.print("is_authorized: ");
  Serial.println(is_authorized);
  Serial.println(email);
  delay(5000);
  return is_authorized;
}

void printWaitingModeMessage() {
  Serial.println();
  Serial.println("-Access Control-");
  Serial.println("Scan tag to see UID");
    
  //LCD Display
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("-Access Control-");
  lcd.setCursor(0,1);
  lcd.print("Scan Your Tag!");
}

void printAuthorized() {
  Serial.println("Valid Tag - Access Granted");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Valid Tag");
  lcd.setCursor(0,1);
  lcd.print("Access Granted");
}

void printNotAuthorized() {
  Serial.println("Invalid Tag - Access Denied");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Invalid Tag");
  lcd.setCursor(0,1);
  lcd.print("Access Denied");
}
