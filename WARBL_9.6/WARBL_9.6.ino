
 /*
 Copyright (C) 2018 Andrew Mowry warbl.xyz
 * Thanks to Michael Eskin to many suggestions, fixes, and additions.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/
 */



//#include <MemoryUsage.h> //can be used to show free RAM
#include <TimerOne.h> //for timer interrupt for reading sensors at a regular interval
#include <DIO2.h> //fast digitalWrite library used for toggling IR LEDs
#include <MIDIUSB.h>
#include <EEPROM.h>
#include <avr/pgmspace.h> // for using PROGMEM for fingering chart storage

#define  GPIO2_PREFER_SPEED  1 //digitalread speed, see: https://github.com/Locoduino/DIO2/blob/master/examples/standard_outputs/standard_outputs.ino

#define TINWHISTLE_EXPLICIT_SIZE 64 //the number of entries in the tinwhistle fingering chart
#define UILLEANN_EXPLICIT_SIZE 128 //the number of entries in the uilleann pipes fingering chart
#define GHB_EXPLICIT_SIZE 128 //the number of entries in the GHB/Scottish smallpipes fingering chart
#define NORTHUMBRIAN_EXPLICIT_SIZE 0 //the number of entries in the Northumbrian smallpipes explicit fingering chart
#define NORTHUMBRIAN_GENERAL_SIZE 9 //the number of entries in the Northumbrian smallpipes general fingering chart
#define GAITA_EXPLICIT_SIZE 256 //the number of entries in the Gaita fingering chart
#define NAF_EXPLICIT_SIZE 128 //the number of entries in the Native American Flute fingering chart
#define KAVAL_EXPLICIT_SIZE 128 //the number of entries in the Kaval fingering chart

#define DEBOUNCE_TIME    0.02  //debounce time, in seconds---Integrating debouncing algorithm is taken from debounce.c, written by Kenneth A. Kuhn:http://www.kennethkuhn.com/electronics/debounce.c
#define SAMPLE_FREQUENCY  200 //button sample frequency, in Hz

#define MAXIMUM  (DEBOUNCE_TIME * SAMPLE_FREQUENCY) //the integrator value required to register a button press

#define VERSION 96 //software version number

//MIDI commands
#define NOTE_OFF 0x80 //127
#define NOTE_ON 0x90 // 144
#define KEY_PRESSURE 0xA0
#define CC 0xB0 // 176
#define PROGRAM_CHANGE 0xC0
#define CHANNEL_PRESSURE 0xD0
#define PITCH_BEND 0xE0

// Fingering Patterns
#define kModeWhistle 0
#define kModeUilleann 1
#define kModeGHB 2
#define kModeNorthumbrian 3
#define kModeChromatic 4
#define kModeGaita 5
#define kModeNAF 6
#define kModeKaval 7
#define kModeNModes 8

// Pitch bend modes
#define kPitchBendSlideVibrato 0
#define kPitchBendVibrato 1
#define kPitchBendNone 2
#define kPitchBendNModes 3

// Pressure modes
#define kPressureSingle 0
#define kPressureBreath 1
#define kPressureThumb 2
#define kPressureBell 3
#define kPressureNModes 4

// Mouthpiece style
#define kMouthpieceStandard 0
#define kMouthpieceVented 1
#define kMouthpieceNModes 2

// Secret function drone control MIDI parameters
#define kDroneVelocity 36
#define kLynchDroneMIDINote 50
#define kCrowleyDroneMIDINote 51

// Drones control mode
#define kNoDroneControl 0
#define kSecretDroneControl 1
#define kBaglessDroneControl 2
#define kPressureDroneControl 3
#define kDroneNModes 4

//Variables in the ED array (all the settings for the Expression and Drones panels)
#define EXPRESSION_ON 0
#define EXPRESSION_DEPTH 1
#define SEND_PRESSURE 2
#define CURVE 3 // (0 is linear, 1 and 2 are power curves)
#define PRESSURE_CHANNEL 4
#define PRESSURE_CC 5
#define INPUT_PRESSURE_MIN 6
#define INPUT_PRESSURE_MAX 7
#define OUTPUT_PRESSURE_MIN 8
#define OUTPUT_PRESSURE_MAX 9
#define DRONES_ON_COMMAND 10
#define DRONES_ON_CHANNEL 11
#define DRONES_ON_BYTE2 12
#define DRONES_ON_BYTE3 13
#define DRONES_OFF_COMMAND 14
#define DRONES_OFF_CHANNEL 15
#define DRONES_OFF_BYTE2 16
#define DRONES_OFF_BYTE3 17
#define DRONES_CONTROL_MODE 18
#define DRONES_PRESSURE_LOW_BYTE 19
#define DRONES_PRESSURE_HIGH_BYTE 20

//GPIO constants
const uint8_t ledPin = 13;
const uint8_t holeTrans[] = {5,9,10,0,1,2,3,11,6}; //the analog pins used for the finger hole phototransistors, in the following order: Bell,R4,R3,R2,R1,L3,L2,L1,Lthumb
const GPIO_pin_t pins[] = {DP11,DP6,DP8,DP5,DP7,DP1,DP0,DP3,DP2}; //the digital pins used for the finger hole leds, in the following order: Bell,R4,R3,R2,R1,L3,L2,L1,Lthumb. Uses a special declaration format for the GPIO library.
const GPIO_pin_t buttons[] = {DP15,DP14,DP16}; //the pins used for the buttons

//instrument
byte mode = 0; // The current mode (instrument), from 0-2.

//variables that can change according to instrument.
int8_t octaveShift = 0; //octave transposition
int8_t noteShift = 0; //note transposition, for changing keys. All fingering patterns are initially based on the key of D, and transposed with this variable to the desired key.
byte pitchBendMode = kPitchBendSlideVibrato; //0 means slide and vibrato are on. 1 means only vibrato is on. 2 is all pitchbend off.
byte offset = 15; 
byte senseDistance = 10;   //the sensor value above which the finger is sensed for bending notes. Needs to higher than the baseline sensor readings, otherwise vibrato will be turned on erroneously.
byte breathMode = kPressureBreath; //the desired presure sensor behavior for each fingering mode. 0 is breath mode (two octave). 1 is bag mode, one octave. 2 is bagless mode (one octave, a button is used to start/stop the sound).
bool secret = 0; //secret button command mode on or off
bool vented = kMouthpieceStandard; //vented mouthpiece on or off (there are different pressure settings for the vented mouthpiece)
bool bagless = 0; //bagless mode on or off
unsigned int vibratoDepth = 1024; //vibrato depth from 0 (no vibrato) to 8191 (one semitone)
bool useLearnedPressure = 0; //whether we use learned pressure for note on threshold, or we use calibration pressure from startup
bool custom = 0; //Off/on for Michael Eskin's custom vibrato approach


//These are containers for the above variables, storing the value used by the different fingering modes.  First variable in array is for fingering mode 1, etc.
byte modeSelector[] = {kModeWhistle,kModeUilleann,kModeGHB}; //the fingering patterns chosen in the configuration tool, for instruments 0, 1, and 2. Pattern 0 is tin whistle, 1 is uillean, 2 is GHB/Scottish smallpipes, 3 is Northumbrian smallpipes, 4 is tinwhistle/flute chromatic, 5 is Gaita.
int8_t octaveShiftSelector[] = {0,0,0};
int8_t noteShiftSelector[] = {0,0,8};
byte pitchBendModeSelector[] = {1,1,2};
byte senseDistanceSelector[] = {10,10,10};
byte breathModeSelector[] = {1,1,0};
byte secretSelector[] = {0,0,0};
byte ventedSelector[] = {0,0,0};
byte baglessSelector[] = {0,0,0};
byte customSelector[] = {0,0,0};
byte useLearnedPressureSelector[] = {0,0,0};
int learnedPressureSelector[] = {0,0,0};
byte LSBlearnedPressure; //used to reconstruct learned pressure from two MIDI bytes.
unsigned int vibratoHolesSelector[] = {0b111111111,0b111111111,0b111111111};
unsigned int vibratoDepthSelector[] = {1024,1024,1024};

bool momentary[3][3] = {{0,0,0},{0,0,0},{0,0,0}} ; //whether momentary click behavior is desired for MIDI on/off message sent with a button. Dimension 0 is mode, dimension 1 is button 0,1,2.

bool invert[3] = {0,0,0}; //whether the functionality for using the right thumb or the bell sensor for increasing the register is inverted.

byte ED[3][21] = //an array that holds all the settings for the Expression and Drones Control panels in the Configuration Tool.
//instrument 0
{{0,  //EXPRESSION_ON
3,    //EXPRESSION_DEPTH (can have a value of 1-8)
0,    //SEND_PRESSURE
0,    //CURVE (0 is linear)
1,    //PRESSURE_CHANNEL
7,    //PRESSURE_CC
0,    //INPUT_PRESSURE_MIN range is from 0-100, to be scaled later up to maximum input values
100,  //INPUT_PRESSURE_MAX range is from 0-100, to be scaled later
0,    //OUTPUT_PRESSURE_MIN range is from 0-127, to be scaled later
127,  //OUTPUT_PRESSURE_MAX range is from 0-127, to be scaled later
0,    //DRONES_ON_COMMAND
1,    //DRONES_ON_CHANNEL
51,    //DRONES_ON_BYTE2
36,    //DRONES_ON_BYTE3
0,    //DRONES_OFF_COMMAND
1,    //DRONES_OFF_CHANNEL
51,    //DRONES_OFF_BYTE2
36,    //DRONES_OFF_BYTE3
0,    //DRONES_CONTROL_MODE (0 is no drone control, 1 is use secret button, 2 is use bagless button, 3 is use pressure.
0,    //DRONES_PRESSURE_LOW_BYTE
0},    //DRONES_PRESSURE_HIGH_BYTE

//same for instrument 1
{0,3,0,0,1,7,0,100,0,127,0,1,51,36,0,1,51,36,0,0,0},

//same for instrument 2
{0,3,0,0,1,7,0,100,0,127,0,1,51,36,0,1,51,36,0,0,0}};

byte pressureSelector[3][12] = //a selector array for all the pressure settings variables that can be changed in the Configuration Tool
//instrument 0
{{15,15,15,15,30,30, //closed mouthpiece: offset, multiplier, jump, drop, jump time, drop time
3,8,28,7,15,22}, //vented mouthpiece: offset, multiplier, jump, drop, jump time, drop time
//instrument 1
{15,15,15,15,30,30,
3,8,28,7,15,22},
//instrument 2
{15,15,15,15,30,30,
3,8,28,7,15,22}};

uint8_t buttonPrefs[3][8][5]= //The button configuration settings. Dimension 1 contains the three fingering modes chosen in the configuration tool. Dimension 2 is the button combination: click 1, click 2, click3, hold 2 click 1, hold 2 click 3, longpress 1, longpress2, longpress3
                      //Dimension 3 is the desired action: Action, MIDI command type (noteon/off, CC, PC), MIDI channel, MIDI byte 2, MIDI byte 3.
                              //the actions are: 0 none, 1 send MIDI message, 2 change pitchbend mode, 3 change fingering mode, 4 play/stop (bagless mode), 5 octave shift up, 6 octave shift down, 7 unused, 8 change breath mode
{{{2,0,0,0,0},  //for example, this means that clicking button 0 will send a CC message, channel 1, byte 2 = 0, byte 3 = 0.
{8,0,0,0,0},   //for example, this means that clicking button 1 will change pitchbend mode.
{3,0,0,0,0},
{5,0,0,0,0},
{6,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0}},

{{2,0,0,0,0},
{8,0,0,0,0},
{3,0,0,0,0},
{5,0,0,0,0},
{6,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0}},

{{2,0,0,0,0},
{8,0,0,0,0},
{3,0,0,0,0},
{5,0,0,0,0},
{6,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0}}};

//other misc. variables
byte hardwareRevision = 22;
unsigned long ledTimer = 0; //for blinking LED
byte blinkNumber = 0; //the number of remaining blinks when blinking LED to indicate control changes
bool LEDon = 0; //whether the LED is currently on
bool play = 0; //turns sound off and on (with the use of a button action) when in bagless mode
bool bellSensor = 0; //whether the bell sensor is plugged in
bool prevBellSensor = 0; //the previous reading of the bell sensor detection pin
unsigned long initialTime = 0; //for testing
unsigned long finalTime = 0; //for testing
byte program = 0; //current MIDI program change value. This always starts at 0 but can be increased/decreased with assigned buttons.

//variables for reading pressure sensor
volatile unsigned int tempSensorValue = 0; //for holding the pressure sensor value inside the ISR
int sensorValue = 0;  // first value read from the pressure sensor
int sensorValue2 = 0; // second value read from the pressure sensor, for measuring rate of change in pressure
int prevSensorValue = 0; // previous sensor reading, used to tell if the pressure has changed and should be sent.
int sensorCalibration = 0; //the sensor reading at startup, used as a base value
int sensorThreshold[] = {260, 0}; //the pressure sensor thresholds for initial note on and shift from octave 1 to octave 2. Threshold 1 will be set to a little greater than threshold 0 after calibration in Setup.
byte newState; //the note/octave state based on the sensor readings (1=not enough force to sound note, 2=enough force to sound first octave, 3 = enough force to sound second octave)
boolean sensorDataReady = 0; //tells us that pressure data is available
bool jump = 0; //whether we jumped directly to second octave from note off because of rapidly increasing pressure.
unsigned long jumpTimer = 0; //records time when we dropped to note off.
int jumpTime = 15; //the amount of time to wait before dropping back down from an octave jump to first octave because of insufficient pressure MAE FOOFOO was 40
bool drop = 0; //whether we dropped directly from second octave to note off
unsigned long dropTimer = 0; //time when we jumped to second octave.
int dropTime = 15 ; //the amount of time to wait (ms) before turning a note back on after dropping directly from second octave to note off MAE FOOFOO was 120
byte jumpValue = 15;
byte dropValue = 15;
byte multiplier = 15; //controls how much more difficult it is to jump to second octave from higher first-octave notes than from lower first-octave notes. Increasing this makes playing with a bag more forgiving but requires more force to reach highest second-octave notes. Can be set according to fingering mode and breath mode (i.e. a higher jump factor may be used for playing with a bag). Array indices 1-3 are for breath mode jump factor, indices 4-6 are for bag mode jump factor.
byte soundTriggerOffset = 3; //the number of sensor values above the calibration setting at which a note on will be triggered (first octave)
int learnedPressure = 0; //the learned pressure reading, used as a base value
unsigned int minIn = 100; //used for sending pressure data as CC
unsigned long maxIn = 800;
unsigned int minOut = 127;
unsigned int maxOut = 16383;

//variables for reading tonehole sensors
volatile byte lastRead = 0; //the transistor that was last read, so we know which to read the next time around the loop.
unsigned int toneholeCovered[] = {0,0,0,0,0,0,0,0,0}; //covered hole tonehole sensor readings for calibration
int toneholeBaseline[] = {0,0,0,0,0,0,0,0,0}; //baseline (uncovered) hole tonehole sensor readings
volatile int tempToneholeRead[] = {0,0,0,0,0,0,0,0,0}; //temporary storage for tonehole sensor readings with IR LED on, written during the timer ISR
int toneholeRead[] = {0,0,0,0,0,0,0,0,0}; //storage for tonehole sensor readings, transferred from the above volatile variable
volatile int tempToneholeReadA[] = {0,0,0,0,0,0,0,0,0}; //temporary storage for ambient light tonehole sensor readings, written during the timer ISR
unsigned int holeCovered = 0; //whether each hole is covered-- each bit corresponds to a tonehole.
unsigned int prevHoleCovered = 0; //so we can track changes.
unsigned int hysteresis = 4;
volatile int tempNewNote = 0;
byte prevNote;
byte newNote = 255; //the next note to be played, based on the fingering chart (does not include transposition).
byte notePlaying; //the actual MIDI note being played, which we remember so we can turn it off again.
volatile bool firstTime = 1; // we have the LEDs off ~50% of the time. This bool keeps track of whether each LED is off or on at the end of each timer cycle
volatile byte timerCycle = 0; //the number of times we've entered the timer ISR with the new sensors. 

//pitchbend variables
unsigned long pitchBendTimer = 0; //to keep track of the last time we sent a pitchbend message
byte pitchBendOn[] = {0,0,0,0,0,0,0,0,0}; //whether pitchbend is currently turned for for a specific hole
int pitchBend = 8191; //total current pitchbend value
int prevPitchBend = 8191; //a record of the previous pitchBend value, so we don't send the same one twice
int iPitchBend[] = {8191,8191,8191,8191,8191,8191,8191,8191,8191}; //current pitchbend value for each tonehole
byte slideHole; //the hole above the current highest uncovered hole. Used for detecting slides between notes.
byte stepsDown = 1; //the number of half steps down from the slideHole to the next lowest note on the scale, used for calculating pitchbend values.
byte vibratoEnable = 0; // if non-zero, send vibrato pitch bend
unsigned int holeLatched = 0b000000000; //holes that are disabled for vibrato because they were covered when the note was triggered. They become unlatched (0) when the finger is removed all the way.
unsigned int vibratoHoles = 0b111111111; //holes to be used for vibrato, left thumb on left, bell sensor far right.
unsigned int toneholeScale[] = {0,0,0,0,0,0,0,0,0}; //a scale for normalizing the range of each sensor, for sliding
unsigned int vibratoScale[] = {0,0,0,0,0,0,0,0,0}; //same as above but for vibrato
int expression = 0; //pitchbend up or down from current note based on pressure
bool customEnabled = 0; //Whether the custom vibrato above is currently enabled based on fingering pattern and pitchbend mode.

//variables for managing MIDI note output
bool noteon = 0; //whether a note is currently turned on
bool shiftState = 0; //whether the octave is shifted (could be combined with octaveShift)
int8_t shift = 0; //the total amount of shift up or down from the base note 62 (D). This takes into account octave shift and note shift.
byte velocity = 127;//MIDI note velocity

//tonehole calibration variables
byte calibration = 0; //whether we're currently calibrating. 1 is for calibrating all sensors, 2 is for calibrating bell sensor only, 3 is for calibrating all sensors plus baseline calibration (normally only done once, in the "factory").
unsigned long calibrationTimer = 0;
byte high; //for rebuiding ints from bytes read from EEPROM
byte low; // "" ""

//variables for reading buttons
unsigned long buttonReadTimer = 0; //for telling when it's time to read the buttons
byte integrator[] = {0,0,0}; //stores integration of button readings. When this reaches MAXIMUM, a button press is registered. When it reaches 0, a release is registered.
bool pressed[] = {0,0,0}; //whether a button is currently presed (this it the output from the integrator)
bool released[] = {0,0,0}; //if a button has just been released
bool justPressed[] = {0,0,0}; //if a button has just been pressed
bool prevOutput[] = {0,0,0}; //previous state of button, to track state through time
bool longPress[] = {0,0,0}; //long button press
unsigned int longPressCounter[] = {0,0,0}; //for counting how many reading each button has been held, to indicate a long button press
bool noteOnOffToggle[] = {0,0,0}; //if using a button to toggle a noteOn/noteOff command, keep track of state.
bool longPressUsed[] = {0,0,0}; //if we used a long button press, we set a flag so we don't use it again unless the button has been released first.
bool buttonUsed = 0; //flags any button activity, so we know to handle it.
bool specialPressUsed[] = {0,0,0};
bool dronesOn = 0; //used to monitor "Secret" drones on/off button command.

//variables for communication with the WARBL Configuration Tool
bool communicationMode = 0; //whether we are currently communicating with the tool.
byte buttonReceiveMode = 100; //which row in the button configuration matrix for which we're currently receiving data.
byte pressureReceiveMode = 100; //which pressure variable we're currently receiving date for. From 1-12: Closed: offset, multiplier, jump, drop, jump time, drop time, Vented: offset, multiplier, jump, drop, jump time, drop time
byte counter = 0; // We use this to know when to send a new pressure reading to the configuration tool. We increment it every time we send a pitchBend message, to use as a simple timer wihout needing to set another actual timer.


void setup() {
  
  DIDR0 = 0xff;   // disable digital input circuits for analog pins
  DIDR2 = 0xf3;  
    
  pinMode2(ledPin, OUTPUT); // Initialize the LED pin as an output (using the fast DIO2 library). 
  pinMode2(17, INPUT_PULLUP); //this pin is used to detect when the bell sensor is plugged in (high when plugged in).
  
  for (byte i = 0; i < 9; i++) //Initialize the tonehole sensor IR LEDs.
    {pinMode2f(pins[i], OUTPUT);} 
  
  pinMode2f(DP15, INPUT_PULLUP); //set buttons as inputs and enable internal pullup
  pinMode2f(DP16, INPUT_PULLUP); 
  pinMode2f(DP14, INPUT_PULLUP); 
  
  //EEPROM.update(44,255); //can be uncommented to force factory settings to be resaved for debugging (after making changes to factory settings). Needs to be recommented again after.

//the four lines below can be uncommented to make a version of the software that will resave factory settings the first time it is loaded.
//  if(EEPROM.read(901) != VERSION){ //a new software version has been loaded
 // EEPROM.update(901, VERSION); //update the stored software version
 // EEPROM.update(44, 255); //change this, which will force the factory settings to be resaved.
 // }

  if(EEPROM.read(900) == 21){ //old sensors
  hardwareRevision = 21;}
  
  if(EEPROM.read(44) != 3) {saveFactorySettings();} //If we're running the software for the first time, copy all settings to EEPROM.
  if(EEPROM.read(37) == 3) {loadCalibration();} //If there has been a calibration saved, reload it at startup.

  loadFingering();
  loadSettingsForAllModes();  

  analogRead(A4); // the first analog readings are sometimes nonsense, so we read a few times and throw them away.
  analogRead(A4);
  sensorCalibration = analogRead(A4);
  sensorValue = sensorCalibration; //an initial reading to "seed" subsequent pressure readings
    
  loadPrefs(); //load the correct user settings based on current instrument.

  digitalWrite2(ledPin, HIGH);
  delay(500);
  digitalWrite2(ledPin, LOW);

  if(hardwareRevision == 21){ //old sensors
  ADCSRA &= ~(bit (ADPS0) | bit (ADPS1) | bit (ADPS2)); // clear ADC prescaler bits
  ADCSRA |= bit (ADPS0) | bit (ADPS2);                 //  32 ADC prescale (takes about 61 usec per analogRead)
  Timer1.initialize(170); //number of microseconds between sensor readings
  Timer1.attachInterrupt(readSensors); // timer ISR to read tonehole sensors at a regular interval
  }

  else{ //new sensors
  Timer1.initialize(100); //this timer is only used to add some additional time after reading all sensors, for power savings.
  Timer1.attachInterrupt(timerDelay);
  Timer1.stop(); //stop the timer because we don't need it until we've read all the sensors once.
  ADC_init(); //initialize the ADC and start conversions
  }
}




void loop() {

  receiveMIDI();
  
  if ((millis() - buttonReadTimer) >= 5) {//read the state of the control buttons and handle any buttons presses every so often
    checkButtons();
    buttonReadTimer = millis();}
  
  if (buttonUsed) {handleButtons();} //if a button had been used, process the command. We only do this when we need to, so we're not wasting time.
  
  if (blinkNumber > 0) {blink();} //blink the LED if necessary (indicating control changes, etc.)


  noInterrupts();
  for (byte i=0; i< 9; i++) {
  toneholeRead[i]=tempToneholeRead[i]; //transfer sensor readings to a variable that won't get modified in the ISR
  }
  interrupts();
 
  if(hardwareRevision > 21 && calibration == 0){ //if we're not calibrating, compensate for baseline sensor offset (the stored sensor reading without the hole covered)
    for (byte i=0; i< 9; i++) {
    toneholeRead[i] = toneholeRead[i] - toneholeBaseline[i];
    }
  }

  for (byte i=0; i< 9; i++) {  
      if (toneholeRead[i] < 0){ //in rare case the adjusted readings can end up being negative.
        toneholeRead[i] = 0;
      }
  }


// finalTime = micros() - initialTime; //for testing only    
//noInterrupts();
//unsigned long timingTest = finalTime; //for testing only
  //interrupts();
// Serial.println(timingTest); //for testing only
 // initialTime = micros(); //for testing only

   
  tempNewNote = get_note(); //get the next MIDI note from the fingering pattern
  
  if (calibration > 0){
    calibrate();} //calibrate/continue calibrating if the command has been received.

  if(!customEnabled && pitchBendMode != kPitchBendNone){
    if(newNote != tempNewNote && tempNewNote != -1) { //If a new note has been triggered,
     holeLatched = holeCovered; //remember the pattern that triggered it
     for (byte i=0; i< 9; i++) {
     iPitchBend[i] = 0; //and reset pitchbend
     pitchBendOn[i] = 0;}
     findStepsDown();}
     }
  
  if (tempNewNote != -1) {
    newNote = tempNewNote;} //update the next note if the fingering pattern is valid
  
  if (sensorDataReady) {
    get_state();//get the breath state from the pressure sensor if there's been a reading.
  }
    
  shift = ((octaveShift * 12) + noteShift); //add up any transposition.
  
  if (newState == 3 || (breathMode == kPressureBell && modeSelector[mode] != kModeUilleann && bitRead(holeCovered,0) == 0) || (breathMode == kPressureThumb && (modeSelector[mode] == kModeWhistle || modeSelector[mode] == kModeChromatic || modeSelector[mode] == kModeNAF) && bitRead(holeCovered,8) == invert[mode])) {
    shift = shift + 12; //add an octave jump to the transposition if necessary.
  }

  if (customEnabled) {   //For finger vibrato on whistle or uilleann
    if (vibratoEnable == 1 || vibratoEnable == 0 ){
      handleFingerVibrato();
    }
  }
  
  if ((millis() - pitchBendTimer) >= 10){ //check pitchbend every so often
    pitchBendTimer = millis();
    if(ED[mode][EXPRESSION_ON] && !bagless) {getExpression();} //calculate pitchbend based on pressure reading
    
   if(!customEnabled && pitchBendMode != kPitchBendNone){handlePitchBend();} 

    if (customEnabled) { 
         if (vibratoEnable == 0b000010){
            handleCustomPitchBend();}
    }
            
  	if(abs(prevSensorValue - sensorValue) > 2) { //if pressure has changed more than a little, send it.
  		sendPressure();
  		prevSensorValue = sensorValue;
  	}    
   
  counter++;
  if(counter == 10) { //we do some things only every 10 ticks


//Serial.println((ED[mode][DRONES_PRESSURE_HIGH_BYTE] << 7 | ED[mode][DRONES_PRESSURE_LOW_BYTE]));
//Serial.println(modeSelector[0]);

//Serial.println(invert[0]);
//for (byte i = 0; i < 9; i++) { 
  //Serial.println(toneholeRead[i]);
//}
//Serial.println(ED[mode][DRONES_CONTROL_MODE]);
  //Serial.println("");

 
    //FREERAM_PRINT; //uncomment to show free ram (also uncomment library include at top)
    //MEMORY_PRINT_STACKSIZE;
     
     counter = 0;         
     bellSensor = (digitalRead2(17)); //check if the bell sensor is plugged in
     if(prevBellSensor != bellSensor){ 
        prevBellSensor = bellSensor;
        if(communicationMode){sendUSBMIDI(CC,7,102,120 + bellSensor);}} //if it's changed, tell the configuration tool.  
    
   }}

      
 if(modeSelector[mode] == kModeGaita){
    shift = shift - 1;  
    }  
  
  if (                                                   //several conditions to tell if we need to turn on a new note.
      (!noteon || (newNote != (notePlaying - shift))) && //if there wasn't any note playing or the current note is different than the previous one
      ((newState > 1 && !bagless) || (bagless && play)) && //and the state machine has determined that a note should be playing, or we're in bagless mode and the sound is turned on
      newNote != 255 &&                               //and the current note isn't 255, meaning the device was just turned on and no note has been selected.
      !(prevNote==62 && (newNote + shift) == 86 && !sensorDataReady) &&  // and if we're currently on a middle D in state 3 (all finger holes covered), we wait until we get a new state reading before switching notes. This it to prevent erroneous octave jumps to a high D.
      !(modeSelector[mode] == kModeNorthumbrian && newNote == 60) && //and if we're in Northumbrian mode don't play a note if all holes are covered. That simulates the closed pipe.
      !(modeSelector[mode] == kModeUilleann && newNote == 62 && (bitRead(holeCovered,0) == 1))) { // and if we're in Uillean mode, don't play a note if the bell sensor is covered and all holes are covered. Again, simulating a closed pipe.
        
        if(noteon){sendUSBMIDI(NOTE_OFF, 1, notePlaying, velocity); //always turn off the previous note before turning on the new one.
        if(!customEnabled && pitchBendMode != kPitchBendNone){
          handlePitchBend();
          pitchBendTimer = millis();}  //check to see if we need to update pitchbend before changing to a new note. 
          } 
  
         sendUSBMIDI(NOTE_ON, 1, newNote + shift, velocity);
         notePlaying = newNote + shift;
         prevNote = newNote;
         noteon = 1; //keep track of the fact that there's a note turned on
         if (ED[mode][DRONES_CONTROL_MODE] == 2 && !dronesOn) { //start drones if drones are being controlled with chanter on/off
            startDrones();
          }

        }

  if(noteon){  //several conditions to turn a note off  
    if (                                                                               
      ((newState == 1 && !bagless) || (bagless && !play)) ||   //if the state drops to 1 (off) or we're in bagless mode and the sound has been turned off
       (modeSelector[mode] == kModeNorthumbrian && newNote == 60) ||                         //or closed Northumbrian pipe
       (modeSelector[mode] == kModeUilleann && newNote == 62 && (bitRead(holeCovered,0) == 1))) { //or closed Uillean pipe
           sendUSBMIDI(NOTE_OFF, 1, notePlaying, 127); //turn the note off if the breath pressure drops or if we're in Uilleann mode, the bell sensor is covered, and all the finger holes are covered.
           noteon = 0; //keep track
           prevPitchBend = -5000; //if we turn a note off we set pitchbend to a weird (negative) number so that we know to resend a newly calculted value when we turn a note back on.
           if (ED[mode][DRONES_CONTROL_MODE] == 2 && dronesOn) { //stop drones if drones are being controlled with chanter on/off
              stopDrones();
           }
      }
   }


   
}
