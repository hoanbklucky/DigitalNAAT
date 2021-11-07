#include "serial.h"

//Variable for BLE server and characteristic
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;

// Variable in dataframe that App send to ESP32 t(set_temp,hold_time,image_every)
double set_temp; //use double to account for 23.56
double hold_time; //hold_time in minute
double image_every; // image_every in minute

// Flags indidating that the assay is running
boolean start_assay = false;

// Variables in dataframe that ESP32 send to App (system_status, current_temp, led_status, image_ready, remaining_time)
String system_status = "idle";
double current_temp;
String led_status = "off";       //ON/OFF
boolean take_image = false;    //true: ready for App to take image; false: not ready
double remaining_time = 0; //assay remaining time in minute
String dataframe = "";

// Flags indicating connection status of ble
bool deviceConnected = false;
bool oldDeviceConnected = false;

String code_version = "heater-module-ble4.ino - 20211106";

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
      Serial.println("Device connected");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
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
        Serial.println(message); // print received message to serial monitor
        delay(10);      
        writeBLE(message);       // echo the received message to App
        // analyse the received message 
         if (message.substring(0, 2) == "t(") {
            parseArgs(args, message);
            set_temp = args[0];
            hold_time = args[1];
            image_every = args[2];
            //print the parsed arguments to serial monitor for troubleshooting
            Serial.print("set_temp = ");
            Serial.print(set_temp);
            Serial.print(",");
            Serial.print("hold_time = ");
            Serial.print(hold_time);
            Serial.print(",");
            Serial.print("image_every = ");
            Serial.println(image_every);
            //turn on flag indicating that assay is running
            start_assay = true;
  
         } else if (message == "ledon") {
            ledcWrite(PWMChannelLed, 255);    //Turn on LED
            led_status = "on";               //Change status of image_ready to true
            Serial.println("Led is ON");
         } else if (message == "ledoff") {
            ledcWrite(PWMChannelLed, 0);    //Turn off LED
            led_status = "off";            //Change status of image_ready to false
            Serial.println("Led is OFF");
         } else if (message == "imaged") {
            ledcWrite(PWMChannelLed, 0);    //Turn off LED
            led_status = "off";            //Change status of image_ready to false
            Serial.println("Led is OFF");
            take_image = false;
         }
       }
    }
};

void setupBLE() {
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
  // Print to serial port
  Serial.println("<ESP32-TEMP-READY>");
  
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
