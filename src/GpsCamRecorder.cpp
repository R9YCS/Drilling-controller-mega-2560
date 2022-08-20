#include <SPI.h>
#include <SD.h>
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
#include <EncButton2.h>

EncButton2<EB_BTN> CTR_BTN(INPUT_PULLUP, 9);

#define LED_PIN_RED 6
#define LED_PIN_GREEN 5
#define RELAY_PIN 8

enum Mode
{
  Initialiazation_SD,
  Write_Log,
  NoMode
};

Mode mode = NoMode;

File LogFile;
TinyGPSPlus gps;

static const int RXPin = 2, TXPin = 3;
static const uint32_t GPSBaud = 9600;
unsigned long TimerTime;
unsigned long LedBlink;

bool led_green = false;
bool led_red = false;
bool relay = false;
bool err = false;

float Lat_long, Lng_long;

SoftwareSerial ss(RXPin, TXPin);

void setup() {

  Serial.begin(115200);
  ss.begin(GPSBaud);

  pinMode(LED_PIN_GREEN, OUTPUT);
  pinMode(LED_PIN_RED, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);

}

void write_log() {

  //Serial.println(gps.satellites.value());

  if (gps.satellites.value() >= 5) {
    bool write_sd;
    led_red = true;

    if (millis() - TimerTime > 2000) {
      TimerTime = millis();

      LogFile = SD.open("Log.txt", FILE_WRITE);
      unsigned long filesize = LogFile.size();

      led_green = true;
      relay = true;

      if (LogFile) {
        Serial.println("Write SD");
        LogFile.print((gps.time.hour()) + 7);
        LogFile.print(":");
        LogFile.print(gps.time.minute());
        LogFile.print(":");
        LogFile.print(gps.time.second());
        LogFile.print("\t");

        LogFile.print(gps.date.day());
        LogFile.print(".");
        LogFile.print(gps.date.month());
        LogFile.print(".");
        LogFile.print(gps.date.year());
        LogFile.print("\t");

        Lat_long = gps.location.lat();
        Lng_long = gps.location.lng();

        LogFile.print(Lat_long, 7);
        LogFile.print("\t");
        LogFile.print(Lng_long, 7);
        LogFile.print("\t");

        LogFile.print(gps.altitude.meters());
        LogFile.println("");

        LogFile.close();

      }
    } else {
      if (millis() - TimerTime > 1000) {
        led_green = false;
        relay = false;
      }
    }
  } else {
    if (round(millis() / 500) % 2 == 0) {
      led_red = true;
    } else {
      led_red = false;
    }
    led_green = false;
  }

  if (CTR_BTN.press()) {
    mode = NoMode;
  }
}

void initialiazation_SD() {

  while (!Serial) {
  }
  if (!SD.begin(4)) {
    //while (1);
    mode = NoMode;
    err = true;
  } else {
    mode = Write_Log;
    err = false;
  }

}
void nomode() {

  if (CTR_BTN.hold()) {
    mode = Initialiazation_SD;
  }

  if (gps.satellites.value() <= 4) {
    if (round(millis() / 500) % 2 == 0) {
      led_red = true;
    } else {
      led_red = false;
    }
  } else {
    led_red = true;
  }

  if (err) {
    if (round(millis() / 250) % 2 == 0) {
      led_green = true;
    } else {
      led_green = false;
    }
  } else {
    led_green = true;
  }


}
void loop() {

  CTR_BTN.tick();

  while (ss.available())
    gps.encode(ss.read());

  if (led_green == true) {
    digitalWrite(LED_PIN_GREEN, true);
  } else {
    digitalWrite(LED_PIN_GREEN, false);
  }
  if (led_red == true) {
    digitalWrite(LED_PIN_RED, true);
  } else {
    digitalWrite(LED_PIN_RED, false);
  }
  if (relay == true) {
    digitalWrite(RELAY_PIN, true);
  } else {
    digitalWrite(RELAY_PIN, false);
  }

  switch (mode)
  {
    case Initialiazation_SD:
      initialiazation_SD();
      break;
    case Write_Log:
      write_log();
      break;
    case NoMode:
      nomode();
      break;
  }
}