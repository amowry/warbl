
 /*
 Copyright (C) 2018 Andrew Mowry warbl.xyz
 * 
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

// MAE FOOFOO 17Nov2018 For finger vibrato 
#define TINWHISTLE_EXPLICIT_SIZE 64 //the number of entries in the tinwhistle fingering chart
#define UILLEANN_EXPLICIT_SIZE 128 //the number of entries in the uilleann pipes fingering chart
#define GHB_EXPLICIT_SIZE 128 //the number of entries in the GHB/Scottish smallpipes fingering chart
#define NORTHUMBRIAN_EXPLICIT_SIZE 0 //the number of entries in the Northumbrian smallpipes explicit fingering chart
#define NORTHUMBRIAN_GENERAL_SIZE 9 //the number of entries in the Northumbrian smallpipes general fingering chart
#define GAITA_EXPLICIT_SIZE 256 //the number of entries in the Gaita fingering chart
#define DEBOUNCE_TIME    0.02  //debounce time, in seconds---Integrating debouncing algorithm is taken from debounce.c, written by Kenneth A. Kuhn:http://www.kennethkuhn.com/electronics/debounce.c
#define SAMPLE_FREQUENCY  200 //button sample frequency, in Hz

#define MAXIMUM  (DEBOUNCE_TIME * SAMPLE_FREQUENCY) //the integrator value required to register a button press

#define VERSION 85 //software version number

//MIDI commands
#define NOTE_OFF 0x80 
#define NOTE_ON 0x90
#define KEY_PRESSURE 0xA0
#define CC 0xB0
#define PROGRAM_CHANGE 0xC0
#define CHANNEL_PRESSURE 0xD0
#define PITCH_BEND 0xE0

// Instrument Modes
#define kModeWhistle 0
#define kModeUilleann 1
#define kModeGHB 2
#define kModeNorthumbrian 3
#define kModeChromatic 4
#define kModeGaita 5
#define kModeNModes 6

// Pitch bend modes
#define kPitchBendSlideVibrato 0
#define kPitchBendVibrato 1
#define kPitchBendNone 2
#define kPitchBendNModes 3

// Pressure modes
#define kPressureBreath 0
#define kPressureSingle 1
#define kPressureBagless 2
#define kPressureNModes 3

// Mouthpiece style
#define kMouthpieceStandard 0
#define kMouthpieceVented 1
#define kMouthpieceNModes 2

// Secret function drone control MIDI parameters
#define kDroneVelocity 36
#define kLynchDroneMIDINote 50
#define kCrowleyDroneMIDINote 51

//misc constants
const uint8_t ledPin = 13;
uint8_t holeTrans[] = {5,9,10,0,1,2,3,11,6}; //the analog pins used for the finger hole phototransistors, in the following order: Bell,R4,R3,R2,R1,L3,L2,L1,Lthumb
GPIO_pin_t pins[] = {DP11,DP6,DP8,DP5,DP7,DP1,DP0,DP3,DP2}; //the digital pins used for the finger hole leds, in the following order: Bell,R4,R3,R2,R1,L3,L2,L1,Lthumb. Uses a special declration format for the GPIO library.
GPIO_pin_t buttons[] = {DP15,DP14,DP16}; //the pins used for the buttons

//instrument mode
volatile byte tempMode;//
byte mode = 0; // The current mode (instrument), from 0-2.

//variables that can change according to fingering mode.
int8_t octaveShift = 0; //octave transposition
int8_t noteShift = 0; //note transposition, for changing keys. All fingering patterns are initially based on the key of D, and transposed with this variable to the desired key.
byte pitchBendMode = kPitchBendSlideVibrato; //0 means slide and vibrato are on. 1 means only vibrato is on. 2 is all pitchbend off.
byte offset = 15; 
byte senseDistance = 10;   //the sensor value above which the finger is sensed for bending notes. Needs to higher than the baseline sensor readings, otherwise vibrato will be turned on erroneously.
byte breathMode = kPressureBreath; //the desired presure sensor behavior for each fingering mode. 0 is breath mode (two octave). 1 is bag mode, one octave. 2 is bagless mode (one octave, a button is used to start/stop the sound).
bool secret = 0; //secret button command mode on or off
bool vented = kMouthpieceStandard; //vented mouthpiece on or off (there are different pressure settings for the vented mouthpiece)

//These are containers for the above variables, storing the value used by the different fingering modes.  First variable in array is for fingering mode 1, etc.
byte modeSelector[] = {kModeWhistle,kModeUilleann,kModeGHB}; //the fingering patterns chosen in the configuration tool, for instruments 0, 1, and 2. Pattern 0 is tin whistle, 1 is uillean, 2 is GHB/Scottish smallpipes, 3 is Northumbrian smallpipes, 4 is tinwhistle/flute chromatic, 5 is Gaita.
int8_t octaveShiftSelector[] = {0,0,0};
int8_t noteShiftSelector[] = {0,0,8};
byte pitchBendModeSelector[] = {0,0,2};
byte senseDistanceSelector[] = {10,10,10};
byte breathModeSelector[] = {0,0,1};
byte secretSelector[] = {0,0,0};
byte ventedSelector[] = {0,0,0};
byte customSelector[] = {0,0,0};
byte useLearnedPressureSelector[] = {0,0,0};
int learnedPressureSelector[] = {0,0,0};
unsigned int vibratoHolesSelector[] = {0b111111111,0b111111111,0b111111111};
unsigned int vibratoDepthSelector[] = {1024,1024,1024};
bool expressionOnSelector[] = {0,0,0};

bool momentary[3][3] = {{0,0,0},{0,0,0},{0,0,0}} ; //whether momentary click behavior is desired for MIDI on/off message sent with a button. Dimension 0 is mode, dimension 1 is button 0,1,2.

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
{{{2,0,0,0,0},  //for example, this means that clicking button 0 will send a CC message, channel 1, byte 2 = 2, byte 3 = 0.
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
//unsigned long time1 = 0; //used for testing--timing various processes.
unsigned long ledTimer = 0; //for blinking LED
byte blinkNumber = 0; //the number of remaining blinks when blinking LED to indicate control changes
bool LEDon = 0; //whether the LED is currently on
bool play = 0; //turns sound off and on (with the use of a button action) when in bagless mode
bool bellSensor = 0; //whether the bell sensor is plugged in
bool prevBellSensor = 0; //the previous reading of the bell sensor detection pin

//variables for reading pressure sensor
volatile unsigned int tempSensorValue = 0; //for holding the pressure sensor value inside the ISR
int sensorValue = 0;  // first value read from the pressure sensor
int sensorValue2 = 0; // second value read from the pressure sensor, for measuring rate of change in pressure
int sensorCalibration = 0; //the sensor reading at startup, used as a base value
int sensorThreshold[] = {260, 0}; //the pressure sensor thresholds for initial note on and shift from octave 1 to octave 2. Threshold 1 will be set to a little greater than threshold 0 after calibration in Setup.
byte prevState; //the previous sensor range (1=not enough force to sound note, 2=enough force to sound first octave, 3 = enough force to sound second octave)
byte newState; //the current sensor range (1=not enough force to sound note, 2=enough force to sound first octave, 3 = enough force to sound second octave)
boolean sensorDataReady = 0; //tells us if we've restarted the sensor timer
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
bool useLearnedPressure = 0; //whether we use learned pressure for note on threshold, or we use calibration pressure from startup
int learnedPressure = 0; //the learned pressure reading, used as a base value

//variables for reading tonehole sensors
volatile byte lastRead = 0; //the transistor that was last read, so we know which to read the next time around the loop.
unsigned int toneholeCovered[] = {0,0,0,0,0,0,0,0,0}; //covered hole tonehole sensor readings for calibration
volatile int tempToneholeRead[] = {0,0,0,0,0,0,0,0,0}; //temporary storage for tonehole sensor readings with IR LED on, written during the timer ISR
unsigned int toneholeRead[] = {0,0,0,0,0,0,0,0,0}; //storage for tonehole sensor readings, transferred from the above volatile variable
volatile int tempToneholeReadA[] = {0,0,0,0,0,0,0,0,0}; //temporary storage for ambient light tonehole sensor readings, written during the timer ISR
unsigned int holeCovered = 0; //whether each hole is covered-- each bit corresponds to a tonehole.
unsigned int prevHoleCovered = 0; //so we can track changes.
unsigned int hysteresis = 20;
volatile int tempNewNote = 0;
byte prevNote;
byte newNote = 255; //the next note to be played, based on the fingering chart (does not include transposition).
byte notePlaying; //the actual MIDI note being played, which we remember so we can turn it off again.
volatile bool firstTime = 0; // we have the LEDs off ~50% of the time. This bool keeps track of whether each LED is off or on at the end of each timer cycle

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
bool custom = 0; //Off/on for Michael Eskin's custom vibrato approach
bool customEnabled = 0; //Whether the custom vibrato above is currently enabled based on fingering pattern and pitchbend mode.
unsigned int toneholeScale[] = {0,0,0,0,0,0,0,0,0}; //a scale for normalizing the range of each sensor, for sliding
unsigned int vibratoScale[] = {0,0,0,0,0,0,0,0,0}; //same as above but for vibrato
unsigned int vibratoDepth = 1024; //vibrato depth from 0 (no vibrato) to 8191 (one semitone)
int expression = 0; //pitchbend up or down from current note based on pressure
byte expressionDepth = 4; //a factor for calculating pitchBend depth with pressure expression. Can have values of around 1-8
bool expressionOn = 0;


//variables for managing MIDI note output
bool noteon = 0; //whether a note is currently turned on
bool shiftState = 0; //whether the octave is shifted (could be combined with octaveShift)
int8_t shift = 0; //the total amount of shift up or down from the base note 62 (D). This takes into account octave shift and note shift.
byte velocity = 127;//MIDI note velocity

//tonehole calibration variables
byte calibration = 0; //whether we're currently calibrating. 1 is for calibrating all sensors, 2 is for calibrating bell sensor only.
unsigned long calibrationTimer = 0;
byte high; //for rebuiding ints from bytes read from EEPROM
byte low; // "" ""

//variables for reading buttons
unsigned long buttonReadTimer = 0; //for telling when it's time to read the buttons
byte integrator[] = {0,0,0}; //stores instegration of button readings. When this reaches MAXIMUM, a button press is registered. When it reaches 0, a release is registered.
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

// Serial.begin(9600);  
 //while (!Serial){
 //  ;
 // }

  
  ADCSRA &= ~(bit (ADPS0) | bit (ADPS1) | bit (ADPS2)); // clear ADC prescaler bits
  //ADCSRA |= bit (ADPS2);                               //  16 ADC prescale (takes about 41 usec per analogRead)
  ADCSRA |= bit (ADPS0) | bit (ADPS2);                 //  32 ADC prescale (takes about 61 usec per analogRead)
  //ADCSRA |= bit (ADPS1) | bit (ADPS2);                 //  64 
  //ADCSRA |= bit (ADPS0) | bit (ADPS1) | bit (ADPS2);   // 128
  
  DIDR0 = 0xff;   // disable digital input circuits for analog pins
  DIDR2 = 0xf3;  
    
  pinMode2(ledPin, OUTPUT); // Initialize the LED pin as an output (using the fast DIO2 library).
  
  pinMode2(17, INPUT_PULLUP); //this pin is used to detect when the bell sensor is plugged in (high when plugged in).
  
  for (byte i = 0; i < 9; i++) //Initialize the tonehole sensor IR LEDs.
    {pinMode2f(pins[i], OUTPUT);} 
  
  pinMode2f(DP15, INPUT_PULLUP); //set buttons as inputs and enable internal pullup
  pinMode2f(DP16, INPUT_PULLUP); 
  pinMode2f(DP14, INPUT_PULLUP); 
  
  //pinMode2(11, OUTPUT); //bell IR LED (can be used for debugging)
  //EEPROM.write(44,255); //can be uncommented to force factory settings to be resaved for debugging (after making changes to factory settings). Needs to be recommented again after.
  
  if(EEPROM.read(44) != 3) {saveFactorySettings();} //If we're running the software for the first time, copy all settings to EEPROM.
  if(EEPROM.read(37) == 3) {loadCalibration();} //If there has been a calibration saved, reload it at startup.

  loadFingering();
  loadSettingsForAllModes();  

  analogRead(A4); // the first analog readings are sometimes nonsense, so we read a few times and throw them away.
  analogRead(A4);
  sensorCalibration = analogRead(A4);
  sensorValue = sensorCalibration; //an initial reading to "seed" subsequent pressure readings
    
  loadPrefs(); //load the correct user settings base on current instrument.


  digitalWrite2(ledPin, HIGH);
  delay(500);
  digitalWrite2(ledPin, LOW);
  
  Timer1.initialize(170); //number of microseconds between sensor readings
  Timer1.attachInterrupt(readSensors); // timer ISR to read tonehole sensors at a regular interval

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
  
  if (sensorDataReady && breathMode != kPressureBagless) {
    get_state();//get the breath state from the pressure sensor if there's been a reading and we're not in bagless mode.

  }
    
  shift = ((octaveShift * 12) + noteShift); //add up any transposition.
  
  if (prevState == 3) {
    shift = shift + 12;} //add an octave jump from the state machine to the transposition if necessary.

      // MAE 17Nov2018 - For finger vibrato on whistle or uilleann
  if (customEnabled) { 
    if (vibratoEnable == 1 || vibratoEnable == 0 ){
      handleFingerVibrato();
    }
  }
  
  if ((millis() - pitchBendTimer) >= 10){ //check pitchbend very so often
    pitchBendTimer = millis();
    if(expressionOn && breathMode != kPressureBagless) {getExpression();} //calculate pitchbend based on pressure reading
    
   if(!customEnabled && pitchBendMode != kPitchBendNone){handlePitchBend();} 

    if (customEnabled) { 
         if (vibratoEnable == 0b000010){
            handleCustomPitchBend();}}   
   
  counter++;
  if(counter == 10) { //we do some things only every 10 ticks


   // Serial.println("");
    //Serial.println(expressionOn);

 
    //FREERAM_PRINT; //uncomment to show free ram (also uncomment library define at top)
    //MEMORY_PRINT_STACKSIZE;
     
     counter = 0;         
     bellSensor = (digitalRead2(17)); //check if the bell sensor is plugged in
     if(prevBellSensor != bellSensor){ 
        prevBellSensor = bellSensor;
        if(communicationMode){sendUSBMIDI(CC,7,102,120 + bellSensor);}} //if it's changed, tell the configuration tool.  
    if(communicationMode){
       int n = tempSensorValue - 100;
      if(n < 0){n = 0;}
       sendUSBMIDI(CC,7,116,n >> 3);}} //send a pressure sensor reading to the configuration tool when the counter rolls over. We bit shift so we can send over MIDI with only 7 bits.       
   }

      
 if(modeSelector[mode] == kModeGaita){
    shift = shift - 1;  
    }  
  
  if (                                                   //several conditions to tell if we need to turn on a new note.
      (!noteon || (newNote != (notePlaying - shift))) && //if there wasn't any note playing or the current note is different than the previous one
      (prevState > 1 || (breathMode == kPressureBagless && play)) && //and the state machine has determined that a note should be playing, or we're in bagless mode and the sound is turned on
      newNote != 255 &&                               //and the current note isn't 255, meaning the device was just turned on and no note has been selected.
      !(prevNote==62 && (newNote + shift) == 86 && !(breathMode != kPressureBagless && sensorDataReady)) &&  // and if we're currently on a middle D in state 3 (all finger holes covered), we wait until we get a new state reading before switching notes. This it to prevent erroneous octave jumps to a high D.
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
        }
      
  if (                                                                               //several conditions to turn a note off
    (noteon && ((breathMode != kPressureBagless && prevState == 1) || (breathMode == kPressureBagless && !play))) ||   //if the state drops to 1 (off) or we're in bagless mode and the sound has been turned off
      (noteon && modeSelector[mode] == kModeNorthumbrian && newNote == 60) ||                         //closed Northumbrian pipe
      (noteon && modeSelector[mode] == kModeUilleann && newNote == 62 && (bitRead(holeCovered,0) == 1))) { //closed Uillean pipe
          sendUSBMIDI(NOTE_OFF, 1, notePlaying, 127); //turn the note off if the breath pressure drops or if we're in Uilleann mode, the bell sensor is covered, and all the finger holes are covered.
          noteon = 0; //keep track
          prevPitchBend = -5000;} //if we turn a note off we set pitchbend to a weird (negative) number so that we know to resend a newly calculted value when we turn a note back on.

   
}
