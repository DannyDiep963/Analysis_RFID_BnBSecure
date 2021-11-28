/***** Still in Working Progress *****/
#include <SPI.h>
#include <MFRC522.h>

//Hardware Pin
#define SS_PIN 53 //Slave Select pin
#define RST_PIN 49  //Reset Pin
#define LED_G 24  //Green LED
#define LED_R 25  // Red LED

//Instantiate MFRC522 object Class
  MFRC522 rfidReader(SS_PIN, RST_PIN); // Instance of the class

//Global Constants
  const long timeout = 30000;

/* ***********************************************************
 *                      Global Variables                     *
 * ********************************************************* */
 char* myTags[100] = {};
 int tagsCount = 0;
 String tagID = "";
 
bool readRFID(long _timeout=timeout, bool useTimeout=false){
    /*  readRFID will continuously check the RFID reader for the presence of
     *    a tag from and will attempt to get tag IDs. Updates global value
     *    tagID via getTagID function.
     *  Parameters:
     *    _timeout   - [optional] the length of time before functio gives up
     *                 default value = global timout value
     *    useTimeout - [optional] boolean to enforce timout period or wait 
     *                 indefinately.  Default value = false.
     *  Returns:
     *    true  -  successfully reads tag ID
     *    false -  unsuccessful in reading the tag ID          
     */
    bool successRead = false;

    unsigned long startTime = millis();
    unsigned long currentTime = startTime;
    // S'U'+S'T'
    // T  = (currentTime-startTime) > timeout
    // T' = (currentTime-startTime) < timeout
    while (((successRead==false)&&(useTimeout==false)) || ((successRead==false)&&((currentTime - startTime) < _timeout))) {    
        if (isTagPresent() == true){ 
          successRead = getTagID(); 
          }
        currentTime = millis();
    }
    return successRead;
}

/* ***********************************************************
 *                         Void Setup                        *
 * ********************************************************* */
void setup() {
    // Initiating
    Serial.begin(9600);                     // Start the serial monitor
    SPI.begin();                            // Start SPI bus
    rfidReader.PCD_Init();                  // Start MFRC522 object
    
    //LED start up sequence
    pinMode(LED_G, OUTPUT);
    pinMode(LED_R, OUTPUT);
    digitalWrite(LED_R, HIGH);
    delay(200);
    digitalWrite(LED_R, LOW);
    delay(200);
    digitalWrite(LED_G, HIGH);
    delay(200);
    digitalWrite(LED_G, LOW);
    delay(200);
    
    //Print Firmware Version
    rfidReader.PCD_DumpVersionToSerial();
    Serial.println(F("Scann PICC to see UID, SAK, type, and data blocks..."));
    
    while (!Serial);                        // Do nothing if no serial port is opened
    
    // Obviously this is an over simplified sketch
    // Master tags would be save in flash storage and
    // retrieved here.  OR a special PIN entered to set
    // Master Tag.
    // But for the sake of simplicity, the sketch will 
    // obtain a new master tag when restarted.
    
    // Prints the initial message
    Serial.println(F("-No Master Tag!-"));
    Serial.println(F("    SCAN NOW"));

    // readRFID will wait until a master card is scanned
    if (readRFID() == true) {
        myTags[tagsCount] = strdup(tagID.c_str()); // Sets the master tag into position 0 in the array
        Serial.println(F("Master Tag is Set!"));
        tagsCount++;
    }

    printNormalModeMessage();
    
}


/* ***********************************************************
 *                         Void Loop                         *
 * ********************************************************* */
void loop() {
    if (isTagPresent()==true){
        getTagID();
        checkTagID();
    } else {
        delay(50);
        //return;
    }
}    

/* ***********************************************************
 *                         Functions                         *
 * ********************************************************* */
bool isTagPresent() {
  /*  isTagPresent uses the MFRC522 methods to determine if 
   *    a tag is present or the read card serial is enabled.
   *  Parameters: (none)
   *  Returns:
   *    true  - if tag detected or read card serial is true
   *    false - no tag detected or no read card serial true
   */
   //Not a new PICC_IsNewCardPresent in RFID reader
   //Or
   //Not a PICC_ReadCardSerial active in Serial
   if (!rfidReader.PICC_IsNewCardPresent() || !rfidReader.PICC_ReadCardSerial()) {
    return false;
   }
   return true;
}

byte checkMyTags(String tagID) {
  /* checkMyTags function loops through the array of myTags
   *   Parameters:
   *     tagID    - a string to look for
   *   Returns:
   *     tagIndex - index in the array of myTags
   *                default 0     
   */
   byte tagIndex = 0;
   Serial.println("Checking Tag Started");
   //Zerp is reserved for master tag
   for(int i = 1; i < 100 ; i++){
    if(tagID == myTags[i]){
      tagIndex = i;
    }
   }
   Serial.println("Checking Tag Ended");
   return tagIndex;
}

void checkTagID() {
  /* checkTagID check the tag ID for authorized tag ID values
   *   if Master tag found switch to program mode
   * Parameters: (none)
   * Returns: (none)
   */
   // Checks for Master tag
   if(tagID == myTags[0]) {
    //Switch to program mode
    //Serial.println(F("Program Mode: "));
    //Serial.println(F("Add/Remove Tag"));
    //Check for authorized tag
     byte tagIndex = checkMyTags(tagID);
     if (tagIndex ==0){
      Serial.println(F("Access Granted!"));
      digitalWrite(LED_G,HIGH);
      delay(300);
      digitalWrite(LED_G,LOW);
      delay(300);
      digitalWrite(LED_G,HIGH);
      delay(300);
      digitalWrite(LED_G,LOW);
      delay(300);
   } else{
      byte tagIndex = checkMyTags(tagID);
      Serial.println(F("Access Denied!"));
      digitalWrite(LED_R,HIGH);
      delay(300);
      digitalWrite(LED_R,LOW);
      delay(300);
      digitalWrite(LED_R,HIGH);
      delay(300);
      digitalWrite(LED_R,LOW);
      delay(300);
      
      Serial.println(F("New UID & Contents"));
      rfidReader.PICC_DumpToSerial(&(rfidReader.uid));
     }
    }
   printNormalModeMessage();
}

bool getTagID() {
  /*  getTagID retrieves the tag ID.  Modifies global variable tagID
   *           
   *    Parameters: (none)    
   *    Returns: true
   */
   tagID = "";

   Serial.print(F(" UID Tag: "));
   for (byte i = 0; i < rfidReader.uid.size; i++){
    // The MIFARE PICCs that we use have 4 byte UID
    Serial.print(rfidReader.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(rfidReader.uid.uidByte[i], HEX);

    //Adds the bytes in a single String variable 
    tagID.concat(String(rfidReader.uid.uidByte[i] < 0x10 ? " 0" : " "));
    tagID.concat(String(rfidReader.uid.uidByte[i], HEX));
   }
   Serial.println();
   Serial.println();
   tagID.toUpperCase();
   rfidReader.PICC_HaltA(); //Stop Reading
   return true;
}

void printNormalModeMessage() {
    /*  printNormalModeMessage sends the standard greeting
     *    to the serial monitor.
     *  Parameters: (none)
     *  Returns: (none)
     */
    delay(1500);
    Serial.println();
    Serial.println(F("-Access Control-"));
    Serial.println(F(" Scan Your Tag!"));
}
