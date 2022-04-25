#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <ESP_Mail_Client.h>
#include <string>

const char* ssid     = "";
const char* password = "";

#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
#define AUTHOR_EMAIL "bnbsecure491@gmail.com"
#define AUTHOR_PASSWORD ""
String RECIPIENT_EMAIL = "";
String RECIPIENT = "";
SMTPSession smtp;
void smtpCallBack(SMTP_Status status);

//#define DEBUG   //If you comment this line, the DPRINT & DPRINTLN lines are defined as blank.
#ifdef DEBUG    //Macros are usually in all capital letters.
  #define DPRINT(...)    Serial.print(__VA_ARGS__)     //DPRINT is a macro, debug print
  #define DPRINTLN(...)  Serial.println(__VA_ARGS__)   //DPRINTLN is a macro, debug print with new line
#else
  #define DPRINT(...)     //now defines a blank line
  #define DPRINTLN(...)   //now defines a blank line
#endif

#define RX_PIN 21
#define TX_PIN 22
SoftwareSerial rfidModule(RX_PIN, TX_PIN); // RX to TX Pin, TX to RX Pin to the MEGA

void setup() {
  // put your setup code here, to run once:
  Serial.begin(4800);
  delay(10);
 
  // Start connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  // Waiting to connect
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // When connected to the WIFI
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  
}

const byte numChars = 20;
char receivedChars[numChars]; // an array to store the received data
boolean newData = false;

void loop() {
  // put your main code here, to run repeatedly:
  receiveWithEndMarker();
  processRFIDTag();
}

void receiveWithEndMarker() {
  static byte index =0;
  char endMarker = '\n';
  char rc;

  while (Serial.available() > 0 && newData == false) {
    rc = Serial.read();
    if (rc != endMarker) {
      receivedChars[index] = rc;
      index++;
      if (index >= numChars) {
        index = numChars - 1;
      }
     }else{
        receivedChars[index] = '\0'; // terminate the string
        index = 0;
        newData = true;
     }
  }    
}

void processRFIDTag() {
  if (newData == true) {
    DPRINT("This just in ... ");
    DPRINTLN(receivedChars);
    newData = false;
    isUserAuthorized(receivedChars);
    }
}

void isUserAuthorized(String RFIDTag){
    // wait for WiFi connection
  if ((WiFi.status() == WL_CONNECTED)) {
    WiFiClient client;
    HTTPClient http;
    // NOTE:  CHANGE THIS TO THE IP ADDRESS WHERE YOUR APPLICATION IS RUNNING
    String url = "http://192.168.1.209:8080/user/isauthorized?rfid_tag=";
    String encodedRFIDTAG = urlEncode(RFIDTag);
    url += encodedRFIDTAG;
    DPRINT("url: ");
    DPRINTLN(url);
    http.setTimeout(20000);
    if (http.begin(client, url)) {  // HTTP

      // start connection and send HTTP header
      int httpCode = http.GET();
      DPRINT("httpCode :: ");
      DPRINTLN(httpCode);

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled

        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = http.getString();
          DPRINTLN(payload);
          sendDataBackToArduino(payload);
          if(payload.indexOf("true") > 0) {
            sendEmail();
          }
        }
      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
    } else {
      Serial.printf("[HTTP} Unable to connect\n");
    }
  }
}

String urlEncode(String str) {
    String encodedString="";
    char c;
    char code0;
    char code1;
    char code2;
    for (int i =0; i < str.length(); i++){
      c = str.charAt(i);
      if (c == ' '){
        encodedString += '+';
      } else if (isalnum(c)){
        encodedString += c;
      } else {
        code1=(c & 0xf)+'0';
        if ((c & 0xf) >9){
            code1=(c & 0xf) - 10 + 'A';
        }
        c=(c>>4)&0xf;
        code0=c+'0';
        if (c > 9){
            code0=c - 10 + 'A';
        }
        code2='\0';
        encodedString+='%';
        encodedString+=code0;
        encodedString+=code1;
        //encodedString+=code2;
      }
      yield();
    }
    return encodedString;   
}

void sendDataBackToArduino(String payload){
  Serial.flush();
  RECIPIENT_EMAIL = "";
  RECIPIENT = "";
  const size_t capacity = JSON_OBJECT_SIZE(1) + 256;
  DynamicJsonDocument doc(capacity);
    // Parse JSON object
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }
  serializeJson(doc, Serial);
  bool is_authorized = doc["is_authorized"] == "true";
  String email = doc["email"];
  String user = doc["user"];
  if (is_authorized) {
    RECIPIENT_EMAIL = email;
    RECIPIENT = user;
  }
}

void smtpCallBack(SMTP_Status status) {
  if (status.success()) {
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failled: %d\n", status.failedCount());

    struct tm dt;
    for (size_t i = 0; i < smtp.sendingResult.size(); i++)
      {
        /* Get the result item */
        SMTP_Result result = smtp.sendingResult.getItem(i);
        time_t ts = (time_t)result.timestamp;
        localtime_r(&ts, &dt);
  
        ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
        ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
        ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
        ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
        ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
      }
    smtp.sendingResult.clear();
  }
}

void sendEmail() {
   /** Enable the debug via Serial port
   * 0 for no debugging
   * 1 for basic level debugging
   *
   * Debug port can be changed via ESP_MAIL_DEFAULT_DEBUG_PORT in ESP_Mail_FS.h
  */
  smtp.debug(0);
  /* Set the callback function to get the sending results */
  smtp.callback(smtpCallBack);
  
  /* Declare the session config data */
  ESP_Mail_Session session;

  /* Set the session config */
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;
  session.login.user_domain = F("mydomain.net");

  /* Set the NTP config time */
  session.time.ntp_server = F("pool.ntp.org,time.nist.gov");
  session.time.gmt_offset = 3;
  session.time.day_light_offset = 0;

  /* Declare the message class */
  SMTP_Message message;

  /* Set the message headers */
  message.sender.name = F("BnB SECURE");
  message.sender.email = AUTHOR_EMAIL;
  message.subject = F("ROOM ALERT");
  message.addRecipient(RECIPIENT, RECIPIENT_EMAIL);

  String textMsg = "YOUR ROOM IS OPENED. PLEASE CALL THE FRONT RECEPTIONIST IF IT IS NOT YOU!";
  message.text.content = textMsg;
  message.text.charSet = F("us-ascii");

  message.text.transfer_encoding = Content_Transfer_Encoding::enc_base64;

  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;

  message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

  /* Set the custom message header */
  message.addHeader("Message-ID: <abcde.fghij@gmail.com>");
  
  /* Connect to server with the session config */
  if (!smtp.connect(&session))
    return;

  /* Start sending Email and close the session */
  if (!MailClient.sendMail(&smtp, &message))
    Serial.println("Error sending Email, " + smtp.errorReason());

  //to clear sending result log
  //smtp.sendingResult.clear();

  ESP_MAIL_PRINTF("Free Heap: %d\n", MailClient.getFreeHeap());
}
