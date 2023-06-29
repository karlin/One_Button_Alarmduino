// Based on example DS1307 code written by Tony DiCola for Adafruit Industries.
// Alarm clock functions added by Karlin Fox
//
// Released under a MIT license: https://opensource.org/licenses/MIT
//
// Adafruit invests time and resources providing this open source code,
// please support Adafruit and open-source hardware by purchasing
// products from Adafruit!

#include <avr/interrupt.h>
#include <Wire.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"
#include "RTClib.h"


#define SKEW_CORRECT_MINUTES  5
#define TIME_24_HOUR          true
#define DISPLAY_I2C_ADDRESS   0x70
#define SPEAKER_PIN           8
#define NOTE_C4               262
#define PUSHBUTTON_PIN        2

Adafruit_7segment clockDisplay = Adafruit_7segment();
RTC_DS1307 rtc = RTC_DS1307();

int hours = 0;
int minutes = 0;
int seconds = 0;
int weekday = 0;
int displayValue;

const int weekendAlarm = 930;
const int weekdayAlarm = 845; // 08:45
const int disarmHoldSeconds = 5;
bool snoozing = false;
bool alarmOn = false;
bool blinkColon = false;
int alarmTime = weekdayAlarm;
int snoozeDelayMinutes = 11;
int unSnoozeTime;
volatile bool buttonPressed = false;
int buttonHeldSeconds = 0;
struct snoozeTime {
  int hours;
  int minutes;
} snoozeUntil;

int displayTimeFromHrMin(int hours, int minutes) {
  return hours * 100 + minutes;
}

void setup() {
  interrupts();                             // Enable global interrupts
  pinMode(PUSHBUTTON_PIN, INPUT_PULLUP);    // Enable pullup resistor
  attachInterrupt(
    digitalPinToInterrupt(PUSHBUTTON_PIN),
    pushed,
    FALLING);                               // Trigger INT0 on falling edge
//  Serial.begin(115200);

  clockDisplay.begin(DISPLAY_I2C_ADDRESS);
  clockDisplay.setBrightness(0);
  rtc.begin();

  bool setClockTime = !rtc.isrunning();

  //setClockTime = true;
  if (setClockTime) {
    // Set the DS1307 time to the exact date and time the sketch was compiled:
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void loop() {
  noTone(SPEAKER_PIN);

  if (minutes % SKEW_CORRECT_MINUTES == 0 ) {
    DateTime now = rtc.now();
    hours = now.hour();
    minutes = now.minute();
    //Serial.print(now.dayOfTheWeek());
    weekday = now.dayOfTheWeek();
  }

  if (weekday == 0 or weekday == 6) {
    alarmTime = weekendAlarm;
  } else {
    alarmTime = weekdayAlarm;
  }

//  Serial.print(alarmOn ? 'A' : '_');
//  Serial.print(snoozing ? 'S' : '_');
//  Serial.print(buttonPressed ? 'B' : '_');
//  Serial.println(buttonHeldSeconds);
  displayValue = displayTimeFromHrMin(hours, minutes);

  if (alarmOn) {
    if (buttonPressed) {
      snoozeUntil.hours = hours;
      snoozeUntil.minutes = minutes + snoozeDelayMinutes;
      if (snoozeUntil.minutes > 59) {
        snoozeUntil.hours = (hours + 1) % 24;
        snoozeUntil.minutes = snoozeUntil.minutes - 60;
      }

      unSnoozeTime = displayTimeFromHrMin(snoozeUntil.hours, snoozeUntil.minutes);
      snoozing = true;
      alarmOn = false;
      buttonPressed = false;
    } else {
      if (!buttonPressed) {
        tone(SPEAKER_PIN, NOTE_C4, 250);
      }
    }
  } else {
    buttonPressed = false;
    if (displayValue == alarmTime && !snoozing) { // && !buttonPressed) {
      alarmOn = true;
    }
  }

  clockDisplay.print(displayValue, DEC);

  if (hours == 0) {
    // Pad hour 0.
    clockDisplay.writeDigitNum(0, 0);
    clockDisplay.writeDigitNum(1, 0);
    // Also pad when the 10's minute is 0 and should be padded.
    if (minutes < 10) {
      clockDisplay.writeDigitNum(3, 0);
    }
  }

  // If alarm is on but snoozed, blink the colon @ 1Hz
  if (snoozing) {

    // Check if snooze timer is up. If so, clear button state and reset alarm & snooze states.
    if (displayValue >= unSnoozeTime) {
      alarmOn = true;
      snoozing = false;
      buttonPressed = false;
    }

    blinkColon = !blinkColon;
    clockDisplay.drawColon(blinkColon);
  } else {
    clockDisplay.drawColon(true);
  }

  clockDisplay.writeDisplay();
  delay(1000);

  // Increment button hold count if pressed after the 1s delay
  if (digitalRead(PUSHBUTTON_PIN) == LOW) {
    buttonHeldSeconds += 1;
  } else {
    buttonHeldSeconds = 0;
  }

  if (buttonHeldSeconds > disarmHoldSeconds) {
    buttonHeldSeconds = 0;
    alarmOn = false;
    snoozing = false;
    buttonPressed = false;
  }

  seconds += 1;
  if (seconds > 59) {
    seconds = 0;
    minutes += 1;
    if (minutes > 59) {
      minutes = 0;
      hours += 1;
      if (hours > 23) {
        hours = 0;
      }
    }
  }
}

void pushed() {
  buttonPressed = true;
}
