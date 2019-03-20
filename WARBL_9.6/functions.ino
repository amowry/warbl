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


// ADC complete ISR (for reading new sensors, used in hardware revisions greater than 2.1.) We read the sensors asynchronously by starting a conversion and then coming back by interrupt when it is complete. 
ISR (ADC_vect)
  {
  
  byte prev = 8; 
  if(lastRead != 0) {
  	prev = lastRead - 1;}
  	
  byte next = lastRead + 1;
  if (lastRead == 8) {
  	next = 0;}
  	
  if (lastRead == 9) { //this time is different because we're about to take a short break and read the pressure sensor.
    
    //finalTime = micros() - initialTime; //for testing only
    //initialTime = micros(); //for testing only
 
  		digitalWrite2f(pins[8], LOW); //turn off the previous LED
  		tempToneholeRead[8] = (ADC); //get the previous illuminated reading
      tempToneholeRead[8] = tempToneholeRead[8] - tempToneholeReadA[8]; //subtract the ambient light reading
      if (tempToneholeRead[8] < 0) {
        tempToneholeRead[8] = 0;
    }
    	Timer1.resume(); //start the timer to take a short break, conserving some power.
    	return;
  	 }  	

  if (firstTime) {
  	if(lastRead == 0){    
      tempSensorValue = (ADC); //we've just returned from starting the pressure sensor reading, so get the conversion and flag that it is ready.
      sensorDataReady = 1; 
    }
    else{
  		digitalWrite2f(pins[prev], LOW); //turn off the previous LED
  		tempToneholeRead[prev] = (ADC); //get the previous illuminated reading       
      tempToneholeRead[prev] = tempToneholeRead[prev] - tempToneholeReadA[prev]; //subtract the ambient light reading 
      if (tempToneholeRead[prev] < 0) {
        tempToneholeRead[prev] = 0;
    }
    	
    }
  		ADC_read(holeTrans[next]); // start ambient reading for next sensor
  		firstTime = 0;
      if(lastRead != 0){
      digitalWrite2f(pins[lastRead], HIGH); //turn on the current LED        
      }
  		return;
   }

 //if !firstTime (implied, because we wouldn't have gotten this far otherwise)
    tempToneholeReadA[next] = (ADC); //get the ambient reading for the next sensor
 	 	ADC_read(holeTrans[lastRead]); //start illuminated reading for current sensor
 		firstTime = 1;
 		lastRead ++;
		return;
}


//Timer ISR for adding a small delay after reading new sensors, to conserve a bit of power and give the processor time to catch up if necessary.
void timerDelay(void) {

  timerCycle ++;

  if(timerCycle == 2){
    digitalWrite2f(pins[0], HIGH); //turn on the LED for the bell sensor (this won't use any power if the sensor isn't plugged in). It will be on for ~170 uS, which is necesary because it's slower sensor than the others.
    return;
  }

  if(timerCycle == 3){
	  Timer1.stop(); //stop the timer after running twice to add a delay.
	  ADC_read(4); //start reading the pressure sensor (pinA4).
	  firstTime = 1;
    lastRead = 0;
    timerCycle = 0;
  }
  //if timerCycle is 1 we just return and wait until next time.
}





//Timer ISR for reading tonehole sensors and pressure sensor, only used in hardware revision 2.1.
void readSensors(void) {

  if (lastRead == 9) {
    tempSensorValue = analogRead(A4);  //we read the pressure sensor after reading all the tonehole sensors
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


    if (modeSelector[mode] == kModeNAF) { //Native American Flute mode

    uint8_t tempCovered = (0b011111110 & holeCovered) >> 1; //ignore thumb hole and bell sensor
      ret = pgm_read_byte(&naf_explicit[tempCovered].midi_note);
      return ret;
  }


    if (modeSelector[mode] == kModeKaval) { //Kaval mode

     //If back thumb is open, always play the B
      if ((holeCovered & 0b100000000) == 0){
     return 71;
    }

    uint8_t tempCovered = (0b011111110 & holeCovered) >> 1; //ignore thumb hole and bell sensor
      ret = pgm_read_byte(&kaval_explicit[tempCovered].midi_note);
      return ret;
  }

  
  return ret;
}




//State machine that models the way that a tinwhistle etc. begins sounding and jumps octaves in response to breath pressure.
void get_state() {

  noInterrupts();
  sensorValue2 = tempSensorValue; //transfer last reading to a non-volatile variable
  interrupts();

  if(ED[mode][DRONES_CONTROL_MODE] == 3) { //use pressure to control drones if that option has been selected. There's a small amount of hysteresis added.
   
    if (!dronesState && sensorValue2 > 5 + (ED[mode][DRONES_PRESSURE_HIGH_BYTE] << 7 | ED[mode][DRONES_PRESSURE_LOW_BYTE])) {
      dronesState = 1;
      if(!dronesOn) {startDrones();}

    }
        if (dronesState && sensorValue2 < (ED[mode][DRONES_PRESSURE_HIGH_BYTE] << 7 | ED[mode][DRONES_PRESSURE_LOW_BYTE])) {
       dronesState = 0;
       if(dronesOn) {stopDrones();}
    }
  }
  

  if (((millis() - jumpTimer) >= jumpTime) && jump) {
    jump = 0; //make it okay to switch octaves again if some time has past since we "jumped" or "dropped" because of rapid pressure change.
  }
  if (((millis() - dropTimer) >= dropTime) && drop) {
    drop = 0;
  }

  if (((sensorValue2 - sensorValue) > jumpValue) && !jump && !drop && breathMode == kPressureBreath && (sensorValue2 > sensorThreshold[0])) {  //if the pressure has increased rapidly (since the last reading) and there's a least enough pressure to turn a note on, jump immediately to the second octave
    newState = 3;
    jump = 1;
    jumpTimer = millis();
    sensorValue = sensorValue2;
    return;
  }

  if (((sensorValue - sensorValue2) > dropValue) && !jump  && (breathMode > kPressureSingle) && !drop && newState == 3) {  //if we're in second octave and the pressure has dropped rapidly, turn the note off (this lets us drop directly from the second octave to note off).
    newState = 1;
    drop = 1;
    dropTimer = millis();
    sensorValue = sensorValue2;
    return;
  }

  //if there haven't been rapid pressure changes and we haven't just jumped octaves, choose the state based solely on current pressure.
  if (sensorValue2 <= sensorThreshold[0]) {
    newState = 1;
  }
  if ((breathMode != kPressureBreath) && sensorValue2 > sensorThreshold[0]) {
    newState = 2;
  }
  if (!jump && !drop  && (breathMode > kPressureSingle) && sensorValue2 > sensorThreshold[0] && (sensorValue2 <= (sensorThreshold[1] + ((newNote - 60) * multiplier)))) {
    newState = 2; //threshold 1 is increased based on the note that will be played (low notes are easier to "kick up" to the next octave than higher notes).
  }
  if (!drop  && breathMode == kPressureBreath && (sensorValue2 > (sensorThreshold[1] + ((newNote - 60) * multiplier)))) {
    newState = 3;
  }

  sensorValue = sensorValue2; //we'll use the current reading as the baseline next time around, so we can monitor the rate of change.
  sensorDataReady = 0; //we've used the sensor reading, so don't use it again


}



//calculate pitchbend expression based on pressure
void getExpression() {
	

//find the pressure value that is in the middle of the pressure range for the current note.
	int lowerBound = sensorThreshold[0];
	int upperBound = (sensorThreshold[1] + ((newNote - 60) * multiplier));
	unsigned int halfway = ((upperBound - lowerBound) >> 1) + lowerBound;


	  if(newState == 3) { //if we're in the second register, the halfway point is higher.
       halfway = upperBound + halfway;
       lowerBound = upperBound;
  	}  
	
  	if (sensorValue < halfway){
       byte scale = (((halfway-sensorValue) * ED[mode][EXPRESSION_DEPTH] * 20)/(halfway - lowerBound)); //lower the pitchbend value using a power curve if pressure is below the halfway point.
	     expression = - ((scale * scale) >> 3);
	     }
    else {expression = (sensorValue - halfway) * ED[mode][EXPRESSION_DEPTH];   //raise the pitchbend linearly if we're above the halfway point.
      }


if(expression > ED[mode][EXPRESSION_DEPTH] * 200) {expression = ED[mode][EXPRESSION_DEPTH] * 200;} //put a cap on it, because in the upper register or in single-register mode, there's no upper limit


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
   else if(modeSelector[mode] == kModeKaval){
      stepsDown = 1;} //this chart has a half step between every note on the scale.  
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
      pitchBend = vibratoDepth; //cap at max vibrato depth if they combine to add up to more than that (changed by AM in v. 8.5 to fix an unwanted oscillation when both fingers were contributing.)
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
  if (pitchBend < 0) {
    pitchBend = 0;
  }

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
if (pitchBend < 0) {
  pitchBend = 0;
  }

if(prevPitchBend != pitchBend){
   if (pitchBendMode < kPitchBendNone && noteon) {sendUSBMIDI(PITCH_BEND,1,pitchBend & 0x7F,pitchBend >> 7);
   prevPitchBend = pitchBend;}
   }
       
}



//Calibrate the sensors and store them in EEPROM
//mode 1 calibrates all sensors, mode 2 calibrates bell sensor only.
void calibrate() {

  if (!LEDon) {
    digitalWrite(ledPin, HIGH);
    LEDon = 1;
    calibrationTimer = millis();
    
    if(calibration == 1){ //calibrate all sensors if we're in calibration "mode" 1
      for (byte i = 1; i < 9; i++) {
       toneholeCovered[i] = 0; //first set the calibration to 0 for all of the sensors so it can only be increassed by calibrating
       toneholeBaseline[i] = 255; //and set baseline high so it can only be reduced
    }}
      if (bellSensor) {
        toneholeCovered[0] = 0; //also zero the bell sensor if it's plugged in (doesn't matter which calibration mode for this one).
        toneholeBaseline[0] = 255;
  }
  return; //we return once to make sure we've gotten some new sensor readings.
  }

  if ((calibration == 1 && ((millis() - calibrationTimer) <= 10000)) || (calibration == 2 && ((millis() - calibrationTimer) <= 5000))) { //then set the calibration to the highest reading during the next ten seconds(or five seconds if we're only calibrating the bell sensor).
    if(calibration == 1){
      for (byte i = 1; i < 9; i++) { 
        if (toneholeCovered[i] < toneholeRead[i]) { //covered calibration
          toneholeCovered[i] = toneholeRead[i];
        }

        if (toneholeBaseline[i] > toneholeRead[i]) { //baseline calibration
          toneholeBaseline[i] = toneholeRead[i];
      }
    }
  }

    if (bellSensor && toneholeCovered[0] < toneholeRead[0]) {
      toneholeCovered[0] = toneholeRead[0]; //calibrate the bell sensor too if it's plugged in.
    }
     if (bellSensor && toneholeBaseline[0] > toneholeRead[0]) {
    toneholeBaseline[0] = toneholeRead[0]; //calibrate the bell sensor too if it's plugged in.
    }
  }

  if ((calibration == 1 && ((millis() - calibrationTimer) > 10000)) || (calibration == 2 && ((millis() - calibrationTimer) > 5000))) {
    saveCalibration();
    for (byte i = 0; i < 9; i++) { //here we precalculate a scaling factor for each tonehole sensor for use later when we are controlling pitchbend. It maps the distance of the finger from a tonehole to the maximum pitchbend range for that tonehole.
      toneholeScale[i] = ((8 * 8191) / (toneholeCovered[i] - 50 - senseDistance) / 2);
    } //We multiply by 8 first to reduce rounding errors. We'll divide again later.
  }
}





//save sensor calibration (EEPROM bytes up to 34 are used (plus byte 37 to indicate a saved calibration)
void saveCalibration() {

  for (byte i = 0; i < 9; i++) {
    EEPROM.update((i + 9) * 2, highByte(toneholeCovered[i]));
    EEPROM.update(((i + 9) * 2) + 1, lowByte(toneholeCovered[i]));
    EEPROM.update((721 + i), lowByte(toneholeBaseline[i])); //the baseline readings can be stored in a single byte because they should be close to zero.
  }
  //EEPROM.update(36, hysteresis);
  calibration = 0;
  EEPROM.update(37, 3); //we write a 3 to address 37 to indicate that we have stored a set of calibrations.
  digitalWrite2(ledPin, LOW);
  LEDon = 0;

}





//Load the stored sensor calibrations from EEPROM
void loadCalibration() {
  for (byte i = 0; i < 9; i++) {
    high = EEPROM.read((i + 9) * 2);
    low = EEPROM.read(((i + 9) * 2) + 1);
    toneholeCovered[i] = word(high, low);
    toneholeBaseline[i] = EEPROM.read(721 + i);
  }
   // hysteresis = EEPROM.read(36);
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

  
  midiEventPacket_t rx;
  do {
    noInterrupts();
    rx = MidiUSB.read();   //check for MIDI input
    interrupts();
    if (rx.header != 0) {

       //Serial.println(rx.byte1 &0x0f);
      // Serial.println(rx.byte2);
      // Serial.println(rx.byte3);
     //  Serial.println("");

      if (rx.byte2 < 119) { //Chrome sends CC 121 and 123 on all channels when it connects, so ignore these.  

      if ((rx.byte1 & 0x0f) == 6) { //if we're on channel 7, we may be receiving messages from the configuration tool.
          blinkNumber = 1; //blink once, indicating a received message. Some commands below will change this to three (or zero) blinks.
          ledTimer = millis();
        if (rx.byte2 == 102) {  //many settings are controlled by a value in CC 102 (always channel 7).
          if (rx.byte3 > 0 && rx.byte3 <= 18){ //handle sensor calibration commands from the configuration tool.
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
            blinkNumber = 0;
            calibration = 1;
          }

          if (rx.byte3 == 126) { //when communication is established, send all current settings to tool.
            communicationMode = 1;
            blinkNumber = 0;
            sendSettings();
          }

          if (rx.byte3 == 63) { //change secret button command mode
            secretSelector[mode] = 0;
            loadPrefs();
          }
          if (rx.byte3 == 64) {
            secretSelector[mode] = 1;
            loadPrefs();
          }

          if (rx.byte3 == 65) { //change vented option
            ventedSelector[mode] = 0;
           loadPrefs();
          }
          if (rx.byte3 == 66) {
            ventedSelector[mode] = 1;
           loadPrefs();
          }

          if (rx.byte3 == 67) { //change bagless option
            baglessSelector[mode] = 0;
           loadPrefs();
          }
          if (rx.byte3 == 68) {
            baglessSelector[mode] = 1;
           loadPrefs();
          }

          for (byte i = 3; i < 6; i++) { // update the three selected fingering modes if prompted by the tool.
            if (rx.byte3 >= i * 10 && rx.byte3 <= (i * 10) + 7) {
              modeSelector[i - 3] = rx.byte3 - (i * 10);
              loadPrefs();
            }
          }         

          for (byte i = 0; i < 3; i++) { //update current mode (instrument) if directed.
            if (rx.byte3 == 60 + i) {
              mode = i;
              play = 0;
             loadPrefs(); //load the correct user settings based on current instrument.
              sendSettings(); //send settings for new mode to tool.
              blinkNumber = abs(mode) + 1;
            }
          }

          for (byte i = 0; i < 3; i++) { //update current pitchbend mode if directed.
            if (rx.byte3 == 70 + i) {
              pitchBendModeSelector[mode] = i;
              loadPrefs();
              blinkNumber = abs(pitchBendMode) + 1;         
            }
          }

          for (byte i = 0; i < 5; i++) { //update current breath mode if directed.
            if (rx.byte3 == 80 + i) {
              breathModeSelector[mode] = i;
              loadPrefs(); //load the correct user settings based on current instrument.
              blinkNumber = abs(breathMode) + 1;
            }
          }        

          for (byte i = 0; i < 8; i++) { //update button receive mode (this indicates the row in the button settings for which the next received byte will be).
            if (rx.byte3 == 90 + i) {
              buttonReceiveMode = i;
            }
          }

          for (byte i = 0; i < 8; i++) { //update button configuration
            if (buttonReceiveMode == i) {
              for (byte j = 0; j < 12; j++) { //update column 0 (action).
                if (rx.byte3 == 100 + j) {
                  buttonPrefs[mode][i][0] = j;
                  blinkNumber = 0;
                }
              }
              for (byte k = 0; k < 5; k++) { //update column 1 (MIDI action).
                if (rx.byte3 == 112 + k) {
                  buttonPrefs[mode][i][1] = k;
                }
              }
            }
          }

          for (byte i = 0; i < 3; i++) { //update momentary
            if (buttonReceiveMode == i) {
              if (rx.byte3 == 117) {
                momentary[mode][i] = 0;
                noteOnOffToggle[i] = 0;
              }
              if (rx.byte3 == 118) {
                momentary[mode][i] = 1;
                noteOnOffToggle[i] = 0;
              }
            }           
          }


          if (rx.byte3 == 123) { //save settings as the defaults for the current instrument
            saveSettings(mode);
            loadFingering();
  			loadSettingsForAllModes();
  			blinkNumber = 3;
  			ledTimer = millis();
          }
          

          if (rx.byte3 == 124) { //Save settings as the defaults for all instruments
            for (byte k = 0; k < 3; k++) {
            	saveSettings(k);            
  				loadFingering();
  				loadSettingsForAllModes();
  				blinkNumber = 3;
  				ledTimer = millis();
            }
          }

          if (rx.byte3 == 125) { //restore all factory settings
            restoreFactorySettings();
            blinkNumber = 3;
          }
        }


        if (rx.byte2 == 103) {
          senseDistanceSelector[mode] = rx.byte3;
         loadPrefs();
        }


        if (rx.byte2 == 117) {
          unsigned long v = rx.byte3 * 8191UL / 100;
          vibratoDepthSelector[mode] = v; //scale vibrato depth in cents up to pitchbend range of 0-8191
          loadPrefs();
        }
        

        if (rx.byte2 == 111) {
          if(rx.byte3 < 50){
            noteShiftSelector[0] = rx.byte3;}
          else{noteShiftSelector[0] = - 127 + rx.byte3;}
          loadPrefs();
        }

        if (rx.byte2 == 112) {
          if(rx.byte3 < 50){
            noteShiftSelector[1] = rx.byte3;}
          else{noteShiftSelector[1] = - 127 + rx.byte3;}
         loadPrefs();
        }

        if (rx.byte2 == 113) {
          if(rx.byte3 < 50){
            noteShiftSelector[2] = rx.byte3;}
          else{noteShiftSelector[2] = - 127 + rx.byte3;}
          loadPrefs();
        }      

        if (rx.byte2 == 104) { //update receive mode, used for advanced pressure range sliders and expression and drones panel settings (this indicates the variable for which the next received byte on CC 105 will be).
              pressureReceiveMode = rx.byte3-1;        
          }
        
        if (rx.byte2 == 105) {
          if(pressureReceiveMode < 12){
          pressureSelector[mode][pressureReceiveMode] = rx.byte3;  //advanced pressure values
          loadPrefs();
        }
      
      		else if(pressureReceiveMode < 33){
      	  	ED[mode][pressureReceiveMode-12] = rx.byte3; //expression and drones settings
            loadPrefs();
        	}
        
        	else if(pressureReceiveMode == 33){
      	 	LSBlearnedPressure = rx.byte3;

        	}
        
            else if(pressureReceiveMode == 34) {
      	  	learnedPressureSelector[mode] = (rx.byte3 << 7) | LSBlearnedPressure;
      	  	loadPrefs();
        	}}
        
        
        
        

       // if (rx.byte2 == 109) {
        //  hysteresis = rx.byte3;
       // }

      if (rx.byte2 == 106 && rx.byte3 >15) {
        if(rx.byte3 == 16){ //update custom
          customSelector[mode] = 0;
         loadPrefs();
         }
          
        if(rx.byte3 == 17){
          customSelector[mode] = 1;
          loadPrefs();
          }

        if(rx.byte3 > 19 && rx.byte3 < 29){ //update enabled vibrato holes for "universal" vibrato
          bitSet(vibratoHolesSelector[mode],rx.byte3-20);
          loadPrefs();
          }

        if(rx.byte3 > 29 && rx.byte3 < 39){
          bitClear(vibratoHolesSelector[mode],rx.byte3-30);
          loadPrefs();
          }
          
        if(rx.byte3 == 39){
          useLearnedPressureSelector[mode] = 0;
          loadPrefs();
          }  

        if(rx.byte3 == 40){
          useLearnedPressureSelector[mode] = 1;
          loadPrefs();
          }

        if(rx.byte3 == 41){
          learnedPressureSelector[mode] = sensorValue;
          sendUSBMIDI(CC, 7, 104, 34); //indicate that LSB of learned pressure is about to be sent
          sendUSBMIDI(CC, 7, 105, learnedPressureSelector[mode] & 0x7F); //send LSB of learned pressure 
          sendUSBMIDI(CC, 7, 104, 35); //indicate that MSB of learned pressure is about to be sent
          sendUSBMIDI(CC, 7, 105, learnedPressureSelector[mode] >> 7); //send MSB of learned pressure
          loadPrefs();
          }

        if(rx.byte3 == 42){  //autocalibrate bell sensor only                       
          calibration = 2;
          blinkNumber = 0;
          }


        if(rx.byte3 == 43){
          int tempPressure = sensorValue;
          ED[mode][DRONES_PRESSURE_LOW_BYTE] = tempPressure & 0x7F;
          ED[mode][DRONES_PRESSURE_HIGH_BYTE] = tempPressure >> 7;
          sendUSBMIDI(CC, 7, 104, 32); //indicate that LSB of learned drones pressure is about to be sent
          sendUSBMIDI(CC, 7, 105, ED[mode][DRONES_PRESSURE_LOW_BYTE]); //send LSB of learned drones pressure 
          sendUSBMIDI(CC, 7, 104, 33); //indicate that MSB of learned drones pressure is about to be sent
          sendUSBMIDI(CC, 7, 105, ED[mode][DRONES_PRESSURE_HIGH_BYTE]); //send MSB of learned drones pressure
          }

        if(rx.byte3 == 46){ //update invert
          invert[mode] = 0;
         }
          
        if(rx.byte3 == 47){
          invert[mode] = 1;
          }
          

          

        if(rx.byte3 == 45){  //save current sensor calibration as factory calibration
          for (byte i = 0; i < 18; i++) {
            EEPROM.update(i, EEPROM.read(i+18));
          }
          for (int i = 731; i < 741; i++) { //save baseline calibration as factory baseline
            EEPROM.update(i, EEPROM.read(i-10));
          }
          //EEPROM.update(38, EEPROM.read(36)); //hysteresis
          }       


         
      }
  


        for (byte i = 0; i < 8; i++) { //update channel, byte 2, byte 3 for MIDI message for button MIDI command for row i
          if (buttonReceiveMode == i) {
            if (rx.byte2 == 106 && rx.byte3 < 16) {
              buttonPrefs[mode][i][2] = rx.byte3;
            }
            if (rx.byte2 == 107) {
              buttonPrefs[mode][i][3] = rx.byte3;
            }
            if (rx.byte2 == 108) {
              buttonPrefs[mode][i][4] = rx.byte3;
            }
          }
        }

      }

    }

    } //end of ignore CCs 121, 123
    
  } while (rx.header != 0);
  
}



//save settings for current instrument as defaults for given instrument (i)
void saveSettings(byte i) {

    EEPROM.update(316 + i, invert[mode]);
    EEPROM.update(80 + i, customSelector[mode]);
    EEPROM.update(40 + i, modeSelector[mode]);
    EEPROM.update(53 + i, noteShiftSelector[mode]);
    EEPROM.update(56 + i, ventedSelector[mode]);
    EEPROM.update(313 + i, baglessSelector[mode]);
    EEPROM.update(45 + i, secretSelector[mode]);
    EEPROM.update(50 + i, senseDistanceSelector[mode]);
    EEPROM.update(60 + i, pitchBendModeSelector[mode]); 
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
            
    for (byte n = 0; n < 21; n++) {
       EEPROM.update((741 + n + (i * 21)), ED[mode][n]);
    }         
            
    for (byte h = 0; h < 3; h++) {
      EEPROM.update(250 + (i * 3) + h, momentary[mode][h]);
    }

    for (byte j = 0; j < 5; j++) { //save button configuration for current mode
        for (byte k = 0; k < 8; k++) {
          EEPROM.update(100 + (i * 50) + (j * 10) + k, buttonPrefs[mode][k][j]);
        }
    }
    
   // EEPROM.update(36, hysteresis);
    
}




//load saved fingering patterns
void loadFingering() {

  for (byte i = 0; i < 3; i++) {
    modeSelector[i] = EEPROM.read(40 + i);
    noteShiftSelector[i] = (int8_t)EEPROM.read(53 + i);
  
    if (communicationMode) {
      sendUSBMIDI(CC, 7, 102, (30 + i * 10) + modeSelector[i]); //send currently selected 3 fingering patterns.
      if(noteShiftSelector[i] >=0){
        sendUSBMIDI(CC, 7, (111 + i), noteShiftSelector[i]);} //send noteShift, with a transformation for sending negative values over MIDI.
        else{sendUSBMIDI(CC, 7, (111 + i), noteShiftSelector[i] + 127);}
  }
  }
}




//load settings for all three instruments from EEPROM
void loadSettingsForAllModes() {
  for (byte i = 0; i < 3; i++) {

    invert[i] = EEPROM.read(316 + i);
    customSelector[i] = EEPROM.read(80 + i);
    ventedSelector[i] = EEPROM.read(56 + i);
    baglessSelector[i] = EEPROM.read(313 + i);
    secretSelector[i] = EEPROM.read(45 + i);   
    pitchBendModeSelector[i] =  EEPROM.read(60 + i);
    breathModeSelector[i] = EEPROM.read(70 + i);
    senseDistanceSelector[i] = EEPROM.read(50 + i);
    vibratoHolesSelector[i] = word(EEPROM.read(84 + (i * 2)),EEPROM.read(83 + (i * 2)));
    vibratoDepthSelector[i] = word(EEPROM.read(90 + (i * 2)),EEPROM.read(89 + (i * 2)));
    useLearnedPressureSelector[i] = EEPROM.read(95 + i);
    learnedPressureSelector[i] = word(EEPROM.read(274 + (i * 2)),EEPROM.read(273 + (i * 2)));      

    for (byte n = 0; n < 21; n++) {
       ED[i][n] = EEPROM.read(741 + n + (i * 21));
    } 

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


  }

     
}




//this is used only once, the first time the software is run, to copy all the default settings to EEPROM, where they can later be restored
void saveFactorySettings() {

  for (byte i = 0; i < 3; i++) { //then we save all the current settings for all three instruments.
	mode = i;
	saveSettings(i);
    }
  mode = 0; //switch back to mode 0

    blinkNumber = 3;
    ledTimer = millis();
  

  for (int i = 40; i < 320; i++) { //then we read every byte in EEPROM from 40 to 319
    byte reading = EEPROM.read(i);
    EEPROM.update(400 + i, reading); //and rewrite them from 440 to 719. Then they'll be available to easily restore later if necessary.
  }
  
  for (int i = 741; i < 804; i++) { //then we do the same for the expression and drones variables
    byte reading2 = EEPROM.read(i);
    EEPROM.update(63 + i, reading2);
  }
  

  //EEPROM.update(38, EEPROM.read(36)); //do the same for hysteresis, which is stored in a different place.

  EEPROM.update(44, 3); //indicates settings have been saved
}






//restore original settings from EEPROM
void restoreFactorySettings() {
  byte reading;
  for (int i = 440; i < 720; i++) { //then we read every byte in EEPROM from 440 to 719
    reading = EEPROM.read(i);
    EEPROM.update(i - 400, reading);
  } //and rewrite them from 40 to 319.
  
  for (int i = 804; i < 867; i++) { //then the same for expression and drones settings.
    reading = EEPROM.read(i);
    EEPROM.update(i - 63, reading);
  }

if(hardwareRevision > 21){
    for (byte i = 0; i < 18; i++) { //do the same with sensor calibration (copy 0-17 to 18-35) if it's the new hardware with factory calibration saved.
    EEPROM.update((i+18), EEPROM.read(i));
    }


    for (int i = 731; i < 741; i++) { //do the same with sensor baseline (copy 731-740 to 721-730)
    EEPROM.update((i - 10), EEPROM.read(i));
    }

    }

   //EEPROM.update(36,EEPROM.read(38)); //hysteresis
    
  loadFingering();
  loadCalibration();
  loadSettingsForAllModes(); //load the newly restored settings
  communicationMode = 1;
  loadPrefs();
  sendSettings(); //send the new settings
  
}




//send all settings for current instrument to the WARBL Configuration Tool.
void sendSettings() {


  sendUSBMIDI(CC, 7, 110, VERSION); //send software version
  sendUSBMIDI(CC, 7, 102, 30 + modeSelector[0]); //send currently selected 3 fingering patterns.
  sendUSBMIDI(CC, 7, 102, 40 + modeSelector[1]);
  sendUSBMIDI(CC, 7, 102, 50 + modeSelector[2]);
  sendUSBMIDI(CC, 7, 102, 60 + mode); //send current instrument
  
  sendUSBMIDI(CC, 7, 103, senseDistance); //send sense distance
  sendUSBMIDI(CC, 7, 117, vibratoDepth * 100UL / 8191); //send vibrato depth, scaled down to cents 
  sendUSBMIDI(CC, 7, 102, 70 + pitchBendMode); //send current pitchBend mode
  sendUSBMIDI(CC, 7, 102, 80 + breathMode); //send current breathMode
  sendUSBMIDI(CC, 7, 102, 120 + bellSensor); //send bell sensor state
  sendUSBMIDI(CC, 7, 102, 63 + secret); //send secret button command option
  sendUSBMIDI(CC, 7, 102, 65 + vented); //send vented option
  sendUSBMIDI(CC, 7, 102, 67 + bagless); //send bagless option
  sendUSBMIDI(CC, 7, 106, 46 + invert); //send invert option
  sendUSBMIDI(CC, 7, 106, 16 + custom); //send custom option
  sendUSBMIDI(CC, 7, 106, 39 + useLearnedPressure); //send calibration option
  //sendUSBMIDI(CC, 7, 109, hysteresis); //send hysteresis
  sendUSBMIDI(CC, 7, 104, 34); //indicate that LSB of learned pressure is about to be sent
  sendUSBMIDI(CC, 7, 105, learnedPressure & 0x7F); //send LSB of learned pressure 
  sendUSBMIDI(CC, 7, 104, 35); //indicate that MSB of learned pressure is about to be sent
  sendUSBMIDI(CC, 7, 105, learnedPressure >> 7); //send MSB of learned pressure

  
  if(noteShiftSelector[0] >=0){
    sendUSBMIDI(CC, 7, 111, noteShiftSelector[0]);} //send noteShift, with a transformation for sending negative values over MIDI.
    else{sendUSBMIDI(CC, 7, 111, noteShiftSelector[0] + 127);}
    
  if(noteShiftSelector[1] >=0){
    sendUSBMIDI(CC, 7, 112, noteShiftSelector[1]);} //send noteShift
    else{sendUSBMIDI(CC, 7, 112, noteShiftSelector[1] + 127);}
    
  if(noteShiftSelector[2] >=0){
    sendUSBMIDI(CC, 7, 113, noteShiftSelector[2]);} //send noteShift
    else{sendUSBMIDI(CC, 7, 113, noteShiftSelector[2] + 127);}


  for (byte i = 0; i < 9; i++) {
    sendUSBMIDI(CC, 7, 106, 20 + i + (10 * (bitRead(vibratoHolesSelector[mode],i)))); //send enabled vibrato holes  
  }  

  for (byte i = 0; i < 8; i++) {
    sendUSBMIDI(CC, 7, 102, 90 + i); //indicate that we'll be sending data for button commands row i (click 1, click 2, etc.)
    sendUSBMIDI(CC, 7, 102, 100 + buttonPrefs[mode][i][0]); //send action (i.e. none, send MIDI message, etc.)
    if (buttonPrefs[mode][i][0] == 1) { //if the action is a MIDI command, send the rest of the MIDI info for that row.
      sendUSBMIDI(CC, 7, 102, 112 + buttonPrefs[mode][i][1]);
      sendUSBMIDI(CC, 7, 106, buttonPrefs[mode][i][2]);
      sendUSBMIDI(CC, 7, 107, buttonPrefs[mode][i][3]);
      sendUSBMIDI(CC, 7, 108, buttonPrefs[mode][i][4]);
    }
  }
  
  
  for (byte i = 0; i < 21; i++) { //send settings for expression and drones control panels
  	sendUSBMIDI(CC, 7, 104, i + 13);
  	sendUSBMIDI(CC, 7, 105, ED[mode][i]);
   }
  

  for (byte i = 0; i < 3; i++) {
    sendUSBMIDI(CC, 7, 102, 90 + i); //indicate that we'll be sending data for momentary
    sendUSBMIDI(CC, 7, 102, 117 + momentary[mode][i]);
  }


  for (byte i = 0; i < 12; i++) {
     sendUSBMIDI(CC, 7, 104, i+1); //indicate which pressure variable we'll be sending data for
     sendUSBMIDI(CC, 7, 105, pressureSelector[mode][i]); //send the data
    }


}





void blink() { //blink LED given number of times

  if ((millis() - ledTimer) >= 200) {
  	ledTimer = millis();

    if (LEDon) {
      digitalWrite2(ledPin, LOW);
      blinkNumber--;
      LEDon = 0;

     if(sensorCalibration == 0) {     //this is a good time to take an initial reading to calibrate the pressure sensor, because this will happen right afer blinking the LED at startup.
      sensorCalibration = sensorValue;
     }
      
      return;
    }

    if (!LEDon) {
      digitalWrite2(ledPin, HIGH);
      LEDon = 1;
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

  if (ED[mode][DRONES_CONTROL_MODE] == 1) {
    if (justPressed[0] && !pressed[2] && !pressed[1] && !(calibration>0) && holeCovered >> 1 == 0b00001000) { //turn drones on/off if button 0 is pressed and fingering pattern is 0 0001000.
      justPressed[0] = 0;
      specialPressUsed[0] = 1;
      if(!dronesOn){
          startDrones();
      }
      else{
         stopDrones();
      }
    }
  }

  if (secret) {
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

    if (justPressed[0] && !pressed[2] && !pressed[1] && !(calibration>0) && holeCovered >> 1 == 0b00000010) { //change instrument if button 0 is pressed and fingering pattern is 0 0000001.
      justPressed[0] = 0;
      specialPressUsed[0] = 1;
      mode++; //set instrument
      if (mode == 3) {
        mode = 0;
      }
      play = 0;
      loadPrefs(); //load the correct user settings for offset, pitchBendMode, senseDistance, and breathMode, based on current fingering mode.
      blinkNumber = abs(mode) + 1;
      ledTimer = millis();
      if (communicationMode) {
        sendSettings(); //tell configuration tool to switch mode and send all settings for current mode.
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
  if (justPressed[0] && ((!secret) || (holeCovered >> 1 != 0b00001000 && holeCovered >> 1 != 0b00000100 && holeCovered >> 1 != 0b00000010 && momentary[mode][0]))) { //do action for button 0 press.
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

    case 1: //MIDI command

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

      if (buttonPrefs[mode][action][1] == 3) { //increase program change
        if(program < 127){
       program++;
       }
       else{program = 0;
       }
       sendUSBMIDI(PROGRAM_CHANGE, buttonPrefs[mode][action][2], program);
       blinkNumber = 1;
       ledTimer = millis();
      }

      if (buttonPrefs[mode][action][1] == 4) { //decrease program change
        if(program > 0){
        program--;
        }
       else{program = 127;
        }
       sendUSBMIDI(PROGRAM_CHANGE, buttonPrefs[mode][action][2], program);
        blinkNumber = 1;
        ledTimer = millis();
        }

      break;

    case 2:  //set vibrato/slide mode
      pitchBendModeSelector[mode] ++;
      if (pitchBendModeSelector[mode] == kPitchBendNModes) {
        pitchBendModeSelector[mode] = kPitchBendSlideVibrato;     
      }
      loadPrefs();
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
      break;

    case 5:
      if (octaveShift < 3) {
        octaveShiftSelector[mode]++; //adjust octave shift up or down, within reason
        loadPrefs();
      }
      blinkNumber = abs(octaveShift);
      ledTimer = millis();
      break;

    case 6:
      if (octaveShift > -4) {
        octaveShiftSelector[mode]--;
        loadPrefs();
      }
      blinkNumber = abs(octaveShift);
      ledTimer = millis();
      break;

    case 7:
            for (byte i = 1; i < 17; i++) { //send MIDI panic
        sendUSBMIDI(CC, i, 123, 0);
        dronesOn = 0; //remember that drones are off, because MIDI panic will have most likely turned them off in all apps.
        }
      break;

    case 8:
      breathModeSelector[mode]++; //set breath mode
      if (breathModeSelector[mode] == kPressureNModes) {
        breathModeSelector[mode] = kPressureSingle;
      }
      loadPrefs();
      play = 0;
      blinkNumber = abs(breathMode) + 1;
      ledTimer = millis();
      if (communicationMode) {
        sendUSBMIDI(CC, 7, 102, 80 + breathMode); //send current breathMode
      }
      break;


    case 9: //toggle drones
      blinkNumber = 1;
      ledTimer = millis();
      if(!dronesOn){
          startDrones();
      }
      else{
         stopDrones();
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



//load the correct user settings for the current instrument. This is used at startup and any time settings are changed.
void loadPrefs() {

      vented = ventedSelector[mode];
      bagless = baglessSelector[mode];
      custom = customSelector[mode];
      vibratoHoles = vibratoHolesSelector[mode];    
      secret = secretSelector[mode];
      octaveShift = octaveShiftSelector[mode];
      noteShift = noteShiftSelector[mode];
      pitchBendMode = pitchBendModeSelector[mode];
      useLearnedPressure = useLearnedPressureSelector[mode];
      learnedPressure = learnedPressureSelector[mode]; 
      senseDistance = senseDistanceSelector[mode];
      vibratoDepth = vibratoDepthSelector[mode];
      breathMode = breathModeSelector[mode];    
      
      
       //some special settings for vented mode
      offset = pressureSelector[mode][(vented * 6) + 0];
      multiplier = pressureSelector[mode][(vented * 6) + 1];
      jumpValue = pressureSelector[mode][(vented * 6) + 2];
      dropValue = pressureSelector[mode][(vented * 6) + 3];
      jumpTime = pressureSelector[mode][(vented * 6) + 4];
      dropTime = pressureSelector[mode][(vented * 6) + 5];
          

      pitchBend = 8191;
      expression = 0;
      pitchBendTimer = millis();
      sendUSBMIDI(PITCH_BEND, 1, pitchBend & 0x7F, pitchBend >> 7);
      
      for (byte i=0; i< 9; i++) {
            iPitchBend[i] = 0;
             pitchBendOn[i] = 0;
             }

        if (custom && ((modeSelector[mode] == kModeWhistle || modeSelector[mode] == kModeChromatic || modeSelector[mode] == kModeUilleann) && pitchBendMode == kPitchBendVibrato)) {
          customEnabled = 1;
        }
        else (customEnabled = 0); //decide here whether custom vibrato can currently be used, so we don't have to do it every time we need to check pitchBend.       
          
        if(!useLearnedPressure){
            sensorThreshold[0] = (sensorCalibration + soundTriggerOffset); //pressure sensor calibration at startup. We set the on/off threshhold just a bit higher than the reading at startup.
        }
        
        else{sensorThreshold[0] = (learnedPressure + soundTriggerOffset);
        } 
        
        sensorThreshold[1] = sensorThreshold[0] + (offset << 2); //threshold for move to second octave
      
  
    for (byte i=0; i < 9; i++) {
       toneholeScale[i] = ((8 * 8191)/(toneholeCovered[i] - 50 - senseDistance)/2); // Precalculate scaleing factors for pitchbend. This one is for sliding. We multiply by 8 first to reduce rounding errors. We'll divide again later.
       vibratoScale[i] = ((8 * vibratoDepth)/(toneholeCovered[i] - 50 - senseDistance)/2);} //This one is for vibrato

  minIn = 100 + (ED[mode][INPUT_PRESSURE_MIN] * 8); //precalculate input and output pressure ranges for sending pressure as CC
  minOut = (ED[mode][OUTPUT_PRESSURE_MIN] * 129);
  maxOut = (ED[mode][OUTPUT_PRESSURE_MAX] * 129);
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

// Send a debug MIDI message with a value up 16383 (14 bits)
void debug_log(int msg){
  sendUSBMIDI(CC, 7, 106, 48); //indicate that LSB is about to be sent
  sendUSBMIDI(CC, 7, 119, msg & 0x7F); //send LSB
  sendUSBMIDI(CC, 7, 106, 49); //indicate that MSB is about to be sent
  sendUSBMIDI(CC, 7, 119, msg >> 7); //send MSB
    sendUSBMIDI(CC, 7, 106, 51); //indicates end of two-byte message
}



//initialize the ADC with custom settings because we aren't using analogRead.
void ADC_init(void)  
{ 

   ADCSRA &= ~(bit (ADPS0) | bit (ADPS1) | bit (ADPS2)); // clear ADC prescaler bits

 //ADCSRA = (1<<ADEN) | ((1<<ADPS2) | (1<<ADPS0));   // enable ADC Division Factor 32 (60 us)
 ADCSRA = (1<<ADEN) | ((1<<ADPS2));   // enable ADC Division Factor 16 (36 us)

 
 ADMUX = (1<<REFS0); //Voltage reference from Avcc (5v) 
 ADC_read(1); //start an initial conversion (pressure sensor), which will also enable the ADC complete interrupt and trigger other conversions.
 //while(!(ADCSRA & (1<<ADIF))); // waiting for ADIF, conversion complete
}



//start an ADC conversion.
void ADC_read(byte pin)
{

  if (pin >= 18) pin -= 18; // allow for channel or pin numbers
  pin = analogPinToChannel(pin);

   ADCSRB = (ADCSRB & ~(1 << MUX5)) | (((pin >> 3) & 0x01) << MUX5);
   ADMUX = (1<<REFS0) | (pin & 0x07);
     
  ADCSRA |= bit (ADSC) | bit (ADIE); //start a conversion and enable the ADC complete interrupt

  } 



void startDrones()
{
     dronesOn = 1;
//          Serial.println("on");
  switch (ED[mode][DRONES_ON_COMMAND]) {
   case 0:
      sendUSBMIDI(NOTE_ON, ED[mode][DRONES_ON_CHANNEL], ED[mode][DRONES_ON_BYTE2], ED[mode][DRONES_ON_BYTE3]);
      break;
   case 1:
      sendUSBMIDI(NOTE_OFF, ED[mode][DRONES_ON_CHANNEL], ED[mode][DRONES_ON_BYTE2], ED[mode][DRONES_ON_BYTE3]);
      break;
   case 2:
      sendUSBMIDI(CC, ED[mode][DRONES_ON_CHANNEL], ED[mode][DRONES_ON_BYTE2], ED[mode][DRONES_ON_BYTE3]);
      break;
  }
  }



void stopDrones()
{
     dronesOn = 0;
    // Serial.println("off");
  switch (ED[mode][DRONES_OFF_COMMAND]) {
   case 0:
      sendUSBMIDI(NOTE_ON, ED[mode][DRONES_OFF_CHANNEL], ED[mode][DRONES_OFF_BYTE2], ED[mode][DRONES_OFF_BYTE3]);
            break;
   case 1:
      sendUSBMIDI(NOTE_OFF, ED[mode][DRONES_OFF_CHANNEL], ED[mode][DRONES_OFF_BYTE2], ED[mode][DRONES_OFF_BYTE3]);
            break;
   case 2:
      sendUSBMIDI(CC, ED[mode][DRONES_OFF_CHANNEL], ED[mode][DRONES_OFF_BYTE2], ED[mode][DRONES_OFF_BYTE3]);
            break;
  }
  }




//send pressure data to configuration tool and as CC if that option is selected  
void sendPressure()
{
	unsigned long n = sensorValue;
    
	if(communicationMode){      
          sendUSBMIDI(CC, 7, 116, n & 0x7F); //send LSB of current pressure to configuration tool
          sendUSBMIDI(CC, 7, 118, n >> 7); //send MSB of current pressure
          }
          
	if(ED[mode][SEND_PRESSURE] == 1) {
  	n = constrain(map(n, minIn, (100 + (ED[mode][INPUT_PRESSURE_MAX] * 8)), 100, 900), 100, 900); //map the sensorvalue from the selected input range up to 100-900, to maximize sensitivity at smaller ranges
          
		if (ED[mode][CURVE] == 1) { //for this curve, cube the input and scale back down to avoid variable overflow.
        	maxIn = 72900;
			n = ((n * n * n) / 10000); 
        }

   		else if (ED[mode][CURVE] == 2) { //for this curve, scale up, divide by n, and invert.
       		maxIn = 7300;
        	n = 8200 - (810000 / n);
        }

   else {maxIn = 900;} //max input value for linear

	
	sendUSBMIDI(CC, ED[mode][PRESSURE_CHANNEL], ED[mode][PRESSURE_CC], constrain((map(n, 100, maxIn, minOut, maxOut)), minOut, maxOut) >> 7); //send MSB of pressure mapped to the output range
  }

}
