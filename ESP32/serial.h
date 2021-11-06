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
 
#define BAUD 57600

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

extern HardwareSerial Serial;

extern double init_time;
extern char rc;
extern String message;
extern int time_int; 
extern boolean stopFlag;
extern boolean serial_read; 
extern double set_temp;
extern double hold_time;
extern boolean assay_start;
extern boolean assay_done;

extern int imagingevery;
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
