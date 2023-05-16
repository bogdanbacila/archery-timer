
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <FastLED.h>

// For led chips like Neopixels, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN
#define DATA_PIN_TIMER 5
#define DATA_PIN_DETAIL 6

// How many leds in your strip?
#define NUM_LEDS_TIMER 63
#define NUM_LEDS_DETAIL 37

#define BRIGHTNESS 50 // temporary keep brightness down for testing. Increase after sprting power supply! 

byte ledsInSegment = 3;
byte ledsInDigit = 21;

byte buzzerPin = 8;
bool buzzerState = LOW;
unsigned long previousMillis = 0; 
const int buzzerInterval  = 500;

int receivedTime;
int receivedDetail;
int receivedDigit;
int receivedMod;
int receivedUnder;
int receivedBuzz;

// Define the arrays of leds
CRGB timerLeds[NUM_LEDS_TIMER];
CRGB detailLeds[NUM_LEDS_DETAIL];
CRGB cols[] = {CRGB::Green, CRGB::Orange, CRGB::Red};

RF24 radio(9, 10); // CE, CSN
const byte address[6] = "00001";

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

bool detailTemplate[6][NUM_LEDS_DETAIL]=
{
  { //AB
    0, 1, 1, 0, 0, 0, 0, 0, 1, 1,  0,  1,  1,  0,  0,  0,  0,  0,  1,  1,  0,  1,  1,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1
 // 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36
  },
  { //CD
    0, 1, 1, 0, 0, 0, 0, 0, 1, 1,  0,  0,  0,  0,  0,  1,  1,  0,  1,  1,  0,  1,  1,  1,  1,  0,  0,  1,  1,  0,  0,  0,  0,  1,  1,  1,  1
 // 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36
  }, 
  { //EF
    0, 1, 1, 0, 1, 1, 0, 0, 1, 1,  0,  1,  1,  0,  0,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  0,  0,  0,  0,  1,  1,  1,  1
 // 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36
  },  
  { //d1
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  1,  1,  0,  0,  1,  1,  0,  0,  0,  0,  1,  1,  1,  1,  0,  0,  0,  0,  1,  1,  1,  1,  0,  0,  1,  1
 // 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36
  },
  { //d2
    0, 0, 0, 0, 1, 1, 0, 0, 1, 1,  0,  1,  1,  0,  0,  1,  1,  0,  1,  1,  0,  0,  0,  1,  1,  0,  0,  1,  1,  1,  1,  1,  1,  0,  0,  1,  1
 // 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36
  },
  { //d3
    0, 0, 0, 0, 1, 1, 0, 0, 1, 1,  0,  1,  1,  0,  0,  1,  1,  0,  1,  1,  0,  1,  1,  1,  1,  0,  0,  0,  0,  1,  1,  1,  1,  0,  0,  1,  1
 // 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36
  },
};

bool underline[7] = {0, 0, 0, 0, 0, 1, 0};

void setup() { 
delay(1000);
  Serial.begin(9600); 
//  while (!Serial);

  
  FastLED.addLeds<WS2812B, DATA_PIN_TIMER, GRB>(timerLeds, NUM_LEDS_TIMER); 
  FastLED.addLeds<WS2812B, DATA_PIN_DETAIL, GRB>(detailLeds, NUM_LEDS_DETAIL); 

  FastLED.setBrightness(BRIGHTNESS);
 	
  clearAll();
  
  radio.begin();
  radio.openReadingPipe(0, address);   //Setting the address at which we will receive the data
  radio.setPALevel(RF24_PA_MIN);       //You can set this as minimum or maximum depending on the distance between the transmitter and receiver.
  radio.startListening();    

  writeTime(0, pickColour(0));
  writeDetail(0, CRGB::Blue);

  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, buzzerState);

  soundBuzzer(3);
}

void loop() { 

if (radio.available()) {
    char incomingMessage[32] = "";                 //Saving the incoming data
    radio.read(&incomingMessage, sizeof(incomingMessage));    //Reading the data
    Serial.println(incomingMessage);

    if (incomingMessage[1] == 't'){
      receivedTime = atoi(strtok(incomingMessage, "/t/"));
      Serial.println(receivedTime);
      writeTime(receivedTime, pickColour(receivedTime));
    }

    if (incomingMessage[1] == 'd'){
      receivedDetail = atoi(strtok(incomingMessage, "/d/"));
      Serial.println(receivedDetail);
      if (receivedDetail <= 2){
        writeDetail(receivedDetail, CRGB::Blue);
      }else if (receivedDetail >2){
        writeDetail(receivedDetail, CRGB::Purple);
      }
    }

    if (incomingMessage[1] == 's'){
      receivedDigit = atoi(strtok(incomingMessage, "/s/"));
      Serial.println(receivedDigit);
      writeDigit(receivedDigit%10, CRGB::Blue, receivedDigit/10);
      
    }

    if (incomingMessage[1] == 'm'){
      receivedMod = atoi(strtok(incomingMessage, "/m/"));
      Serial.println(receivedMod);
      writeTime(receivedMod, CRGB::Purple);
    }

    
    if (incomingMessage[1] == 'u'){
      receivedUnder = atoi(strtok(incomingMessage, "/u/"));
      Serial.println(receivedUnder);
      writeUnderline(receivedUnder, CRGB::Red);
    }
    
    if (incomingMessage[1] == 'x'){
      
      setAll(0, CRGB::Red);
    }

    if (incomingMessage[1] == 'b'){
      receivedBuzz = atoi(strtok(incomingMessage, "/b/"));
      Serial.println(receivedBuzz);
      soundBuzzer(receivedBuzz);
    }
    
  }

}


void writeDigit(byte number, CRGB colour, byte digit){
  for (byte i = 0; i < ledsInDigit ; i++){      
    timerLeds[i+digit*ledsInDigit] = (digitTemplate[number][(i/ledsInSegment)]) ? colour : CRGB::Black;
  }
  FastLED.show();
}

void writeUnderline(byte digit, CRGB colour){
  for (byte i = 0; i < ledsInDigit ; i++){ 
    if (underline[(i/ledsInSegment)]){
      timerLeds[i+digit*ledsInDigit] = colour;  
    }
         
  }
  FastLED.show();
}

void writeTime(int timeInSeconds, CRGB colour){
  writeDigit(timeInSeconds/100, colour, 0);
  writeDigit((timeInSeconds/10)%10, colour, 1);
  writeDigit(timeInSeconds%10, colour, 2);
}


void writeDetail(byte number, CRGB colour){
  for (byte i = 0; i < NUM_LEDS_DETAIL ; i++){      
    detailLeds[i] = (detailTemplate[number][i]) ? colour : CRGB::Black;
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
      timerLeds[i+digit*ledsInDigit] =  CRGB::Black;
  }
  FastLED.show();
}

void clearAll(){
  for (byte i=0; i<3; i++){
    clearDigit(i);
  }
}

CRGB pickColour(int seconds){
  if (seconds > 10){
    return CRGB::Green;
  }else if ((seconds <= 10) && (seconds != 0)){
    return CRGB::Orange;
  }else if (seconds == 0){
    return CRGB::Red;  
  }
}

void soundBuzzer(byte times){
  times = times*2;
  while (times){
    
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= buzzerInterval) {
      // save the last time you blinked the LED
      previousMillis = currentMillis;
      
      if (buzzerState == LOW) {
        buzzerState = HIGH;
      } else {
        buzzerState = LOW;
      }

      // set the LED with the ledState of the variable:
    digitalWrite(buzzerPin, buzzerState);
    times--;
    }
  }   
}
