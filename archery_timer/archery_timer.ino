
// These define's must be placed at the beginning before #include "TimerInterrupt.h"
// _TIMERINTERRUPT_LOGLEVEL_ from 0 to 4
// Don't define _TIMERINTERRUPT_LOGLEVEL_ > 0. Only for special ISR debugging only. Can hang the system.
#define TIMER_INTERRUPT_DEBUG         2
#define _TIMERINTERRUPT_LOGLEVEL_     0

#define USE_TIMER_1     true

#define TIMER1_INTERVAL_MS    1000
#define TIMER1_FREQUENCY      (float) (1000.0f / TIMER1_INTERVAL_MS)

#include "TimerInterrupt.h"
#include <RCSwitch.h>
#include <FastLED.h>

// For led chips like Neopixels, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN
#define DATA_PIN 5

// How many leds in your strip?
#define NUM_LEDS 63
#define BRIGHTNESS 20 // temporary keep brightness down for testing. Increase after sprting power supply! 

byte ledsInSegment = 3;
byte ledsInDigit = 21;

volatile int globalSeconds = 0;
volatile int interruptFlag = 0;
volatile int countdownStoppedFlag = 0;
//volatile byte minutes = 0;
//volatile byte seconds = 0;

int maxSeconds = 0;
unsigned long prevMsA = 0;
unsigned long prevMsB = 0;
unsigned long prevMsC = 0;
unsigned long prevMsD = 0;
int buttonA = 11467944;
int buttonB = 11467892;
int buttonC = 11467890;
int buttonD = 11467889;
bool toggleA = 0;
bool toggleB = 0;
int pressedButton = 0;

bool setModeFlag = 0;
byte setDigitCount = 0;
byte setDigitValue = 0;
byte savedTimeValues[3] = {0, 3, 0};

// Define the array of leds
CRGB leds[NUM_LEDS];
CRGB cols[] = {CRGB::Green, CRGB::Orange, CRGB::Red};

RCSwitch mySwitch = RCSwitch();

bool digitTemplate[10][7]=
{
  { //0
    0, 1, 1, 1, 1, 1, 1
  },
  { //1
    0, 1, 0, 0, 0, 0, 1
  }, 
  { //2
    1, 1, 1, 0, 1, 1, 0
  },  
  { //3
    1, 1, 1, 0, 0, 1, 1
  },
  { //4
    1, 1, 0, 1, 0, 0, 1
  },
  { //5
    1, 0, 1, 1, 0, 1, 1
  }, 
  { //6
    1, 0, 1, 1, 1, 1, 1
  },
  { //7
    0, 1, 1, 0, 0, 0, 1
  },
  { //8
    1, 1, 1, 1, 1, 1, 1
  }, 
  { //9
    1, 1, 1, 1, 0, 1, 1
  },
};



void setup() { 
delay(1000);
  Serial.begin(115200); 
//  while (!Serial);

  Serial.print(F("\nStarting TimerInterruptTest on "));
  Serial.println(BOARD_TYPE);
  Serial.println(TIMER_INTERRUPT_VERSION);
  Serial.print(F("CPU Frequency = ")); Serial.print(F_CPU / 1000000); Serial.println(F(" MHz"));
  
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS); 
  FastLED.setBrightness(BRIGHTNESS);
 	
  clearAll();
  
  ITimer1.init();
  
  if (ITimer1.attachInterruptInterval(TIMER1_INTERVAL_MS, ISR_TimerHandler1))
  {
    Serial.print(F("Starting  ITimer1 OK, millis() = ")); Serial.println(millis());
  }
  else
    Serial.println(F("Can't set ITimer1. Select another freq. or timer"));

  stopCountdown();
  
  mySwitch.enableReceive(0);  // Receiver on interrupt 0 => that is pin #2
   
    
}

void loop() { 

  if (mySwitch.available()) {
    pressedButton = mySwitch.getReceivedValue();
    
    if (pressedButton == buttonA && millis()-prevMsA>200){ 
      Serial.println("ButtonA"); 
      switch(toggleA){
        case 0:
          startCountdown(savedTimeValues[0]*60+savedTimeValues[1]*10+savedTimeValues[2]);
          break;
        case 1:
          stopCountdown();
          break;
        default:
          break;
      }
      toggleA = !toggleA;
      prevMsA = millis();
    }else prevMsA = millis();

    if (pressedButton == buttonB && millis()-prevMsB>200){
      Serial.println("ButtonB");
      switch(toggleB){
        case 0:
          pauseCountdown();
          break;
        case 1:
          resumeCountdown();
          break;
        default:
          break;
      }
      toggleB = !toggleB;
      prevMsB = millis();
    }else prevMsB = millis();
    
    if (pressedButton == buttonC && millis()-prevMsC>200){
      Serial.println("ButtonC");

      if (setModeFlag){
        if (setDigitValue == 9){
          setDigitValue = 0;
          writeDigit(setDigitValue, CRGB::Blue, setDigitCount);
        }else{
          setDigitValue++; 
          writeDigit(setDigitValue, CRGB::Blue, setDigitCount);  
        }
        
      }
      prevMsC = millis();
    }else prevMsC = millis();
    
    if (pressedButton == buttonD && millis()-prevMsD>200){
      Serial.println("ButtonD");
      
      if (!setModeFlag){
        setTimerMode();
      }else{
//        Serial.println(setDigitCount);
        if (setDigitCount == 2){
          
          savedTimeValues[setDigitCount] = setDigitValue;
          setDigitCount = 0;
          writeDigit(0, CRGB::Blue, setDigitCount);
          saveTimer();
        }else{
          
          savedTimeValues[setDigitCount] = setDigitValue;
          setDigitCount++;
          setDigitValue = 0;
          writeDigit(0, CRGB::Blue, setDigitCount);
        }

          
      }
      prevMsD = millis();
    }else prevMsD = millis(); 
 
    mySwitch.resetAvailable();
  }
 
  
  if (interruptFlag){
      
    manageCountdown(globalSeconds);
    interruptFlag = 0;
  }

}


void setTimerMode(){
  setAll(0, CRGB::Purple);
  writeDigit(0, CRGB::Blue, 0);

  setDigitValue = 0;
  setDigitCount = 0;
  setModeFlag = 1; 
}

void saveTimer(){
  setAll(0, CRGB::Red);
  setModeFlag = 0; 
}



void writeDigit(byte number, CRGB colour, byte digit){
  for (byte i = 0; i < ledsInDigit ; i++){      
    leds[i+digit*ledsInDigit] = (digitTemplate[number][(i/ledsInSegment)]) ? colour : CRGB::Black;
  }
  FastLED.show();
}

void setAll(byte number, CRGB colour){
   for (byte i=0; i<3; i++){
    writeDigit(number, colour, i);
  }
}

void clearDigit(byte digit){
  for (byte i = 0; i < ledsInDigit ; i++){      
      leds[i+digit*ledsInDigit] =  CRGB::Black;
  }
  FastLED.show();
}

void clearAll(){
  for (byte i=0; i<3; i++){
    clearDigit(i);
  }
}

CRGB pickColour(int seconds){
  if (seconds > 20){
    return CRGB::Green;
  }else if ((seconds <= 20) && (seconds != 0)){
    return CRGB::Orange;
  }else if (seconds == 0){
    return CRGB::Red;  
  }
}

void ISR_TimerHandler1()
{
  globalSeconds++;
  //manageCountdown(globalSeconds);
  interruptFlag = 1;
}


void manageCountdown(int currentSeconds){ 
  int remainingSeconds = maxSeconds - currentSeconds;  

  if (remainingSeconds == 0){
    stopCountdown();
  }
  else{
    byte minutes = remainingSeconds * 0.01666667;
    byte seconds = (static_cast<float>(remainingSeconds) * 0.01666667 - minutes) *  60;
  //  Serial.print(minutes);
  //  Serial.print(":");
  //  Serial.print(seconds/10);
  //  Serial.println(seconds%10);
    writeDigit(minutes, pickColour(remainingSeconds), 0);
    writeDigit(seconds/10, pickColour(remainingSeconds), 1);
    writeDigit(seconds%10, pickColour(remainingSeconds), 2);
  }


}

void resetTimer(){
  globalSeconds = 0;
}

void startCountdown(byte inSeconds){
  maxSeconds = inSeconds;
  resetTimer();
  countdownStoppedFlag = 0;
  ITimer1.resumeTimer();
}

void stopCountdown(){
  ITimer1.pauseTimer(); 
  resetTimer();
  countdownStoppedFlag = 1;
  writeDigit(0, pickColour(0), 0);
  writeDigit(0, pickColour(0), 1);
  writeDigit(0, pickColour(0), 2);
}

void pauseCountdown(){
  ITimer1.pauseTimer(); 
}

void resumeCountdown(){
  ITimer1.resumeTimer(); 
}
