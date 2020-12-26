#include <Wire.h>

#include <Wire.h>

/***************************************************************************
  This is a library for the CCS811 air

  This sketch reads the sensor

  Designed specifically to work with the Adafruit CCS811 breakout
  ----> http://www.adafruit.com/products/3566

  These sensors use I2C to communicate. The device's I2C address is 0x5A

  Adafruit invests time and resources providing this open source code,
  please support Adafruit andopen-source hardware by purchasing products
  from Adafruit!

  Written by Dean Miller for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ***************************************************************************/

#include "Adafruit_CCS811.h"
#include "Wire.h"


#define TCAADDR 0x70

void tcaselect(uint8_t i) {
  if (i > 7) return;

  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();
}

Adafruit_CCS811 ccs;

void setup() {
  while (!Serial);
  delay(1000);
  Wire.begin();
  Serial.begin(9600);

  Serial.println("CCS811 test");

//   for (uint8_t t=0; t<8; t++) {
//     tcaselect(t);
//     Serial.print("TCA Port #"); Serial.println(t);
//
//     for (uint8_t addr = 0; addr<=127; addr++) {
//       if (addr == TCAADDR) continue;
//
//       uint8_t data;
//       if (! twi_writeTo(addr, &data, 0, 1, 1)) {
//            Serial.print("Found I2C 0x");  Serial.println(addr,HEX);
//       }
//     }
//   }
//   Serial.println("\ndone");

  // Initialize CCS811
  tcaselect(0);
  if(!ccs.begin()){
    Serial.println("Failed to start CCS811 sensor! Please check your wiring.");
    while(1);
  }

  // Wait for the sensor to be ready
  while(!ccs.available());
}

void loop() {
  tcaselect(0);
  if(ccs.available()){
    if(!ccs.readData()){
      Serial.print("CO2: ");
      Serial.print(ccs.geteCO2());
      Serial.print("ppm, TVOC: ");
      Serial.println(ccs.getTVOC());
    }
    else{
      Serial.println("ERROR!");
      while(1);
    }
  }
  delay(500);
}
