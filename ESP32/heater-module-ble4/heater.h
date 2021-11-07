#ifndef _HEATER_H
#define _HEATER_H

#include "serial.h"

// define pin
#define RLEDPin 25 ///TinyICO
#define HeaterPin 26 ///TinyICO
#define R0Pin 27     ///TinyICO
#define RxPin 15     ///TinyICO

// const variables for PWM
const int freq = 5000; ///TinyICO
const int PWMChannel = 0; ///TinyICO
const int PWMChannelLed = 1; ///TinyICO
const int resolution = 8; ///TinyICO

//#define interval 25

// PID Parameters
extern float Kp;
extern float Kd;
extern float Ki;

// Cycle Parameters
extern int rt_temp; // Reverse Transcription
extern int rt_time;
extern int hs_temp; // Hot Start
extern int hs_time;
extern int an_temp; // Annealing
extern int an_time;
extern int de_temp; // Denature
extern int de_time;
extern int cycle_num;

extern boolean FAM_CHANNEL;
extern boolean CY5_CHANNEL;

// LED Parameters
extern int BLED_PWM;
extern int RLED_PWM;
extern int LED_STATE;

void setupHeater();

double readTemp();
void setTemp(float setPoint, float hold_time, float image_every);
void setTemp(float setPoint, float hold_time, float image_every, boolean detect);
void cycle();

#endif
