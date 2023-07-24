//Monitor the status of the 3 buttons. The integrating debouncing algorithm is taken from debounce.c, written by Kenneth A. Kuhn:http://www.kennethkuhn.com/electronics/debounce.c
void checkButtons()
{


    for (byte j = 0; j < 3; j++) {


        if (digitalRead2f(buttons[j]) == 0) { //if the button reads low, reduce the integrator by 1
            if (integrator[j] > 0) {
                integrator[j]--;
            }
        } else if (integrator[j] < MAXIMUM) { //if the button reads high, increase the integrator by 1
            integrator[j]++;
        }


        if (integrator[j] == 0) { //the button is pressed.
            pressed[j] = 1; //we make the output the inverse of the input so that a pressed button reads as a "1".
            buttonUsed = 1; //flag that there's been button activity, so we know to handle it.

            if (prevOutput[j] == 0 && !longPressUsed[j]) {
                justPressed[j] = 1; //the button has just been pressed
            }

            else {
                justPressed[j] = 0;
            }

            if (prevOutput[j] == 1) { //increase a counter so we know when a button has been held for a long press
                longPressCounter[j]++;
            }
        }



        else if (integrator[j] >= MAXIMUM) { //the button is not pressed
            pressed[j] = 0;
            integrator[j] = MAXIMUM; // defensive code if integrator got corrupted

            if (prevOutput[j] == 1 && !longPressUsed[j]) {
                released[j] = 1; //the button has just been released
                buttonUsed = 1;
            }

            longPress[j] = 0;
            longPressUsed[j] = 0; //if a button is not pressed, reset the flag that tells us it's been used for a long press.
            longPressCounter[j] = 0;
        }



        if (longPressCounter[j] > 300 && !longPressUsed[j]) { //if the counter gets to a certain level, it's a long press
            longPress[j] = 1;
        }


        prevOutput[j] = pressed[j]; //keep track of state for next time around.

    }
}





// ADC complete ISR for reading sensors. We read the sensors asynchronously by starting a conversion and then coming back by interrupt when it is complete.
ISR (ADC_vect)
{

    byte prev = 8;
    if (lastRead != 0) {
        prev = lastRead - 1;
    }

    byte next = lastRead + 1;
    if (lastRead == 8) {
        next = 0;
    }

    if (lastRead == 9) { //this time is different because we're about to take a short break and read the pressure sensor.

        digitalWrite2f(pins[8], LOW); //turn off the previous LED
        tempToneholeRead[8] = (ADC) - tempToneholeReadA[8]; //get the previous illuminated reading and subtract the ambient light reading
        Timer1.resume(); //start the timer to take a short break, conserving some power.
        return;
    }

    if (firstTime) {
        if (lastRead == 0) {
            tempSensorValue = (ADC); //we've just returned from starting the pressure sensor reading, so get the conversion and flag that it is ready.
            sensorDataReady = 1;
        } else {
            digitalWrite2f(pins[prev], LOW); //turn off the previous LED
            tempToneholeRead[prev] = (ADC) - tempToneholeReadA[prev]; //get the previous illuminated reading and subtract the ambient light reading
        }
        ADC_read(holeTrans[next]); // start ambient reading for next sensor
        firstTime = 0;
        if (lastRead != 0) {
            digitalWrite2f(pins[lastRead], HIGH); //turn on the current LED
        }
        return;
    }

    //if !firstTime (implied, because we wouldn't have gotten this far otherwise)
    tempToneholeReadA[next] = (ADC); //get the ambient reading for the next sensor
    ADC_read(holeTrans[lastRead]); //start illuminated reading for current sensor
    firstTime = 1;
    lastRead ++;
}


//Timer ISR for adding a small delay after reading new sensors, to conserve a bit of power and give the processor time to catch up if necesary.
void timerDelay(void)
{

    timerCycle ++;

    if (timerCycle == 2) {
        digitalWrite2f(pins[0], HIGH); //turn on the LED for the bell sensor (this won't use any power if the sensor isn't plugged in). It will be on for ~170 uS, which is necesary because it's a slower sensor than the others.
        return;
    }

    if (timerCycle == 3) {
        Timer1.stop(); //stop the timer after running twice to add a delay.
        ADC_read(4); //start reading the pressure sensor (pinA4).
        firstTime = 1;
        lastRead = 0;
        timerCycle = 0;
    }
    //if timerCycle is 1 we just return and wait until next time.
}






//Determine which holes are covered
void get_fingers()
{

    for (byte i = 0; i < 9; i++) {
        if ((toneholeRead[i]) > (toneholeCovered[i] - 50)) {
            bitWrite(holeCovered, i, 1); //use the tonehole readings to decide which holes are covered
        } else if ((toneholeRead[i]) <= (toneholeCovered[i] - 54)) {
            bitWrite(holeCovered, i, 0); //decide which holes are uncovered -- the "hole uncovered" reading is a little less then the "hole covered" reading, to prevent oscillations.
        }
    }
}





//Send the pattern of covered holes to the Configuration Tool
void send_fingers()
{

    if (communicationMode) { //send information about which holes are covered to the Configuration Tool if we're connected. Because it's MIDI we have to send it in two 7-bit chunks.
        sendUSBMIDI(CC, 7, 114, holeCovered >> 7);
        sendUSBMIDI(CC, 7, 115, lowByte(holeCovered));
    }
}





//Return a MIDI note number (0-127) based on the current fingering. The analog readings of the 9 hole sensors are also stored in the tempToneholeRead variable for later use.
int get_note(unsigned int fingerPattern)
{


    int ret = -1;  //default for unknown fingering

    switch (modeSelector[mode]) { //determine the note based on the fingering pattern


        case kModeWhistle: //these first two are the same

        case kModeChromatic:

            tempCovered = (0b011111100 & fingerPattern) >> 2; //use bitmask and bitshift to ignore thumb sensor, R4 sensor, and bell sensor when using tinwhistle fingering. The R4 value will be checked later to see if a note needs to be flattened.
            ret = pgm_read_byte(&tinwhistle_explicit[tempCovered].midi_note);
            if (modeSelector[mode] == kModeChromatic && bitRead(fingerPattern, 1) == 1) {
                ret--;    //lower a semitone if R4 hole is covered and we're using the chromatic pattern
            }
            if (fingerPattern == holeCovered) {
                vibratoEnable = pgm_read_byte(&tinwhistle_explicit[tempCovered].vibrato);
            }
            return ret;



        case kModeUilleann: //these two are the same, with the exception of cancelling accidentals.

        case kModeUilleannStandard: //the same as the previous one, but we cancel the accidentals Bb and F#


            //If back D open, always play the D and allow finger vibrato
            if ((fingerPattern & 0b100000000) == 0) {
                if (fingerPattern == holeCovered) {
                    vibratoEnable = 0b000010;
                }
                return 74;
            }
            tempCovered = (0b011111110 & fingerPattern) >> 1; //ignore thumb hole and bell sensor
            ret = pgm_read_byte(&uilleann_explicit[tempCovered].midi_note);
            if (fingerPattern == holeCovered) {
                vibratoEnable = pgm_read_byte(&uilleann_explicit[tempCovered].vibrato);
            }
            if (modeSelector[mode] == kModeUilleannStandard) { //cancel accidentals if we're in standard uilleann mode
                if (tempCovered == 0b1001000 || tempCovered == 0b1001010) {
                    return 71;
                }

                if (tempCovered == 0b1101000 || tempCovered == 0b1101010) {
                    return 69;
                }
            }
            return ret;



        case kModeGHB:

            //If back A open, always play the A
            if ((fingerPattern & 0b100000000) == 0) {
                return 74;
            }
            tempCovered = (0b011111110 & fingerPattern) >> 1; //ignore thumb hole and bell sensor
            ret = pgm_read_byte(&GHB_explicit[tempCovered].midi_note);
            return ret;



        case kModeRecorder: //this is especially messy, should be cleaned up

            //If back thumb and L1 are open
            if ((((fingerPattern & 0b110000000) == 0) && (fingerPattern & 0b011111111) != 0) && (fingerPattern >> 1) != 0b00101100) {
                return 76; //play D
            }
            if (fingerPattern  >> 1 == 0b01011010) return 88; //special fingering for high D
            if (fingerPattern  >> 1 == 0b01001100) return 86; //special fingering for high C
            //otherwise check the chart.
            tempCovered = (0b011111110 & fingerPattern) >> 1; //ignore thumb hole and bell sensor
            ret = pgm_read_byte(&recorder_explicit[tempCovered].midi_note);
            //If back thumb is open
            if ((fingerPattern & 0b100000000) == 0 && (fingerPattern >> 1) != 0b00101100) {
                ret = ret + 12;
            }
            return ret;



        case kModeNorthumbrian:

            //If back A open, always play the A
            if ((fingerPattern & 0b100000000) == 0) {
                return 74;
            }
            tempCovered = fingerPattern >> 1; //bitshift once to ignore bell sensor reading
            tempCovered = findleftmostunsetbit(tempCovered);  //here we find the index of the leftmost uncovered hole, which will be used to determine the note from the general chart.
            for (uint8_t i = 0; i < 9; i++) {
                if (tempCovered == pgm_read_byte(&northumbrian_general[i].keys)) {
                    ret = pgm_read_byte(&northumbrian_general[i].midi_note);
                    return ret;
                }
            }
            break;


        case kModeGaita:

            tempCovered = fingerPattern >> 1; //bitshift once to ignore bell sensor reading
            ret = pgm_read_byte(&gaita_explicit[tempCovered].midi_note);
            if (ret == 0) {
                ret = -1;
            }
            return ret;



        case kModeGaitaExtended:

            tempCovered = fingerPattern >> 1; //bitshift once to ignore bell sensor reading
            ret = pgm_read_byte(&gaita_extended_explicit[tempCovered].midi_note);
            if (ret == 0) {
                ret = -1;
            }
            return ret;



        case kModeNAF:

            tempCovered = (0b011111110 & fingerPattern) >> 1; //ignore thumb hole and bell sensor
            ret = pgm_read_byte(&naf_explicit[tempCovered].midi_note);
            return ret;



        case kModeEVI:

            tempCovered = (0b011111110 & fingerPattern) >> 1; //ignore thumb hole and bell sensor
            ret = pgm_read_byte(&evi_explicit[tempCovered].midi_note);
            ret = ret + 4; //transpose up to D so that key selection in the Configuration Tool works properly
            return ret;



        case kModeKaval:

            //If back thumb is open, always play the B
            if ((fingerPattern & 0b100000000) == 0) {
                return 71;
            }
            tempCovered = (0b011111110 & fingerPattern) >> 1; //ignore thumb hole and bell sensor
            ret = pgm_read_byte(&kaval_explicit[tempCovered].midi_note);
            return ret;



        case kModeXiao:

            //Catch a few specific patterns with the thumb hole open:
            if ((fingerPattern & 0b100000000) == 0) {
                if ((fingerPattern >> 4) == 0b01110) { //if the top 5 holes are as shown
                    return 70; //play Bb
                }
                if ((fingerPattern & 0b010000000) == 0) { //if hole L1 is also open
                    return 71; //play B
                }
                return 72; //otherwise play a C
            } else if ((fingerPattern & 0b010000000) == 0) {
                return 69; //if thumb is closed but L1 is open play an A
            }
            //otherwise check the chart.
            tempCovered = (0b001111110 & fingerPattern) >> 1; //ignore thumb hole, L1 hole, and bell sensor
            ret = pgm_read_byte(&xiao_explicit[tempCovered].midi_note);
            return ret;



        case kModeSax:

            //check the chart.
            tempCovered = (0b011111100 & fingerPattern) >> 2; //ignore thumb hole, R4 hole, and bell sensor
            ret = pgm_read_byte(&sax_explicit[tempCovered].midi_note);
            if (((fingerPattern & 0b000000010) != 0) && ret != 47 && ret != 52 && ret != 53 && ret != 54 && ret != 59 && ret != 60 && ret != 61) { //sharpen the note if R4 is covered and the note isn't one of the ones that can't be sharpened (a little wonky but works and keep sthe chart shorter ;)
                ret++;
            }
            if ((fingerPattern & 0b100000000) != 0 && ret > 49) { //if the thumb hole is covered, raise the octave
                ret = ret + 12;
            }
            return ret;



        case kModeSaxBasic:

            //check the chart.
            tempCovered = (0b011111110 & fingerPattern) >> 1; //ignore thumb hole and bell sensor
            ret = pgm_read_byte(&saxbasic_explicit[tempCovered].midi_note);
            if ((fingerPattern & 0b100000000) != 0 && ret > 49) { //if the thumb hole is covered, raise the octave
                ret = ret + 12;
            }
            return ret;



        case kModeShakuhachi:

            //ignore all unused holes by extracting bits and then logical OR
            {
                //braces necessary for scope
                byte high = (fingerPattern >> 4) & 0b11000;
                byte middle = (fingerPattern >> 4) & 0b00011;
                middle = middle << 1;
                byte low = (fingerPattern >> 2) & 0b0000001;
                tempCovered = high | middle;
                tempCovered = tempCovered | low;
                ret = pgm_read_byte(&shakuhachi_explicit[tempCovered].midi_note);
                return ret;
            }



        case kModeSackpipaMajor:

        case kModeSackpipaMinor: //the same except we'll change C# to C

            //check the chart.
            tempCovered = (0b011111100 & fingerPattern) >> 2; //ignore thumb hole, R4 hole, and bell sensor
            if ((fingerPattern & 0b111111110) >> 1 == 0b11111111) { //play D if all holes are covered
                return 60; //play D
            }
            if ((fingerPattern & 0b100000000) == 0) { //if the thumb hole is open, play high E
                return 74; //play E
            }
            ret = pgm_read_byte(&sackpipa_explicit[tempCovered].midi_note);
            if (modeSelector[mode] == kModeSackpipaMinor) { //flatten the C# if we're in "minor" mode
                if (ret == 71) {
                    return 70; //play C natural instead
                }
            }
            return ret;



        case kModeCustom:
            tempCovered = (0b011111110 & fingerPattern) >> 1; //ignore thumb hole and bell sensor for now
            uint8_t leftmost = findleftmostunsetbit(tempCovered);  //here we find the index of the leftmost uncovered hole, which will be used to determine the note from the chart.

            for (uint8_t i = 0; i < 6; i++) { //look only at leftmost uncovered hole for lower several notes
                if (leftmost == i) {
                    customScalePosition = 47 - i;
                }
            }

            //several ugly special cases
            if (tempCovered >> 3 == 0b0111) {
                customScalePosition = 39;
            }

            else if (tempCovered >> 3 == 0b0110) {
                customScalePosition = 41;
            }

            else if (tempCovered >> 5 == 0b00) {
                customScalePosition = 40;
            }

            if (tempCovered == 0b1111111) {
                if (!switches[mode][R4_FLATTEN]) { //all holes covered but not R4 flatten
                    customScalePosition = 48;
                } else {
                    customScalePosition = 47;
                }
            }

            if (fingerPattern >> 8 == 0 && !switches[mode][THUMB_AND_OVERBLOW] && !breathMode == kPressureThumb && ED[mode][38] != 0) { //thumb hole is open and we're not using it for register
                customScalePosition = 38;
            }

            ret = ED[mode][customScalePosition];

            if (bitRead(tempCovered, 0) == 1 && switches[mode][R4_FLATTEN] && ret != 0) { //flatten one semitone if using R4 for that purpose
                ret = ret - 1;
            }

            return ret;



        default:
            return ret;
    }
}






//Add up any transposition based on key and register.
void get_shift()
{

    shift = ((octaveShift * 12) + noteShift); //adjust for key and octave shift.

    if (newState == 3 && !(modeSelector[mode] == kModeEVI || (modeSelector[mode] == kModeSax  && newNote < 62) || (modeSelector[mode] == kModeSaxBasic && newNote < 74) || (modeSelector[mode] == kModeRecorder && newNote < 76)) && !(newNote == 62 && (modeSelector[mode] == kModeUilleann || modeSelector[mode] == kModeUilleannStandard))) {  //if overblowing (except EVI, sax in the lower register, and low D with uilleann fingering, which can't overblow)
        shift = shift + 12; //add a register jump to the transposition if overblowing.
        if (modeSelector[mode] == kModeKaval) { //Kaval only plays a fifth higher in the second register.
            shift = shift - 5;
        }
    }

    if (breathMode == kPressureBell && modeSelector[mode] != kModeUilleann && modeSelector[mode] != kModeUilleannStandard) { //if we're using the bell sensor to control register
        if (bitRead(holeCovered, 0) == switches[mode][INVERT]) {
            shift = shift + 12; //add a register jump to the transposition if necessary.
            if (modeSelector[mode] == kModeKaval) {
                shift = shift - 5;
            }
        }
    }

    else if ((breathMode == kPressureThumb && (modeSelector[mode] == kModeWhistle || modeSelector[mode] == kModeChromatic || modeSelector[mode] == kModeNAF || modeSelector[mode] == kModeCustom)) || (breathMode == kPressureBreath && modeSelector[mode] == kModeCustom && switches[mode][THUMB_AND_OVERBLOW])) { //if we're using the left thumb to control the regiser with a fingering patern that doesn't normally use the thumb

        if (bitRead(holeCovered, 8) == switches[mode][INVERT]) {
            shift = shift + 12;   //add an octave jump to the transposition if necessary.
        }
    }

    //Some charts require another transposition to bring them to the correct key
    if (modeSelector[mode] == kModeGaita || modeSelector[mode] == kModeGaitaExtended) {
        shift = shift - 1;
    }

    if (modeSelector[mode] == kModeSax) {
        shift = shift + 2;
    }

    //  if ((holeCovered & 0b100000000) == 0 && (modeSelector[mode] == kModeWhistle || modeSelector[mode] == kModeChromatic) && newState == 3){ //with whistle, if we're overblowing and the thumb is uncovered, play the third octave.
    // shift = shift + 12;
    // }
}






//State machine that models the way that a tinwhistle etc. begins sounding and jumps octaves in response to breath pressure.
void get_state()
{

    noInterrupts();
    sensorValue2 = tempSensorValue; //transfer last reading to a non-volatile variable
    interrupts();

    if (sensorValue == sensorValue2) {
        return; //don't bother going further if the pressure hasn't changed.
    }

    byte scalePosition;

    if (modeSelector[mode] == kModeCustom) {
        scalePosition = 110 - customScalePosition; //scalePosition is used to tell where we are on the scale, because higher notes are more difficult to overblow.
    } else {
        scalePosition = newNote;
    }

    pressureChangeRate = sensorValue2 - sensorValue; //calculate the rate of change

    if (ED[mode][DRONES_CONTROL_MODE] == 3) { //use pressure to control drones if that option has been selected. There's a small amount of hysteresis added.

        if (!dronesState && sensorValue2 > 5 + (ED[mode][DRONES_PRESSURE_HIGH_BYTE] << 7 | ED[mode][DRONES_PRESSURE_LOW_BYTE])) {
            dronesState = 1;
            if (!dronesOn) {
                startDrones();
            }

        }

        else if (dronesState && sensorValue2 < (ED[mode][DRONES_PRESSURE_HIGH_BYTE] << 7 | ED[mode][DRONES_PRESSURE_LOW_BYTE])) {
            dronesState = 0;
            if (dronesOn) {
                stopDrones();
            }
        }
    }


    upperBound = (sensorThreshold[1] + ((scalePosition - 60) * multiplier)); //calculate the threshold between state 2 and state 3. This will also be used to calculate expression.


    if (jump && ((millis() - jumpTimer) >= jumpTime)) {
        jump = 0; //make it okay to switch registers again if some time has past since we "jumped" or "dropped" because of rapid pressure change.
    }

    else if (drop && ((millis() - dropTimer) >= dropTime)) {
        drop = 0;
    }


    if (!jump && !drop) {

        if ((breathMode == kPressureBreath || (breathMode == kPressureThumb && modeSelector[mode] == kModeCustom && switches[mode][THUMB_AND_OVERBLOW])) && ((sensorValue2 - sensorValue) > jumpValue) && (sensorValue2 > sensorThreshold[0])) {  //if the pressure has increased rapidly (since the last reading) and there's a least enough pressure to turn a note on, jump immediately to the second register
            newState = 3;
            jump = 1;
            jumpTimer = millis();
            sensorValue = sensorValue2;
            return;
        }

        if (newState == 3 && breathMode > kPressureSingle && ((sensorValue - sensorValue2) > dropValue)) {  //if we're in second register and the pressure has dropped rapidly, turn the note off (this lets us drop directly from the second register to note off).
            newState = 1;
            drop = 1;
            dropTimer = millis();
            sensorValue = sensorValue2;
            return;
        }
    }

    //if there haven't been rapid pressure changes and we haven't just jumped registers, choose the state based solely on current pressure.
    if (sensorValue2 <= sensorThreshold[0]) {
        newState = 1;
    }

    //added very small amount of hysteresis for state 2, 4/25/20
    else if (sensorValue2 > sensorThreshold[0] + 1 && (((breathMode != kPressureBreath) && !(breathMode == kPressureThumb && modeSelector[mode] == kModeCustom && switches[mode][THUMB_AND_OVERBLOW])) || (!jump && !drop  && (breathMode > kPressureSingle) && (sensorValue2 <= upperBound)))) { //single register mode or within the bounds for state 2
        newState = 2;
    } else if (!drop && (sensorValue2 > upperBound)) { //we're in two-register mode and above the upper bound for state 2
        newState = 3;
    }

    sensorValue = sensorValue2; //we'll use the current reading as the baseline next time around, so we can monitor the rate of change.
    sensorDataReady = 0; //we've used the sensor reading, so don't use it again


}





//calculate pitchbend expression based on pressure
void getExpression()
{

    //calculate the center pressure value for the current note, regardless of register, unless "override" is turned on and we're not in overblow mode. In that case, use the override bounds instead


    int lowerBound;
    int useUpperBound;

    if (switches[mode][OVERRIDE] && (breathMode != kPressureBreath)) {
        lowerBound = (ED[mode][EXPRESSION_MIN] * 9) + 100;
        useUpperBound = (ED[mode][EXPRESSION_MAX] * 9) + 100;
    } else {
        lowerBound = sensorThreshold[0];
        useUpperBound = upperBound;

    }

    unsigned int halfway = ((useUpperBound - lowerBound) >> 1) + lowerBound;

    if (newState == 3) {
        halfway = useUpperBound + halfway;
        lowerBound = useUpperBound;
    }

    if (sensorValue < halfway) {
        byte scale = (((halfway - sensorValue) * ED[mode][EXPRESSION_DEPTH] * 20) / (halfway - lowerBound)); //should maybe figure out how to do this without dividing.
        expression = - ((scale * scale) >> 3);
    } else {
        expression = (sensorValue - halfway) * ED[mode][EXPRESSION_DEPTH];
    }


    if (expression > ED[mode][EXPRESSION_DEPTH] * 200) {
        expression = ED[mode][EXPRESSION_DEPTH] * 200;   //put a cap on it, because in the upper register or in single-register mode, there's no upper limit
    }


    if (pitchBendMode == kPitchBendNone) { //if we're not using vibrato, send the pitchbend now instead of adding it in later.
        pitchBend = 0;
        sendPitchbend();
    }
}






//find how many steps down to the next lower note on the scale. Doing this in a function because the fingering charts can't be read from the main page, due to compilation order.
void findStepsDown()
{

    slideHole = findleftmostunsetbit(holeCovered); //determine the highest uncovered hole, to use for sliding
    if (slideHole == 127) { //this means no holes are covered.
        // this could mean the highest hole is starting to be uncovered, so use that as the slideHole
        slideHole = 7;
        //return;
    }
    unsigned int closedSlideholePattern = holeCovered;
    bitSet(closedSlideholePattern, slideHole); //figure out what the fingering pattern would be if we closed the slide hole
    stepsDown = constrain(tempNewNote - get_note(closedSlideholePattern), 0, 2); //and then figure out how many steps down it would be if a new note were triggered with that pattern.
}








//Custom pitchbend algorithms, tin whistle and uilleann by Michael Eskin
void handleCustomPitchBend()
{

  iPitchBend[2] = 0; //reset pitchbend for the holes that are being used
  iPitchBend[3] = 0;

    if (pitchBendMode == kPitchBendSlideVibrato || pitchBendMode == kPitchBendLegatoSlideVibrato) { //calculate slide if necessary.
        getSlide();
    }


    if (modeSelector[mode] != kModeGHB && modeSelector[mode] != kModeNorthumbrian) { //only used for whistle and uilleann
        if (vibratoEnable == 1) { //if it's a vibrato fingering pattern
            if (slideHole != 2) {
                iPitchBend[2] = adjvibdepth; //just assign max vibrato depth to a hole that isn't being used for sliding (it doesn't matter which hole, it's just so it will be added in later).
                iPitchBend[3] = 0;
            } else {
                iPitchBend[3] = adjvibdepth;
                iPitchBend[2] = 0;
            }
        }

  


        if (vibratoEnable == 0b000010) { //used for whistle and uilleann, indicates that it's a pattern where lowering finger 2 or 3 partway would trigger progressive vibrato.

            if (modeSelector[mode] == kModeWhistle || modeSelector[mode] == kModeChromatic) {
                for (byte i = 2; i < 4; i++) {
                    if ((toneholeRead[i] > senseDistance) && (bitRead(holeCovered, i) != 1 && (i != slideHole))) { //if the hole is contributing, bend down
                        iPitchBend[i] = ((toneholeRead[i] - senseDistance) * vibratoScale[i]) >> 3;
                    } else if (i != slideHole) {
                        iPitchBend[i] = 0;
                    }
                }
                if (iPitchBend[2] + iPitchBend[3] > adjvibdepth) {
                    iPitchBend[2] = adjvibdepth; //cap at max vibrato depth if they combine to add up to more than that (just set one to max and the other to zero)
                    iPitchBend[3] = 0;
                }
            }


            else if (modeSelector[mode] == kModeUilleann || modeSelector[mode] == kModeUilleannStandard) {

                // If the back-D is open, and the vibrato hole completely open, max the pitch bend
                if ((holeCovered & 0b100000000) == 0) {
                    if (bitRead(holeCovered, 3) == 1) {
                        iPitchBend[3] = 0;
                    } else {
                        // Otherwise, bend down proportional to distance
                        if (toneholeRead[3] > senseDistance) {
                            iPitchBend[3] = adjvibdepth - (((toneholeRead[3] - senseDistance) * vibratoScale[3]) >> 3);
                        } else {
                            iPitchBend[3] = adjvibdepth;
                        }
                    }
                } else {

                    if ((toneholeRead[3] > senseDistance) && (bitRead(holeCovered, 3) != 1) && 3 != slideHole) {
                        iPitchBend[3] = ((toneholeRead[3] - senseDistance) * vibratoScale[3]) >> 3;
                    }

                    else if ((toneholeRead[3] < senseDistance) || (bitRead(holeCovered, 3) == 1)) {
                        iPitchBend[3] = 0; // If the finger is removed or the hole is fully covered, there's no pitchbend contributed by that hole.
                    }

                    //if (iPitchBend[3] > adjvibdepth) {
                       // iPitchBend[3] = adjvibdepth; //cap at 8191 (no pitchbend) if for some reason they add up to more than that
                    //}
                }
            }
            
        }
        
    }


    else if (modeSelector[mode] == kModeGHB || modeSelector[mode] == kModeNorthumbrian) { //this one is designed for closed fingering patterns, so raising a finger sharpens the note.
        for (byte i = 2; i < 4; i++) { //use holes 2 and 3 for vibrato
            if (i != slideHole || (holeCovered & 0b100000000) == 0) {
                static unsigned int testNote; // the hypothetical note that would be played if a finger were lowered all the way
                if (bitRead(holeCovered, i) != 1) { //if the hole is not fully covered
                    if (fingersChanged) { //if the fingering pattern has changed
                        testNote = get_note(bitSet(holeCovered, i)); //check to see what the new note would be
                        fingersChanged = 0;
                    }
                    if (testNote == newNote) { //if the hole is uncovered and covering the hole wouldn't change the current note (or the left thumb hole is uncovered, because that case isn't included in the fingering chart)
                        if (toneholeRead[i] > senseDistance) {
                            iPitchBend[i] = 0 - (((toneholeCovered[i] - 50 - toneholeRead[i]) * vibratoScale[i]) >> 3); //bend up, yielding a negative pitchbend value
                        } else {
                            iPitchBend[i] = 0 - adjvibdepth; //if the hole is totally uncovered, max the pitchbend
                        }
                    }
                } else { //if the hole is covered
                    iPitchBend[i] = 0; //reset the pitchbend to 0
                }
            }
        }
        if ((((iPitchBend[2] + iPitchBend[3]) * -1) > adjvibdepth) && ((slideHole != 2 && slideHole != 3) || (holeCovered & 0b100000000) == 0)) { //cap at vibrato depth if more than one hole is contributing and they add to up to more than the vibrato depth.
            iPitchBend[2] = 0 - adjvibdepth; //assign max vibrato depth to a hole that isn't being used for sliding
            iPitchBend[3] = 0;
        }
    }
    sendPitchbend();
}






//Andrew's version
void handlePitchBend()
{


    if (pitchBendMode == kPitchBendSlideVibrato || pitchBendMode == kPitchBendLegatoSlideVibrato) { //calculate slide if necessary.
        getSlide();
    }


    for (byte i = 0; i < 9; i++) {

        if (bitRead(holeLatched, i) == 1 && toneholeRead[i] < senseDistance) {
            (bitWrite(holeLatched, i, 0)); //we "unlatch" (enable for vibrato) a hole if it was covered when the note was triggered but now the finger has been completely removed.
        }

        if (bitRead(vibratoHoles, i) == 1 && bitRead(holeLatched, i) == 0 && (pitchBendMode == kPitchBendVibrato || i != slideHole)) { //if this is a vibrato hole and we're in a mode that uses vibrato, and the hole is unlatched
            if (toneholeRead[i] > senseDistance) {
                if (bitRead(holeCovered, i) != 1) {
                    iPitchBend[i] = (((toneholeRead[i] - senseDistance) * vibratoScale[i]) >> 3); //bend downward
                    pitchBendOn[i] = 1;
                }
            } else {
                pitchBendOn[i] = 0;
                if (bitRead(holeCovered, i) == 1) {
                    iPitchBend[i] = 0;
                }
            }

            if (pitchBendOn[i] == 1 && (bitRead(holeCovered, i) == 1)) {
                iPitchBend[i] = adjvibdepth; //set vibrato to max downward bend if a hole was being used to bend down and now is covered
            }
        }
    }

    sendPitchbend();
}






//calculate slide pitchBend, to be added with vibrato.
void getSlide()
{
    for (byte i = 0; i < 9; i++) {
        if (toneholeRead[i] > senseDistance && i == slideHole && stepsDown > 0) {
            if (bitRead(holeCovered, i) != 1) {
                iPitchBend[i] = ((toneholeRead[i] - senseDistance) * toneholeScale[i]) >> (4 - stepsDown); //bend down toward the next lowest note in the scale, the amount of bend depending on the number of steps down.
            }
        } else {
            iPitchBend[i] = 0;
        }
    }
}







void sendPitchbend()
{


    pitchBend = 0; //reset the overall pitchbend in preparation for adding up the contributions from all the toneholes.
    for (byte i = 0; i < 9; i++) {
        pitchBend = pitchBend + iPitchBend[i];
    }

    int noteshift = 0;
    if (noteon && pitchBendModeSelector[mode] == kPitchBendLegatoSlideVibrato) {
        noteshift = (notePlaying - shift) - newNote;
        pitchBend += noteshift * pitchBendPerSemi;
    }

    pitchBend = 8192 - pitchBend + expression;
    if (pitchBend < 0) {
        pitchBend = 0;
    } else if (pitchBend > 16383) {
        pitchBend = 16383;
    }

    if (prevPitchBend != pitchBend) {

        if (noteon) {

            sendUSBMIDI(PITCH_BEND, mainMidiChannel, pitchBend & 0x7F, pitchBend >> 7);
            prevPitchBend = pitchBend;
        }
    }
}


void calculateAndSendPitchbend()
{
    if (ED[mode][EXPRESSION_ON] && !switches[mode][BAGLESS]) {
        getExpression(); //calculate pitchbend based on pressure reading
    }

    if (!customEnabled && pitchBendMode != kPitchBendNone) {
        handlePitchBend();
    } else if (customEnabled) {
        handleCustomPitchBend();
    }
}




//send MIDI NoteOn/NoteOff events when necessary
void sendNote()
{
    const int velDelayMs = switches[mode][SEND_AFTERTOUCH] != 0 ? 3 : 16 ; // keep this minimal to avoid latency if also sending aftertouch, but enough to get a good reading, otherwise use longer

    if (     //several conditions to tell if we need to turn on a new note.
        (!noteon
         || ( pitchBendModeSelector[mode] != kPitchBendLegatoSlideVibrato && newNote != (notePlaying - shift))
         || ( pitchBendModeSelector[mode] == kPitchBendLegatoSlideVibrato && abs(newNote - (notePlaying - shift)) > midiBendRange - 1 ) ) && //if there wasn't any note playing or the current note is different than the previous one
        ((newState > 1 && !switches[mode][BAGLESS]) || (switches[mode][BAGLESS] && play)) && //and the state machine has determined that a note should be playing, or we're in bagless mode and the sound is turned on
        !(prevNote == 62 && (newNote + shift) == 86 && !sensorDataReady) && // and if we're currently on a middle D in state 3 (all finger holes covered), we wait until we get a new state reading before switching notes. This it to prevent erroneous octave jumps to a high D.
        !(switches[mode][SEND_VELOCITY] && !noteon && ((millis() - velocityDelayTimer) < velDelayMs)) && // and not waiting for the pressure to rise to calculate note on velocity if we're transitioning from not having any note playing.
        !(modeSelector[mode] == kModeNorthumbrian && newNote == 60) && //and if we're in Northumbrian mode don't play a note if all holes are covered. That simulates the closed pipe.
        !(breathMode != kPressureBell && bellSensor && holeCovered == 0b111111111)) { // don't play a note if the bell sensor and all other holes are covered, and we're not in "bell register" mode. Again, simulating a closed pipe.

        int notewason = noteon;
        int notewasplaying = notePlaying;

        // if this is a fresh/tongued note calculate pressure now to get the freshest initial velocity/pressure
        if (!notewason) {
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
        }

        if (notewason && !switches[mode][LEGATO]) {
            // send prior noteoff now if legato is selected.
            sendUSBMIDI(NOTE_OFF, mainMidiChannel, notePlaying, 64);
            notewason = 0;
        }

        // need to send pressure prior to note, in case we are using it for velocity
        if (ED[mode][SEND_PRESSURE] == 1 || switches[mode][SEND_AFTERTOUCH] != 0 || switches[mode][SEND_VELOCITY] == 1) {
            sendPressure(true);
        }

        // set it now so that send pitchbend will operate correctly
        noteon = 1; //keep track of the fact that there's a note turned on
        notePlaying = newNote + shift;

        // send pitch bend immediately prior to note if necessary
        if (switches[mode][IMMEDIATE_PB]) {
            calculateAndSendPitchbend();
        }


        sendUSBMIDI(NOTE_ON, mainMidiChannel, newNote + shift, velocity); //send the new note

        if (notewason) {
            // turn off the previous note after turning on the new one (if it wasn't already done above)
            // We do it after to signal to synths that the notes are legato (not all synths will respond to this).
            sendUSBMIDI(NOTE_OFF, mainMidiChannel, notewasplaying, 64);
        }

        pitchBendTimer = millis(); //for some reason it sounds best if we don't send pitchbend right away after starting a new note.
        noteOnTimestamp = pitchBendTimer;

        prevNote = newNote;

        if (ED[mode][DRONES_CONTROL_MODE] == 2 && !dronesOn) { //start drones if drones are being controlled with chanter on/off
            startDrones();
        }
    }


    if (noteon) { //several conditions to turn a note off
        if (
            ((newState == 1 && !switches[mode][BAGLESS]) || (switches[mode][BAGLESS] && !play)) ||   //if the state drops to 1 (off) or we're in bagless mode and the sound has been turned off
            (modeSelector[mode] == kModeNorthumbrian && newNote == 60) ||                         //or closed Northumbrian pipe
            (breathMode != kPressureBell && bellSensor && holeCovered == 0b111111111)) { //or completely closed pipe
            sendUSBMIDI(NOTE_OFF, mainMidiChannel, notePlaying, 64); //turn the note off if the breath pressure drops or if we're in uilleann mode, the bell sensor is covered, and all the finger holes are covered.
            noteon = 0; //keep track

            sendPressure(true);

            if (ED[mode][DRONES_CONTROL_MODE] == 2 && dronesOn) { //stop drones if drones are being controlled with chanter on/off
                stopDrones();
            }
        }
    }
}





//Calibrate the sensors and store them in EEPROM
//mode 1 calibrates all sensors, mode 2 calibrates bell sensor only.
void calibrate()
{

    if (!LEDon) {
        digitalWrite2(ledPin, HIGH);
        LEDon = 1;
        calibrationTimer = millis();

        if (calibration == 1) { //calibrate all sensors if we're in calibration "mode" 1
            for (byte i = 1; i < 9; i++) {
                toneholeCovered[i] = 0; //first set the calibration to 0 for all of the sensors so it can only be increassed by calibrating
                toneholeBaseline[i] = 255; //and set baseline high so it can only be reduced
            }
        }
        if (bellSensor) {
            toneholeCovered[0] = 0; //also zero the bell sensor if it's plugged in (doesn't matter which calibration mode for this one).
            toneholeBaseline[0] = 255;
        }
        return; //we return once to make sure we've gotten some new sensor readings.
    }

    if ((calibration == 1 && ((millis() - calibrationTimer) <= 10000)) || (calibration == 2 && ((millis() - calibrationTimer) <= 5000))) { //then set the calibration to the highest reading during the next ten seconds(or five seconds if we're only calibrating the bell sensor).
        if (calibration == 1) {
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
        loadPrefs(); //do this so pitchbend scaling will be recalculated.
    }
}






//save sensor calibration (EEPROM bytes up to 34 are used (plus byte 37 to indicate a saved calibration)
void saveCalibration()
{

    for (byte i = 0; i < 9; i++) {
        EEPROM.update((i + 9) * 2, highByte(toneholeCovered[i]));
        EEPROM.update(((i + 9) * 2) + 1, lowByte(toneholeCovered[i]));
        EEPROM.update((721 + i), lowByte(toneholeBaseline[i])); //the baseline readings can be stored in a single byte because they should be close to zero.
    }
    calibration = 0;
    EEPROM.update(37, 3); //we write a 3 to address 37 to indicate that we have stored a set of calibrations.
    digitalWrite2(ledPin, LOW);
    LEDon = 0;

}





//Load the stored sensor calibrations from EEPROM
void loadCalibration()
{
    for (byte i = 0; i < 9; i++) {
        byte high = EEPROM.read((i + 9) * 2);
        byte low = EEPROM.read(((i + 9) * 2) + 1);
        toneholeCovered[i] = word(high, low);
        toneholeBaseline[i] = EEPROM.read(721 + i);
    }
}





//send MIDI messages
void sendUSBMIDI(uint8_t m, uint8_t c, uint8_t d1, uint8_t d2)    // send a 3-byte MIDI event over USB
{
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






void sendUSBMIDI(uint8_t m, uint8_t c, uint8_t d)    // send a 2-byte MIDI event over USB
{
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
void receiveMIDI()
{

    midiEventPacket_t rx;
    do {
        noInterrupts();
        rx = MidiUSB.read();   //check for MIDI input
        interrupts();
        if (rx.header != 0) {

            //Serial.println(rx.byte2);
            //Serial.println(rx.byte3);
            //Serial.println("");

            if (rx.byte2 < 119) { //Chrome sends CC 121 and 123 on all channels when it connects, so ignore these.

                if ((rx.byte1 & 0x0f) == 6) { //if we're on channel 7, we may be receiving messages from the configuration tool.
                    blinkNumber = 1; //blink once, indicating a received message. Some commands below will change this to three (or zero) blinks.
                    if (rx.byte2 == 102) {  //many settings are controlled by a value in CC 102 (always channel 7).
                        if (rx.byte3 > 0 && rx.byte3 <= 18) { //handle sensor calibration commands from the configuration tool.
                            if ((rx.byte3 & 1) == 0) {
                                toneholeCovered[(rx.byte3 >> 1) - 1] -= 5;
                                if ((toneholeCovered[(rx.byte3 >> 1) - 1] - 54) < 5) { //if the tonehole calibration gets set too low so that it would never register as being uncovered, send a message to the configuration tool.
                                    sendUSBMIDI(CC, 7, 102, (20 + ((rx.byte3 >> 1) - 1)));
                                }
                            } else {
                                toneholeCovered[((rx.byte3 + 1) >> 1) - 1] += 5;
                            }
                        }

                        if (rx.byte3 == 19) { //save calibration if directed.
                            saveCalibration();
                            blinkNumber = 3;
                        }

                        else if (rx.byte3 == 127) { //begin auto-calibration if directed.
                            blinkNumber = 0;
                            calibration = 1;
                        }

                        else if (rx.byte3 == 126) { //when communication is established, send all current settings to tool.
                            communicationMode = 1;
                            blinkNumber = 0;
                            sendSettings();
                        }


                        else if (rx.byte3 == 122) { // dump EEPROM
                            for (int i = 0 ; i < EEPROM.length() ; i++) {
                                debug_log(EEPROM.read(i));
                                delay(3);
                                blinkNumber = 3;
                            }
                        }


                        for (byte i = 0; i < 3; i++) { // update the three selected fingering patterns if prompted by the tool.
                            if (rx.byte3 == 30 + i) {
                                fingeringReceiveMode = i;
                            }
                        }

                        if (rx.byte3 > 32 && rx.byte3 < 60) {
                            modeSelector[fingeringReceiveMode] = rx.byte3 - 33;
                            loadPrefs();
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

                        for (byte i = 0; i < 4; i++) { //update current pitchbend mode if directed.
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
                                if (rx.byte3 == 119) { //this is a special value for autocalibration because I ran out of values in teh range 0-12 below.
                                    buttonPrefs[mode][i][0] = 19;
                                    blinkNumber = 0;
                                }
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
                                } else if (rx.byte3 == 118) {
                                    momentary[mode][i] = 1;
                                    noteOnOffToggle[i] = 0;
                                }
                            }
                        }

                        if (rx.byte3 == 85) { //set current Instrument as default and save default to settings.
                            defaultMode = mode;
                            EEPROM.update(48, defaultMode);
                        }


                        if (rx.byte3 == 123) { //save settings as the defaults for the current instrument
                            saveSettings(mode);
                            blinkNumber = 3;
                        }


                        else if (rx.byte3 == 124) { //Save settings as the defaults for all instruments
                            for (byte k = 0; k < 3; k++) {
                                saveSettings(k);
                            }
                            loadFingering();
                            loadSettingsForAllModes();
                            loadPrefs();
                            blinkNumber = 3;

                        }

                        else if (rx.byte3 == 125) { //restore all factory settings
                            EEPROM.update(44, 255); //indicates that settings should be resaved at next startup
                            wdt_enable(WDTO_30MS);//restart the device in order to trigger resaving default settings
                            while (true) {}
                        }
                    }


                    else if (rx.byte2 == 103) {
                        senseDistanceSelector[mode] = rx.byte3;
                        loadPrefs();
                    }

                    else if (rx.byte2 == 117) {
                        unsigned long v = rx.byte3 * 8191UL / 100;
                        vibratoDepthSelector[mode] = v; //scale vibrato depth in cents up to pitchbend range of 0-8191
                        loadPrefs();
                    }


                    for (byte i = 0; i < 3; i++) { //update noteshift
                        if (rx.byte2 == 111 + i) {
                            if (rx.byte3 < 50) {
                                noteShiftSelector[i] = rx.byte3;
                            } else {
                                noteShiftSelector[i] = - 127 + rx.byte3;
                            }
                            loadPrefs();
                        }
                    }


                    if (rx.byte2 == 104) { //update receive mode, used for advanced pressure range sliders, switches, and expression and drones panel settings (this indicates the variable for which the next received byte on CC 105 will be).
                        pressureReceiveMode = rx.byte3 - 1;
                    }

                    else if (rx.byte2 == 105) {
                        if (pressureReceiveMode < 12) {
                            pressureSelector[mode][pressureReceiveMode] = rx.byte3;  //advanced pressure values
                            loadPrefs();
                        }

                        else if (pressureReceiveMode < 33) {
                            ED[mode][pressureReceiveMode - 12] = rx.byte3; //expression and drones settings
                            loadPrefs();
                        }

                        else if (pressureReceiveMode == 33) {
                            LSBlearnedPressure = rx.byte3;

                        }

                        else if (pressureReceiveMode == 34) {
                            learnedPressureSelector[mode] = (rx.byte3 << 7) | LSBlearnedPressure;
                            loadPrefs();
                        }


                        else if (pressureReceiveMode < 53) {
                            switches[mode][pressureReceiveMode - 39] = rx.byte3; //switches in the slide/vibrato and register control panels
                            loadPrefs();
                        }

                        else if (pressureReceiveMode == 60) {
                            midiBendRangeSelector[mode] = rx.byte3;
                            loadPrefs();
                        }

                        else if (pressureReceiveMode == 61) {
                            midiChannelSelector[mode] = rx.byte3;
                            loadPrefs();
                        }

                        else if (pressureReceiveMode < 98) {
                            ED[mode][pressureReceiveMode - 48] = rx.byte3; //more expression and drones settings
                            loadPrefs();

                        }

                    }



                    if (rx.byte2 == 106 && rx.byte3 > 15) {


                        if (rx.byte3 > 19 && rx.byte3 < 29) { //update enabled vibrato holes for "universal" vibrato
                            bitSet(vibratoHolesSelector[mode], rx.byte3 - 20);
                            loadPrefs();
                        }

                        else if (rx.byte3 > 29 && rx.byte3 < 39) {
                            bitClear(vibratoHolesSelector[mode], rx.byte3 - 30);
                            loadPrefs();
                        }

                        else if (rx.byte3 == 39) {
                            useLearnedPressureSelector[mode] = 0;
                            loadPrefs();
                        }

                        else if (rx.byte3 == 40) {
                            useLearnedPressureSelector[mode] = 1;
                            loadPrefs();
                        }

                        else if (rx.byte3 == 41) {
                            learnedPressureSelector[mode] = sensorValue;
                            sendUSBMIDI(CC, 7, 104, 34); //indicate that LSB of learned pressure is about to be sent
                            sendUSBMIDI(CC, 7, 105, learnedPressureSelector[mode] & 0x7F); //send LSB of learned pressure
                            sendUSBMIDI(CC, 7, 104, 35); //indicate that MSB of learned pressure is about to be sent
                            sendUSBMIDI(CC, 7, 105, learnedPressureSelector[mode] >> 7); //send MSB of learned pressure
                            loadPrefs();
                        }

                        else if (rx.byte3 == 42) { //autocalibrate bell sensor only
                            calibration = 2;
                            blinkNumber = 0;
                        }


                        else if (rx.byte3 == 43) {
                            int tempPressure = sensorValue;
                            ED[mode][DRONES_PRESSURE_LOW_BYTE] = tempPressure & 0x7F;
                            ED[mode][DRONES_PRESSURE_HIGH_BYTE] = tempPressure >> 7;
                            sendUSBMIDI(CC, 7, 104, 32); //indicate that LSB of learned drones pressure is about to be sent
                            sendUSBMIDI(CC, 7, 105, ED[mode][DRONES_PRESSURE_LOW_BYTE]); //send LSB of learned drones pressure
                            sendUSBMIDI(CC, 7, 104, 33); //indicate that MSB of learned drones pressure is about to be sent
                            sendUSBMIDI(CC, 7, 105, ED[mode][DRONES_PRESSURE_HIGH_BYTE]); //send MSB of learned drones pressure
                        }


                        else if (rx.byte3 == 45) { //save current sensor calibration as factory calibration
                            for (byte i = 0; i < 18; i++) {
                                EEPROM.update(i, EEPROM.read(i + 18));
                            }
                            for (int i = 731; i < 741; i++) { //save baseline calibration as factory baseline
                                EEPROM.update(i, EEPROM.read(i - 10));
                            }
                        }
                    }



                    for (byte i = 0; i < 8; i++) { //update channel, byte 2, byte 3 for MIDI message for button MIDI command for row i
                        if (buttonReceiveMode == i) {
                            if (rx.byte2 == 106 && rx.byte3 < 16) {
                                buttonPrefs[mode][i][2] = rx.byte3;
                            } else if (rx.byte2 == 107) {
                                buttonPrefs[mode][i][3] = rx.byte3;
                            } else if (rx.byte2 == 108) {
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
void saveSettings(byte i)
{

    EEPROM.update(40 + i, modeSelector[mode]);
    EEPROM.update(53 + i, noteShiftSelector[mode]);
    EEPROM.update(50 + i, senseDistanceSelector[mode]);

    for (byte n = 0; n < kSWITCHESnVariables; n++) {
        EEPROM.update((56 + n + (i * kSWITCHESnVariables)), switches[mode][n]);
    }

    EEPROM.update(333 + (i * 2), lowByte(vibratoHolesSelector[mode]));
    EEPROM.update(334 + (i * 2), highByte(vibratoHolesSelector[mode]));
    EEPROM.update(339 + (i * 2), lowByte(vibratoDepthSelector[mode]));
    EEPROM.update(340 + (i * 2), highByte(vibratoDepthSelector[mode]));
    EEPROM.update(345 + i, useLearnedPressureSelector[mode]);

    for (byte j = 0; j < 5; j++) { //save button configuration for current mode
        for (byte k = 0; k < 8; k++) {
            EEPROM.update(100 + (i * 50) + (j * 10) + k, buttonPrefs[mode][k][j]);
        }
    }

    for (byte h = 0; h < 3; h++) {
        EEPROM.update(250 + (i * 3) + h, momentary[mode][h]);
    }

    for (byte q = 0; q < 12; q++) {
        EEPROM.update((260 + q + (i * 20)), pressureSelector[mode][q]);
    }

    EEPROM.update(273 + (i * 2), lowByte(learnedPressureSelector[mode]));
    EEPROM.update(274 + (i * 2), highByte(learnedPressureSelector[mode]));

    EEPROM.update(313 + i, pitchBendModeSelector[mode]);
    EEPROM.update(316 + i, breathModeSelector[mode]);
    EEPROM.update(319 + i, midiBendRangeSelector[mode]);
    EEPROM.update(322 + i, midiChannelSelector[mode]);

    for (byte n = 0; n < kEXPRESSIONnVariables; n++) {
        EEPROM.update((351 + n + (i * kEXPRESSIONnVariables)), ED[mode][n]);
    }


}




//load saved fingering patterns
void loadFingering()
{

    for (byte i = 0; i < 3; i++) {
        modeSelector[i] = EEPROM.read(40 + i);
        noteShiftSelector[i] = (int8_t)EEPROM.read(53 + i);

        if (communicationMode) {
            sendUSBMIDI(CC, 7, 102, 30 + i);  //indicate that we'll be sending the fingering pattern for instrument i
            sendUSBMIDI(CC, 7, 102, 33 + modeSelector[i]);  //send

            if (noteShiftSelector[i] >= 0) {
                sendUSBMIDI(CC, 7, (111 + i), noteShiftSelector[i]);
            } //send noteShift, with a transformation for sending negative values over MIDI.
            else {
                sendUSBMIDI(CC, 7, (111 + i), noteShiftSelector[i] + 127);
            }
        }
    }
}




//load settings for all three instruments from EEPROM
void loadSettingsForAllModes()
{

    defaultMode = EEPROM.read(48); //load default mode

    for (byte i = 0; i < 3; i++) {

        senseDistanceSelector[i] = EEPROM.read(50 + i);

        for (byte n = 0; n < kSWITCHESnVariables; n++) {
            switches[i][n] = EEPROM.read(56 + n + (i * kSWITCHESnVariables));
        }

        vibratoHolesSelector[i] = word(EEPROM.read(334 + (i * 2)), EEPROM.read(333 + (i * 2)));
        vibratoDepthSelector[i] = word(EEPROM.read(340 + (i * 2)), EEPROM.read(339 + (i * 2)));
        useLearnedPressureSelector[i] = EEPROM.read(345 + i);

        for (byte j = 0; j < 5; j++) {
            for (byte k = 0; k < 8; k++) {
                buttonPrefs[i][k][j] = EEPROM.read(100 + (i * 50) + (j * 10) + k);
            }
        }

        for (byte h = 0; h < 3; h++) {
            momentary[i][h] = EEPROM.read(250 + (i * 3) + h);
        }

        for (byte m = 0; m < 12; m++) {
            pressureSelector[i][m] =  EEPROM.read(260 + m + (i * 20));
        }

        learnedPressureSelector[i] = word(EEPROM.read(274 + (i * 2)), EEPROM.read(273 + (i * 2)));


        pitchBendModeSelector[i] =  EEPROM.read(313 + i);
        breathModeSelector[i] = EEPROM.read(316 + i);

        midiBendRangeSelector[i] = EEPROM.read(319 + i);
        midiBendRangeSelector[i] = midiBendRangeSelector[i] > 96 ? 2 : midiBendRangeSelector[i]; // sanity check in case uninitialized

        midiChannelSelector[i] = EEPROM.read(322 + i);
        midiChannelSelector[i] = midiChannelSelector[i] > 16 ? 1 : midiChannelSelector[i]; // sanity check in case uninitialized


        for (byte n = 0; n < kEXPRESSIONnVariables; n++) {
            ED[i][n] = EEPROM.read(351 + n + (i * kEXPRESSIONnVariables));
        }



    }


}




//This is used the first time the software is run, to copy all the default settings to EEPROM, and is also used to restore factory settings.
void saveFactorySettings()
{

    for (byte i = 0; i < 3; i++) { //save all the current settings for all three instruments.
        mode = i;
        saveSettings(i);
    }

    for (byte i = 0; i < 18; i++) { //copy sensor calibration from factory settings location (copy 0-17 to 18-35).
        EEPROM.update((i + 18), EEPROM.read(i));
    }

    for (int i = 721; i < 731; i++) { //copy sensor baseline calibration from factory settings location.
        EEPROM.update((i), EEPROM.read(i + 10));
    }

    EEPROM.update(48, defaultMode); //save default mode

    EEPROM.update(44, 3); //indicates settings have been saved

    blinkNumber = 3;
}






//send all settings for current instrument to the WARBL Configuration Tool.
void sendSettings()
{


    sendUSBMIDI(CC, 7, 110, VERSION); //send software version


    for (byte i = 0; i < 3; i++) {
        sendUSBMIDI(CC, 7, 102, 30 + i);  //indicate that we'll be sending the fingering pattern for instrument i
        sendUSBMIDI(CC, 7, 102, 33 + modeSelector[i]);  //send

        if (noteShiftSelector[i] >= 0) {
            sendUSBMIDI(CC, 7, 111 + i, noteShiftSelector[i]);
        } //send noteShift, with a transformation for sending negative values over MIDI.
        else {
            sendUSBMIDI(CC, 7, 111 + i, noteShiftSelector[i] + 127);
        }
    }

    sendUSBMIDI(CC, 7, 102, 60 + mode); //send current instrument
    sendUSBMIDI(CC, 7, 102, 85 + defaultMode); //send default instrument

    sendUSBMIDI(CC, 7, 103, senseDistance); //send sense distance

    sendUSBMIDI(CC, 7, 117, vibratoDepth * 100UL / 8191); //send vibrato depth, scaled down to cents
    sendUSBMIDI(CC, 7, 102, 70 + pitchBendMode); //send current pitchBend mode
    sendUSBMIDI(CC, 7, 102, 80 + breathMode); //send current breathMode
    sendUSBMIDI(CC, 7, 102, 120 + bellSensor); //send bell sensor state
    sendUSBMIDI(CC, 7, 106, 39 + useLearnedPressure); //send calibration option
    sendUSBMIDI(CC, 7, 104, 34); //indicate that LSB of learned pressure is about to be sent
    sendUSBMIDI(CC, 7, 105, learnedPressure & 0x7F); //send LSB of learned pressure
    sendUSBMIDI(CC, 7, 104, 35); //indicate that MSB of learned pressure is about to be sent
    sendUSBMIDI(CC, 7, 105, learnedPressure >> 7); //send MSB of learned pressure

    sendUSBMIDI(CC, 7, 104, 61); // indicate midi bend range is about to be sent
    sendUSBMIDI(CC, 7, 105, midiBendRange); //midi bend range

    sendUSBMIDI(CC, 7, 104, 62); // indicate midi channel is about to be sent
    sendUSBMIDI(CC, 7, 105, mainMidiChannel); //midi bend range



    for (byte i = 0; i < 9; i++) {
        sendUSBMIDI(CC, 7, 106, 20 + i + (10 * (bitRead(vibratoHolesSelector[mode], i)))); //send enabled vibrato holes
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

    for (byte i = 0; i < kSWITCHESnVariables; i++) { //send settings for switches in the slide/vibrato and register control panels
        sendUSBMIDI(CC, 7, 104, i + 40);
        sendUSBMIDI(CC, 7, 105, switches[mode][i]);
    }

    for (byte i = 0; i < 21; i++) { //send settings for expression and drones control panels
        sendUSBMIDI(CC, 7, 104, i + 13);
        sendUSBMIDI(CC, 7, 105, ED[mode][i]);
    }

    for (byte i = 21; i < 49; i++) { //more settings for expression and drones control panels
        sendUSBMIDI(CC, 7, 104, i + 49);
        sendUSBMIDI(CC, 7, 105, ED[mode][i]);
    }

    for (byte i = 0; i < 3; i++) {
        sendUSBMIDI(CC, 7, 102, 90 + i); //indicate that we'll be sending data for momentary
        sendUSBMIDI(CC, 7, 102, 117 + momentary[mode][i]);
    }

    for (byte i = 0; i < 12; i++) {
        sendUSBMIDI(CC, 7, 104, i + 1); //indicate which pressure variable we'll be sending data for
        sendUSBMIDI(CC, 7, 105, pressureSelector[mode][i]); //send the data
    }
}





void blink()   //blink LED given number of times
{

    if ((millis() - ledTimer) >= 200) {
        ledTimer = millis();

        if (LEDon) {
            digitalWrite2(ledPin, LOW);
            blinkNumber--;
            LEDon = 0;
            return;
        }

        else {
            digitalWrite2(ledPin, HIGH);
            LEDon = 1;
        }
    }
}




//interpret button presses. If the button is being used for momentary MIDI messages we ignore other actions with that button (except "secret" actions involving the toneholes).
void handleButtons()
{


    //first, some housekeeping

    if (shiftState == 1 && released[1] == 1) { //if button 1 was only being used along with another button, we clear the just-released flag for button 1 so it doesn't trigger another control change.
        released[1] = 0;
        buttonUsed = 0; //clear the button activity flag, so we won't handle them again until there's been new button activity.
        shiftState = 0;
    }



    //then, a few hard-coded actions that can't be changed by the configuration tool:
    //_______________________________________________________________________________

    if (justPressed[0] && !pressed[2] && !pressed[1]) {
        if (ED[mode][DRONES_CONTROL_MODE] == 1) {
            if (holeCovered >> 1 == 0b00001000) { //turn drones on/off if button 0 is pressed and fingering pattern is 0 0001000.
                justPressed[0] = 0;
                specialPressUsed[0] = 1;
                if (!dronesOn) {
                    startDrones();
                } else {
                    stopDrones();
                }
            }
        }

        if (switches[mode][SECRET]) {
            if (holeCovered >> 1 == 0b00010000) { //change pitchbend mode if button 0 is pressed and fingering pattern is 0 0000010.
                justPressed[0] = 0;
                specialPressUsed[0] = 1;
                changePitchBend();
            }

            else if (holeCovered >> 1 == 0b00000010) { //change instrument if button 0 is pressed and fingering pattern is 0 0000001.
                justPressed[0] = 0;
                specialPressUsed[0] = 1;
                changeInstrument();
            }
        }
    }


    //now the button actions that can be changed with the configuration tool.
    //_______________________________________________________________________________


    for (byte i = 0; i < 3; i++) {


        if (released[i] && (momentary[mode][i] || (pressed[0] + pressed[1] + pressed[2] == 0))) { //do action for a button release ("click") NOTE: button array is zero-indexed, so "button 1" in all documentation is button 0 here (same for others).
            if (!specialPressUsed[i]) {  //we ignore it if the button was just used for a hard-coded command involving a combination of fingerholes.
                performAction(i);
            }
            released[i] = 0;
            specialPressUsed[i] = 0;
        }


        if (longPress[i] && (pressed[0] + pressed[1] + pressed[2] == 1) && !momentary[mode][i]) { //do action for long press, assuming no other button is pressed.
            performAction(5 + i);
            longPressUsed[i] = 1;
            longPress[i] = 0;
            longPressCounter[i] = 0;
        }


        //presses of individual buttons (as opposed to releases) are special cases used only if we're using buttons to send MIDI on/off messages and "momentary" is selected. We'll handle these in a separate function.
        if (justPressed[i]) {
            justPressed[i] = 0;
            handleMomentary(i); //do action for button press.
        }

    }


    if (pressed[1]) {
        if (released[0] && !momentary[mode][0]) { //do action for button 1 held and button 0 released
            released[0] = 0;
            shiftState = 1;
            performAction(3);
        }

        if (released[2] && !momentary[mode][1]) { //do action for button 1 held and button 2 released
            released[2] = 0;
            shiftState = 1;
            performAction(4);
        }

    }


    buttonUsed = 0; // Now that we've caught any important button acticity, clear the flag so we won't enter this function again until there's been new activity.
}



//perform desired action in response to buttons
void performAction(byte action)
{


    switch (buttonPrefs[mode][action][0]) {

        case 0:
            break; //if no action desired for button combination

        case 1: //MIDI command

            if (buttonPrefs[mode][action][1] == 0) {
                if (noteOnOffToggle[action] == 0) {
                    sendUSBMIDI(NOTE_ON, buttonPrefs[mode][action][2], buttonPrefs[mode][action][3], buttonPrefs[mode][action][4]);
                    noteOnOffToggle[action] = 1;
                } else if (noteOnOffToggle[action] == 1) {
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
                if (program < 127) {
                    program++;
                } else {
                    program = 0;
                }
                sendUSBMIDI(PROGRAM_CHANGE, buttonPrefs[mode][action][2], program);
                blinkNumber = 1;
            }

            if (buttonPrefs[mode][action][1] == 4) { //decrease program change
                if (program > 0) {
                    program--;
                } else {
                    program = 127;
                }
                sendUSBMIDI(PROGRAM_CHANGE, buttonPrefs[mode][action][2], program);
                blinkNumber = 1;
            }

            break;

        case 2:  //set vibrato/slide mode
            changePitchBend();
            break;

        case 3:
            changeInstrument();
            break;

        case 4:
            play = !play; //turn sound on/off when in bagless mode
            break;

        case 5:
            if (!momentary[mode][action]) { //shift up unless we're in momentary mode, otherwise shift down
                octaveShiftUp();
                blinkNumber = abs(octaveShift);
            } else {
                octaveShiftDown();
            }
            break;

        case 6:
            if (!momentary[mode][action]) { //shift down unless we're in momentary mode, otherwise shift up
                octaveShiftDown();
                blinkNumber = abs(octaveShift);
            } else {
                octaveShiftUp();
            }
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
            if (communicationMode) {
                sendUSBMIDI(CC, 7, 102, 80 + breathMode); //send current breathMode
            }
            break;


        case 9: //toggle drones
            blinkNumber = 1;
            if (!dronesOn) {
                startDrones();
            } else {
                stopDrones();
            }
            break;


        case 10: //semitone shift up
            if (!momentary[mode][action]) {
                noteShift++; //shift up if we're not in momentary mode
            } else {
                noteShift--; //shift down if we're in momentary mode, because the button is being released and a previous press has shifted up.
            }
            break;


        case 11: //semitone shift down
            if (!momentary[mode][action]) {
                noteShift--; //shift up if we're not in momentary mode
            } else {
                noteShift++; //shift down if we're in momentary mode, because the button is being released and a previous press has shifted up.
            }
            break;


        case 19: //autocalibrate
            calibration = 1;
            break;


        default:
            return;
    }
}





void octaveShiftUp()
{
    if (octaveShift < 3) {
        octaveShiftSelector[mode]++; //adjust octave shift up, within reason
        octaveShift = octaveShiftSelector[mode];
    }
}





void octaveShiftDown()
{
    if (octaveShift > -4) {
        octaveShiftSelector[mode]--;
        octaveShift = octaveShiftSelector[mode];
    }
}




//cycle through pitchbend modes
void changePitchBend()
{
    pitchBendModeSelector[mode] ++;
    if (pitchBendModeSelector[mode] == kPitchBendNModes) {
        pitchBendModeSelector[mode] = kPitchBendSlideVibrato;
    }
    loadPrefs();
    blinkNumber = abs(pitchBendMode) + 1;
    if (communicationMode) {
        sendUSBMIDI(CC, 7, 102, 70 + pitchBendMode); //send current pitchbend mode to configuration tool.
    }
}






//cycle through instruments
void changeInstrument()
{
    mode++; //set instrument
    if (mode == 3) {
        mode = 0;
    }
    play = 0;
    loadPrefs(); //load the correct user settings based on current instrument.
    blinkNumber = abs(mode) + 1;
    if (communicationMode) {
        sendSettings(); //tell communications tool to switch mode and send all settings for current instrument.
    }
}





void handleMomentary(byte button)
{

    if (momentary[mode][button]) {
        if (buttonPrefs[mode][button][0] == 1 && buttonPrefs[mode][button][1] == 0) { //handle momentary press if we're sending a MIDI message
            sendUSBMIDI(NOTE_ON, buttonPrefs[mode][button][2], buttonPrefs[mode][button][3], buttonPrefs[mode][button][4]);
            noteOnOffToggle[button] = 1;
        }

        //handle presses for shifting the octave or semitone up or down
        if (buttonPrefs[mode][button][0] == 5) {
            octaveShiftUp();
        }

        if (buttonPrefs[mode][button][0] == 6) {
            octaveShiftDown();
        }

        if (buttonPrefs[mode][button][0] == 10) {
            noteShift++;
        }

        if (buttonPrefs[mode][button][0] == 11) {
            noteShift--;
        }

    }

}





// find leftmost unset bit, used for finding the uppermost uncovered hole when reading from some fingering charts, and for determining the slidehole.
byte findleftmostunsetbit(uint16_t n)
{

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
void debug_log(int msg)
{
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
    ADCSRA = (1 << ADEN) | ((1 << ADPS2)); // enable ADC Division Factor 16 (36 us)
    ADMUX = (1 << REFS0); //Voltage reference from Avcc (3.3v)
    ADC_read(1); //start an initial conversion (pressure sensor), which will also enable the ADC complete interrupt and trigger subsequent conversions.

}




//start an ADC conversion.
void ADC_read(byte pin)
{

    if (pin >= 18) pin -= 18; // allow for channel or pin numbers
    pin = analogPinToChannel(pin);

    ADCSRB = (ADCSRB & ~(1 << MUX5)) | (((pin >> 3) & 0x01) << MUX5);
    ADMUX = (1 << REFS0) | (pin & 0x07);

    ADCSRA |= bit (ADSC) | bit (ADIE); //start a conversion and enable the ADC complete interrupt

}



void startDrones()
{
    dronesOn = 1;
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







//load the correct user settings for the current instrument. This is used at startup and any time settings are changed.
void loadPrefs()
{

    vibratoHoles = vibratoHolesSelector[mode];
    octaveShift = octaveShiftSelector[mode];
    noteShift = noteShiftSelector[mode];
    pitchBendMode = pitchBendModeSelector[mode];
    useLearnedPressure = useLearnedPressureSelector[mode];
    learnedPressure = learnedPressureSelector[mode];
    senseDistance = senseDistanceSelector[mode];
    vibratoDepth = vibratoDepthSelector[mode];
    breathMode = breathModeSelector[mode];
    midiBendRange = midiBendRangeSelector[mode];
    mainMidiChannel = midiChannelSelector[mode];


    //set these variables depending on whether "vented" is selected
    offset = pressureSelector[mode][(switches[mode][VENTED] * 6) + 0];
    multiplier = pressureSelector[mode][(switches[mode][VENTED] * 6) + 1];
    jumpValue = pressureSelector[mode][(switches[mode][VENTED] * 6) + 2];
    dropValue = pressureSelector[mode][(switches[mode][VENTED] * 6) + 3];
    jumpTime = pressureSelector[mode][(switches[mode][VENTED] * 6) + 4];
    dropTime = pressureSelector[mode][(switches[mode][VENTED] * 6) + 5];


    pitchBend = 8192;
    expression = 0;
    sendUSBMIDI(PITCH_BEND, mainMidiChannel, pitchBend & 0x7F, pitchBend >> 7);

    for (byte i = 0; i < 9; i++) {
        iPitchBend[i] = 0; //turn off pitchbend
        pitchBendOn[i] = 0;
    }

    if (switches[mode][CUSTOM] && pitchBendMode != kPitchBendNone) {
        customEnabled = 1;
    } else (customEnabled = 0); //decide here whether custom vibrato can currently be used, so we don't have to do it every time we need to check pitchBend.

    if (switches[mode][FORCE_MAX_VELOCITY]) {
        velocity = 127; //set velocity
    } else {
        velocity = 64;
    }

    if (!useLearnedPressure) {
        sensorThreshold[0] = (sensorCalibration + soundTriggerOffset); //pressure sensor calibration at startup. We set the on/off threshhold just a bit higher than the reading at startup.
    }

    else {
        sensorThreshold[0] = (learnedPressure + soundTriggerOffset);
    }

    sensorThreshold[1] = sensorThreshold[0] + (offset << 2); //threshold for move to second octave

    for (byte i = 0; i < 9; i++) {
        toneholeScale[i] = ((8 * (16383 / midiBendRange)) / (toneholeCovered[i] - 50 - senseDistance) / 2); // Precalculate scaling factors for pitchbend. This one is for sliding. We multiply by 8 first to reduce rounding errors. We'll divide again later.
        vibratoScale[i] = ((8 * 2 * (vibratoDepth / midiBendRange)) / (toneholeCovered[i] - 50 - senseDistance) / 2); //This one is for vibrato
    }

    adjvibdepth = vibratoDepth / midiBendRange; //precalculations for pitchbend range
    pitchBendPerSemi = 8192 / midiBendRange;

    inputPressureBounds[0][0] = (ED[mode][INPUT_PRESSURE_MIN] * 9); //precalculate input and output pressure ranges for sending pressure as CC
    inputPressureBounds[0][1] = (ED[mode][INPUT_PRESSURE_MAX] * 9);
    inputPressureBounds[1][0] = (ED[mode][VELOCITY_INPUT_PRESSURE_MIN] * 9); //precalculate input and output pressure ranges for sending pressure as velocity
    inputPressureBounds[1][1] = (ED[mode][VELOCITY_INPUT_PRESSURE_MAX] * 9);
    inputPressureBounds[2][0] = (ED[mode][AFTERTOUCH_INPUT_PRESSURE_MIN] * 9); //precalculate input and output pressure ranges for sending pressure as aftertouch
    inputPressureBounds[2][1] = (ED[mode][AFTERTOUCH_INPUT_PRESSURE_MAX] * 9);
    inputPressureBounds[3][0] = (ED[mode][POLY_INPUT_PRESSURE_MIN] * 9); //precalculate input and output pressure ranges for sending pressure as poly
    inputPressureBounds[3][1] = (ED[mode][POLY_INPUT_PRESSURE_MAX] * 9);

    for (byte j = 0; j < 4; j++) { // CC, velocity, aftertouch, poly
        pressureInputScale[j] = (1048576 / (inputPressureBounds[j][1] - inputPressureBounds[j][0])); //precalculate scaling factors for pressure input, which will be used to scale it up to a range of 1024.
        inputPressureBounds[j][2] = (inputPressureBounds[j][0] * pressureInputScale[j]) >> 10;
    }

    outputBounds[0][0] = ED[mode][OUTPUT_PRESSURE_MIN];  //move all these variables to a more logical order so they can be accessed in FOR loops
    outputBounds[0][1] = ED[mode][OUTPUT_PRESSURE_MAX];
    outputBounds[1][0] = ED[mode][VELOCITY_OUTPUT_PRESSURE_MIN];
    outputBounds[1][1] = ED[mode][VELOCITY_OUTPUT_PRESSURE_MAX];
    outputBounds[2][0] = ED[mode][AFTERTOUCH_OUTPUT_PRESSURE_MIN];
    outputBounds[2][1] = ED[mode][AFTERTOUCH_OUTPUT_PRESSURE_MAX];
    outputBounds[3][0] = ED[mode][POLY_OUTPUT_PRESSURE_MIN];
    outputBounds[3][1] = ED[mode][POLY_OUTPUT_PRESSURE_MAX];

    curve[0] = ED[mode][CURVE];
    curve[1] = ED[mode][VELOCITY_CURVE];
    curve[2] = ED[mode][AFTERTOUCH_CURVE];
    curve[3] = ED[mode][POLY_CURVE];

}






//calculate pressure data for CC, velocity, channel pressure, and key pressure if those options are selected
void calculatePressure(byte pressureOption)
{

    long scaledPressure = sensorValue - 100; // input pressure range is 100-1000. Bring this down to 0-900
    scaledPressure = constrain (scaledPressure, inputPressureBounds[pressureOption][0], inputPressureBounds[pressureOption][1]);
    scaledPressure = (((scaledPressure * pressureInputScale[pressureOption]) >> 10) - inputPressureBounds[pressureOption][2]); //scale input pressure up to a range of 0-1024 using the precalculated scale factor

    if (curve[pressureOption] == 1) { //for this curve, cube the input and scale back down.
        scaledPressure = ((scaledPressure * scaledPressure * scaledPressure)  >> 20);
    }

    else if (curve[pressureOption] == 2) { //approximates a log curve with a piecewise linear function.
        switch (scaledPressure >> 6) {
            case 0:
                scaledPressure = scaledPressure << 3;
                break;
            case 1 ... 2:
                scaledPressure = (scaledPressure << 1) + 376;
                break;
            case 3 ... 5:
                scaledPressure = scaledPressure + 566;
                break;
            default:
                scaledPressure = (scaledPressure >> 3) + 901;
                break;
        }
        if (scaledPressure > 1024) {
            scaledPressure = 1024;
        }
    }

    //else curve 0 is linear, so no transformation

    inputPressureBounds[pressureOption][3] = (scaledPressure * (outputBounds[pressureOption][1] - outputBounds[pressureOption][0]) >> 10) + outputBounds[pressureOption][0]; //map to output pressure range


    if (pressureOption == 1) { //set velocity to mapped pressure if desired
        velocity = inputPressureBounds[pressureOption][3];
    }
}





//send pressure data
void sendPressure(bool force)
{

    if (ED[mode][SEND_PRESSURE] == 1 && (inputPressureBounds[0][3] != prevCCPressure || force)) {
        sendUSBMIDI(CC, ED[mode][PRESSURE_CHANNEL], ED[mode][PRESSURE_CC], inputPressureBounds[0][3]); //send MSB of pressure mapped to the output range
        prevCCPressure = inputPressureBounds[0][3];
    }

    if ((switches[mode][SEND_AFTERTOUCH] & 1)) {
        // hack
        int sendm = (!noteon && sensorValue <= 100) ? 0 : inputPressureBounds[2][3];
        if (sendm != prevChanPressure || force) {
            sendUSBMIDI(CHANNEL_PRESSURE, mainMidiChannel, sendm); //send MSB of pressure mapped to the output range
            prevChanPressure = sendm;
        }
    }

    // poly aftertouch uses 2nd lowest bit of ED flag
    if ((switches[mode][SEND_AFTERTOUCH] & 2) && noteon) {
        // hack
        int sendm = (!noteon && sensorValue <= 100) ? 0 : inputPressureBounds[3][3];
        if (sendm != prevPolyPressure || force) {
            sendUSBMIDI(KEY_PRESSURE, mainMidiChannel, notePlaying, sendm); //send MSB of pressure mapped to the output range
            prevPolyPressure = sendm;
        }
    }
}
