// These define's must be placed at the beginning before #include "TimerInterrupt.h"
// _TIMERINTERRUPT_LOGLEVEL_ from 0 to 4
// Don't define _TIMERINTERRUPT_LOGLEVEL_ > 0. Only for special ISR debugging only. Can hang the system.
#define TIMER_INTERRUPT_DEBUG         2
#define _TIMERINTERRUPT_LOGLEVEL_     0

#define USE_TIMER_1     true

#define TIMER1_INTERVAL_MS    1000
#define TIMER1_FREQUENCY      (float) (1000.0f / TIMER1_INTERVAL_MS)

#include "TimerInterrupt.h"
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
RF24 radio(9, 10); // CE, CSN       

const byte address[6] = "00001";     //Byte of array representing the address. This is the address where we will send the data. This should be same on the receiving side.


volatile int globalSeconds = 0;
volatile int interruptFlag = 0;
volatile int countdownStoppedFlag = 0;
int remainingSeconds = 0;
int maxSeconds = 0;
int preCountSeconds = 10;
volatile bool preCountComplete = false;
bool toggle = 0;
bool setTimeMode = false;

bool setModeFlag = 0;
byte setDigitCount = 0;
byte setDigitValue = 0;
byte setDetail = 0;

char savedTimeValues[3] = {2, 4, 0};
char savedPrepareTime[3] = {0, 1, 0};
char provisionalTimeValues[3] = {0, 0, 0};

volatile char stopChr[6] = "/t/0";
volatile char startBuzz[6] = "/b/1";
volatile char prepareBuzz[6] = "/b/2";
volatile char stopBuzz[6] = "/b/3";
volatile char fastBuzz[6] = "/b/5";

char detailChr[4];
char detail = 0;
byte digitToChange = 0;
byte noOfDetails = 2;
byte detailCount = 0;

byte buttons[] = {4,5,6,7,8}; // pin numbers of the buttons that we'll use
#define NUMBUTTONS sizeof(buttons)
int buttonState[NUMBUTTONS];
int lastButtonState[NUMBUTTONS];
boolean buttonIsPressed[NUMBUTTONS];  
// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
long lastDebounceTime = 0; // the last time the output pin was toggled
long debounceDelay = 50; // the debounce time; increase if the output flickers


void setup() {
  
  Serial.begin(9600); 

//  Serial.print(F("\nStarting TimerInterruptTest on "));
//  Serial.println(BOARD_TYPE);
//  Serial.println(TIMER_INTERRUPT_VERSION);
//  Serial.print(F("CPU Frequency = ")); Serial.print(F_CPU / 1000000); Serial.println(F(" MHz"));

  ITimer1.init();
  if (ITimer1.attachInterruptInterval(TIMER1_INTERVAL_MS, ISR_TimerHandler1))
  {
    Serial.print(F("Starting  ITimer1 OK, millis() = ")); Serial.println(millis());
  }
  else
    Serial.println(F("Can't set ITimer1. Select another freq. or timer"));

  stopCountdown();
 
  radio.begin();                  //Starting the Wireless communication
  radio.openWritingPipe(address); //Setting the address where we will send the data
  radio.setPALevel(RF24_PA_MIN);  //You can set it as minimum or maximum depending on the distance between the transmitter and receiver.
  radio.stopListening();          //This sets the module as transmitter

  // define pins:
  for (int i=0; i<NUMBUTTONS; i++) {
    pinMode(buttons[i],INPUT_PULLUP);
    lastButtonState[i]=HIGH;
    buttonIsPressed[i]=false; 
  }

  memcpy(provisionalTimeValues,savedTimeValues, sizeof provisionalTimeValues);

}
  
void loop(){
    
  check_buttons();
  action();

  if (interruptFlag){    
    manageCountdown(globalSeconds);
    interruptFlag = 0;
  }

}

void check_buttons() {
  for (int currentButton=0; currentButton<(NUMBUTTONS); currentButton++) {
    // read the state of the switch into a local variable:
    int reading = digitalRead(buttons[currentButton]); // check to see if you just pressed the button and you've waited long enough since the last press to ignore any noise:
    // If the switch changed, due to noise or pressing then reset the debouncing timer
    if (reading != lastButtonState[currentButton]) { lastDebounceTime = millis(); }
    // whatever the reading is at, it's been there for longer than the debounce delay, so take it as the actual current state:
    if ((millis() - lastDebounceTime) > debounceDelay) {
      // if the button state has changed:
      if (reading != buttonState[currentButton]) {
        buttonState[currentButton] = reading;
        if (buttonState[currentButton]==LOW) { //pushing down on the button
          buttonIsPressed[currentButton]=true; // set your flag for the adjustment function
        }
      }
    }
    // save the reading.  Next time through the loop, it'll be the lastButtonState:
    lastButtonState[currentButton] = reading;
  }
}

void action() {
  for (int currentButton=0; currentButton<(NUMBUTTONS); currentButton++) {
    if (buttonIsPressed[currentButton]) {
//      Serial.print("button "); Serial.println(buttons[currentButton]);
      if (buttons[currentButton]==4) { // -------- Button 4 controls action A
        // do action A stuff here

        char setModeChr[6] = "/s/";
        char outDigitChr[6] = "";
        
        if (setTimeMode){

          if (digitToChange <3){
            provisionalTimeValues[digitToChange]++;
            if (provisionalTimeValues[digitToChange] == 10){
              provisionalTimeValues[digitToChange] = 0;
            }
               
            itoa(provisionalTimeValues[digitToChange]+digitToChange*10, outDigitChr, 10);
            strcat(setModeChr, outDigitChr);
            radio.write(&setModeChr, sizeof(setModeChr));  //Sending the message to receiver 
          } else if (digitToChange ==3){
            noOfDetails++;
            if (noOfDetails == 4){
              noOfDetails = 1;
            }
            sendDetail(noOfDetails+2); 
          }
        }else {
          if (countdownStoppedFlag){
            changeDetail("inc");
          }else{
            if (detailCount >= noOfDetails-1){
              stopDetail();
            }else{
              skipDetail();
            }
          }
        }
        
      } else if (buttons[currentButton]==5) { // -------- Button 5 controls action B
        // do action B stuff here


        stopDetail();
//        stopCountdown();
//        radio.write(&stopChr, sizeof(stopChr));  //Sending the message to receiver 
//        radio.write(&stopBuzz, sizeof(stopBuzz));  //Sending the message to receiver 
        
        
      } else if (buttons[currentButton]==6) { // -------- Button 6 controls action C
        // do action C stuff here

        char setModeChr[6] = "/s/";
        char outDigitChr[6] = "";
        
        if (setTimeMode){

          if (digitToChange <3){
            provisionalTimeValues[digitToChange]--;
            if (provisionalTimeValues[digitToChange] == -1){
              provisionalTimeValues[digitToChange] = 9;
            }
               
            itoa(provisionalTimeValues[digitToChange]+digitToChange*10, outDigitChr, 10);
            strcat(setModeChr, outDigitChr);
            radio.write(&setModeChr, sizeof(setModeChr));  //Sending the message to receiver 
          }else if (digitToChange ==3){
            noOfDetails--;
            if (noOfDetails == 0){
              noOfDetails = 3;
            }
            sendDetail(noOfDetails+2); 
          }
        }else {
          if (countdownStoppedFlag){
            changeDetail("dec");      
          }else{
                          
          }
        }
      } else if (buttons[currentButton]==7) { // -------- Button 7 controls action D
        // do action D stuff here

        toggleCountdown(savedTimeValues[0]*100+savedTimeValues[1]*10+savedTimeValues[2]);
   
      } else if (buttons[currentButton]==8) { // -------- Button 8 controls action E
        // do action E stuff here
        char setModeChr[6] = "/m/";
        char outDigitChr[6] = "";
        char setUnderChr[6] = "/u/";
        char outUnderChr[6] = "";
        
        if (!setTimeMode){
          
          setTimeMode = true;  

          itoa(provisionalTimeValues[2]+provisionalTimeValues[1]*10+provisionalTimeValues[0]*100, outDigitChr, 10);
          strcat(setModeChr, outDigitChr);
          radio.write(&setModeChr, sizeof(setModeChr));  //Sending the message to receiver 
  
          itoa(digitToChange, outUnderChr, 10);
          strcat(setUnderChr, outUnderChr);
          radio.write(&setUnderChr, sizeof(setUnderChr));  //Sending the message to receiver 
          
        } else {
          digitToChange++;
          if (digitToChange == 3){
            saveTimer();
            sendDetail(noOfDetails+2);
          }else if (digitToChange <3){
            
            itoa(digitToChange, outUnderChr, 10);
            strcat(setUnderChr, outUnderChr);
            radio.write(&setUnderChr, sizeof(setUnderChr));  //Sending the message to receiver 
          
          }else if (digitToChange == 4){
            detail = 0;
            sendDetail(detail);
            digitToChange = 0; 
            setTimeMode = false;
          }
        }

        
      }
      buttonIsPressed[currentButton]=false; //reset the button
    }
  }
}


void saveTimer(){
  char saveChr[6] = "/x/";
  bool setTimeZero = true;
  for (byte i = 0; i <= 2; i++){
    if (provisionalTimeValues[i] != 0){
      setTimeZero = false;
      break;
    }
    
  }

  if (!setTimeZero) {
    memcpy(savedTimeValues, provisionalTimeValues, sizeof savedTimeValues);
    radio.write(&saveChr, sizeof(saveChr));  //Sending the message to receiver 
  }
  
}


void ISR_TimerHandler1()
{
  globalSeconds++;
  interruptFlag = 1;
}


void manageCountdown(int currentSeconds){ 
  char startChr[7] = "/t/";
  char tmpChr[4] = "";
  char timeChr[3];
  char outChr[6] = "/t/";


  
  if (!preCountComplete){
    remainingSeconds = preCountSeconds - currentSeconds;

  } else {
    remainingSeconds = maxSeconds - currentSeconds;  

  }
   
  itoa(remainingSeconds, timeChr, 10);
  strcat(outChr, timeChr);

  if (remainingSeconds == 0){
 
    if (preCountComplete){
      if (detailCount >= noOfDetails-1){
        stopDetail();
      }else{
        skipDetail();
      }
      
    } else {   
      resetTimer();
      preCountComplete = true;
      itoa(maxSeconds, tmpChr, 10);
      strcat(startChr, tmpChr);
      radio.write(&startChr, sizeof(startChr));  //Sending the message to receiver 
      radio.write(&startBuzz, sizeof(startBuzz));  //Sending the message to receiver 
    }
    
  }else{

    radio.write(&outChr, sizeof(outChr));  //Sending the message to receiver 

//    byte minutes = remainingSeconds * 0.01666667;
//    byte seconds = (static_cast<float>(remainingSeconds) * 0.01666667 - minutes) *  60;
//    Serial.println(remainingSeconds);
     
//    Serial.print(":");
//    Serial.print(seconds/10);
//    Serial.println(seconds%10);

  }

}

void resetTimer(){
  globalSeconds = 0;
}

void toggleCountdown(int inSeconds){
  char startChr[7] = "/t/";
  char tmpChr[4] = "";
  if(countdownStoppedFlag){
    maxSeconds = inSeconds;
    resetTimer();
    countdownStoppedFlag = 0;
    itoa(preCountSeconds, tmpChr, 10);
    strcat(startChr, tmpChr);
    radio.write(&startChr, sizeof(startChr));  //Sending the message to receiver 
    radio.write(&prepareBuzz, sizeof(prepareBuzz));  //Sending the message to receiver 
    ITimer1.resumeTimer();
    toggle = 0;
  } else {
    switch(toggle){
      case 0:
        ITimer1.pauseTimer();
        radio.write(&fastBuzz, sizeof(fastBuzz));  //Sending the message to receiver 
        break;
      case 1:
        ITimer1.resumeTimer();
        radio.write(&startBuzz, sizeof(startBuzz));  //Sending the message to receiver 
        break;
      default:
      break;
    }
    toggle=!toggle;

  }
}

void sendDetail(byte detail){
  char detailChr[6] = "/d/";
  char outDetail[3] = "";

  itoa(detail, outDetail, 10);
  strcat(detailChr, outDetail);        
  radio.write(&detailChr, sizeof(detailChr));  //Sending the message to receiver 
  Serial.println(detailChr);

}

void changeDetail(String incdec){

//  char detailChr[6] = "/d/";
//  char outDetail[3] = "";

  if (incdec == "dec") {
    detail--;
    if (detail == -1){
      detail = noOfDetails-1;
    }
  }else if (incdec == "inc"){
    detail++;
    if (detail == noOfDetails){
      detail = 0;
    }
  }

  sendDetail(detail);
//  itoa(detail, outDetail, 10);
//  strcat(detailChr, outDetail);        
//  radio.write(&detailChr, sizeof(detailChr));  //Sending the message to receiver 
}

void skipDetail(){
  changeDetail("inc");
  detailCount++;
  stopCountdown();
  toggleCountdown(savedTimeValues[0]*100+savedTimeValues[1]*10+savedTimeValues[2]);
}

void stopDetail(){
  stopCountdown();
  radio.write(&stopChr, sizeof(stopChr));  //Sending the message to receiver 
  radio.write(&stopBuzz, sizeof(stopBuzz));  //Sending the message to receiver   
  detailCount = 0;
  for (byte i = 0; i< noOfDetails - 2 ; i++){
    changeDetail("dec");
  }
}

void stopCountdown(){
  ITimer1.pauseTimer(); 
  resetTimer();
  countdownStoppedFlag = 1;
  preCountComplete = false;
}

void pauseCountdown(){
  ITimer1.pauseTimer(); 
}

void resumeCountdown(){
  ITimer1.resumeTimer(); 
}
