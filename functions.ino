//Monitor the status of the 3 buttons. The efficient integrating debouncing algorithm is taken from debounce.c, written by Kenneth A. Kuhn:http://www.kennethkuhn.com/electronics/debounce.c
void checkButtons() {


  for (byte j = 0; j < 3; j++) {


    if (longPressUsed[j]) { //if we've used a long button press, don't flag it as another long press until it's been released.
      longPress[j] = 0;
      longPressCounter[j] = 0;
    }


    if (digitalRead2f(buttons[j]) == 0)
    { if (integrator[j] > 0)
        integrator[j]--;
    }
    else if (integrator[j] < MAXIMUM)
      integrator[j]++;
    if (integrator[j] == 0) {
      pressed[j] = 1; //we make the output the inverse of the input so that a pressed button reads as a "1".
      buttonUsed = 1;
    } //flag that there's been button activity, so we know to handle it.
    else if (integrator[j] >= MAXIMUM)
    { pressed[j] = 0;
      integrator[j] = MAXIMUM;
    }  // defensive code if integrator got corrupted

    if (prevOutput[j] == 1 && pressed[j] == 0 && !longPressUsed[j]) {
      released[j] = 1; //determine whether a button has just been released
      buttonUsed = 1;
    }

    if (prevOutput[j] == 0 && pressed[j] == 1 && !longPressUsed[j]) {
      justPressed[j] = 1; //determine whether a button has just been pressed
      buttonUsed = 1;
    }
    else {
      justPressed[j] = 0;
    }

    if (prevOutput[j] == 1 && pressed[j] == 1) { //determine whether a button has been held for a long press
      longPressCounter[j]++;
    }
    if (longPressCounter[j] > 300 && !longPressUsed[j]) {
      longPress[j] = 1;
      buttonUsed = 1;
    } //flag that there's been button activity, so we know to handle it.

    if (pressed[j] == 0) {
      longPress[j] = 0;
      longPressCounter[j] = 0;
    }

    prevOutput[j] = pressed[j]; //keep track of state for catching releases and long presses.

    if (pressed[j] == 0) {
      longPressUsed[j] = 0; //if a button is not pressed, reset the flag that tells us it's been used for a long press.
    }
  }


}



//Timer ISR for reading tonehole sensors and pressure sensor
void readSensors(void) {


  if (lastRead == 9) {
    // digitalWrite2(11,HIGH); //for debug using oscilloscope plugged into bell sensor jack
    tempSensorValue = analogRead(A4);  //we read the pressure sensor after reading all the tonehole sensors
    // digitalWrite2(11,LOW); //for debug using oscilloscope plugged into bell sensor jack
    lastRead = 0;
    sensorDataReady = 1;
    return;
  }

  if (firstTime) { //We've waited once without an LED on, to reduce average power consumption. Now turn an LED on for the current hole, take an ambient light reading for the next hole, then exit and wait for the timer again.
    digitalWrite2f(pins[lastRead], HIGH);
    if (lastRead == 8) {
      tempToneholeReadA[0] = analogRead(holeTrans[0]); //take an ambient light reading for the next hole with LED off
    }
    else {
      tempToneholeReadA[lastRead + 1] = analogRead(holeTrans[lastRead + 1]);
    }
    firstTime = 0;//toggle this so next time we know to take a reading.
    return;
  }

  if (!firstTime) { //If enough time has past again, read the current transistor with its LED on.

    tempToneholeRead[lastRead] = (analogRead(holeTrans[lastRead])); //take a reading with the LED on
    digitalWrite2f(pins[lastRead], LOW);
    tempToneholeRead[lastRead] = tempToneholeRead[lastRead] - tempToneholeReadA[lastRead]; //subtract the ambient light reading
    if (tempToneholeRead[lastRead] < 0) {
      tempToneholeRead[lastRead] = 0;
    }
    lastRead++;
    firstTime = 1;
  } //toggle this so we know to take an ambient reading next time
}


//Return a MIDI note number (0-127) based on the current fingering. The analog readings of the 9 hole sensors are also stored in the tempToneholeRead variable for later use.
int get_note() {

  int ret = -1;  //default for unknown fingering

  for (byte i = 0; i < 9; i++) {
    if ((toneholeRead[i]) > (toneholeCovered[i] - 50)) {
      bitWrite(holeCovered, i, 1); //use the tonehole readings to decide which holes are newly covered
    }
    if ((toneholeRead[i]) <= (toneholeCovered[i] - 50 - hysteresis)) {
      bitWrite(holeCovered, i, 0); //decide which holes have been uncovered -- the "hole uncovered" reading is a little less then the "hole covered" reading, to prevent oscillations.
    }
  }

 slideHole = findleftmostunsetbit(holeCovered); //use highest uncovered hole for sliding
 //if(slideHole == 127) {slideHole = 8;} //can turn this on to include the thumb hole as a slide hole.

  // MAE FOOFOO 21Nov2018 - Checking communicationMode first to avoid second test if off
  if (communicationMode && prevHoleCovered != holeCovered) { //send information about which holes are covered to the Configuration Tool if we're connected.
    byte a = highByte(holeCovered) << 1;
    if (bitRead(lowByte(holeCovered), 7) == 1) {
      bitSet(a, 0);}
    else {bitClear(a, 0); }
    sendUSBMIDI(CC, 7, 114, a);
    sendUSBMIDI(CC, 7, 115, lowByte(holeCovered));
    prevHoleCovered = holeCovered;
  }

  // MAE FOOFOO 17 Nov 2018 - Added full key map and vibrato flags
  if (modeSelector[mode] == kModeWhistle || modeSelector[mode] == kModeChromatic) {

    uint8_t tempCovered = (0b011111100 & holeCovered) >> 2; //use bitmask and bitshift to ignore thumb sensor, R4 sensor, and bell sensor when using tinwhistle fingering. The R4 value will be checked later to see if a note needs to be flattened.
    ret = pgm_read_byte(&tinwhistle_explicit[tempCovered].midi_note);
    if (modeSelector[mode] == kModeChromatic && bitRead(holeCovered, 1) == 1){ret--;
    
    } //lower a semitone if R4 hole is covered and we're using the chromatic pattern
    vibratoEnable = pgm_read_byte(&tinwhistle_explicit[tempCovered].vibrato);
    return ret;
  }

  // MAE FOOFOO 17 Nov 2018 - Added full key map and vibrato flags
  if (modeSelector[mode] == kModeUilleann) { //uilleann pipe mode

    //If back D open, always play the D and allow finger vibrato
    if ((holeCovered & 0b100000000) == 0){
    vibratoEnable = 0b000010;
     return 74;
    }
    
    uint8_t tempCovered = (0b011111110 & holeCovered) >> 1; //ignore thumb hole and bell sensor

      ret = pgm_read_byte(&uilleann_explicit[tempCovered].midi_note);
      vibratoEnable = pgm_read_byte(&uilleann_explicit[tempCovered].vibrato);
      return ret;
  }

  if (modeSelector[mode] == kModeGHB) { //GHB/Scottish smallpipe mode

     //If back A open, always play the A
    if ((holeCovered & 0b100000000) == 0){
     return 74;
    }

    uint8_t tempCovered = (0b011111110 & holeCovered) >> 1; //ignore thumb hole and bell sensor
          ret = pgm_read_byte(&GHB_explicit[tempCovered].midi_note);
        return ret;
  }

  if (modeSelector[mode] == kModeNorthumbrian) { //Northumbriam smallpipe mode

    uint16_t tempCovered = holeCovered >> 1; //bitshift once to ignore bell sensor reading

    bitWrite(tempCovered, 8, 1); //we set a bit above the highest hole, which will allow us to see if the highest hole is uncovered.

    tempCovered = findleftmostunsetbit(tempCovered);  //here we find the index of the leftmost uncovered hole, which will be used to determine the note from the general chart.

    for (uint8_t i = 0; i < NORTHUMBRIAN_GENERAL_SIZE; i++) {
      if (tempCovered == pgm_read_byte(&northumbrian_general[i].keys)) {
        ret = pgm_read_byte(&northumbrian_general[i].midi_note);
        return ret;
      }
    }
  }


  if (modeSelector[mode] == kModeGaita) { //Gaita mode

    uint16_t tempCovered = holeCovered >> 1; //bitshift once to ignore bell sensor reading
    ret = pgm_read_byte(&gaita_explicit[tempCovered].midi_note);
    if(ret == 0){ret = -1;}
    return ret;
  }

  
  return ret;



}

//State machine that models the way that a tinwhistle etc. begins sounding and jumps octaves in response to breath pressure.
void get_state() {

  noInterrupts();
  sensorValue2 = tempSensorValue; //transfer last reading to a non-volatile variable
  interrupts();

  if (((millis() - jumpTimer) >= jumpTime) && jump) {
    jump = 0; //make it okay to switch octaves again if some time has past since we "jumped" or "dropped" because of rapid pressure change.
  }
  if (((millis() - dropTimer) >= dropTime) && drop) {
    drop = 0;
  }

  if (((sensorValue2 - sensorValue) > jumpValue) && !jump && !drop && breathMode < kPressureSingle && (sensorValue2 > sensorThreshold[0])) {  //if the pressure has increased rapidly (since the last reading) and there's a least enough pressure to turn a note on, jump immediately to the second octave
    newState = 3;
    jump = 1;
    jumpTimer = millis();
    if (newState != prevState) {
      prevState = newState;
    }
    sensorValue = sensorValue2;
    return;
  }

  if (((sensorValue - sensorValue2) > dropValue) && !jump  && breathMode < kPressureSingle && !drop && newState == 3) {  //if we're in second octave and the pressure has dropped rapidly, turn the note off (this lets us drop directly from the second octave to note off).
    newState = 1;
    drop = 1;
    dropTimer = millis();
    if (newState != prevState) {
      prevState = newState;
    }
    sensorValue = sensorValue2;
    return;
  }

  //if there haven't been rapid pressure changes and we haven't just jumped octaves, choose the state based solely on current pressure.
  if (sensorValue2 <= sensorThreshold[0]) {
    newState = 1;
  }
  if (breathMode == kPressureSingle && sensorValue2 > sensorThreshold[0]) {
    newState = 2;
  }
  if (!jump && !drop  && breathMode < kPressureSingle && sensorValue2 > sensorThreshold[0] && (sensorValue2 <= (sensorThreshold[1] + ((newNote - 60) * multiplier)))) {
    newState = 2; //threshold 1 is increased based on the note that will be played (low notes are easier to "kick up" to the next octave than higher notes).
  }
  if (!drop  && breathMode < kPressureSingle && (sensorValue2 > (sensorThreshold[1] + ((newNote - 60) * multiplier)))) {
    newState = 3;
  }
  if (newState != prevState) {
    prevState = newState;
  }

  sensorValue = sensorValue2; //we'll use the current reading as the baseline next time around, so we can monitor the rate of change.
  sensorDataReady = 0; //we've used the sensor reading, so don't use it again


}



//calculate pitchbend expression based on pressure
void getExpression() {
	


	int lowerBound = sensorThreshold[0];
	int upperBound = (sensorThreshold[1] + ((newNote - 60) * multiplier));
	unsigned int halfway = ((upperBound - lowerBound) >> 1) + lowerBound;


	  if(newState == 3) {
       halfway = upperBound + halfway;
       lowerBound = upperBound;
  	}  
	
  	if (sensorValue < halfway){
       byte scale = (((halfway-sensorValue) * expressionDepth * 20)/(halfway - lowerBound)); //should figure out how to do this without dividing.
	     expression = - ((scale * scale) >> 3);
	     }
    else {expression = (sensorValue - halfway) * expressionDepth;   
      }


  //Serial.println(expression);


if(expression > expressionDepth * 200) {expression = expressionDepth * 200;} //put cap on it, because in the upper register or in single-register mode, there's no upper limit


 if(pitchBendMode == kPitchBendNone) { //if we're not using vibrato, send the pitchbend now instead of adding it in later.
pitchBend = 8191 + expression;


if(prevPitchBend != pitchBend){ 
   if (noteon) {sendUSBMIDI(PITCH_BEND,1,pitchBend & 0x7F,pitchBend >> 7);
   prevPitchBend = pitchBend;}
   }
 }
}






//find how many steps down to the next lower note on the scale. Doing this in a function because the fingering charts can't be read from the main page, due to compilation order.
void findStepsDown() { 
   if(modeSelector[mode] == kModeGaita){ 
     stepsDown = pgm_read_byte(&steps[tempNewNote-60]);} //read stepsDown from the chart.
   else if(modeSelector[mode] == kModeGHB){
      stepsDown = pgm_read_byte(&steps[tempNewNote-52]);}
   else {stepsDown = pgm_read_byte(&steps[tempNewNote-59]);} 
}





// finger vibrato support for whistle and uilleann fingering
void handleFingerVibrato(){
        
    pitchBend = 8191; //reset the overall pitchbend
    
    if (vibratoEnable==1){
      pitchBend = 8191 - vibratoDepth;
    }

    pitchBend = pitchBend + expression;
      
    if (prevPitchBend != pitchBend) {
      sendUSBMIDI(PITCH_BEND, 1, pitchBend & 0x7F, pitchBend >> 7);
      prevPitchBend = pitchBend;      
      //pitchBendTimer = millis();
    }

}

//Michael's version
void handleCustomPitchBend() {


  pitchBend = 0;


  if ((modeSelector[mode] == kModeWhistle || modeSelector[mode] == kModeChromatic) && (vibratoEnable == 0b000010)) 
  

  {
  
    // Are both fingers contributing to the vibrato?
    if (((toneholeRead[3] > senseDistance) && (bitRead(holeCovered, 3) != 1)) && ((toneholeRead[2] > senseDistance) && (bitRead(holeCovered, 2) != 1))) {

        // Each contributes half of the bend
        pitchBend = ((toneholeRead[3] - senseDistance) * vibratoScale[3]) >> 2; 
      
        pitchBend += ((toneholeRead[2] - senseDistance) * vibratoScale[2]) >> 2; 
        
    }
    else
    
    {
      
      // Only one finger contributing
      if ((toneholeRead[3] > senseDistance) && (bitRead(holeCovered, 3) != 1)) {
        pitchBend = ((toneholeRead[3] - senseDistance) * vibratoScale[3]) >> 2; 
      }
      
      if ((toneholeRead[2] > senseDistance) && (bitRead(holeCovered, 2) != 1)) {
        pitchBend += ((toneholeRead[2] - senseDistance) * vibratoScale[2]) >> 2; 
      }
    
    }
     
    if (pitchBend > vibratoDepth) {
      pitchBend = vibratoDepth; //cap at max vibrato depth if they combine to add up to more than that (changed by AM in v. 8.5 to fix an unwanted oscillation when both fingers were contributing.
    }


    
    
  }
  else
  if (modeSelector[mode] == kModeUilleann){
            
      if (vibratoEnable == 0b000010) {
        
        // If the back-D is open, and the vibrato hole completely open, max the pitch bend
        if ((holeCovered & 0b100000000) == 0){
           if (bitRead(holeCovered, 3) == 1){
               pitchBend = 0;
            }
           else{
              // Otherwise, bend down proportional to distance
              if (toneholeRead[3] > senseDistance){                
                pitchBend = vibratoDepth - (((toneholeRead[3] - senseDistance) * vibratoScale[3]) >> 2);
              }
              else{
                pitchBend = vibratoDepth;
              }
           }
        }
        else{
          
          if ((toneholeRead[3] > senseDistance) && (bitRead(holeCovered, 3) != 1)) {
            pitchBend = ((toneholeRead[3] - senseDistance) * vibratoScale[3]) >> 2; 
          }
          
          if ((toneholeRead[3] < senseDistance) || (bitRead(holeCovered, 3) == 1)) {
            pitchBend = 0; // If the finger is removed or the hole is fully covered, there's no pitchbend contributed by that hole.
          }
     
          if (pitchBend > 8191) {
            pitchBend = 8191; //cap at 8191 (no pitchbend) if for some reason they add up to more than that
          }
        }
      }
  }

  pitchBend = 8191 - pitchBend + expression;

  if (prevPitchBend != pitchBend) {
    
    if (noteon && pitchBendMode < kPitchBendNone) {
      sendUSBMIDI(PITCH_BEND, 1, pitchBend & 0x7F, pitchBend >> 7);
      prevPitchBend = pitchBend;
    }
    
    
  }

}




//Andrew's version
void handlePitchBend() {


for (byte i=0; i< 9; i++) { 

  

  if (pitchBendMode == kPitchBendSlideVibrato){
    if(toneholeRead[i] > senseDistance && i == slideHole && (bitRead(holeCovered, i)!=1)){
    if (stepsDown == 1)  {iPitchBend[i] = ((toneholeRead[i]-senseDistance) * toneholeScale[i])>>3;} //bend down toward the next lowest note in the scale, assuming it's a half step away
    if (stepsDown == 2)  {iPitchBend[i] = ((toneholeRead[i]-senseDistance) * toneholeScale[i])>>2;} //If there are two half steps down to the next note on the scale, we have to bend twice as rapidly as we approach it.     
    }
  
      if (toneholeRead[i] < senseDistance) {
        pitchBendOn[i] = 0;
        iPitchBend[i] = 0;
      }
  }

  if (toneholeRead[i] < senseDistance && bitRead(holeLatched, i) == 1){(bitWrite(holeLatched, i, 0));} //we "unlatch" (enable for vibrato) a hole if it was covered when the note was triggered but now the finger has been compltely removed.

  if (bitRead(vibratoHoles, i) == 1 && bitRead(holeLatched, i) == 0 && (pitchBendMode == kPitchBendVibrato || (pitchBendMode == kPitchBendSlideVibrato && i != slideHole))) { //if this is a vibrato hole and we're in a mode that uses vibrato, and the hole is unlatched       
            if(toneholeRead[i] > senseDistance && (bitRead(holeCovered, i)!=1)){
               iPitchBend[i] = (((toneholeRead[i]-senseDistance) * vibratoScale[i])>>2); //bend downward
               pitchBendOn[i] = 1;}
            if ((toneholeRead[i] < senseDistance) || (bitRead(holeCovered, i) == 1)) {iPitchBend[i] = 0;}
            if (toneholeRead[i] < senseDistance) {pitchBendOn[i] = 0;}


    if(pitchBendOn[i] == 1 && (bitRead(holeCovered, i)==1)) {iPitchBend[i] = vibratoDepth;} //set vibrato to max downward bend if a hole was being used to bend down and now is covered 


        }

       }
               
pitchBend = 0; //reset the overall pitchbend in preparation for adding up the contributions from all the toneholes.
 for (byte i=0; i< 9; i++) {
     pitchBend = pitchBend + iPitchBend[i]; }
           
if (pitchBend > 8191) {pitchBend = 8191;} //cap at 8191 (no pitchbend) if for some reason they add up to more than that
 

pitchBend = 8191 - pitchBend + expression;

if(prevPitchBend != pitchBend){
   if (pitchBendMode < kPitchBendNone && noteon) {sendUSBMIDI(PITCH_BEND,1,pitchBend & 0x7F,pitchBend >> 7);
   prevPitchBend = pitchBend;}
   }
       
}


//Calibrate the sensors and store them in EEPROM

void calibrate() {

  if (!LEDon) {
    digitalWrite(ledPin, HIGH);
    LEDon = 1;
    calibrationTimer = millis();
    
    if(calibration == 1){ //calibrate all sensors if we're in calibration "mode" 1
      for (byte i = 1; i < 9; i++) {
       toneholeCovered[i] = 0; //first set the calibration to 0 for all of the sensors
    }}
      if (bellSensor) {
        toneholeCovered[0] = 0; //also zero the bell sensor if it's plugged in (doesn't matter which calibration mode for this one).
  }
  }

  if ((calibration == 1 && ((millis() - calibrationTimer) <= 10000)) || (calibration == 2 && ((millis() - calibrationTimer) <= 5000))) { //then set the calibration to the highest reading during the next ten seconds.
    if(calibration == 1){
      for (byte i = 1; i < 9; i++) { //hole pad calibration
        if (toneholeCovered[i] < toneholeRead[i]) {
          toneholeCovered[i] = toneholeRead[i];
        }
      }
    }
  }

  if (bellSensor && toneholeCovered[0] < toneholeRead[0]) {
    toneholeCovered[0] = toneholeRead[0]; //calibrate the bell sensor too if it's plugged in.
  }

  if ((calibration == 1 && ((millis() - calibrationTimer) > 10000)) || (calibration == 2 && ((millis() - calibrationTimer) > 5000))) {
    saveCalibration();
    for (byte i = 0; i < 9; i++) { //here we precalculate a scaling factor for each tonehole sensor for use later when we are controlling pitchbend. It maps the distance of the finger from a tonehole to the maximum pitchbend range for that tonehole.
      toneholeScale[i] = ((8 * 8191) / (toneholeCovered[i] - 50 - senseDistance) / 2);
    } //We multiply by 8 first to reduce rounding errors. We'll divide again later.
  }
}


//save sensor calibration (EEPROM bytes up to 34 are used (plus byte 100 to indicate a saved calibration)
void saveCalibration() {

  for (byte i = 0; i < 9; i++) {
    EEPROM.update((i + 9) * 2, highByte(toneholeCovered[i]));
    EEPROM.update(((i + 9) * 2) + 1, lowByte(toneholeCovered[i]));
  }
  EEPROM.update(36, hysteresis);
  calibration = 0;
  EEPROM.update(37, 3); //we write a 3 to address 100 to indicate that we have stored a set of calibrations.
  digitalWrite(ledPin, LOW);
  LEDon = 0;

}

//Load the stored sensor calibrations from EEPROM
void loadCalibration() {
  for (byte i = 0; i < 9; i++) {
    high = EEPROM.read((i + 9) * 2);
    low = EEPROM.read(((i + 9) * 2) + 1);
    hysteresis = EEPROM.read(36);
    toneholeCovered[i] = word(high, low);
  }
}

//send MIDI messages
void sendUSBMIDI(uint8_t m, uint8_t c, uint8_t d1, uint8_t d2) {  // send a 3-byte MIDI event over USB
  c--; // Channels are zero-based
  m &= 0xF0;
  c &= 0xF;
  d1 &= 0x7F;
  d2 &= 0x7F;
  midiEventPacket_t msg = {m >> 4, m | c, d1, d2};
  noInterrupts();
  MidiUSB.sendMIDI(msg);
  MidiUSB.flush();
  interrupts();

}

void sendUSBMIDI(uint8_t m, uint8_t c, uint8_t d) {  // send a 2-byte MIDI event over USB
  c--; // Channels are zero-based
  m &= 0xF0;
  c &= 0xF;
  d &= 0x7F;
  midiEventPacket_t msg = {m >> 4, m | c, d, 0};
  noInterrupts();
  MidiUSB.sendMIDI(msg);
  MidiUSB.flush();
  interrupts();

}

//check for and handle incoming MIDI messages from the WARBL Configuration Tool.
void receiveMIDI() {

  midiEventPacket_t rx; //check for MIDI input
  do {
    noInterrupts();
    rx = MidiUSB.read();
    interrupts();
    if (rx.header != 0) {

      // Serial.println(rx.byte1 &0x0f);
       //Serial.println(rx.byte3);
      // Serial.println("");

      if ((rx.byte1 & 0x0f) == 6) { //if we're on channel 7, we may be receiving messages from the configuration tool.
        if (rx.byte2 == 102) {  //most settings are controlled by a value in CC 102 (always channel 7).
          if (rx.byte3 > 0 && rx.byte3 <= 18){ //handle sensor calibration commands from the configuration tool.
            blinkNumber = (1);
          if ((rx.byte3 & 1) == 0) {
            toneholeCovered[(rx.byte3 >> 1) - 1] -= 5;
            if ((toneholeCovered[(rx.byte3 >> 1) - 1] - 50 - hysteresis) < 5) { //if the tonehole calibration gets set too low so that it would never register as being uncovered, send a message to the configuration tool.
              sendUSBMIDI(CC, 7, 102, (20 + ((rx.byte3 >> 1) - 1)));
            }
          }
          else {
            toneholeCovered[((rx.byte3 + 1) >> 1) - 1] += 5;
          }}

          if (rx.byte3 == 19) { //save calibration if directed.
            saveCalibration();
            blinkNumber = 3;
          }

          if (rx.byte3 == 127) { //begin auto-calibration if directed.
            calibration = 1;
          }

          if (rx.byte3 == 126) { //when communication is established, send all current settings to tool.
            communicationMode = 1;
            sendSettings();
          }

          if (rx.byte3 == 63) { //change secret button command mode
            secretSelector[mode] = 0;
            loadPrefs();
            blinkNumber = 1;
          }
          if (rx.byte3 == 64) {
            secretSelector[mode] = 1;
            loadPrefs();
            blinkNumber = 1;
          }

          if (rx.byte3 == 65) { //change vented option
            ventedSelector[mode] = 0;
            loadPrefs();
            blinkNumber = 1;
          }
          if (rx.byte3 == 66) {
            ventedSelector[mode] = 1;
            loadPrefs();
            blinkNumber = 1;
          }

          for (byte i = 3; i < 6; i++) { // update the three selected fingering modes if prompted by the tool.
            if (rx.byte3 >= i * 10 && rx.byte3 <= (i * 10) + 5) {
              modeSelector[i - 3] = rx.byte3 - (i * 10);
              loadPrefs();
              blinkNumber = 1;
              ledTimer = millis();
            }
          }         

          for (byte i = 0; i < 3; i++) { //update current mode (instrument) if directed.
            if (rx.byte3 == 60 + i) {
              mode = i;
              play = 0;
              loadPrefs(); //load the correct user settings based on current instrument.
              sendSettings(); //send settings for new mode to tool.
              blinkNumber = abs(mode) + 1;
              ledTimer = millis();
            }
          }

          for (byte i = 0; i < 3; i++) { //update current pitchbend mode if directed.
            if (rx.byte3 == 70 + i) {
              pitchBendModeSelector[mode] = i;
              loadPrefs();
              blinkNumber = abs(pitchBendMode) + 1;         
              ledTimer = millis();
            }
          }

          for (byte i = 0; i < 3; i++) { //update current breath mode if directed.
            if (rx.byte3 == 80 + i) {
              breathModeSelector[mode] = i;
              loadPrefs(); //load the correct user settings based on current instrument.
              blinkNumber = abs(breathMode) + 1;
              ledTimer = millis();
            }
          }        

          for (byte i = 0; i < 8; i++) { //update button receive mode (this indicates the row in the button settings for which the next received byte will be).
            if (rx.byte3 == 90 + i) {
              buttonReceiveMode = i;
            }
          }

          for (byte i = 0; i < 8; i++) { //update button configuration
            if (buttonReceiveMode == i) {
              for (byte j = 0; j < 9; j++) { //update column 0 (action).
                if (rx.byte3 == 100 + j) {
                  buttonPrefs[mode][i][0] = j;
                  blinkNumber = 1;
                  ledTimer = millis();
                }
              }
              for (byte k = 0; k < 3; k++) { //update column 1 (MIDI action).
                if (rx.byte3 == 110 + k) {
                  buttonPrefs[mode][i][1] = k;
                  blinkNumber = 1;
                  ledTimer = millis();
                }
              }
            }
          }

          for (byte i = 0; i < 3; i++) { //update momentary
            if (buttonReceiveMode == i) {
              if (rx.byte3 == 113) {
                momentary[mode][i] = 0;
                blinkNumber = 1;
                ledTimer = millis();
                noteOnOffToggle[i] = 0;
              }
              if (rx.byte3 == 114) {
                momentary[mode][i] = 1;
                blinkNumber = 1;
                ledTimer = millis();
                noteOnOffToggle[i] = 0;
              }
            }           
          }


          if (rx.byte3 == 123) { //save settings as the defaults for the selected fingering mode
            EEPROM.update(40 + mode, modeSelector[mode]);
            EEPROM.update(53 + mode, noteShiftSelector[mode]);
            EEPROM.update(80 + mode, customSelector[mode]);
            EEPROM.update(56 + mode, ventedSelector[mode]);
            EEPROM.update(45 + mode, secretSelector[mode]);
            EEPROM.update(50 + mode, senseDistanceSelector[mode]);
            EEPROM.update(60 + mode, pitchBendModeSelector[mode]);
            EEPROM.update(63 + mode, expressionOnSelector[mode]);
            EEPROM.update(70 + mode, breathModeSelector[mode]);
            EEPROM.update(83 + (mode * 2), lowByte(vibratoHolesSelector[mode]));
            EEPROM.update(84 + (mode * 2), highByte(vibratoHolesSelector[mode]));           
            EEPROM.update(89 + (mode * 2), lowByte(vibratoDepthSelector[mode]));
            EEPROM.update(90 + (mode * 2), highByte(vibratoDepthSelector[mode]));
            EEPROM.update(95 + mode, useLearnedPressureSelector[mode]);
            EEPROM.update(273 + (mode * 2), lowByte(learnedPressureSelector[mode]));
            EEPROM.update(274 + (mode * 2), highByte(learnedPressureSelector[mode]));
            

            for (byte i = 0; i < 12; i++) {
              EEPROM.update((260 + i + (mode * 20)), pressureSelector[mode][i]);
            }
 
            for (byte i = 0; i < 3; i++) {
              EEPROM.update(250 + (mode * 3) + i, momentary[mode][i]);

              for (byte j = 0; j < 5; j++) { //save button configuration for current mode
                for (byte k = 0; k < 8; k++) {
                  EEPROM.update(100 + (mode * 50) + (j * 10) + k, buttonPrefs[mode][k][j]);
                }
              }
            }

            blinkNumber = 3;
            ledTimer = millis();
          }

          if (rx.byte3 == 124) { //Save settings as the defaults for all fingering modes
            saveSettingsForAllModes();
          }

          if (rx.byte3 == 125) { //restore all factory settings
            restoreFactorySettings();
            blinkNumber = 3;
            ledTimer = millis();
          }
        }


        if (rx.byte2 == 103) {
          blinkNumber = 1;
          ledTimer = millis();
          senseDistanceSelector[mode] = rx.byte3;
          loadPrefs();
        }


          if (rx.byte2 == 117) {
          blinkNumber = 1;
          ledTimer = millis();
          unsigned long v = rx.byte3 * 8191UL / 100;
          vibratoDepthSelector[mode] = v; //scale vibrato depth in cents up to pitchbend range of 0-8191
          loadPrefs();
        }
        

        if (rx.byte2 == 111) {
          blinkNumber = 1;
          ledTimer = millis();
          if(rx.byte3 < 50){
            noteShiftSelector[0] = rx.byte3;}
          else{noteShiftSelector[0] = - 127 + rx.byte3;}
          loadPrefs();
        }

        if (rx.byte2 == 112) {
          blinkNumber = 1;
          ledTimer = millis();
          if(rx.byte3 < 50){
            noteShiftSelector[1] = rx.byte3;}
          else{noteShiftSelector[1] = - 127 + rx.byte3;}
          loadPrefs();
        }

        if (rx.byte2 == 113) {
          blinkNumber = 1;
          ledTimer = millis();
          if(rx.byte3 < 50){
            noteShiftSelector[2] = rx.byte3;}
          else{noteShiftSelector[2] = - 127 + rx.byte3;}
          loadPrefs();
        }      


        if (rx.byte2 == 104) {
         for (byte i = 1; i < 13; i++) { //update pressure receive mode (this indicates the pressure variable for which the next received byte will be).
            if (rx.byte3 == i) {
              pressureReceiveMode = i-1;
            }
          }}

        if (rx.byte2 == 105) {
          blinkNumber = 1;
          ledTimer = millis();
          pressureSelector[mode][pressureReceiveMode] = rx.byte3;
          loadPrefs();
        }


        if (rx.byte2 == 109) {
          blinkNumber = 1;
          ledTimer = millis();
          hysteresis = rx.byte3;
        }

      if (rx.byte2 == 106 && rx.byte3 >15) {
        if(rx.byte3 == 16){ //update custom
          blinkNumber = 1;
          ledTimer = millis();
          customSelector[mode] = 0;
          loadPrefs();}
        if(rx.byte3 == 17){
          blinkNumber = 1;
          ledTimer = millis();
          customSelector[mode] = 1;
          loadPrefs();}

        if(rx.byte3 > 19 && rx.byte3 < 29){ //update enabled vibrato holes for "universal" vibrato
           blinkNumber = 1;
          ledTimer = millis();
          bitSet(vibratoHolesSelector[mode],rx.byte3-20);
          loadPrefs();
          }

        if(rx.byte3 > 29 && rx.byte3 < 39){
          blinkNumber = 1;
          ledTimer = millis();
          bitClear(vibratoHolesSelector[mode],rx.byte3-30);
          loadPrefs();
          }
          
        if(rx.byte3 == 39){
          blinkNumber = 1;
          ledTimer = millis();
          useLearnedPressureSelector[mode] = 0;
          loadPrefs();
          }  

        if(rx.byte3 == 40){
          blinkNumber = 1;
          ledTimer = millis();
          useLearnedPressureSelector[mode] = 1;
          loadPrefs();
          }

        if(rx.byte3 == 41){
          blinkNumber = 1;
          ledTimer = millis();
          learnedPressureSelector[mode] = sensorValue;
          loadPrefs();
          }

        if(rx.byte3 == 42){  //autocalibrate bell sensor only                       
          calibration = 2;
          }


        if(rx.byte3 == 43){
          blinkNumber = 1;
          ledTimer = millis();
          expressionOnSelector[mode] = 0;
          loadPrefs();}


         if(rx.byte3 == 44){
          blinkNumber = 1;
          ledTimer = millis();
          expressionOnSelector[mode] = 1;
          loadPrefs();}

            


          
      }



        


        for (byte i = 0; i < 8; i++) { //update channel, byte 2, byte 3 for MIDI message for button MIDI command for row i
          if (buttonReceiveMode == i) {
            if (rx.byte2 == 106 && rx.byte3 < 16) {
              buttonPrefs[mode][i][2] = rx.byte3;
              blinkNumber = 1;
              ledTimer = millis();
            }
            if (rx.byte2 == 107) {
              buttonPrefs[mode][i][3] = rx.byte3;
              blinkNumber = 1;
              ledTimer = millis();
            }
            if (rx.byte2 == 108) {
              buttonPrefs[mode][i][4] = rx.byte3;
              blinkNumber = 1;
              ledTimer = millis();
            }
          }
        }

      }

    }
  } while (rx.header != 0);
}



//save settings for current instrument as defaults for all three instruments
void saveSettingsForAllModes() {


  for (byte i = 0; i < 3; i++) {
  

    EEPROM.update(80 + i, customSelector[mode]);
    EEPROM.update(40 + i, modeSelector[mode]);
    EEPROM.update(53 + i, noteShiftSelector[mode]);
    EEPROM.update(56 + i, ventedSelector[mode]);
    EEPROM.update(45 + i, secretSelector[mode]);
    EEPROM.update(50 + i, senseDistanceSelector[mode]);
    EEPROM.update(60 + i, pitchBendModeSelector[mode]);
    EEPROM.update(63 + i, expressionOnSelector[mode]);    
    EEPROM.update(70 + i, breathModeSelector[mode]);
    EEPROM.update(83 + (i * 2), lowByte(vibratoHolesSelector[mode]));
    EEPROM.update(84 + (i * 2), highByte(vibratoHolesSelector[mode]));
    EEPROM.update(89 + (i * 2), lowByte(vibratoDepthSelector[mode]));
    EEPROM.update(90 + (i * 2), highByte(vibratoDepthSelector[mode]));
    EEPROM.update(95 + i, useLearnedPressureSelector[mode]);
    EEPROM.update(273 + (i * 2), lowByte(learnedPressureSelector[mode]));
    EEPROM.update(274 + (i * 2), highByte(learnedPressureSelector[mode]));

    for (byte q = 0; q < 12; q++) {
      EEPROM.update((260 + q + (i * 20)), pressureSelector[mode][q]);
            }

    for (byte h = 0; h < 3; h++) {
      EEPROM.update(250 + (i * 3) + h, momentary[mode][h]);
      }

      for (byte j = 0; j < 5; j++) { //save button configuration for current mode
        for (byte k = 0; k < 8; k++) {
          EEPROM.update(100 + (i * 50) + (j * 10) + k, buttonPrefs[mode][k][j]);
        }
      }
    
  }
  loadFingering();
  loadSettingsForAllModes();
  blinkNumber = 3;
  ledTimer = millis();
}



//load saved fingering modes
void loadFingering() {
  modeSelector[0] = EEPROM.read(40);
  modeSelector[1] = EEPROM.read(41);
  modeSelector[2] = EEPROM.read(42);
  // mode =  EEPROM.read(43);
  noteShiftSelector[0] = (int8_t)EEPROM.read(53);
  noteShiftSelector[1] = (int8_t)EEPROM.read(54);
  noteShiftSelector[2] = (int8_t)EEPROM.read(55);
  if (communicationMode) {
    sendUSBMIDI(CC, 7, 102, 30 + modeSelector[0]); //send currently selected 3 fingering modes.
    sendUSBMIDI(CC, 7, 102, 40 + modeSelector[1]);
    sendUSBMIDI(CC, 7, 102, 50 + modeSelector[2]);
    sendUSBMIDI(CC, 7, 111, noteShiftSelector[0]); //send noteShift
    sendUSBMIDI(CC, 7, 112, noteShiftSelector[1]); //send noteShift
    sendUSBMIDI(CC, 7, 113, noteShiftSelector[2]); //send noteShift
  }
}



//load settings for all three instruments from EEPROM
void loadSettingsForAllModes() {
  for (byte i = 0; i < 3; i++) {

    customSelector[i] = EEPROM.read(80 + i);
    ventedSelector[i] = EEPROM.read(56 + i);
    secretSelector[i] = EEPROM.read(45 + i);   
    expressionOnSelector[i] = EEPROM.read(63 + i); 
    pitchBendModeSelector[i] =  EEPROM.read(60 + i);
    breathModeSelector[i] = EEPROM.read(70 + i);
    vibratoHolesSelector[i] = word(EEPROM.read(84 + (i * 2)),EEPROM.read(83 + (i * 2)));
    vibratoDepthSelector[i] = word(EEPROM.read(90 + (i * 2)),EEPROM.read(89 + (i * 2)));
    useLearnedPressureSelector[i] = EEPROM.read(95 + i);
    learnedPressureSelector[i] = word(EEPROM.read(274 + (i * 2)),EEPROM.read(273 + (i * 2)));
         

    for (byte m = 0; m < 12; m++) {
    pressureSelector[i][m] =  EEPROM.read(260 + m + (i * 20));
     }

    sensorThreshold[1] = sensorThreshold[0] + (offset  << 2);
    for (byte h = 0; h < 3; h++) {
      momentary[i][h] = EEPROM.read(250 + (i * 3) + h);
    }
    for (byte j = 0; j < 5; j++) {
      for (byte k = 0; k < 8; k++) {
        buttonPrefs[i][k][j] = EEPROM.read(100 + (i * 50) + (j * 10) + k);
      }
    }

    senseDistanceSelector[i] = EEPROM.read(50 + i);
  }

     
}




//this is used only once, the first time the software is run, to copy all the default settings to EEPROM, where they can later be restored
void saveFactorySettings() {

  //saveFingeringModes(); //first save the fingering modes.

  for (byte i = 0; i < 3; i++) { //then we save all the current settings for all three instruments.
    EEPROM.update(56 + i, ventedSelector[i]);
    EEPROM.update(45 + i, secretSelector[i]);
    EEPROM.update(50 + i, senseDistanceSelector[i]);
    EEPROM.update(60 + i, pitchBendModeSelector[i]);
    EEPROM.update(70 + i, breathModeSelector[i]);
    EEPROM.update(80 + i, customSelector[i]);
    EEPROM.update(63 + i, expressionOnSelector[i]);
    EEPROM.update(60 + i, pitchBendModeSelector[i]);
    EEPROM.update(53 + i, noteShiftSelector[i]);
    EEPROM.update(40 + i, modeSelector[i]);
    EEPROM.update(83 + (i * 2), lowByte(vibratoHolesSelector[i]));
    EEPROM.update(84 + (i * 2), highByte(vibratoHolesSelector[i]));
    EEPROM.update(89 + (i * 2), lowByte(vibratoDepthSelector[i]));
    EEPROM.update(90 + (i * 2), highByte(vibratoDepthSelector[i]));
    EEPROM.update(95 + i, useLearnedPressureSelector[i]);
    EEPROM.update(273 + (i * 2), lowByte(learnedPressureSelector[i]));
    EEPROM.update(274 + (i * 2), highByte(learnedPressureSelector[i]));

    for (byte h = 0; h < 3; h++) {
      EEPROM.update(250 + (i * 3) + h, momentary[i][h]);

    for (byte m = 0; m < 12; m++) {
       EEPROM.update((260 + m + (i * 10)), pressureSelector[i][m]);
    }

      for (byte j = 0; j < 5; j++) { //save button configuration for current mode
        for (byte k = 0; k < 8; k++) {
          EEPROM.update(100 + (i * 50) + (j * 10) + k, buttonPrefs[i][k][j]);
        }
      }
    }

    blinkNumber = 3;
    ledTimer = millis();
  }

  for (int i = 40; i < 320; i++) { //then we read every byte in EEPROM from 40 to 320
    byte reading = EEPROM.read(i);
    EEPROM.update(400 + i, reading); //and rewrite them from 440 to 720. Then they'll be available to easily restore later if necessary.
    delay(5);
  }

  EEPROM.update(44, 3); //indicates settings have been saved
}



//restore original settings from EEPROM
void restoreFactorySettings() {
  byte reading;
  for (int i = 440; i < 720; i++) { //then we read every byte in EEPROM from 440 to 720
    reading = EEPROM.read(i);
    EEPROM.update(i - 400, reading);
  } //and rewrite them from 40 to 320.
  loadFingering();
  loadSettingsForAllModes(); //load the newly restored settings
  if (communicationMode) {
    sendSettings(); //send the new settings
  }
}




//send all settings for current mode to the WARBL Configuration Tool.
void sendSettings() {

  sendUSBMIDI(CC, 7, 110, VERSION); //send software version
  sendUSBMIDI(CC, 7, 102, 30 + modeSelector[0]); //send currently selected 3 fingering modes.
  sendUSBMIDI(CC, 7, 102, 40 + modeSelector[1]);
  sendUSBMIDI(CC, 7, 102, 50 + modeSelector[2]);
  sendUSBMIDI(CC, 7, 102, 60 + mode); //send current fingering mode
  sendUSBMIDI(CC, 7, 103, senseDistance); //send sense distance
  sendUSBMIDI(CC, 7, 117, vibratoDepth * 100UL / 8191); //send vibrato depth, scaled down to cents 
  sendUSBMIDI(CC, 7, 102, 70 + pitchBendMode); //send current pitchBend mode
  sendUSBMIDI(CC, 7, 102, 80 + breathMode); //send current breathMode
  sendUSBMIDI(CC, 7, 109, hysteresis); //send offset for bag mode

  
  if(noteShiftSelector[0] >=0){
    sendUSBMIDI(CC, 7, 111, noteShiftSelector[0]);} //send noteShift, with a transformation for sending negative values over MIDI.
    else{sendUSBMIDI(CC, 7, 111, noteShiftSelector[0] + 127);}
    
  if(noteShiftSelector[1] >=0){
    sendUSBMIDI(CC, 7, 112, noteShiftSelector[1]);} //send noteShift
    else{sendUSBMIDI(CC, 7, 112, noteShiftSelector[1] + 127);}
    
  if(noteShiftSelector[2] >=0){
    sendUSBMIDI(CC, 7, 113, noteShiftSelector[2]);} //send noteShift
    else{sendUSBMIDI(CC, 7, 113, noteShiftSelector[2] + 127);}

  
  sendUSBMIDI(CC, 7, 102, 120 + bellSensor); //send bell sensor state
  sendUSBMIDI(CC, 7, 102, 63 + secret); //send secret button command option
  sendUSBMIDI(CC, 7, 102, 65 + vented); //send vented option
  sendUSBMIDI(CC, 7, 106, 16 + custom); //send custom option
  sendUSBMIDI(CC, 7, 106, 39 + useLearnedPressure); //send calibration option
  sendUSBMIDI(CC, 7, 106, 43 + expressionOn); //send expressionOn

  for (byte i = 0; i < 9; i++) {
    sendUSBMIDI(CC, 7, 106, 20 + i + (10 * (bitRead(vibratoHolesSelector[mode],i)))); //send enabled vibrato holes  
  }  

  for (byte i = 0; i < 8; i++) {
    sendUSBMIDI(CC, 7, 102, 90 + i); //indicate that we'll be sending data for button commands row i (click 1, click 2, etc.)
    sendUSBMIDI(CC, 7, 102, 100 + buttonPrefs[mode][i][0]); //send action (i.e. none, send MIDI message, etc.)
    if (buttonPrefs[mode][i][0] == 1) { //if the action is a MIDI command, send the rest of the MIDI info for that row.
      sendUSBMIDI(CC, 7, 102, 110 + buttonPrefs[mode][i][1]);
      sendUSBMIDI(CC, 7, 106, buttonPrefs[mode][i][2]);
      sendUSBMIDI(CC, 7, 107, buttonPrefs[mode][i][3]);
      sendUSBMIDI(CC, 7, 108, buttonPrefs[mode][i][4]);
    }
  }

  for (byte i = 0; i < 3; i++) {
    sendUSBMIDI(CC, 7, 102, 90 + i); //indicate that we'll be sending data for button commands row i (click 1, click 2, etc.)
    sendUSBMIDI(CC, 7, 102, 113 + momentary[mode][i]);
  } //send momentary



  for (byte i = 0; i < 12; i++) {
     sendUSBMIDI(CC, 7, 104, i+1); //indicate which pressure variable we'll be sending data for
     sendUSBMIDI(CC, 7, 105, pressureSelector[mode][i]); //send the data
    }

}


void blink() { //blink LED given number of times

  if ((millis() - ledTimer) >= 200) {

    if (LEDon) {
      digitalWrite2(ledPin, LOW);
      blinkNumber--;
      LEDon = 0;
      ledTimer = millis();
      return;
    }

    if (!LEDon) {
      digitalWrite2(ledPin, HIGH);
      LEDon = 1;
      ledTimer = millis();
    }
  }
}


//interpret button presses. If the button is being used for momentary MIDI messages we ignore other actions with that button (except "secret" actions involving the toneholes).
void handleButtons() {


  //first, some housekeeping

  if (shiftState == 1 && released[1] == 1) { //if button 1 was only being used along with another button, we clear the just-released flag for button 1 so it doesn't trigger another control change.
    released[1] = 0;
    buttonUsed = 0; //clear the button activity flag, so we won't handle them again until there's been new button activity.
    shiftState = 0;
  }



  //then, a few hard-coded actions that can't be changed by the configuration tool:
  //_______________________________________________________________________________

  if (secret) {
    if (justPressed[0] && !pressed[2] && !pressed[1] && !(calibration>0) && holeCovered >> 1 == 0b00001000) { //turn Celtic sounds module Lynch drones on if button 0 is pressed and fingering pattern is 0 0001000. The volume is set low.
      justPressed[0] = 0;
      specialPressUsed[0] = 1;
      if(!dronesOn){
         sendUSBMIDI(NOTE_ON, 1, kLynchDroneMIDINote, kDroneVelocity);
         sendUSBMIDI(NOTE_ON, 15, kLynchDroneMIDINote, kDroneVelocity); //this works for EPipes drones on channel 15
          dronesOn = 1;
      }
      else{
         sendUSBMIDI(NOTE_ON, 1, kLynchDroneMIDINote, kDroneVelocity);
         sendUSBMIDI(NOTE_ON, 15, kLynchDroneMIDINote, 0);  //this works for EPipes drones on channel 15
         dronesOn = 0;
      }
    }

    if (justPressed[0] && !pressed[2] && !pressed[1] && !(calibration>0) && holeCovered >> 1 == 0b00000100) { //turn Celtic sounds module Crowley drones on if button 0 is pressed and fingering pattern is 0 0001000. The volume is set low.
      justPressed[0] = 0;
      specialPressUsed[0] = 1;
            if(!dronesOn){
         sendUSBMIDI(NOTE_ON, 1, kCrowleyDroneMIDINote, kDroneVelocity);
         sendUSBMIDI(NOTE_ON, 15, kCrowleyDroneMIDINote, kDroneVelocity); //this works for EPipes drones on channel 15
          dronesOn = 1;
      }
      else{
         sendUSBMIDI(NOTE_ON, 1, kCrowleyDroneMIDINote, kDroneVelocity);
         sendUSBMIDI(NOTE_ON, 15, kCrowleyDroneMIDINote, 0);  //this works for EPipes drones on channel 15
         dronesOn = 0;
      }
    }

    if (justPressed[0] && !pressed[2] && !pressed[1] && !(calibration>0) && holeCovered >> 1 == 0b00010000) { //change pitchbend mode if button 0 is pressed and fingering pattern is 0 0000010.
      justPressed[0] = 0;
      specialPressUsed[0] = 1;
      pitchBendMode++;//set vibrato mode
      if (pitchBendMode == kPitchBendNModes) {
        pitchBendMode = kPitchBendSlideVibrato;
      }
      if (pitchBendMode == kPitchBendNone) {
        pitchBend = 8191;
        sendUSBMIDI(PITCH_BEND, 1, pitchBend & 0x7F, pitchBend >> 7);
      } //turn off any pitch bend if vibrato is off, so there's no carryover.
      blinkNumber = abs(pitchBendMode) + 1;
      ledTimer = millis();
      if (communicationMode) {
        sendUSBMIDI(CC, 7, 102, 70 + pitchBendMode); //send current pitchbend mode to communications tool.
      }
    }

    if (justPressed[0] && !pressed[2] && !pressed[1] && !(calibration>0) && holeCovered >> 1 == 0b00000010) { //change fingering mode if button 0 is pressed and fingering pattern is 0 0000001.
      justPressed[0] = 0;
      specialPressUsed[0] = 1;
      mode++; //set fingering mode
      if (mode == 3) {
        mode = 0;
      }
      play = 0;
      loadPrefs(); //load the correct user settings for offset, pitchBendMode, senseDistance, and breathMode, based on current fingering mode.
      blinkNumber = abs(mode) + 1;
      ledTimer = millis();
      if (communicationMode) {
        sendSettings(); //tell communications tool to switch mode and send all settings for current mode.
      }
    }
  }


  //now the button actions that can be changed with the configuration tool.
  //_______________________________________________________________________________
  //single button clicks if we're not using momentary on/off mode for each button
  if (released[0] && !pressed[2] && !pressed[1] && !(calibration>0)) { //do action for button 0 release ("click") NOTE: button array is zero-indexed, so "button 1" in all documentation is button 0 here (same for others).
    if (!specialPressUsed[0]) {
      performAction(0); //we ignore it if the button was just used for a hard-coded command involving a combination of fingerholes.
    }
    released[0] = 0;
    specialPressUsed[0] = 0;
  }

  if  (released[1] && !pressed[0] && !pressed[2] && !momentary[mode][1]) {  //do action for button 1 release ("click")
    if (!specialPressUsed[1]) {
      performAction(1);
    }
    released[1] = 0;
    specialPressUsed[1] = 0;
  }

  if (released[2] && !pressed[0] && !pressed[1] && !momentary[mode][2]) {  //do action for button 2 release ("click")
    if (!specialPressUsed[2]) {
      performAction(2);
    }
    released[2] = 0;
    specialPressUsed[2] = 0;
  }

  //single button clicks if we are using momentary on/off mode for each button. In this case we ignore the other buttons.
  if (released[0] && !(calibration>0) && momentary[mode][0]) { //do action for button 0 release ("click") NOTE: button array is zero-indexed, so "button 1" in all documentation is button 0 here (same for others).
    if (!specialPressUsed[0]) {
      performAction(0);
    }
    released[0] = 0;
    specialPressUsed[0] = 0;
  }

  if (released[1] && momentary[mode][1]) {  //do action for button 1 release ("click")
    if (!specialPressUsed[1]) {
      performAction(1);
    }
    released[1] = 0;
    specialPressUsed[1] = 0;
  }

  if (released[2] && momentary[mode][2]) {  //do action for button 2 release ("click")
    if (!specialPressUsed[2]) {
      performAction(2);
    }
    released[2] = 0;
    specialPressUsed[2] = 0;
  }



  //other button combinations
  if (released[0] && pressed[1] && !momentary[mode][0] && !momentary[mode][1]) { //do action for button 1 held and button 0 released
    released[0] = 0;
    shiftState = 1;
    performAction(3);
  }

  if (released[2] && pressed[1] && !momentary[mode][1] && !momentary[mode][2]) { //do action for button 1 held and button 2 released
    released[2] = 0;
    shiftState = 1;
    performAction(4);
  }

  if (longPress[0] && !pressed[1] && !pressed[2] && !momentary[mode][0]) { //do action for long press 0.
    performAction(5);
    longPressUsed[0] = 1;
    longPress[0] = 0;
    longPressCounter[0] = 0;
  }

  if  (longPress[1] && !pressed[0] && !pressed[2] && !momentary[mode][1]) { //do action for long press 1.
    performAction(6);
    longPressUsed[1] = 1;
    longPress[1] = 0;
    longPressCounter[1] = 0;
  }

  if  (longPress[2] && !pressed[0] && !pressed[1] && !momentary[mode][2]) { //do action for long press 2.
    performAction(7);
    longPressUsed[2] = 1;
    longPress[2] = 0;
    longPressCounter[2] = 0;
  }


  //presses of individual buttons (as opposed to releases) are special cases used only if we're using buttons to send MIDI on/off messages and "momentary" is selected. We'll handle these in a separate function.
  if (justPressed[0] && (!secret || (holeCovered >> 1 != 0b00001000 && holeCovered >> 1 != 0b00000100 && holeCovered >> 1 != 0b00000010 && momentary[mode][0]))) { //do action for button 0 press.
    justPressed[0] = 0;
    handleMomentary(0);
  }

  if (justPressed[1] && momentary[mode][1]) { //do action for button 1 press.
    justPressed[1] = 0;
    handleMomentary(1);
  }

  if (justPressed[2] && momentary[mode][2]) { //do action for button 2 press.
    justPressed[2] = 0;
    handleMomentary(2);
  }


  buttonUsed = 0; // Now that we've caught any important button acticity, clear the flag so we won't enter this function again until there's been new activity.
}



//perform desired action in response to buttons
void performAction(byte action) {


  switch (buttonPrefs[mode][action][0]) {

    case 0:
      break; //if no action desired for button combination

    case 1:

      if (buttonPrefs[mode][action][1] == 0) {
        if (noteOnOffToggle[action] == 0) {
          sendUSBMIDI(NOTE_ON, buttonPrefs[mode][action][2], buttonPrefs[mode][action][3], buttonPrefs[mode][action][4]);
          noteOnOffToggle[action] = 1;
        }
        else if (noteOnOffToggle[action] == 1) {
          sendUSBMIDI(NOTE_OFF, buttonPrefs[mode][action][2], buttonPrefs[mode][action][3], buttonPrefs[mode][action][4]);
          noteOnOffToggle[action] = 0;
        }
      }

      if (buttonPrefs[mode][action][1] == 1) {
        sendUSBMIDI(CC, buttonPrefs[mode][action][2], buttonPrefs[mode][action][3], buttonPrefs[mode][action][4]);
      }

      if (buttonPrefs[mode][action][1] == 2) {
        sendUSBMIDI(PROGRAM_CHANGE, buttonPrefs[mode][action][2], buttonPrefs[mode][action][3]);
      }

      break;

    case 2: 
      pitchBendModeSelector[mode] ++;//set vibrato mode
      if (pitchBendModeSelector[mode] == kPitchBendNModes) {
        pitchBendModeSelector[mode] = kPitchBendSlideVibrato;     
      }
      loadPrefs();loadPrefs();
      blinkNumber = abs(pitchBendMode) + 1;
      ledTimer = millis();     
      if (communicationMode) {
        sendUSBMIDI(CC, 7, 102, 70 + pitchBendMode); //send current pitchbend mode to configuration tool.
      }
      break;

    case 3:
      mode++; //set instrument
      if (mode == 3) {
        mode = 0;
      }
      play = 0;
      loadPrefs(); //load the correct user settings based on current instrument.
      blinkNumber = abs(mode) + 1;
      ledTimer = millis();
      if (communicationMode) {
        sendSettings(); //tell communications tool to switch mode and send all settings for current instrument.
      }
      break;

    case 4:
      play = !play; //turn sound on/off when in bagless mode
      if(!play) { 
        for (byte i = 1; i < 17; i++) { //send MIDI panic to turn off drones as well when stopping play in bagless mode. Should see if this is undesirable in some case (some may prefer to turn drones off later).
        sendUSBMIDI(CC, i, 123, 0);
        }
      }
      break;

    case 5:
      if (octaveShift < 3) {
        octaveShift++; //adjust octave shift up or down, within reason
      }
      blinkNumber = abs(octaveShift);
      ledTimer = millis();
      break;

    case 6:
      if (octaveShift > -4) {
        octaveShift--;
      }
      blinkNumber = abs(octaveShift);
      ledTimer = millis();
      break;

    case 7:
            for (byte i = 1; i < 17; i++) { //send MIDI panic to turn off drones as well when stopping play in bagless mode. Should see if this is undesirable in some case (some may prefer to turn drones off later).
        sendUSBMIDI(CC, i, 123, 0);
        }
      break;

    case 8:
      breathModeSelector[mode]++; //set breath mode
      if (breathModeSelector[mode] == kPressureNModes) {
        breathModeSelector[mode] = kPressureBreath;
      }
      loadPrefs();
      play = 0;
      blinkNumber = abs(breathMode) + 1;
      ledTimer = millis();
      if (communicationMode) {
        sendUSBMIDI(CC, 7, 102, 80 + breathMode); //send current breathMode
      }
      break;


    default:
      return;
  }
}

void handleMomentary(byte button) {


  if (momentary[mode][button] && buttonPrefs[mode][button][1] == 0) {
    sendUSBMIDI(NOTE_ON, buttonPrefs[mode][button][2], buttonPrefs[mode][button][3], buttonPrefs[mode][button][4]);
    noteOnOffToggle[button] = 1;
  }

}



//load the correct user settings for offset, pitchBendMode, senseDistance, and breathMode, based on current fingering mode.
void loadPrefs() {


  for (byte i = 0; i < 3; i++) {
    if (mode == i) {
      vented = ventedSelector[i];
      custom = customSelector[i];
      vibratoHoles = vibratoHolesSelector[i];    
      secret = secretSelector[i];
      octaveShift = octaveShiftSelector[i];
      noteShift = noteShiftSelector[i];
      pitchBendMode = pitchBendModeSelector[i];
      useLearnedPressure = useLearnedPressureSelector[i];
      learnedPressure = learnedPressureSelector[i];
      expressionOn = expressionOnSelector[i];

        if (pitchBendMode == kPitchBendNone) {
          pitchBend = 8191;
          pitchBendTimer = millis();
          sendUSBMIDI(PITCH_BEND, 1, pitchBend & 0x7F, pitchBend >> 7);} //turn off any pitch bend if vibrato is off, so there's no carryover.
          pitchBend = 0;  //reset pitchbend
        for (byte i=0; i< 9; i++) {
            iPitchBend[i] = 0;
             pitchBendOn[i] = 0;}

        if (custom && ((modeSelector[mode] == kModeWhistle || modeSelector[mode] == kModeChromatic || modeSelector[mode] == kModeUilleann) && pitchBendMode == kPitchBendVibrato)) {
          customEnabled = 1;
        }
        else (customEnabled = 0); //decide here whether custom vibrato can currently be used, so we don't have to do it every time we need to check pitchBend.        
      
      senseDistance = senseDistanceSelector[i];
      vibratoDepth = vibratoDepthSelector[i];
      breathMode = breathModeSelector[i];
      if (breathMode < kPressureSingle) {
        if (vented) { //some special settings for vented mode
          offset = pressureSelector[i][6];
          multiplier = pressureSelector[i][7];
          jumpValue = pressureSelector[i][8];
          dropValue = pressureSelector[i][9];
          jumpTime = pressureSelector[i][10];
          dropTime = pressureSelector[i][11];
        }
        else {
          offset = pressureSelector[i][0];
          multiplier = pressureSelector[i][1];
          jumpValue = pressureSelector[i][2];
          dropValue = pressureSelector[i][3];
          jumpTime = pressureSelector[i][4];
          dropTime = pressureSelector[i][5];
        }


        if(!useLearnedPressure){
            sensorThreshold[0] = (sensorCalibration + soundTriggerOffset); //pressure sensor calibration at startup. We set the on/off threshhold just a bit higher than the reading at startup.
        }
        else{sensorThreshold[0] = (learnedPressure + soundTriggerOffset);
        } 
        
        sensorThreshold[1] = sensorThreshold[0] + (offset << 2); //threshold for move to second octave
      }
    } 
  }
    for (byte i=0; i < 9; i++) {
       toneholeScale[i] = ((8 * 8191)/(toneholeCovered[i] - 50 - senseDistance)/2); //This one is for sliding. We multiply by 8 first to reduce rounding errors. We'll divide again later.
        vibratoScale[i] = ((8 * vibratoDepth)/(toneholeCovered[i] - 50 - senseDistance)/2);} //This one is for vibrato


}

// find leftmost unset bit, used for finding the uppermost uncovered hole when reading from the fingering charts
byte findleftmostunsetbit(uint16_t n) {

  if ((n & (n + 1)) == 0) {
    return 127; // if number contains all 0s then return 127
  }

  int pos = 0;
  for (int temp = n, count = 0; temp > 0; temp >>= 1, count++) // Find position of leftmost unset bit.

    if ((temp & 1) == 0)   // if temp L.S.B is zero then unset bit pos is
      pos = count;

  return pos;

}

// Send a debug MIDI message on channel 8
void debug_log(byte msg){
    sendUSBMIDI(CC,8,42,msg);
}
void debug_log(int msg){
    sendUSBMIDI(CC,8,msg>>8,msg&0xFF);

}
