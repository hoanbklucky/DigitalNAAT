#include "serial.h"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
//uint32_t value = 0;
//bool rqsNotify;
double set_temp = 0;
double hold_time = 0;
boolean assay_start = false;
boolean assay_done = false;

String code_version = "heater-module.ino - 20210710";

boolean serialOpen = false;

String timestamp;
int time_int = 1000; // number of milliseconds between print statements
double init_time = millis(); // holds time at start of assay
double args[16];

static boolean recvInProgress = false;
char rc;
String message;
String messagerx;
String reply;
static byte ndx = 0;
char startMarker = '<';
char endMarker = '>';
boolean serial_read = false;

const int numChars = 64;
char serialCmd[numChars];
char cmdBuffer[numChars]; // temp holder in parse_input()
char tempChars[numChars]; // temp holder in parse_split()

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      //rqsNotify = false;
      Serial.println("Device connected");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      //rqsNotify = false;
      Serial.println("Device disconnected");
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      if (value.length() > 0) {
        message = "";       
        for (int i = 0; i < value.length(); i++){
          message = message + value[i];
        }
       Serial.println(message);       
       //writeBLE(message);
       delay(10);
       
       if (message.substring(0, 2) == "t(") {
          messagerx = "received";
          pCharacteristic->setValue(messagerx.c_str()); // Return status
          pCharacteristic->notify(); 
          parseArgs(args, message);
          set_temp = args[0];
          hold_time = args[1];
          assay_start = true;
          //pCharacteristic->setValue("hello"); // Return status
          //pCharacteristic->notify();
          //setTemp(args[0], args[1]);
          //imagingevery = args[2];
       }

       if (message == "on12") {digitalWrite(LED12,HIGH); pCharacteristic->setValue("LED12 ON...");}
       if (message == "off12"){digitalWrite(LED12,LOW);  pCharacteristic->setValue("LED12 OFF...");}
       if (message == "check"){
         reply ="";
         if (digitalRead(LED12) == HIGH) {reply = "LED12 ON,";} else {reply = "LED12 OFF,";}
         pCharacteristic->setValue(reply.c_str()); // Return status
         pCharacteristic->notify();
       }
      }
    }
};

void setupBLE() {
//  pinMode(LED12, OUTPUT);
//  pinMode(LED14, OUTPUT);
//  pinMode(LED27, OUTPUT);
//  Serial.begin(115200);

  BLEDevice::init("MyESP32");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE|
                                         BLECharacteristic::PROPERTY_NOTIFY |
                                         BLECharacteristic::PROPERTY_INDICATE
                                       );
  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());
  
  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->setValue("Iniciado.");
  
  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
}

void writeBLE(String message) {
    pCharacteristic->setValue(message.c_str()); // Return status
    pCharacteristic->notify(); 
}

void setupSerial(){

  Serial.begin(BAUD); // for Arduino troubleshooting
  delay(500);
  // Tell Pi serial connection is successful
  Serial.println("<NANO-TEMP-READY>");
  
}
//
// Read input from Serial, output in String
void input(boolean interpretFlag) {

  while (Serial.available() > 0 && !serial_read) {
    rc = Serial.read();
    //Serial.println(rc);
    if (recvInProgress == true) {    // && rc != NULL) {
      if (rc != endMarker) {
        //message += rc;
        serialCmd[ndx] = rc;
        ndx++;
        if (ndx >= numChars) {
          ndx = numChars - 1;
        }
      }
      else {
        serialCmd[ndx] = '\0'; // terminate the string
        ndx = 0;
        recvInProgress = false;
        serial_read = true;
      }
    }
    else if (rc == startMarker) {
      recvInProgress = true;
      message = "";
    }
  }

  if (serial_read) {
    message = serialCmd;
    if (message == "reset") {
      setup();
    }
    else if (interpretFlag) {
      cmd_interpret();
    }
    serial_read = false;
  }
}

// Command line interpreter: parses strings into commands
void cmd_interpret() {
  //message.toLowerCase();
  Serial.println(message);

  char msg_char[22];  // character array for PROGMEM comparisons to save dynamic memory
  if (message.length() < 22) {
    message.toCharArray(msg_char, 22);
  }
  else {
    String msg = "null";
    msg.toCharArray(msg_char, 22);
  }
  // Version check
  if (message == "version") {
    Serial.println(code_version);
  }

  // RESET command
  else if (message == "reset") {
    Serial.end();
    delay(100);
    setup();
  }

  // FAN control
  else if (message == "fan on") {
//    digitalWrite(fanPin, HIGH);
  }
  else if (message == "fan off") {
//    digitalWrite(fanPin, LOW);
  }

  // Heat block thermistor reading
  else if (message == "temp") {
    double temp = readTemp();

    Serial.print("Temperature: " + String(temp));
    Serial.print('\n');
  }

  else if (message == "cycle") {
    cycle();
  }

  // For interpreting commands with arguments
    
  else if (message.substring(0, 2) == "t(") {
    parseArgs(args, message);
    setTemp(args[0], args[1]);
    //analogWrite(OUT1Pin, 0);
    //analogWrite(OUT2Pin, 0);
    //analogWrite(HeaterPin, 0);
    ledcWrite(PWMChannel, 0);
  }

   else if (message.substring(0, 4) == "rte(") {
    parseArgs(args, message);
    rt_temp = args[0];
  }
  else if (message.substring(0, 4) == "rti(") {
    parseArgs(args, message);
    rt_time = args[0];
  }
  else if (message.substring(0, 4) == "hte(") {
    parseArgs(args, message);
    hs_temp = args[0];
  }
  else if (message.substring(0, 4) == "hti(") {
    parseArgs(args, message);
    hs_time = args[0];
  }
  else if (message.substring(0, 4) == "ate(") {
    parseArgs(args, message);
    an_temp = args[0];
  }
  else if (message.substring(0, 4) == "ati(") {
    parseArgs(args, message);
    an_time = args[0];
  }
  else if (message.substring(0, 4) == "dte(") {
    parseArgs(args, message);
    de_temp = args[0];
  }
  else if (message.substring(0, 4) == "dti(") {
    parseArgs(args, message);
    de_time = args[0];
  }
  else if (message.substring(0, 4) == "cnu(") {
    parseArgs(args, message);
    cycle_num = args[0];
  }
  
  // Fluorescence Channels
  else if (message.substring(0,4) == "cy5(") {
    parseArgs(args, message);
    if(args[0] == 0){
      CY5_CHANNEL = false;
    }
    else if(args[0] ==1){
      CY5_CHANNEL = true;
    }
  }
  else if (message.substring(0,4) == "fam(") {
    parseArgs(args, message);
    if(args[0] == 0){
      FAM_CHANNEL = false;
    }
    else if(args[0] ==1){
      FAM_CHANNEL = true;
    }
  }

  else if (message.substring(0, 4) == "blp(") {
    // change blue LED power (lower # == higher power)
    parseArgs(args, message);
    if(args[0] >=0 && args[0] <=255){
      BLED_PWM = args[0];
      if( LED_STATE == 1){
//        analogWrite(BLEDPin, BLED_PWM);
      }
    }
  }
  else if (message.substring(0, 4) == "rlp(") {
    // change red LED power (lower # == higher power)
    parseArgs(args, message);
    if(args[0] >=0 && args[0] <=255){
      RLED_PWM = args[0];
      if( LED_STATE == 2){
//        analogWrite(RLEDPin, RLED_PWM);
      }
    }
  }
  else if (message.substring(0, 5) == "LEDon") {
    parseArgs(args, message);
    if(int(args[0]) == 1){
//      analogWrite(BLEDPin, BLED_PWM);
//      analogWrite(RLEDPin, 255);
      LED_STATE == 1;
    }
    else if(int(args[0]) == 2){
//      analogWrite(BLEDPin, 255);
//      analogWrite(RLEDPin, RLED_PWM);
      LED_STATE == 2;
    }
  }
  else if (message.substring(0, 6) == "LEDoff") {
//    analogWrite(BLEDPin, 255);
//    analogWrite(RLEDPin, 255);
    LED_STATE == 0;
  }
  
  else if (message == "fan off") {
//    digitalWrite(fanPin, LOW);
  }

  else {
    Serial.print(message);
    Serial.println(F(" :Cannot recognize command."));
  }
  
}

// Argument parsing code: retrieves an int array containing arguments separated by commas
void parseArgs(double args[], String message) {
  String arg = "";
  char msg;
  int k = 0;
  for (int i = 0; i < message.length(); i++) {
    msg = message.charAt(i);
    arg += msg;
    if (msg == ')' || msg == ',' || msg == ';' || i == message.length() - 1) {
      args[k] = strToNum(arg); // convert argument into number and add to array
      arg = "";
      k++;
    }
  }
}

// String argument parsing code: retrieves a string array containing arguments separated by commas
void parseMsg(String args[], String message) {
  String arg = "";
  char msg;
  int k = 0;
  for (int i = 0; i < message.length(); i++) {
    msg = message.charAt(i);
    // if lowercase alphabet/underscore or number, add to argument
    if ((int(msg) <= 122 && int(msg) >= 95) || (int(msg) <= 57 && int(msg) >= 48)) {
      arg += msg;
    }
    if (msg == ')' || msg == ',' || msg == ';' || i == message.length() - 1) {
      args[k] = arg; // add argument to array
      k++;
      arg = "";
    }
  }
}

// converts string to number (handles below decimal points i.e. better than toInt())
double strToNum(String str) {
  double decValue = 0;
  int belowDec = 0;
  int nextInt;
  double mod = 1;

  for (int i = 0; i < str.length(); i++) {
    nextInt = int(str.charAt(i));
    if (nextInt >= 48 && nextInt <= 57) {
      nextInt = map(nextInt, 48, 57, 0, 9);
      nextInt = constrain(nextInt, 0, 9);
      if (belowDec == 0) {
        decValue = (decValue * 10) + nextInt;
      }
      else {
        mod = mod * 0.1;
        decValue = decValue + nextInt * mod;
      }
    }

    if (nextInt == 46) {
      belowDec = 1;
    }

  }
  return decValue;
}

// Hex to decimal converter
long hexToDec(String hexString) {
  long decValue = 0;
  int nextInt;

  for (int i = 0; i < hexString.length(); i++) {
    nextInt = int(hexString.charAt(i));
    if (nextInt >= 48 && nextInt <= 57) nextInt = map(nextInt, 48, 57, 0, 9);
    // Convert A-F to 10-15
    if (nextInt >= 65 && nextInt <= 70) nextInt = map(nextInt, 65, 70, 10, 15);
    if (nextInt >= 97 && nextInt <= 102) nextInt = map(nextInt, 97, 102, 10, 15);
    nextInt = constrain(nextInt, 0, 15);
    decValue = (decValue * 16) + nextInt;
  }
  return decValue;
}

// Hex to string converter (converts every two digits of HEX into one ASCII character)
String hexToStr(String hexString) {

  String strValue = "";
  long nextInt;

  for (int i = 0; i < hexString.length(); i = i + 2) {
    nextInt = hexToDec(hexString.substring(i, i + 2));
    strValue += char(nextInt);
  }
  return strValue;
}

// Decimal to hex converter (converts decimal values into hex of specified digit size (defualt: 4))
String decToHex(int value, int write_size) {
  String strValue = String(value, HEX);
  int zeros = write_size - strValue.length();
  for (int i = 0; i < zeros; i++) {
    strValue = "0" + strValue;
  }
  return strValue;
}
