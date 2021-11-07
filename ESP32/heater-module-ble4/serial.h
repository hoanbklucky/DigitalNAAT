#ifndef _SERIAL_H
#define _SERIAL_H

#include <Arduino.h>
//#include <avr/wdt.h>
#include "heater.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
/*
 * SERIAL COMMAND LIST
 *    
 */
//define baudrate of serial port 
#define BAUD 57600
//define service uuid and characteristic uuid of ble
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

extern HardwareSerial Serial;

extern double init_time;
extern char rc;
extern String message;
extern int time_int; 
extern boolean stopFlag;
extern boolean serial_read; 

// Variable in dataframe that App send to ESP32 t(set_temp,hold_time,image_every)
extern double set_temp; //use double to account for 23.56
extern double hold_time; //hold_time in minute
extern double image_every; // image_every in minute

// Flags indidating that the assay is running
extern boolean start_assay;

// Variables in dataframe that ESP32 send to App (system_status, current_temp, led_status, image_ready, remaining_time)
extern String system_status;
extern double current_temp;
extern String led_status;       //ON/OFF
extern boolean take_image;    //true: ready for App to take image; false: not ready
extern double remaining_time; //assay remaining time in minute
extern String dataframe;

// Flags indicating connection status of ble
extern bool deviceConnected;
extern bool oldDeviceConnected;

extern BLEServer* pServer;
extern BLECharacteristic* pCharacteristic;

void setupBLE();
void writeBLE(String message); //write string to BLE
void setupSerial();

void input(boolean interpretFlag);
void cmd_interpret(/*String message*/);
void parseArgs(double args[], String message);
void parseMsg(String args[], String message);

double strToNum(String str);
long hexToDec(String hexString);
String hexToStr(String hexString);
String decToHex(int value, int write_size);

#endif
