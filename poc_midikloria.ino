#include <MIDI.h>

// Create and binds the MIDI interface to the default hardware Serial port
MIDI_CREATE_DEFAULT_INSTANCE();

/** symbolic pin names **/
#define POTAR_PIN A0
#define SLIDER_PIN A1
#define LED0_PIN 13
#define LED1_PIN 12
#define LED2_PIN 11

/** project related defines **/
#define DEFAULT_CHANNEL 1
#define DEFAULT_VELOCITY 127
#define BUTTONS_NUM 3
#define BUTTONS_CC_OFFSET 15  // control change start id for buttons
#define BUTTONS_NOTE_OFFSET 50  // note start id for buttons

#define MAX_ANALOG_VALUE 714.0f  // choosen by experiments with sensors
#define AVG_SAMPLES_NUM 3  // number of samples used in the moving average
#define ANALOG_DIFF_THRES 1  // difference threshold between last and current value until a MIDI msg is sent
//#define BUTTON_NOTE_MODE  // buttons will send notes msgs. comment this if you want CC msgs. 



/** pot variables **/
int potarValue, sliderValue;
int potarSum = 0, sliderSum = 0;
int potarAvg, lastpotarAvg, sliderAvg, lastsliderAvg;
int avgPos = 0;
int potarArray[AVG_SAMPLES_NUM] = {0}, sliderArray[AVG_SAMPLES_NUM] = {0};

/** button variables (use a structure !!!!!!!!!) **/
byte buttonsPins[BUTTONS_NUM] = {10, 9, 8};  // buttons pins on the nano board
bool buttonsStates[BUTTONS_NUM] = {1};  // save last button state to detect button push and release
bool buttonsValues[BUTTONS_NUM] = {1}; // last value * 127 sent in the MIDI msg when the button is pressed in CC mode

/** functions **/ 
int movingAvg(int *numArray, int arraySize, int *sum, int nextNum, int pos){
	// Subtract the oldest number from the prev sum, add the new number
	*sum = *sum - numArray[pos] + nextNum;

	// Assign the nextNum to the position in the array
	numArray[pos] = nextNum;

	// return the average
	return *sum / arraySize;
}

void updateButton(bool *states, bool *values, byte *pins, byte id){
	bool buttonState = digitalRead(pins[id]);
	
	#ifdef BUTTON_NOTE_MODE  // send note msg
	if (buttonState != states[id]){
		if (!buttonState) MIDI.sendNoteOn(BUTTONS_NOTE_OFFSET + id, DEFAULT_VELOCITY, DEFAULT_CHANNEL);  // note on when button is pressed
		else MIDI.sendNoteOff(BUTTONS_NOTE_OFFSET + id, DEFAULT_VELOCITY, DEFAULT_CHANNEL);  // note off when button is released
	}
	#else  // send CC msg 
	if ((buttonState != states[id]) && !buttonState){  // only when button is pressed
		values[id] = !values[id];  
		MIDI.sendControlChange(BUTTONS_CC_OFFSET + id, values[id]*127, DEFAULT_CHANNEL);  // send enable or disable request regarding the previous value 
	}
	#endif
	states[id] =  buttonState;  // save current button state
}


void setup() {
	// initialize serial and MIDI handler
	MIDI.begin(MIDI_CHANNEL_OMNI);
	Serial.begin(115200);

	// configure button pins as as pull up input
	for(int i=0; i < BUTTONS_NUM; i++){
		pinMode(buttonsPins[i], INPUT_PULLUP);
	}
	// configure leds pin as output
	pinMode(LED0_PIN, OUTPUT);
	pinMode(LED1_PIN, OUTPUT);
	pinMode(LED2_PIN, OUTPUT);

}

/** MAIN LOOP **/
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

	// send MIDI control change if one of the analog value changed
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
	
	// read button values and send MIDI message if necessary
	for(int i=0; i < BUTTONS_NUM; i++){
		updateButton(buttonsStates, buttonsValues, buttonsPins, i);
	}

	// toggle led 0 every loop
	digitalWrite(LED0_PIN, !digitalRead(LED0_PIN));

	// print values
	//  Serial.println(i);
	//  Serial.println(potarRatio*255);


	// wait
	delay(50);  
   

}
