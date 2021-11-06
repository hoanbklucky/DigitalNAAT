#include "serial.h"
#include "heater.h"

/*
 * Pin out:
 * 
 *  A0  R0Pin 
 *  A1  RxPin 
 *  A2
 *  A3
 *  A4    
 *  A5    
 *  
 *  0-  RX
 *  1-  TX
 *  2-   
 *  3~    
 *  4-  Fan switch
 *  5~  Heater OUT1Pin
 *  6~  Heater OUT2Pin
 *  7 
 *  8 
 *  9~  LED - Blue 
 *  10~ LED - Red
 *  11~ 
 *  12    
 *  13    
 * 
 */
//String blockTemp = "";
void setup() {
  setupHeater();
  setupSerial();
  setupBLE();
}

void loop() {
  //input(true); // reads input from Serial
  //delay(1);
    // notify changed value
    if (deviceConnected) {
        //blockTemp = "t=" + String(readTemp());
        //pCharacteristic->setValue(blockTemp.c_str());
        //pCharacteristic->notify();
        //delay(100); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
        if (assay_start == true) {
          setTemp(set_temp,hold_time);
        }
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("Start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }  
}
