#include <Arduino.h>
#define ARDUINO_ESP32S2_DEV 1
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TinyGPSPlus.h>
#include "ESP32_ISR_Servo.h"
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
#define MYSTERY_HOUR 17
#define MAX_TIME_POWER_ON_MS 120000

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library.

#define RELAIS_PIN_1 6
#define RELAIS_PIN_2 7
#define SERVO_PIN 13
#define SERVO_OPEN 80
#define SERVO_CLOSE 0
#define BUZZER_PIN 15
#define BUZZER_CHANNEL 1

#define OLED_RESET 18 // Reset pin # (or -1 if sharing Arduino reset pin)
// See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define SleftEEN_ADDRESS 0x3C

const double TARGET_LAT = 51.39482540756363, TARGET_LON = 5.479256311543883;

Adafruit_SSD1306 display(SleftEEN_WIDTH, SleftEEN_HEIGHT, &Wire, OLED_RESET);

static const int RXPin = 3, TXPin = 4;
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

void InitDisplay()
{
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SleftEEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }

  // display.setRotation(2); // Uncomment to rotate display 180 degrees
  display.setTextSize(TEXT_SIZE);
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

unsigned long distanceToTarget()
{
  return (unsigned long)TinyGPSPlus::distanceBetween(
      gps.location.lat(),
      gps.location.lng(),
      TARGET_LAT,
      TARGET_LON);
}

bool CheckIfDayHasCome(int year, int month, int day)
{

  unsigned long currentDay = gps.date.day() + 100 * gps.date.month() + 10000 * gps.date.year();
  unsigned long dayToCheck = day + 100 * month + 10000 * year;
  return currentDay >= dayToCheck;
}

bool CheckIfHourHasCome(int year, int month, int day, int hour)
{
  unsigned long currentDayAndHour = gps.time.hour() + 100 * gps.date.day() + 10000 * gps.date.month() + 1000000 * gps.date.year();
  unsigned long dayAndHourToCheck = hour + 100 * day + 10000 * month + 1000000 * year;
  return currentDayAndHour >= dayAndHourToCheck;
}

void InitTone()
{
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
  ledcWrite(0, 128); // 50% duty cycle
}

void PlayTone(int freq_hz, int duration_ms)
{
  ledcWriteTone(BUZZER_CHANNEL, freq_hz);
  delay(duration_ms);
}

void PlayNote(char note, int duration_ms)
{
  char names[] = {'C', 'D', 'E', 'F', 'G', 'A', 'B',
                  'c', 'd', 'e', 'f', 'g', 'a', 'b',
                  'x', 'y'};

  int tones[] = {261, 293, 329, 349, 392, 440, 493,
                 523, 587, 659, 698, 783, 880, 987,
                 783, 698};

  int nofTones = sizeof(tones) / sizeof(int);

  int SPEE = 5;

  // play the tone corresponding to the note name

  for (int i = 0; i < nofTones; i++)
  {

    if (names[i] == note)
    {
      int newduration_ms = duration_ms / SPEE;
      PlayTone(tones[i], newduration_ms);
    }
  }
}

void NoTone()
{
  ledcWriteTone(BUZZER_CHANNEL, 0);
}

typedef struct
{
  char notes[64];
  int beats[64];
  int nofNotes;
  int tempo;
} Tune;

enum tuneEnum
{
  Hello,
  Happy,
  HappyBirthday
};

Tune tuneArray[3] = {
    {"CDEc", {2, 2, 2, 2}, 4, 100},
    {"CEGcGc", {1, 1, 1, 2, 1, 6}, 6, 200},
    {"GGAGcB GGAGdc GGxecBA yyecdc", {2, 2, 8, 8, 8, 16, 1, 2, 2, 8, 8, 8, 16, 1, 2, 2, 8, 8, 8, 8, 16, 1, 2, 2, 8, 8, 8, 16}, 28, 200},
};

void PlayTune(Tune tune)
{
  for (int i = 0; i < tune.nofNotes; i++)
  {
    if (tune.notes[i] == ' ')
    {
      delay(tune.beats[i] * tune.tempo); // delay between notes
    }
    else
    {
      PlayNote(tune.notes[i], tune.beats[i] * tune.tempo);
    }
    // time delay between notes
    delay(tune.tempo);
  }
  NoTone();
}

void setup()
{
  Serial.begin(9600);
  Serial.println("Setup started!");

  pinMode(RELAIS_PIN_1, OUTPUT_OPEN_DRAIN);
  pinMode(RELAIS_PIN_2, OUTPUT_OPEN_DRAIN);
  digitalWrite(RELAIS_PIN_1, LOW);
  digitalWrite(RELAIS_PIN_2, LOW);

  InitTone();
  PlayTune(tuneArray[Hello]);

  int servoIndex = ESP32_ISR_Servos.setupServo(SERVO_PIN, 1000, 2000);
  ESP32_ISR_Servos.setPosition(servoIndex, SERVO_CLOSE);

  ss.begin(GPSBaud);
  pinMode(LED_BUILTIN, OUTPUT);

  InitDisplay();

  int tmpDate = gps.date.year();
  double tmpLocation = gps.location.lat();
  unsigned long startTime = millis();
  unsigned long currentTime;
  bool finished = false;
  do
  {
    currentTime = millis();
    ReadGpsData();
    if (gps.date.isValid() && gps.time.isValid() && gps.date.year() > 2020)
    {
      if (!CheckIfDayHasCome(MYSTERY_YEAR, MYSTERY_MONTH, MYSTERY_DAY))
      {
        sprintf(messageBuf, "Het is vandaag nog geen tijd, probeer het morgen nog eens!");
        ShowTextOnDisplay(messageBuf);
        finished = true;
      }
      else
      {
        if (!CheckIfHourHasCome(MYSTERY_YEAR, MYSTERY_MONTH, MYSTERY_DAY, MYSTERY_HOUR))
        {
          sprintf(messageBuf, "Vandaag is de dag, neem me mee!");
          ShowTextOnDisplay(messageBuf);
          finished = true;
        }
        else
        {
          if (gps.location.isValid() && gps.location.lat() != 0.0)
          {
            sprintf(messageBuf, "De afstand tot het target is %d meter!", distanceToTarget());
            ShowTextOnDisplay(messageBuf);
            finished = true;
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
  } while (currentTime - startTime < MAX_TIME_POWER_ON_MS && finished == false);

  // Switch off power relais.
  digitalWrite(RELAIS_PIN_1, HIGH);
  digitalWrite(RELAIS_PIN_2, HIGH);

  Serial.println("Setup finished!");
}

void loop()
{
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  Serial.println("year: " + String(gps.date.year()));
  Serial.println("lat: " + String(gps.location.lat()));
}