/*
  Andrey Lutich
  Example of rewiring Adafruit_CCS811 I2C to non-default pins (33, 25)
  Tested on ESP32

  Based on:
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-i2c-communication-arduino-ide/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_CCS811.h"

#define I2C_SDA 33
#define I2C_SCL 25


TwoWire I2CBMP = TwoWire(0);
Adafruit_CCS811 ccs(&I2CBMP);

unsigned long delayTime;

void setup() {
  Serial.begin(9600);
  I2CBMP.begin(I2C_SDA, I2C_SCL, 100000);

  Serial.println("CCS811 test");
  if(!ccs.begin()){
    Serial.println("Failed to start sensor! Please check your wiring.");
    while(1);
  }

  // Wait for the sensor to be ready
  while(!ccs.available());
  
  delayTime = 1000;

  Serial.println("Setup... done.");
}

void loop() { 
  printValues();
  delay(delayTime);
}

void printValues() {
    Serial.println("CCS811:");
    if(ccs.available()){
      if(!ccs.readData()){
        
        Serial.print("CO2: ");
        Serial.print(ccs.geteCO2());
        Serial.println("ppm");

        Serial.print("TVOC: ");
        Serial.print(ccs.getTVOC());
        Serial.println("ppb");
      }
      else{
        Serial.println("ERROR!");
        while(1);
      }
    }



  Serial.println();
}
