/*
    Copyright (C) 2018-2022 Andrew Mowry warbl.xyz

    Many thanks to Michael Eskin, Jesse Chappell, and Louis Barman for their additions.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see http://www.gnu.org/licenses/
*/

//#include <MemoryUsage.h> //can be used to show free RAM
#include <avr/wdt.h>   //for resetting with watchdog
#include <TimerOne.h>  //for timer interrupt for reading sensors at a regular interval
#include <DIO2.h>      //fast digitalWrite library used for toggling IR LEDs
#include <MIDIUSB.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>  // for using PROGMEM for fingering chart storage

#define GPIO2_PREFER_SPEED 1  //digitalread speed, see: https://github.com/Locoduino/DIO2/blob/master/examples/standard_outputs/standard_outputs.ino

#define DEBOUNCE_TIME 0.02    //button debounce time, in seconds---Integrating debouncing algorithm is taken from debounce.c, written by Kenneth A. Kuhn:http://www.kennethkuhn.com/electronics/debounce.c
#define SAMPLE_FREQUENCY 200  //button sample frequency, in Hz

#define MAXIMUM (DEBOUNCE_TIME * SAMPLE_FREQUENCY)  //the integrator value required to register a button press

#define VERSION 21  //software version number (without decimal point)

//MIDI commands
#define NOTE_OFF 0x80          //127
#define NOTE_ON 0x90           // 144
#define KEY_PRESSURE 0xA0      // 160
#define CC 0xB0                // 176
#define PROGRAM_CHANGE 0xC0    // 192
#define CHANNEL_PRESSURE 0xD0  // 208
#define PITCH_BEND 0xE0        // 224

// Fingering Patterns
#define kModeWhistle 0
#define kModeUilleann 1
#define kModeGHB 2
#define kModeNorthumbrian 3
#define kModeChromatic 4
#define kModeGaita 5
#define kModeNAF 6
#define kModeKaval 7
#define kModeRecorder 8
#define kModeRegulators 9         //only used for a custom regulators implementation, not the "official" software
#define kModeUilleannStandard 10  //contains no accidentals
#define kModeXiao 11
#define kModeSax 12
#define kModeGaitaExtended 13
#define kModeSaxBasic 14
#define kModeEVI 15
#define kModeShakuhachi 16
#define kModeSackpipaMajor 17
#define kModeSackpipaMinor 18
#define kModeCustom 19
#define kModeBombarde 20
#define kModeBaroqueFlute 21
#define kModeNModes 22

// Pitch bend modes
#define kPitchBendSlideVibrato 0
#define kPitchBendVibrato 1
#define kPitchBendNone 2
#define kPitchBendLegatoSlideVibrato 3
#define kPitchBendNModes 4

// Register control modes
#define kPressureSingle 0
#define kPressureBreath 1
#define kPressureThumb 2
#define kPressureBell 3
#define kPressureNModes 4

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

//used in register state machine
#define SILENCE 1
#define BOTTOM_REGISTER 2
#define TOP_REGISTER 3
#define SILENCE_HYSTERESIS 1
#define JUMP 0
#define DROP 1

//Variables in the switches array (settings for the swiches in the slide/vibrato and register control panels)
#define VENTED 0
#define BAGLESS 1
#define SECRET 2
#define INVERT 3
#define CUSTOM 4
#define SEND_VELOCITY 5
#define SEND_AFTERTOUCH 6  //second bit of this one is used for poly
#define FORCE_MAX_VELOCITY 7
#define IMMEDIATE_PB 8
#define LEGATO 9
#define OVERRIDE 10
#define THUMB_AND_OVERBLOW 11
#define R4_FLATTEN 12
#define kSWITCHESnVariables 13

//Variables in the ED array (all the settings for the Expression and Drones panels)
#define EXPRESSION_ON 0
#define EXPRESSION_DEPTH 1
#define SEND_PRESSURE 2
#define CURVE 3  // (0 is linear, 1 and 2 are power curves)
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
#define VELOCITY_INPUT_PRESSURE_MIN 21
#define VELOCITY_INPUT_PRESSURE_MAX 22
#define VELOCITY_OUTPUT_PRESSURE_MIN 23
#define VELOCITY_OUTPUT_PRESSURE_MAX 24
#define AFTERTOUCH_INPUT_PRESSURE_MIN 25
#define AFTERTOUCH_INPUT_PRESSURE_MAX 26
#define AFTERTOUCH_OUTPUT_PRESSURE_MIN 27
#define AFTERTOUCH_OUTPUT_PRESSURE_MAX 28
#define POLY_INPUT_PRESSURE_MIN 29
#define POLY_INPUT_PRESSURE_MAX 30
#define POLY_OUTPUT_PRESSURE_MIN 31
#define POLY_OUTPUT_PRESSURE_MAX 32
#define VELOCITY_CURVE 33
#define AFTERTOUCH_CURVE 34
#define POLY_CURVE 35
#define EXPRESSION_MIN 36
#define EXPRESSION_MAX 37
#define CUSTOM_FINGERING_1 38
#define CUSTOM_FINGERING_2 39
#define CUSTOM_FINGERING_3 40
#define CUSTOM_FINGERING_4 41
#define CUSTOM_FINGERING_5 42
#define CUSTOM_FINGERING_6 43
#define CUSTOM_FINGERING_7 44
#define CUSTOM_FINGERING_8 45
#define CUSTOM_FINGERING_9 46
#define CUSTOM_FINGERING_10 47
#define CUSTOM_FINGERING_11 48
#define kEXPRESSIONnVariables 49

//GPIO constants
const uint8_t ledPin = 13;
const uint8_t holeTrans[] = { 5, 9, 10, 0, 1, 2, 3, 11, 6 };                 //the analog pins used for the tone hole phototransistors, in the following order: Bell,R4,R3,R2,R1,L3,L2,L1,Lthumb
const GPIO_pin_t pins[] = { DP11, DP6, DP8, DP5, DP7, DP1, DP0, DP3, DP2 };  //the digital pins used for the tone hole leds, in the following order: Bell,R4,R3,R2,R1,L3,L2,L1,Lthumb. Uses a special declaration format for the GPIO library.
const GPIO_pin_t buttons[] = { DP15, DP14, DP16 };                           //the pins used for the buttons

//instrument
byte mode = 0;         // The current mode (instrument), from 0-2.
byte defaultMode = 0;  // The default mode, from 0-2.

//variables that can change according to instrument.
int8_t octaveShift = 0;                       //octave transposition
int8_t noteShift = 0;                         //note transposition, for changing keys. All fingering patterns are initially based on the key of D, and transposed with this variable to the desired key.
byte pitchBendMode = kPitchBendSlideVibrato;  //0 means slide and vibrato are on. 1 means only vibrato is on. 2 is all pitchbend off, 3 is legato slide/vibrato.
byte senseDistance = 10;                      //the sensor value above which the finger is sensed for bending notes. Needs to be higher than the baseline sensor readings, otherwise vibrato will be turned on erroneously.
byte breathMode = kPressureBreath;            //the desired presure sensor behavior: single register, overblow, thumb register control, bell register control.
unsigned int vibratoDepth = 1024;             //vibrato depth from 0 (no vibrato) to 8191 (one semitone)
bool useLearnedPressure = 0;                  //whether we use learned pressure for note on threshold, or we use calibration pressure from startup
byte midiBendRange = 2;                       // +/- semitones that the midi bend range represents
byte mainMidiChannel = 1;                     // current MIDI channel to send notes on

//These are containers for the above variables, storing the value used by the three different instruments.  First variable in array is for instrument 0, etc.
byte modeSelector[] = { kModeWhistle, kModeUilleann, kModeGHB };  //the fingering patterns chosen in the configuration tool, for the three instruments.
int8_t octaveShiftSelector[] = { 0, 0, 0 };
int8_t noteShiftSelector[] = { 0, 0, 8 };
byte pitchBendModeSelector[] = { 1, 1, 1 };
byte senseDistanceSelector[] = { 10, 10, 10 };
byte breathModeSelector[] = { 1, 1, 0 };
byte useLearnedPressureSelector[] = { 0, 1, 1 };  //default to using learned pressure for isntruments 2 and 3
int learnedPressureSelector[] = { 0, 280, 475 };  //some reasonable default bag pressure setting for uilleann and GHB
byte LSBlearnedPressure;                          //used to reconstruct learned pressure from two MIDI bytes.
unsigned int vibratoHolesSelector[] = { 0b011111111, 0b011111111, 0b011111111 };
unsigned int vibratoDepthSelector[] = { 1024, 1024, 1024 };
byte midiBendRangeSelector[] = { 2, 2, 2 };
byte midiChannelSelector[] = { 1, 1, 1 };

bool momentary[3][3] = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };  //whether momentary click behavior is desired for MIDI on/off message sent with a button. Dimension 0 is mode (instrument), dimension 1 is button 0,1,2.

byte switches[3][13] =  //the settings for the five switches in the vibrato/slide and register control panels
                        //instrument 0
  {
      {
        1,  // vented (breath/mouthpiece) on or off (there are different pressure settings for breath/mouthpiece)
        0,  // bagless mode off or on
        0,  // secret button command mode off or on
        0,  // whether the functionality for using the right thumb or the bell sensor for increasing the register is inverted.
        0,  // off/on for Michael Eskin's custom vibrato approach
        0,  // send pressure as NoteOn velocity off or on
        0,  // send pressure as aftertouch (channel pressure) off or on, and/or poly aftertouch (2nd bit)
        1,  // force maximum velocity (127)
        0,  // send pitchbend immediately before Note On (recommnded for MPE)
        1,  // send legato (Note On message before Note Off for previous note)
        0,  //override pitch expression pressure range
        0,  //use both thumb and overblowing for register control with custom fingering chart
        0   //use R4 finger to flatten any note one semitone with custom fingering chart
      },

      //same for instrument 1
      { 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0 },

      //same for instrument 2
      { 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0 }
  };

byte ED[3][49] =  //an array that holds all the settings for the Expression and Drones Control panels in the Configuration Tool.
                  //instrument 0
  {
      {
        0,    //EXPRESSION_ON
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
        51,   //DRONES_ON_BYTE2
        36,   //DRONES_ON_BYTE3
        0,    //DRONES_OFF_COMMAND
        1,    //DRONES_OFF_CHANNEL
        51,   //DRONES_OFF_BYTE2
        36,   //DRONES_OFF_BYTE3
        0,    //DRONES_CONTROL_MODE (0 is no drone control, 1 is use secret button, 2 is use chanter, 3 is use pressure.
        0,    //DRONES_PRESSURE_LOW_BYTE
        0,    //DRONES_PRESSURE_HIGH_BYTE
        0,    //VELOCITY_INPUT_PRESSURE_MIN
        100,  //VELOCITY_INPUT_PRESSURE_MAX
        0,    //VELOCITY_OUTPUT_PRESSURE_MIN
        127,  //VELOCITY_OUTPUT_PRESSURE_MAX
        0,    //AFTERTOUCH_INPUT_PRESSURE_MIN
        100,  //AFTERTOUCH_INPUT_PRESSURE_MAX
        0,    //AFTERTOUCH_OUTPUT_PRESSURE_MIN
        127,  //AFTERTOUCH_OUTPUT_PRESSURE_MAX
        0,    //POLY_INPUT_PRESSURE_MIN
        100,  //POLY_INPUT_PRESSURE_MAX
        0,    //POLY_OUTPUT_PRESSURE_MIN
        127,  //POLY_OUTPUT_PRESSURE_MAX
        0,    //VELOCITY_CURVE
        0,    //AFTERTOUCH_CURVE
        0,    //POLY_CURVE
        0,    //EXPRESSION_MIN
        100,  //EXPRESSION_MAX
        0,    //CUSTOM_FINGERING_1
        74,   //CUSTOM_FINGERING_2
        73,   //CUSTOM_FINGERING_3
        72,   //CUSTOM_FINGERING_4
        71,   //CUSTOM_FINGERING_5
        69,   //CUSTOM_FINGERING_6
        67,   //CUSTOM_FINGERING_7
        66,   //CUSTOM_FINGERING_8
        64,   //CUSTOM_FINGERING_9
        62,   //CUSTOM_FINGERING_10
        61    //CUSTOM_FINGERING_11
      },

      //same for instrument 1
      { 0, 3, 0, 0, 1, 7, 0, 100, 0, 127, 0, 1, 51, 36, 0, 1, 51, 36, 0, 0, 0, 0, 127, 0, 127, 0, 127, 0, 127, 0, 127, 0, 127, 0, 0, 0, 0, 100, 0, 74, 73, 72, 71, 69, 67, 66, 64, 62, 61 },

      //same for instrument 2
      { 0, 3, 0, 0, 1, 7, 0, 100, 0, 127, 0, 1, 51, 36, 0, 1, 51, 36, 0, 0, 0, 0, 127, 0, 127, 0, 127, 0, 127, 0, 127, 0, 127, 0, 0, 0, 0, 100, 0, 74, 73, 72, 71, 69, 67, 66, 64, 62, 61 }
  };

byte pressureSelector[3][12] =  //a selector array for all the register control variables that can be changed in the Configuration Tool
                                //instrument 0
  {
      { 50, 20, 20, 15, 50, 75,  //bag: offset, multiplier, hysteresis, drop (now unused), jump time, drop time
        3, 7, 20, 0, 12, 50 },   //breath/mouthpiece: offset, multiplier, hysteresis, **now used for transientFilter**, jump time, drop time
      //instrument 1
      {
        50, 20, 20, 15, 50, 75,
        3, 7, 20, 0, 12, 50 },
      //instrument 2
      {
        50, 20, 20, 15, 50, 75,
        3, 7, 20, 0, 12, 50 }
  };

uint8_t buttonPrefs[3][8][5] =  //The button configuration settings (no default actions as of formware 2.1 to avoid confusion with beginning users). Dimension 1 is the three instruments. Dimension 2 is the button combination: click 1, click 2, click3, hold 2 click 1, hold 2 click 3, longpress 1, longpress2, longpress3
                                //Dimension 3 is the desired action: Action, MIDI command type (noteon/off, CC, PC), MIDI channel, MIDI byte 2, MIDI byte 3.
                                //instrument 0---the actions are: 0 none, 1 send MIDI message, 2 change pitchbend mode, 3 instrument, 4 play/stop (bagless mode), 5 octave shift up, 6 octave shift down, 7 MIDI panic, 8 change register control mode, 9 drones on/off, 10 semitone shift up, 11 semitone shift down, 12 begin autocalibration
  { { { 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0 } },

    //same for instrument 1
    { { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 } },

    //same for instrument 2
    { { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 } } };

//other misc. variables
byte hardwareRevision = 30;
unsigned long ledTimer = 0;     //for blinking LED
byte blinkNumber = 1;           //the number of remaining blinks when blinking LED to indicate control changes
bool LEDon = 0;                 //whether the LED is currently on
bool play = 0;                  //turns sound off and on (with the use of a button action) when in bagless mode
bool bellSensor = 0;            //whether the bell sensor is plugged in
bool prevBellSensor = 0;        //the previous reading of the bell sensor detection pin
unsigned long initialTime = 0;  //for testing
unsigned long finalTime = 0;    //for testing
unsigned long cycles = 0;       //for testing
byte program = 0;               //current MIDI program change value. This always starts at 0 but can be increased/decreased with assigned buttons.
bool dronesState = 0;           //keeps track of whether we're above or below the pressure threshold for turning drones on.

//variables for reading pressure sensor
volatile unsigned int tempSensorValue = 0;  //for holding the pressure sensor value inside the ISR
int sensorValue = 0;                        // first value read from the pressure sensor
int sensorValue2 = 0;                       // second value read from the pressure sensor, for measuring rate of change in pressure
int prevSensorValue = 0;                    // previous sensor reading, used to tell if the pressure has changed and should be sent.
int sensorCalibration = 0;                  //the sensor reading at startup, used as a base value
byte offset = 15;                           //called "threshold" in the Configuration Tool-- used along with the multiplier for calculating the transition to the second register
byte multiplier = 15;                       //controls how much more difficult it is to jump to second octave from higher first-octave notes than from lower first-octave notes. Increasing this makes playing with a bag more forgiving but requires more force to reach highest second-octave notes. Can be set according to fingering mode and breath mode (i.e. a higher jump factor may be used for playing with a bag). Array indices 1-3 are for breath mode jump factor, indices 4-6 are for bag mode jump factor.
byte customScalePosition;                   //used to indicate the position of the current note on the custom chart scale (needed for state calculation)
int sensorThreshold[] = { 260, 0 };         //the pressure sensor thresholds for initial note on and shift from register 1 to register 2, before some transformations.
int upperBound = 255;                       //this represents the pressure transition between the first and second registers. It is calculated on the fly as: (sensorThreshold[1] + ((newNote - 60) * multiplier))
byte newState;                              //the note/octave state based on the sensor readings (1=not enough force to sound note, 2=enough force to sound first octave, 3 = enough force to sound second octave)
byte prevState = 1;                         //the previous state, used to monitor change necessary for adding a small delay when a note is turned on from silence and we're sending not on velocity based on pressure.
unsigned long velocityDelayTimer = 0;       //a timer for the above delay.
boolean sensorDataReady = 0;                //tells us that pressure data is available
int jumpTime = 15;                          //the amount of time to wait before dropping back down from an octave jump to first octave because of insufficient pressure
int dropTime = 15;                          //the amount of time to wait (ms) before turning a note back on after dropping directly from second octave to note off
byte hysteresis = 15;                       //register hysteresis
byte soundTriggerOffset = 3;                //the number of sensor values above the calibration setting at which a note on will be triggered (first octave)
int learnedPressure = 0;                    //the learned pressure reading, used as a base value
int currentState;                           //these several are used by the new state machine
int rateChangeIdx = 0;
int previousPressure = 0;
int previousAverage1 = 0;
int previousAverage2 = 0;
bool holdoffActive = false;
int holdoffCounter = 0;
int upperBoundHigh;                  //register boundary for moving up
int upperBoundLow;                   //register boudary for moving down (with hysteresis)
unsigned long fingeringChangeTimer;  //times how long it's been since the most recent fingering change. Used to hold off the register drop feature until we've "settled" in to a fingering pattern

unsigned int inputPressureBounds[4][4] = {
    //for mapping pressure input range to output range. Dimension 1 is CC, velocity, aftertouch, poly. Dimension 2 is minIn, maxIn, scaledMinIn, mappedPressure
    { 100, 800, 0, 0 },
    { 100, 800, 0, 0 },
    { 100, 800, 0, 0 },
    { 100, 800, 0, 0 },
};

unsigned long pressureInputScale[4] =  // precalculated scale factor for mapping the input pressure range, for CC, velocity, aftertouch, and poly.
  { 0, 0, 0, 0 };

byte outputBounds[4][2] = {  // container for ED output pressure range variables (CC, velocity, aftertouch, poly)-- the ED variables will be copied here so they're in a more logical order. This is a fix for variables that were added later.
    { 0, 127 },
    { 0, 127 },
    { 0, 127 },
    { 0, 127 }
};

byte curve[4] = { 0, 0, 0, 0 };  //similar to above-- more logical odering for the pressure curve variable


//variables for reading tonehole sensors
volatile byte lastRead = 0;                                        //the transistor that was last read, so we know which to read the next time around the loop.
unsigned int toneholeCovered[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };    //covered hole tonehole sensor readings for calibration
int toneholeBaseline[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };            //baseline (uncovered) hole tonehole sensor readings
volatile int tempToneholeRead[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };   //temporary storage for tonehole sensor readings with IR LED on, written during the timer ISR
int toneholeRead[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };                //storage for tonehole sensor readings, transferred from the above volatile variable
volatile int tempToneholeReadA[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };  //temporary storage for ambient light tonehole sensor readings, written during the timer ISR
bool toneholesReady = false;                                       // Indicates when a fresh reading of the tone holes is available
bool toneholesReadyInterupt = false;
unsigned int holeCovered = 0;      //whether each hole is covered-- each bit corresponds to a tonehole.
uint8_t tempCovered = 0;           //used when masking holeCovered to ignore certain holes depending on the fingering pattern.
bool fingersChanged = 1;           //keeps track of when the fingering pattern has changed.
unsigned int prevHoleCovered = 1;  //so we can track changes.
volatile int tempNewNote = 0;
byte prevNote;
byte newNote = -1;             //the next note to be played, based on the fingering chart (does not include transposition).
byte notePlaying;              //the actual MIDI note being played, which we remember so we can turn it off again.
volatile bool firstTime = 1;   // we have the LEDs off ~50% of the time. This bool keeps track of whether each LED is off or on at the end of each timer cycle
volatile byte timerCycle = 0;  //the number of times we've entered the timer ISR with the new sensors.
byte transientFilter = 1;      // small delay for filtering out transient notes

//pitchbend variables
unsigned long pitchBendTimer = 0;                                             //to keep track of the last time we sent a pitchbend message
byte pitchBendOn[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };                           //whether pitchbend is currently turned for for a specific hole
int pitchBend = 8192;                                                         //total current pitchbend value
int prevPitchBend = 8192;                                                     //a record of the previous pitchBend value, so we don't send the same one twice
int iPitchBend[] = { 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192 };  //current pitchbend value for each tonehole
int pitchBendPerSemi = 4096;
int prevChanPressure = 0;
int prevCCPressure = 0;
int prevPolyPressure = 0;
unsigned long pressureTimer = 0;                               //to keep track of the last time we sent a pressure message
unsigned long noteOnTimestamp = 0;                             // ms timestamp the note was activated
byte slideHole;                                                //the hole above the current highest uncovered hole. Used for detecting slides between notes.
byte stepsDown = 1;                                            //the number of half steps down from the slideHole to the next lowest note on the scale, used for calculating pitchbend values.
byte vibratoEnable = 0;                                        // if non-zero, send vibrato pitch bend
unsigned int holeLatched = 0b000000000;                        //holes that are disabled for vibrato because they were covered when the note was triggered. They become unlatched (0) when the finger is removed all the way.
unsigned int vibratoHoles = 0b111111111;                       //holes to be used for vibrato, left thumb on left, bell sensor far right.
unsigned int toneholeScale[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };  //a scale for normalizing the range of each sensor, for sliding
unsigned int vibratoScale[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };   //same as above but for vibrato
int expression = 0;                                            //pitchbend up or down from current note based on pressure
bool customEnabled = 0;                                        //Whether the custom vibrato above is currently enabled based on fingering pattern and pitchbend mode.
int adjvibdepth;                                               //vibrato depth scaled to MIDI bend range.

//variables for managing MIDI note output
bool noteon = 0;      //whether a note is currently turned on
bool shiftState = 0;  //whether the octave is shifted (could be combined with octaveShift)
int8_t shift = 0;     //the total amount of shift up or down from the base note 62 (D). This takes into account octave shift and note shift.
byte velocity = 127;  //default MIDI note velocity

//tonehole calibration variables
byte calibration = 0;  //whether we're currently calibrating. 1 is for calibrating all sensors, 2 is for calibrating bell sensor only, 3 is for calibrating all sensors plus baseline calibration (normally only done once, in the "factory").
unsigned long calibrationTimer = 0;

//variables for reading buttons
unsigned long buttonReadTimer = 0;              //for telling when it's time to read the buttons
byte integrator[] = { 0, 0, 0 };                //stores integration of button readings. When this reaches MAXIMUM, a button press is registered. When it reaches 0, a release is registered.
bool pressed[] = { 0, 0, 0 };                   //whether a button is currently presed (this it the output from the integrator)
bool released[] = { 0, 0, 0 };                  //if a button has just been released
bool justPressed[] = { 0, 0, 0 };               //if a button has just been pressed
bool prevOutput[] = { 0, 0, 0 };                //previous state of button, to track state through time
bool longPress[] = { 0, 0, 0 };                 //long button press
unsigned int longPressCounter[] = { 0, 0, 0 };  //for counting how many readings each button has been held, to indicate a long button press
bool noteOnOffToggle[] = { 0, 0, 0 };           //if using a button to toggle a noteOn/noteOff command, keep track of state.
bool longPressUsed[] = { 0, 0, 0 };             //if we used a long button press, we set a flag so we don't use it again unless the button has been released first.
bool buttonUsed = 0;                            //flags any button activity, so we know to handle it.
bool specialPressUsed[] = { 0, 0, 0 };
bool dronesOn = 0;  //used to monitor drones on/off.

//variables for communication with the WARBL Configuration Tool
bool communicationMode = 0;      //whether we are currently communicating with the tool.
byte buttonReceiveMode = 100;    //which row in the button configuration matrix for which we're currently receiving data.
byte pressureReceiveMode = 100;  //which pressure variable we're currently receiving date for. From 1-12: Closed: offset, multiplier, jump, drop, jump time, drop time, Vented: offset, multiplier, jump, drop, jump time, drop time
byte counter = 0;                // We use this to know when to send a new pressure reading to the configuration tool. We increment it every time we send a pitchBend message, to use as a simple timer wihout needing to set another actual timer.
byte fingeringReceiveMode = 0;   // indicates the mode (instrument) for which a fingering pattern is going to be sent
byte holeDebounceCounter = 0;    // countdown of idenitical tone holes readings before they are accepted.

void setup() {

    DIDR0 = 0xff;  // disable digital input circuits for analog pins
    DIDR2 = 0xf3;

    pinMode2(ledPin, OUTPUT);    // Initialize the LED pin as an output (using the fast DIO2 library).
    pinMode2(17, INPUT_PULLUP);  //this pin is used to detect when the bell sensor is plugged in (high when plugged in).

    for (byte i = 0; i < 9; i++) {  //Initialize the tonehole sensor IR LEDs.
        pinMode2f(pins[i], OUTPUT);
    }

    pinMode2f(DP15, INPUT_PULLUP);  //set buttons as inputs and enable internal pullup
    pinMode2f(DP16, INPUT_PULLUP);
    pinMode2f(DP14, INPUT_PULLUP);

    // EEPROM.update(44,255); //can be uncommented to force factory settings to be resaved for testing (after making changes to factory settings). Needs to be recommented again after.


    if (EEPROM.read(44) != 3 || (EEPROM.read(1011) != 20 && EEPROM.read(1011) != 21)) {
        EEPROM.update(1011, VERSION);  //update the stored software version
        saveFactorySettings();         //If we're running the software for the first time, if a factory reset has been requested, or if the software version is less than 20, copy all settings to EEPROM.
    }

    if (EEPROM.read(1011) == 20) {
        EEPROM.update(1011, VERSION);   //update the stored software version
        for (byte i = 0; i < 3; i++) {  //save new advanced overblowing settings for all three instruments if we've updated from version 20.
            mode = i;
            for (byte q = 0; q < 12; q++) {
                EEPROM.update((260 + q + (i * 20)), pressureSelector[mode][q]);
            }
        }
    }

    if (EEPROM.read(37) == 3) {
        loadCalibration();  //If there has been a calibration saved, reload it at startup.
    }

    loadFingering();
    loadSettingsForAllModes();
    mode = defaultMode;  //set the startup instrument

    analogRead(A4);  // the first analog readings are sometimes nonsense, so we read a few times and throw them away.
    analogRead(A4);
    sensorCalibration = analogRead(A4);
    sensorValue = sensorCalibration;  //an initial reading to "seed" subsequent pressure readings

    loadPrefs();  //load the correct user settings based on current instrument.

    //prepare sensors
    Timer1.initialize(100);  //this timer is only used to add some additional time after reading all sensors, for power savings.
    Timer1.attachInterrupt(timerDelay);
    Timer1.stop();  //stop the timer because we don't need it until we've read all the sensors once.
    ADC_init();     //initialize the ADC and start conversions
}




void loop() {

    //cycles ++; //for testing

    receiveMIDI();

    if ((millis() - buttonReadTimer) >= 5) {  //read the state of the control buttons every so often
        checkButtons();
        buttonReadTimer = millis();
    }

    if (buttonUsed) {
        handleButtons();  //if a button had been used, process the command. We only do this when we need to, so we're not wasting time.
    }

    if (blinkNumber > 0) {
        blink();  //blink the LED if necessary (indicating control changes, etc.)
    }

    if (calibration > 0) {
        calibrate();  //calibrate/continue calibrating if the command has been received.
    }

    noInterrupts();
    for (byte i = 0; i < 9; i++) {
        toneholeRead[i] = tempToneholeRead[i];  //transfer sensor readings to a variable that won't get modified in the ISR
    }
    toneholesReady = toneholesReadyInterupt;
    toneholesReadyInterupt = false;
    interrupts();


    for (byte i = 0; i < 9; i++) {
        if (calibration == 0) {  //if we're not calibrating, compensate for baseline sensor offset (the stored sensor reading with the hole completely uncovered)
            toneholeRead[i] = toneholeRead[i] - toneholeBaseline[i];
        }
        if (toneholeRead[i] < 0) {  //in rare cases the adjusted readings can end up being negative.
            toneholeRead[i] = 0;
        }
    }


    get_fingers();  //find which holes are covered


    unsigned long nowtime = millis();  //get the current time for the timers used below




    if (debounceFingerHoles()) {
        fingersChanged = 1;

        tempNewNote = get_note(holeCovered);  //get the next MIDI note from the fingering pattern if it has changed
        send_fingers();                       //send new fingering pattern to the Configuration Tool
        if (pitchBendMode == kPitchBendSlideVibrato || pitchBendMode == kPitchBendLegatoSlideVibrato) {
            findStepsDown();
        }



        if (tempNewNote != -1 && newNote != tempNewNote) {  //If a new note has been triggered
            if (pitchBendMode != kPitchBendNone) {
                holeLatched = holeCovered;  //remember the pattern that triggered it (it will be used later for vibrato)
                for (byte i = 0; i < 9; i++) {
                    iPitchBend[i] = 0;  //and reset pitchbend
                    pitchBendOn[i] = 0;
                }
            }
        }


        newNote = tempNewNote;
        tempNewNote = -1;
        fingeringChangeTimer = nowtime;  //start timing after the fingering pattern has changed

        get_state();  //recalculate state if the fingering has changed
    }


    if (sensorDataReady) {
        get_state();  //get the breath state from the pressure sensor if there's been a reading.
    }



    if (switches[mode][SEND_VELOCITY]) {  //if we're sending NoteOn velocity based on pressure
        if (prevState == 1 && newState != 1) {
            velocityDelayTimer = nowtime;  //reset the delay timer used for calculating velocity when a note is turned on after silence.
        }
        prevState = newState;
    }

    get_shift();  //shift the next note up or down based on register, key, and characteristics of the current fingering pattern.

    if ((nowtime - pressureTimer) >= ((nowtime - noteOnTimestamp) < 20 ? 2 : 5)) {
        pressureTimer = nowtime;
        if (abs(prevSensorValue - sensorValue) > 1) {  //if pressure has changed more than a little, send it.
            if (ED[mode][SEND_PRESSURE]) {
                calculatePressure(0);
            }
            if (switches[mode][SEND_VELOCITY]) {
                calculatePressure(1);
            }
            if (switches[mode][SEND_AFTERTOUCH] & 1) {
                calculatePressure(2);
            }
            if (switches[mode][SEND_AFTERTOUCH] & 2) {
                calculatePressure(3);
            }

            sendPressure(false);

            if (communicationMode) {
                sendUSBMIDI(CC, 7, 116, sensorValue & 0x7F);  //send LSB of current pressure to configuration tool
                sendUSBMIDI(CC, 7, 118, sensorValue >> 7);    //send MSB of current pressure
            }
            prevSensorValue = sensorValue;
        }
    }

    if ((nowtime - pitchBendTimer) >= 9) {  //check pitchbend and send pressure data every so often
        pitchBendTimer = nowtime;

        calculateAndSendPitchbend();


        counter++;

        if (counter == 10) {  //we check every 10 ticks to see if the bell sensor has been plugged/unplugged.

            counter = 0;
            bellSensor = (digitalRead2(17));  //check if the bell sensor is plugged in
            if (prevBellSensor != bellSensor) {
                prevBellSensor = bellSensor;
                if (communicationMode) {
                    sendUSBMIDI(CC, 7, 102, 120 + bellSensor);  //if it's changed, tell the configuration tool.
                }
            }

            //Serial.println(hysteresis);
            //Serial.println(jumpTime);
            //Serial.println(dropTime);
            //Serial.println("");
            //FREERAM_PRINT
        }
    }

    sendNote();  //send the MIDI note
}