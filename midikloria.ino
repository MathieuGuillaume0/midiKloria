#include <MIDI.h>

// Create and binds the MIDI interface to the default hardware Serial port
MIDI_CREATE_DEFAULT_INSTANCE();

/** project related defines **/
#define BUTTONS_NUM 8
#define LEDS_NUM 3
#define POTARS_NUM 9

#define DEFAULT_CHANNEL 1
#define DEFAULT_VELOCITY 127
#define POTARS_CC_OFFSET 1  // control change start id for buttons
#define BUTTONS_CC_OFFSET (POTARS_CC_OFFSET + POTARS_NUM)  // control change start id for buttons
#define NOTE_OFFSET 50  // note start id for buttons
#define BUTTON_CC_MODE  // buttons will send CC msgs. comment this if you want note msgs

#define AVG_SAMPLES_NUM 3  // number of samples used in the moving average
#define ANALOG_DIFF_THRES 1  // difference threshold between last and current value until a MIDI msg is sent



/** pins **/
byte potarsPins[POTARS_NUM] = {A0, A0, A0, A0, A3, A4, A5, A6, A7}; 
int potarsMuxIds[POTARS_NUM] = {0, 1, 2, 3, -1, -1, -1, -1, -1};

byte buttonsPins[BUTTONS_NUM] = {5, 6, 7, 8, 9, 12, A1, A2};  
byte ledsPins[LEDS_NUM] = {10, 11, 13}; 
byte muxPins[] = {2, 3, 4};

/** pot variables **/
byte avgPos = 0;
struct potar {
	byte pinNumber;
	int lastValue, sum = 0;
	int previousValues[AVG_SAMPLES_NUM] = {0};
	int muxId;
};
struct potar potars[POTARS_NUM];

/** button variables**/
struct button {
	byte pinNumber;
	bool state=1;  // save last button state to detect button push and release
	bool value=1;  // last value * 127 sent in the MIDI msg when the button is pressed in CC mode
};
struct button buttons[BUTTONS_NUM];

/** functions **/ 
int movingAvg(int *numArray, int arraySize, int *sum, int nextNum, int pos){
	// Subtract the oldest number from the prev sum, add the new number
	*sum = *sum - numArray[pos] + nextNum;

	// Assign the nextNum to the position in the array
	numArray[pos] = nextNum;

	// return the average
	return *sum / arraySize;
}

void updateButton(struct button *buttonsArray, byte id){
	struct button *thisButton = buttonsArray + id;
	bool buttonState = digitalRead(thisButton->pinNumber);
	
	#ifdef BUTTON_CC_MODE  // send cc msg
	if ((buttonState != thisButton->state) && !buttonState){  // only when button is pressed
		thisButton->value = !thisButton->value;  
		MIDI.sendControlChange(BUTTONS_CC_OFFSET + id, thisButton->value*127, DEFAULT_CHANNEL);  // send enable or disable request regarding the previous value 
	}
	#else  // send note msg
	if (buttonState != thisButton->state){
		if (!buttonState) MIDI.sendNoteOn(NOTE_OFFSET + id, DEFAULT_VELOCITY, DEFAULT_CHANNEL);  // note on when button is pressed
		else MIDI.sendNoteOff(NOTE_OFFSET + id, DEFAULT_VELOCITY, DEFAULT_CHANNEL);  // note off when button is released
	}
	#endif
	thisButton->state = buttonState;  // save current button state
}

void updatePotar(struct potar *potarsArray, byte id, byte pos){ 
	struct potar *thisPotar = potarsArray + id;
	// get analog value
	if (thisPotar->muxId >= 0){  // drive multiplexer pins before read if this a mux input 
		byte muxId = (byte)thisPotar->muxId;
		for(int i=0; i < 3; i++){
			digitalWrite(muxPins[i], (muxId >> i) & 0x01);
		}
		delay(1);  // wait a bit before reading
	}
	int potarValue = (analogRead(thisPotar->pinNumber) / 1023.0f)*127;

	// moving average 
	int potarAvg = movingAvg(thisPotar->previousValues, AVG_SAMPLES_NUM, &thisPotar->sum, potarValue, pos);

	// send MIDI control change if one of the analog value changed
	if (abs(potarAvg - thisPotar->lastValue) > ANALOG_DIFF_THRES){
		MIDI.sendControlChange(POTARS_CC_OFFSET + id, potarAvg, DEFAULT_CHANNEL);
		thisPotar->lastValue = potarAvg;  // save this value for next loop
	}
}


void setup() {
	// initialize serial and MIDI handler
	MIDI.begin(MIDI_CHANNEL_OMNI);
	Serial.begin(115200);
	
	// initialize potars structure with pins and mux ids
	for(int i=0; i < POTARS_NUM; i++){
		potars[i].pinNumber = potarsPins[i];
		potars[i].muxId = potarsMuxIds[i];
	}
	
	// configure button pins as pull up input and init structure
	for(int i=0; i < BUTTONS_NUM; i++){
		pinMode(buttonsPins[i], INPUT_PULLUP);
		buttons[i].pinNumber = buttonsPins[i];
	}
	
	// configure leds and mux pins as output
	for(int i=0; i < LEDS_NUM; i++){
		pinMode(ledsPins[i], OUTPUT);
	}
	for(int i=0; i < 3; i++){
		pinMode(muxPins[i], OUTPUT);
	}
}

/** MAIN LOOP **/
void loop() {
	// read potar values and send MIDI message if any change
	for(int i=0; i < POTARS_NUM; i++){
		updatePotar(potars, i, avgPos);
	}
	
	// increase moving average positon for the next loop
	avgPos++;
	if (avgPos >= AVG_SAMPLES_NUM) avgPos = 0;
	

	// read button values and send MIDI message (CC or Note) if any change
	for(int i=0; i < BUTTONS_NUM; i++){
		updateButton(buttons, i);
	}

	// toggle led 0 every loop
	digitalWrite(ledsPins[2], !digitalRead(ledsPins[2]));
	
	// set pwm duty cycle of led 1 and 2 according to potar values
	analogWrite(ledsPins[0], potars[0].lastValue*2); 
	analogWrite(ledsPins[1], potars[1].lastValue*2); 
	
	// wait
	delay(5);    
}
