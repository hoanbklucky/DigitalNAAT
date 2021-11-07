#include "heater.h"
#include "Arduino.h"

// Temperature control parameters/variables
float Kp = 40;
float Kd = 800;
float Ki = 0.0;

// RT-PCR cycling parameters (degC, seconds)
int rt_temp = 55; // Reverse Transcription
int rt_time = 0;
int hs_temp = 100; // Hot Start
int hs_time = 10;
int an_temp = 55; // Annealing
int an_time = 2;
int de_temp = 100; // Denature
int de_time = 2;
int cycle_num = 40;
int MAX_PWM = 150;

int BLED_PWM = 0;
int RLED_PWM = 0;
int LED_STATE = 0;

boolean FAM_CHANNEL = true;
boolean CY5_CHANNEL = true;

int outputPWM = 0;

//String temp;
String blockTemp ="";

void setupHeater() {
  // configure PWM channel for Heater
  ledcSetup(PWMChannel, freq, resolution); // TinyPICO
  // attach the PWM channel for Heater to the GPIO pin connected to Heater and set to 0 (no heating) 
  ledcAttachPin(HeaterPin, PWMChannel); // TinyPICO
  ledcWrite(PWMChannel, 0);    // TinyPICO turn off heater at the begining

  // configure PWM channel for Led
  ledcSetup(PWMChannelLed, freq, resolution); // TinyPICO
  // attach the PWM channel for Heater to the GPIO pin connected to Red LED and set to 0 (turn off)
  ledcAttachPin(RLEDPin, PWMChannelLed); // TinyPICO
  ledcWrite(PWMChannelLed, 0);  // TinyPICO turn off LED at the begining
  led_status = "off";

//    pinMode(RLEDPin, OUTPUT);
//    digitalWrite(RLEDPin, 0);
}

double readTemp() {
  double VR0_raw = analogRead(R0Pin);
  double VRx_raw = analogRead(RxPin);
  double VS_raw = 4095; //TinyPICO
  double R0 = 100000;// Use 100K Ohm resistors in Wheatstone Bridge with TinyPICO

  double Vout = VR0_raw - VRx_raw;

  double Rx = (R0 * (0.5 - Vout / VS_raw)) / (0.5 + Vout / VS_raw);
  double logRx = log(Rx);
  
  //100 kohm thermistor Steinhart-Hart coefficients
  double vA = 0.8269925494 * 0.001;
  double vB = 2.088185118 * 0.0001;
  double vC = 0.8054469376 * 0.0000001;
  double temp = 1 / (vA + (vB + (vC * logRx * logRx )) * logRx ) - 273.15;
  return temp;
}


float currentTemp;
float previousError =0;
float integralError =0;
float derivative =0;
float error =0;

// Set temperature (PID controlled)
void setTemp(float setPoint, float hold_time, float image_every){
   setTemp(setPoint, hold_time, image_every, false);
}
void setTemp(float setPoint, float hold_time, float image_every, boolean detect)
{
  previousError = 0;
  integralError = 0;
  derivative = 0;
  error = 0;

  // uses PID to set temperature
  boolean done = false;
  boolean setReached = false;
  boolean FAM_pic = detect && FAM_CHANNEL;
  boolean CY5_pic = detect && CY5_CHANNEL;
  boolean takingPic = false;
  float start_holding_timepoint;
  float image_timepoint;
  float print_timepoint = millis(); //record time point when setTemp function start
  
  float hold_time_mils = hold_time *60 * 1000; //convert from minute to millisecond
  float image_every_mils = image_every * 60 * 1000;//convert from minute to millisecond
  
  Serial.println("Setting temperature to " + String(setPoint) + " degC");
  //Change system status to heating
  system_status = "heating";
  while (!done) {
    // Calculate PID and PWM
    currentTemp = readTemp();
    error = setPoint - currentTemp;
    derivative = error - previousError;
    integralError = integralError + error;
    previousError = error;
    outputPWM = Kp * error + Ki * integralError + Kd * derivative;

    // PWM modifiers
    outputPWM = constrain(map((int)outputPWM, 0, 1000, 0, MAX_PWM), 0, MAX_PWM);

    // Output PWM to Heater
    if (outputPWM < 0) {
      outputPWM = 0;
      ledcWrite(PWMChannel, 0);
    }
    if (outputPWM >= 0) {
      ledcWrite(PWMChannel, outputPWM);    //TinyPICO
    }

    // Timer starts counting down
    if (!setReached && abs(error) < 1)
    {
      // if temp hasn't been reached yet, check if it's within threshold
      setReached = true;        // target temp reached
      system_status = "running"; //change status
      //record this time point 
      start_holding_timepoint = millis();  // record time point when difference between currentTemp and setPoint < 1, ie start holding temp
      image_timepoint = millis(); // record time point 
      //print to serial monitor
      Serial.println(F("\t Target temp reached."));
      //prepare dataframe and send to App and serial monitor
      remaining_time = hold_time - (millis() - start_holding_timepoint)/(1000*60); // remaining time in minute = total hold time - (current time point - time point when set_temp reached and start holding)
      dataframe = system_status + "," + String(currentTemp) + "," + led_status + "," + String(take_image) + "," + String(remaining_time);
      writeBLE(dataframe); // send to App
      Serial.println(dataframe); //print to serial monitor
    }

    // REPORTING send dataframe (info about the system) to serial monitor and App every time_int (eg 1000 milisecond)
    if (millis() - print_timepoint >= time_int) {
      print_timepoint = print_timepoint + time_int; //after every time_int, send current temp to serial port
      //prepare dataframe
      if (setReached == true) {
        remaining_time = hold_time - (millis() - start_holding_timepoint)/(1000*60); // remaining time in minute = total hold time - (current time point - time point when set_temp reached and start holding)
        dataframe = system_status + "," + String(currentTemp) + "," + led_status + "," + String(take_image) + "," + String(remaining_time);
      } else {
        dataframe = system_status + "," + String(currentTemp) + "," + led_status + "," + String(take_image) + "," + String(millis());
      }
      writeBLE(dataframe); // send to App
      Serial.println(dataframe); // send to serial monitor
    }

    if (setReached == true && (millis() - image_timepoint) > image_every_mils) {
      image_timepoint = image_timepoint + image_every_mils;
      ledcWrite(PWMChannelLed, 255);    //Turn on LED
      led_status = "on"; 
      Serial.println("Led is ON");
      //tell App to take image
      take_image = true;  
      remaining_time = hold_time - (millis() - start_holding_timepoint)/(1000*60); // remaining time in minute = total hold time - (current time point - time point when set_temp reached and start holding)
      dataframe = system_status + "," + String(currentTemp) + "," + led_status + "," + String(take_image) + "," + String(remaining_time);
      writeBLE(dataframe); // send to App
      Serial.println(dataframe);
    }

//    // End temperature hold when timer runs out and pictures have been taken
    if (setReached == true && (millis() - start_holding_timepoint) > hold_time_mils) {
      start_assay = false;
      system_status = "done";
      Serial.println(F("\t Hold temp completed."));
      //prepare dataframe
      dataframe = system_status + "," + String(currentTemp) + "," + led_status + "," + String(take_image) + "," + "NA";
      writeBLE(dataframe); // send to App
      Serial.println(dataframe);
    }

    // PID loop interval
    delay(25);
  }
}

void cycle(){
  init_time = millis();
  // Reverse Transcription
  if(rt_time > 0.01){
    Serial.println("L,Reverse Transcription,START");
    setTemp(rt_temp, rt_time, false);
    Serial.println("L,Reverse Transcription,END");
  }
  // Hot Start
  if(hs_time > 0.01){
    Serial.println("L,Hot Start,START");
    setTemp(hs_temp, hs_time, false);
    Serial.println("L,Hot Start, END");
  }
  // Cycling
  // Print "C" at the beginning of each cycle followed by # of cycle - example: C,1 = cycle 1
  if(cycle_num > 0){
    Serial.println("L,Cycling, START");
    for(int i = 0; i<cycle_num; i++){
       Serial.print("C,");
       Serial.println(i+1);
       
       Serial.print("L,Denature,");
       Serial.println(i+1);
       setTemp(de_temp, de_time, false);
       //digitalWrite(fanPin,HIGH);

       Serial.print("L,Anneal,");
       Serial.println(i+1);
       setTemp(an_temp, an_time, FAM_CHANNEL || CY5_CHANNEL);
       //digitalWrite(fanPin,LOW);
    }  
    Serial.println("L,Cycling, END");
  }
  Serial.println("E"); 
}
