#include <MIDI.h>


// symbolic pin names
#define POTAR_PIN A0
#define SLIDER_PIN A1
#define BUTTON0_PIN 10
#define BUTTON1_PIN 9
#define BUTTON2_PIN 8
#define LED0_PIN 13
#define LED1_PIN 12
#define LED2_PIN 11

// project related defines
#define DEFAULT_CHANNEL 1
#define DEFAULT_VELOCITY 127
#define MAX_ANALOG_VALUE 714.0f  // choosen by experiments with sensors
#define AVG_SAMPLES_NUM 3  // number of samples used in the moving average
#define ANALOG_DIFF_THRES 1  // difference threshold between last and current value until a MIDI msg is sent
//#define BUTTON_NOTE_MODE

// project variables
int potarValue, sliderValue;
int potarSum = 0, sliderSum = 0;
int potarAvg, lastpotarAvg, sliderAvg, lastsliderAvg;
int avgPos = 0;
int potarArray[AVG_SAMPLES_NUM] = {0}, sliderArray[AVG_SAMPLES_NUM] = {0};

bool button0Value, button1Value, button2Value;
bool lastbutton0Value = 1, lastbutton1Value = 1, lastbutton2Value = 1;
byte i;

// functions
int movingAvg(int *numArray, int arraySize, int *sum, int nextNum, int pos){
  // Subtract the oldest number from the prev sum, add the new number
  *sum = *sum - numArray[pos] + nextNum;
  
  // Assign the nextNum to the position in the array
  numArray[pos] = nextNum;

  // return the average
  return *sum / arraySize;
}

// Create and binds the MIDI interface to the default hardware Serial port
MIDI_CREATE_DEFAULT_INSTANCE();

void setup() {
  // initialize serial and midi handler:
  MIDI.begin(MIDI_CHANNEL_OMNI);
  Serial.begin(115200);
  
  // configure button pins as as pull up input
  pinMode(BUTTON0_PIN, INPUT_PULLUP);
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);

  // configure leds pin as output
  pinMode(LED0_PIN, OUTPUT);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);

}

void loop() {
  // get analog values
  potarValue = (analogRead(POTAR_PIN) / MAX_ANALOG_VALUE)*127;
  sliderValue = (analogRead(SLIDER_PIN) / MAX_ANALOG_VALUE)*127;

  // moving average off these values
  potarAvg = movingAvg(potarArray, AVG_SAMPLES_NUM, &potarSum, potarValue, avgPos);
  sliderAvg = movingAvg(sliderArray, AVG_SAMPLES_NUM, &sliderSum, sliderValue, avgPos);

  // increase moving average positon for the next loop
  avgPos++;
  if (avgPos >= AVG_SAMPLES_NUM) avgPos = 0;
  
  // send midi control change if one of the analog value changed
  if (abs(potarAvg - lastpotarAvg) > ANALOG_DIFF_THRES){
    lastpotarAvg = potarAvg;
    MIDI.sendControlChange(1, potarAvg, DEFAULT_CHANNEL);
  }
  if (abs(sliderAvg - lastsliderAvg) > ANALOG_DIFF_THRES){
    lastsliderAvg = sliderAvg;
    MIDI.sendControlChange(2, sliderAvg, DEFAULT_CHANNEL);
  }
  
  // set pwm duty cycle of led 2 according to potar value
  analogWrite(LED2_PIN, potarAvg*2); 
  
  // get digital values buttons
  button0Value = digitalRead(BUTTON0_PIN);
  button1Value = digitalRead(BUTTON1_PIN);
  button2Value = digitalRead(BUTTON2_PIN);

  // 
  #ifdef BUTTON_NOTE_MODE  // send note msg
  if (button0Value != lastbutton0Value){
    if (!button0Value) MIDI.sendNoteOn(50, DEFAULT_VELOCITY, DEFAULT_CHANNEL);
    else MIDI.sendNoteOff(50, DEFAULT_VELOCITY, DEFAULT_CHANNEL);
    lastbutton0Value =  button0Value;  
  }
  #else  // send CC msg
  if ((button0Value != lastbutton0Value) && !button0Value){
    if (i%2) MIDI.sendControlChange(3, 127, DEFAULT_CHANNEL);
    else MIDI.sendControlChange(3, 0, DEFAULT_CHANNEL);
    i++; 
  }
  lastbutton0Value =  button0Value; 
  #endif
  
  // toggle led 0 every loop
  digitalWrite(LED0_PIN, !digitalRead(LED0_PIN));
  
  // print values
//  Serial.println(i);
//  Serial.println(potarRatio*255);
  
  
  // wait
  delay(50);  
   

}
