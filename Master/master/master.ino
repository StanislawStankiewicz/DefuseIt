#include <SevSeg.h>
#include <TM1637Display.h>

#define BUZZER_PIN 6
#define TIME 5 * 60 * 1000

SevSeg sevseg;  
TM1637Display tm1637(12, 13);

float version;
long timerEnd;  
bool timerRunning = true;
bool finalTonePlayed = false;
long lastTimerUpdate = 0;

float generateVersion() {
  return random(1, 100) / 10.0;
}

void updateTimer() {
  long currentMillis = millis();

  if (timerRunning && currentMillis - lastTimerUpdate >= 1000) {  
    lastTimerUpdate = currentMillis;
    
    long timeLeft = (timerEnd - currentMillis) / 1000;
    if (timeLeft < 0) {
      timeLeft = 0;
      timerRunning = false;
    }

    int minutes = timeLeft / 60;
    int seconds = timeLeft % 60;

    uint16_t displayTime = (minutes * 100) + seconds;
    tm1637.setSegments(NULL, 0);  
    tm1637.showNumberDecEx(displayTime, 0x40, true);

    if (timerRunning) { 
      tone(BUZZER_PIN, 600, 50);
    }
  }
  
  if (!timerRunning && !finalTonePlayed) {
    tone(BUZZER_PIN, 600, 2000);
    finalTonePlayed = true;
  }
}

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  
  randomSeed(analogRead(A0));  
  version = generateVersion();  

  byte numDigits = 2;
  byte digitPins[] = {3, 4};  
  byte segmentPins[] = {A5, 2, A0, A3, A2, 5, A4, A1};  

  sevseg.begin(COMMON_ANODE, numDigits, digitPins, segmentPins, false);
  sevseg.setBrightness(90);

  tm1637.setBrightness(5);  
  timerEnd = millis() + (TIME);  
}

void loop() {
  sevseg.setNumberF(version, 1);  
  sevseg.refreshDisplay();  

  updateTimer();  
}
