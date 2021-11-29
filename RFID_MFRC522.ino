/***** Still in Working Progress *****/
// include the library code:
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>

//Hardware Pin
#define SS_PIN 53 //Slave Select pin
#define RST_PIN 49  //Reset Pin
int LED_G = 7;  //Green LED
int LED_R = 6;  // Red LED
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

//Instantiate MFRC522 object Class
MFRC522 rfidReader(SS_PIN, RST_PIN); // Instance of the class

//Global Constants
const long timeout = 30000;

/* ***********************************************************
 *                      Global Variables                     *
 * ********************************************************* */
 //char* myTags[10] = {};
 //int tagsCount = 0;
 String myMasterTag = "";
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
    lcd.begin(16, 2);
    //LCD start up sequence
    lcd.setCursor(3,0);
    lcd.print("Welcome To ");
    lcd.setCursor(3,1);
    lcd.print("BnB Secure");
    delay(3000);
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
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("-No Master Tag!-");
    lcd.setCursor(3,1);
    lcd.print("SCAN NOW");
    
    // readRFID will wait until a master card is scanned
    if (readRFID() == true) {
        //myTags[tagsCount] = strdup(tagID.c_str()); // Sets the master tag into position 0 in the array
        myMasterTag = strdup(tagID.c_str());
        Serial.println(F("Master Tag Set!"));
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Master Tag Set!");
        //tagsCount++;
    }

    printNormalModeMessage();
    
}


/* ***********************************************************
 *                         Void Loop                         *
 * ********************************************************* */
void loop() {

  /*
  Serial.println("Red Led on");
  digitalWrite(LED_R, HIGH);
  delay(300);
  Serial.println("Red Led off");
  digitalWrite(LED_R, LOW);
  delay(300);
 */
  if (isTagPresent()==true){
    Serial.println(myMasterTag);
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
   //Zero is reserved for master tag
   for(int i = 1; i < 10 ; i++){
    //if(tagID == myTags[i]){
    if(tagID == myMasterTag) {
      tagIndex = i;
    }
   }
   //Serial.println("Checking Tag Ended");
   return tagIndex;
}

void checkTagID() {
  /* checkTagID check the tag ID for authorized tag ID values
   *   if Master tag found switch to program mode
   * Parameters: (none)
   * Returns: (none)
   */
   // Checks for Master tag
   //if(tagID == myTags[0]) {
   if(tagID == myMasterTag) {
    //Switch to program mode
    //Serial.println(F("Program Mode: "));
    //Serial.println(F("Add/Remove Tag"));
    //Check for authorized tag
    Serial.println(F("Access Granted!"));
    
    //LCD Display
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("UID:" + tagID);
    lcd.setCursor(0,1);
    lcd.print("Access Granted!");
    }else{
    Serial.println(F("Access Denied!"));
    
    //LCD Display
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("UID:" + tagID);
    lcd.setCursor(0,1);
    lcd.print("Access Denied!");

    Serial.println(F("New UID & Contents"));
    rfidReader.PICC_DumpToSerial(&(rfidReader.uid));
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
    
    //Adds the bytes in a single String variable (tagID)
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
    Serial.println(F("Scan Your Tag!"));
    
    //LCD Display
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("-Access Control-");
    lcd.setCursor(0,1);
    lcd.print("Scan Your Tag!");
}
