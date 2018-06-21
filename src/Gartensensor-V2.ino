/******************************************************************************
* gartensensor-v2.ino
*
* Garten-Sensor Programm
* zur Temperatur- und Feuchtigskeitsmesseung
* inklusive Ausgabe auf e-Paper-Display
*
* Autor: Markus Rathke
* Datum: 21.06.2018
*
* Hardware Connections:

  Power Shield ------------- Photon
     D0 -------------------- D0
     D1 (I2C) -------------- D1

   1.54inch e-Paper Mod----- Photon
     GND ------------------- GND
     3.3V ------------------ 3.3V (VCC)
     DIN ------------------- A5 (MOSI/D11)
     CLK ------------------- A3 (SCK/D13)
     CS  ------------------- A2 (SS/D10)
     DC -------------------- WKP (A7/D9)
     RST ------------------- DAC (A6/D8)
     BUSY ------------------ D7
*
*******************************************************************************/

#include "libs/epd1in54/epd1in54.h"
#include "libs/epd1in54/epdpaint.h"
#include <SHT1x.h>
#include "PowerShield.h"

/////////////////////////
// Startup Definitions //
/////////////////////////
STARTUP(WiFi.selectAntenna(ANT_EXTERNAL)); // use the external antenna (via u.FL)

//////////////////////////////////
// e-Paper Display Definitionen //
//////////////////////////////////
#define COLORED     0
#define UNCOLORED   1
unsigned char image[1024];
Paint paint(image, 0, 0);    // width should be the multiple of 8
Epd epd;

///////////////////////////////////
// Humi+Temp-Sensor Definitionen //
///////////////////////////////////
#define DATA_PIN  D3
#define CLOCK_PIN D4
SHT1x htsensor1(DATA_PIN, CLOCK_PIN);
String S1_tempC;
String S1_humi;

//////////////////////////////
// PowerShield Definitionen //
//////////////////////////////
PowerShield batteryMonitor;

////////////////////////
// Debug Definitionen //
////////////////////////
int led1 = D5;
int sleepJmp = D6;

void setup() {
    // Initialize Debug PINs
    pinMode(sleepJmp, INPUT_PULLDOWN);
    pinMode(led1, OUTPUT);
    digitalWrite(led1, HIGH);

    //Initialize BatteryShield
    batteryMonitor.begin();
    batteryMonitor.quickStart();

    // Initialize e-Paper display
    if (epd.Init(lut_partial_update) != 0) {
      Particle.publish("e-Paper init failed!", PRIVATE);
      return;
    }

    /**
     *  there are 2 memory areas embedded in the e-paper display
     *  and once the display is refreshed, the memory area will be auto-toggled,
     *  i.e. the next action of SetFrameMemory will set the other memory area
     *  therefore you have to clear the frame memory twice.
     */
    epd.ClearFrameMemory(0xFF);   // bit set = white, bit reset = black
    epd.DisplayFrame();
    epd.ClearFrameMemory(0xFF);   // bit set = white, bit reset = black
    epd.DisplayFrame();

    // Draw screen header and fill the two memory buffers
    paint.SetRotate(ROTATE_0);
    paint.SetWidth(200);
    paint.SetHeight(24);
    paint.Clear(COLORED);
    paint.DrawStringAt(25, 6, "Gartensensor v2.0", &Font12, UNCOLORED);
    epd.SetFrameMemory(paint.GetImage(), 0, 10, paint.GetWidth(), paint.GetHeight());
    epd.DisplayFrame();
    epd.SetFrameMemory(paint.GetImage(), 0, 10, paint.GetWidth(), paint.GetHeight());
    epd.DisplayFrame();
    delay(2000);

    // Initialize complete (3 blinks)
    digitalWrite(led1, LOW);
    delay(100);
    digitalWrite(led1, HIGH);
    delay(100);
    digitalWrite(led1, LOW);
    delay(100);
    digitalWrite(led1, HIGH);
    delay(100);
    digitalWrite(led1, LOW);
    delay(100);
    digitalWrite(led1, HIGH);
    delay(100);
    digitalWrite(led1, LOW);
    delay(100);
}

void loop() {
    digitalWrite(led1, HIGH);

    // read temperature and humidity
    S1_humi = String(htsensor1.readHumidity(), 1);
    S1_tempC = String(htsensor1.readTemperatureC(), 1);
    // publish temperature and humidity to particle cloud
    Particle.publish("S1_HumiTempC", S1_humi + ";" + S1_tempC);

    // read the state of charge of the single cell battery
    // connected to the Power Shield, from 0% to 100%
    float stateOfCharge = batteryMonitor.getSoC();
    // publish battery alert when below 30%
    if(stateOfCharge <= 30.0) {
      Particle.publish("batteryAlert", PRIVATE);
    }

    // refresh display information
    paint.SetWidth(200);
    paint.SetHeight(52);
    paint.Clear(UNCOLORED);
    paint.DrawStringAt(10, 4, "Feuchtigkeit: " + S1_humi, &Font12, COLORED);
    paint.DrawStringAt(10, 20, "Temperatur (C): " + S1_tempC, &Font12, COLORED);
    paint.DrawStringAt(10, 36, "Batterie: " + String(stateOfCharge, 0), &Font12, COLORED);
    epd.SetFrameMemory(paint.GetImage(), 0, 60, paint.GetWidth(), paint.GetHeight());
    epd.DisplayFrame();

    delay(1000);

    digitalWrite(led1, LOW);
    delay(1000);

    // Enter Sleep Mode if Sleep Jumper is set to HIGH
    // otherwise wait for 60 seconds
    if(digitalRead(sleepJmp) == HIGH) {
      digitalWrite(led1, HIGH);
      delay(100);
      digitalWrite(led1, LOW);
      delay(100);
      digitalWrite(led1, HIGH);
      delay(100);
      digitalWrite(led1, LOW);

      // Sleep system for 5 minutes or interupt via A0
      System.sleep(A0, RISING, 300);
    } else {
      delay(30000);
    }
}
