#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_CCS811.h"
#include "Adafruit_SGP30.h"
#include "Adafruit_BME280.h"
#include "Adafruit_EPD.h"


// Declare the i2c address for the TCA
#define TCAADDR 0x70

// Define pinout for the eink friend
#define EPD_CS      7
#define EPD_DC      6
#define SRAM_CS     5
#define EPD_RESET   4 // can set to -1 and share with microcontroller Reset!
#define EPD_BUSY    3 // can set to -1 to not use a pin (will wait a fixed delay)

Adafruit_CCS811 ccs;
Adafruit_SGP30 sgp;



Adafruit_BME280 bme; // use I2C interface
Adafruit_Sensor *bme_temp = bme.getTemperatureSensor();
Adafruit_Sensor *bme_pressure = bme.getPressureSensor();
Adafruit_Sensor *bme_humidity = bme.getHumiditySensor();


// Declare eink display
#define COLOR1 EPD_BLACK
#define COLOR2 EPD_RED

Adafruit_IL0373 display(212, 104, EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);
#define FLEXIBLE_213


void tcaselect(uint8_t i) {
  if (i > 7) return;

  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();
}

/* return absolute humidity [mg/m^3] with approximation formula
* @param temperature [°C]
* @param humidity [%RH]
*/
uint32_t getAbsoluteHumidity(float temperature, float humidity) {
    // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
    const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
    const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity); // [mg/m^3]
    return absoluteHumidityScaled;
}

void testdrawtext(char *text, uint16_t color) {
  display.setCursor(0, 0);
  display.setTextColor(color);
  display.setTextWrap(true);
  display.print(text);
}

void displayOnMessage() {
  display.setRotation(0);

  // large block of text
  display.clearBuffer();
  display.setTextWrap(true);

  display.setCursor(10, 10);
  display.setTextSize(5);
  display.setTextColor(EPD_BLACK);
  display.print("Hello!");

  display.display();
}



void setup() {
//   while (!Serial);
  delay(1000);


  // init eink display
  Serial.println("Setting up display");
  display.begin();

#if defined(FLEXIBLE_213) || defined(FLEXIBLE_290)
  // The flexible displays have different buffers and invert settings!
  display.setBlackBuffer(1, false);
  display.setColorBuffer(1, false);
#endif

  // large block of text
  displayOnMessage();


  Wire.begin();
  Serial.begin(115200);

  Serial.println("CCS811 and SGP30 integrated");

  // Initialize CCS811
  tcaselect(0);
  if(!ccs.begin()){
    Serial.println("Failed to start CCS811 sensor! Please check your wiring.");
    while(1);
  }

  // Wait for the sensor to be ready
  while(!ccs.available());

  // Initialize SGP30
  tcaselect(1);
  if (! sgp.begin()){
    Serial.println("Sensor SGP30 not found :(");
    Serial.println("Failed to start SGP30 sensor! Please check your wiring.");
    while (1);
  }
  Serial.print("Found SGP30 serial #");
  Serial.print(sgp.serialnumber[0], HEX);
  Serial.print(sgp.serialnumber[1], HEX);
  Serial.println(sgp.serialnumber[2], HEX);

  // If you have a baseline measurement from before you can assign it to start, to 'self-calibrate'
  //sgp.setIAQBaseline(0x8E68, 0x8F41);  // Will vary for each sensor!

  // Initialize BME280
  tcaselect(2);
  if (!bme.begin()) {
    Serial.println(F("Could not find a valid BME280 sensor, check wiring!"));
    Serial.println("Failed to start SGP30 sensor! Please check your wiring.");
    while (1) delay(10);
  }
  bme_temp->printSensorDetails();
  bme_pressure->printSensorDetails();
  bme_humidity->printSensorDetails();


}

void readCCS811data() {
  tcaselect(0);

  uint16_t current_eCO2, current_TVOC;




  Serial.println("CCS811:  ");
  if(ccs.available()){
    if(!ccs.readData()){
      current_eCO2 = ccs.geteCO2();
      current_TVOC = ccs.getTVOC();
      Serial.print("CO2: ");
      Serial.print(current_eCO2);
      Serial.print("ppm, TVOC: ");
      Serial.println(current_TVOC);
    }
    else{
      Serial.println("ERROR!");
      while(1);
    }
  }
  display.println("CCS811:  ");
  display.print("CO2: ");
  display.print(current_eCO2);
  display.print("     ");
  display.print("TVOC: ");
  display.println(current_TVOC);
  display.println("");
  delay(500);
}

int counter = 0;

void readSGP30data() {
  tcaselect(1);
  // If you have a temperature / humidity sensor, you can set the absolute humidity to enable the humditiy compensation for the air quality signals
  //float temperature = 22.1; // [°C]
  //float humidity = 45.2; // [%RH]
  //sgp.setHumidity(getAbsoluteHumidity(temperature, humidity));

  Serial.print("SGP30:   ");
  if (! sgp.IAQmeasure()) {
    Serial.println("Measurement failed");
    return;
  }

  Serial.print("TVOC "); Serial.print(sgp.TVOC); Serial.print(" ppb\t");
  Serial.print("eCO2 "); Serial.print(sgp.eCO2); Serial.println(" ppm");

  Serial.print("SGP30:   ");
  if (! sgp.IAQmeasureRaw()) {
    Serial.println("Raw Measurement failed");
    return;
  }
  Serial.print("Raw H2 "); Serial.print(sgp.rawH2); Serial.print(" \t");
  Serial.print("Raw Ethanol "); Serial.print(sgp.rawEthanol); Serial.println("");

  delay(1000);


  counter++;
  if (counter == 30) {
    counter = 0;

    uint16_t TVOC_base, eCO2_base;
    if (! sgp.getIAQBaseline(&eCO2_base, &TVOC_base)) {
	  Serial.print("SGP30:   ");
      Serial.println("Failed to get baseline readings");
      return;
    }
	Serial.print("SGP30:   ");
    Serial.print("****Baseline values: eCO2: 0x"); Serial.print(eCO2_base, HEX);
    Serial.print(" & TVOC: 0x"); Serial.println(TVOC_base, HEX);
  }
  display.println("SGP30:   ");
  display.print("TVOC "); display.print(sgp.TVOC); display.print(" ppb\t");
  display.print("eCO2 "); display.print(sgp.eCO2); display.println(" ppm");
  display.print("Raw H2 "); display.print(sgp.rawH2); display.print(" \t");
  display.print("Raw Ethanol "); display.print(sgp.rawEthanol); display.println("");
  display.println();
}

void readBME260data() {
  tcaselect(2);
  sensors_event_t temp_event, pressure_event, humidity_event;
  bme_temp->getEvent(&temp_event);
  bme_pressure->getEvent(&pressure_event);
  bme_humidity->getEvent(&humidity_event);

  Serial.print("BME280:  ");
  Serial.print(F("Temperature = "));
  Serial.print(temp_event.temperature);
  Serial.println(" *C");

  Serial.print("BME280:  ");
  Serial.print(F("Humidity = "));
  Serial.print(humidity_event.relative_humidity);
  Serial.println(" %");

  Serial.print("BME280:  ");
  Serial.print(F("Pressure = "));
  Serial.print(pressure_event.pressure);
  Serial.println(" hPa");


  display.println("BME280:  ");
  display.print(F("Temp = "));
  display.print(temp_event.temperature);
  display.println(" *C");

  display.print(F("Humidity = "));
  display.print(humidity_event.relative_humidity);
  display.println(" %");

  display.print(F("Pressure = "));
  display.print(pressure_event.pressure);
  display.println(" hPa");
  display.println();

  delay(1000);
}

void prepDisplay() {
  display.clearBuffer();
  // large block of text

  display.setCursor(10, 10);
  display.setTextSize(1);
  display.setTextColor(EPD_BLACK);
}

void writeDisplay() {
  display.display();
}


void loop() {
  prepDisplay();

  readBME260data();
  readCCS811data();
  readSGP30data();
  writeDisplay();
  delay(10000);
}
