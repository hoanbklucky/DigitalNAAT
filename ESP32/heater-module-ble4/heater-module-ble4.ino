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

void setup() {
  setupHeater(); //
  setupSerial();
  setupBLE();
}

void loop() {
    // notify changed value
    if (deviceConnected) {
        //blockTemp = "t=" + String(readTemp());
        //pCharacteristic->setValue(blockTemp.c_str());
        //pCharacteristic->notify();
        delay(100); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
        if (start_assay == true) {
          setTemp(set_temp,hold_time,image_every);
        }
        else {
          dataframe = system_status + "," + String(readTemp()) + "," + led_status + "," + String(take_image) + "," + String(millis());
          writeBLE(dataframe);
          Serial.println(dataframe);
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
