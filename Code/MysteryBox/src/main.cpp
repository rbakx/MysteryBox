#include <Arduino.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>

#define LED_BUILTIN 10

#define SleftEEN_WIDTH 128 // OLED display width, in pixels
#define SleftEEN_HEIGHT 32 // OLED display height, in pixels
#define PIXELS_PER_CHARACTER 6
#define TEXT_SIZE 3
#define GPS_GET_DATA_TIME_MS 1000
#define MYSTERY_YEAR 2022
#define MYSTERY_MONTH 6
#define MYSTERY_DAY 4
#define MYSTERY_HOUR 23

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library.

#define OLED_RESET 18 // Reset pin # (or -1 if sharing Arduino reset pin)
// See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define SleftEEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SleftEEN_WIDTH, SleftEEN_HEIGHT, &Wire, OLED_RESET);

static const int RXPin = 4, TXPin = 3;
static const uint32_t GPSBaud = 9600;

// The TinyGPSPlus object
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);
char messageBuf[80];

void displayInfo()
{
  Serial.print(F("Location: "));
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F("  Date/Time: "));
  if (gps.date.isValid())
  {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F(" "));
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10)
      Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10)
      Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10)
      Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F("."));
    if (gps.time.centisecond() < 10)
      Serial.print(F("0"));
    Serial.print(gps.time.centisecond());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.println();
}

void InitializeDisplay()
{
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SleftEEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }

  // display.setRotation(2); // Uncomment to rotate display 180 degrees
  display.setTextSize(3);              // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setTextWrap(false);
  display.cp437(true); // Use full 256 char 'Code Page 437' font
}

void ShowTextOnDisplay(char *message)
{
  int x, minX;

  x = display.width();
  minX = -TEXT_SIZE * PIXELS_PER_CHARACTER * strlen(message);

  do
  {
    display.clearDisplay();
    display.setCursor(x, 6);
    display.print(message);
    display.display();
  } while (--x >= minX);
}

static void ReadGpsData()
{
  unsigned long start = millis();
  do
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < GPS_GET_DATA_TIME_MS);
}

bool CheckIfDayHasCome(int year, int month, int day)
{

  unsigned long currentDay = gps.date.day() + 100 * gps.date.month() + 10000 * gps.date.year();
  unsigned long dayToCheck = day + 100 * month + 10000 * year;
  return currentDay >= dayToCheck;
}

bool CheckIfHourHasCome(int hour)
{
  return gps.time.hour() >= hour;
}

void setup()
{
  Serial.begin(9600);
  Serial.println("Setup started!");

  pinMode(6, OUTPUT_OPEN_DRAIN);
  pinMode(7, OUTPUT_OPEN_DRAIN);
  digitalWrite(6, LOW);
  digitalWrite(7, LOW);

  ss.begin(GPSBaud);
  pinMode(LED_BUILTIN, OUTPUT);

  InitializeDisplay();

  ReadGpsData();
  if (gps.date.isValid() && gps.time.isValid())
  {
    if (!CheckIfDayHasCome(MYSTERY_YEAR, MYSTERY_MONTH, MYSTERY_DAY))
    {
      sprintf(messageBuf, "Het is vandaag nog geen tijd, probeer het morgen nog eens!");
      ShowTextOnDisplay(messageBuf);
    }
    else
    {
      if (!CheckIfHourHasCome(MYSTERY_HOUR))
      {
        sprintf(messageBuf, "Vandaag is de dag, neem me mee!");
        ShowTextOnDisplay(messageBuf);
      }
      else
      {
        if (gps.location.isValid())
        {
          sprintf(messageBuf, "De afstand tot het target is 12345 meter!");
          ShowTextOnDisplay(messageBuf);
        }
        else
        {
          sprintf(messageBuf, "Neem de Mysterybox mee naar buiten!");
          ShowTextOnDisplay(messageBuf);
        }
      }
    }
  }
  else
  {
    sprintf(messageBuf, "Neem de Mysterybox mee naar buiten!");
    ShowTextOnDisplay(messageBuf);
  }

  // Switch off power relais.
  digitalWrite(6, LOW);
  digitalWrite(7, LOW);

  Serial.println("Setup finished!");
}

void loop()
{
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);

  ReadGpsData();
  if (gps.date.isValid() && gps.time.isValid())
  {
    sprintf(messageBuf, "hour: %d", gps.time.hour());
    ShowTextOnDisplay(messageBuf);
  }
}