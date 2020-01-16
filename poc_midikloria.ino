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
#define MAX_ANALOG_VALUE 714.0f  // choosen by experiments with sensors
#define DEFAULT_CHANNEL 1

int potarValue, sliderValue;
int lastpotarValue = -1, lastsliderValue = -1;
float potarRatio, sliderRatio;
bool button0Value, button1Value, button2Value;

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
  potarValue = analogRead(POTAR_PIN);
  sliderValue = analogRead(SLIDER_PIN);
  potarRatio = potarValue / MAX_ANALOG_VALUE;
  sliderRatio = sliderValue / MAX_ANALOG_VALUE;
  
  // send midi control change if one of the analog value changed
  if (potarValue != lastpotarValue){
    lastpotarValue = potarValue;
    MIDI.sendControlChange(1, (byte)(potarRatio*127), DEFAULT_CHANNEL);
  }
  if (sliderValue != lastsliderValue){
    lastsliderValue = sliderValue;
    MIDI.sendControlChange(2, (byte)(sliderValue*127), DEFAULT_CHANNEL);
  }
  
  // set pwm duty cycle of led 2 according to potar ratio
  analogWrite(LED2_PIN, (byte) (potarRatio*255));
  
  // get digital values buttons
  button0Value = digitalRead(BUTTON0_PIN);
  button1Value = digitalRead(BUTTON1_PIN);
  button2Value = digitalRead(BUTTON2_PIN);

  // toggle led 0 every loop
  digitalWrite(LED0_PIN, !digitalRead(LED0_PIN));
  
  // print values
//  Serial.println(potarValue / MAX_ANALOG_VALUE);
//  Serial.println(potarRatio*255);

  
  // wait
  delay(50);  
   

}
