//Chart for determining the numbers of steps down from the current note to the next lower note on the scale, for sliding
const PROGMEM byte steps[] = {

  1, //B 59
  1, //C 60
  2, //C# 61
  1, //D 62//Ionian starts here
  1, //D# 63
  2, //E 64
  1, //F 65
  2, //F# 66
  1, //G 67
  1, //G# 68
  2, //A 69//Mixolydian starts here
  1, //Bb 70//natural minor starts here
  2, //B 71
  1, //C 72
  2, //C# 73
  1, //D 74
  2, //D# 75
  2, //E 76
  1, //F 77
  2, //F# 78
  1, //G 79
  1, //G# 80
  2, //A 81
  1, //Bb 82
  2, //B 83
  1, //C 84
 };



//Fingering Charts
//these are stored in PROGMEM to save SRAM space.
//The fingering charts can be either explicit, meaning an exact pattern is required to return a note, or general, where the highest uncovered hole is used to determine the note. This allows use to ignore all the holes below that, so we catch various open fingering patterns without having to list all of them.
//we check the explicit patterns first, and if there's not a match we check the general patterns (if desired).


// MAE FOOFOO 17 Nov 2018 - Added full key map and vibrato flags
//Tin whistle/Irish flute
struct tinwhistle_explicit_entry {
  uint8_t keys;
  uint8_t midi_note;
  uint8_t vibrato; //send pitch bend down
};

const struct tinwhistle_explicit_entry tinwhistle_explicit[TINWHISTLE_EXPLICIT_SIZE] PROGMEM = {  //explicit fingering patterns that aren't covered by the general patterns below. Sensor numbers from left to right are L1,L2,L3,R1,R2,R3,R4. The Lthumb, R4 (pinky) and bell sensors are ignored for now.
   {0b000000, 73, 0},
  //kNoteNameCs;
  {0b000001, 73, 0},
  // kNoteNameCs;
  {0b000010, 73, 0},
  // kNoteNameCs;
  {0b000011, 73, 0},
  // kNoteNameCs;
  {0b000100, 73, 0},
  // kNoteNameCs;
  {0b000101, 73, 0},
  // kNoteNameCs;
  {0b000110, 73, 0},
  // kNoteNameCs;
  {0b000111, 73, 0},
  // kNoteNameCs;
  {0b001000, 73, 0},
  // kNoteNameCs;
  {0b001001, 73, 0},
  // kNoteNameCs;
  {0b001010, 73, 0},
  // kNoteNameCs;
  {0b001011, 73, 0},
  // kNoteNameCs;
  {0b001100, 73, 0},
  // kNoteNameCs;
  {0b001101, 73, 0},
  // kNoteNameCs;
  {0b001110, 73, 0},
  // kNoteNameCs; 
  {0b001111, 73, 0},
  // kNoteNameCs; 
  {0b010000, 73, 0},
  // kNoteNameCs;
  {0b010001, 73, 0},
  // kNoteNameCs; 
  {0b010010, 73, 0},
  // kNoteNameCs;
  {0b010011, 73, 0},
  // kNoteNameCs;
  {0b010100, 73, 0},
  // kNoteNameCs;
  {0b010101, 73, 0},
  // kNoteNameCs; 
  {0b010110, 73, 0},
  // kNoteNameCs; 
  {0b010111, 73, 0},
  // kNoteNameCs;   
  {0b011000, 72, 0b000010},
  // kNoteNameC;  
  {0b011001, 72, 1},
  // kNoteNameC | kVibratoMask;
  {0b011010, 72, 1},
  // kNoteNameC | kVibratoMask; 
  {0b011011, 72, 1},
  // kNoteNameC | kVibratoMask; 
  {0b011100, 72, 0},
  // kNoteNameC; 
  {0b011101, 72, 0},
  // kNoteNameC; 
  {0b011110, 72, 0},
  // kNoteNameC; 
  {0b011111, 74, 0},
  // kNoteNameD_oct;
  {0b100000, 71, 0b000010},  
  // kNoteNameB;
  {0b100001, 71, 1},
  // kNoteNameB | kVibratoMask;
  {0b100010, 71, 1},
  // kNoteNameB | kVibratoMask;
  {0b100011, 71, 1},
  // kNoteNameB | kVibratoMask;
  {0b100100, 71, 0},
  // kNoteNameB;
  {0b100101, 71, 0},
  // kNoteNameB;
  {0b100110, 71, 0},
  // kNoteNameB;
  {0b100111, 71, 0},
  // kNoteNameB;
  {0b101000, 71, 0},
  // kNoteNameB;
  {0b101001, 71, 0},
  // kNoteNameB;
  {0b101010, 71, 0},
  // kNoteNameB;
  {0b101011, 71, 0},
  // kNoteNameB ;
  {0b101100, 71, 0},
  // kNoteNameB;
  {0b101101, 71, 0},
  // kNoteNameB;
  {0b101110, 71, 0},
  // kNoteNameB;
  {0b101111, 71, 0},
  // kNoteNameB;
  {0b110000, 69, 0b000010},
  // kNoteNameA;
  {0b110001, 69, 1},
  // kNoteNameA | kVibratoMask;
  {0b110010, 69, 1},
  // kNoteNameA | kVibratoMask; 
  {0b110011, 69, 1},
  // kNoteNameA | kVibratoMask; 
  {0b110100, 69, 0},
  // kNoteNameA;
  {0b110101, 69, 0},
  // kNoteNameA;
  {0b110110, 69, 0},
  // kNoteNameA;
  {0b110111, 68, 0},
  // kNoteNameG#;
  {0b111000, 67, 0b000010},
  // kNoteNameG;  
  {0b111001, 67, 1},
  // kNoteNameG | kVibratoMask;
  {0b111010, 67, 1},
  // kNoteNameG | kVibratoMask; 
  {0b111011, 67, 1},
  // kNoteNameG | kVibratoMask; 
  {0b111100, 66, 0},
  // kNoteNameFs; 
  {0b111101, 65, 0},
  // kNoteNameF;
  {0b111110, 64, 0},
  // kNoteNameE; 
  {0b111111, 62, 0},
  // kNoteName  D;
 };


// MAE FOOFOO 17 Nov 2018 - Added full key map and vibrato flags
//Uilleann
struct uilleann_explicit_entry {
  uint8_t keys;
  uint8_t midi_note;
  uint8_t vibrato; //send pitch bend down
};

const struct uilleann_explicit_entry uilleann_explicit[UILLEANN_EXPLICIT_SIZE] PROGMEM = {  //explicit fingering patterns that aren't covered by the general patterns below. Sensor numbers from left to right are L1,L2,L3,R1,R2,R3,R4. The bell sensor is ignored for now.
    {0b0000000, 73, 0},
    // kNoteNameHighD;
    {0b0000001, 73, 0b000010},
    // kNoteNameCs;
    {0b0000010, 73, 0},
    // kNoteNameCs;
    {0b0000011, 73, 0},
    // kNoteNameCs;
    {0b0000100, 73, 0},
    // kNoteNameCs;
    {0b0000101, 73, 1},
    // kNoteNameCs;
    {0b0000110, 73, 0},
    // kNoteNameCs;
    {0b0000111, 73, 0},
    // kNoteNameCs;
    {0b0001000, 73, 0},
    // kNoteNameCs;
    {0b0001001, 73, 0},
    // kNoteNameCs;
    {0b0001010, 73, 0},
    // kNoteNameCs;
    {0b0001011, 73, 0},
    // kNoteNameCs;
    {0b0001100, 73, 0},
    // kNoteNameCs;
    {0b0001101, 73, 0},
    // kNoteNameCs;
    {0b0001110, 73, 0},
    // kNoteNameCs;
    {0b0001111, 73, 0},
    // kNoteNameCs;
    {0b0010000, 73, 0},
    // kNoteNameCs;
    {0b0010001, 73, 0},
    // kNoteNameCs;
    {0b0010010, 73, 0},
    // kNoteNameCs;
    {0b0010011, 73, 0},
    // kNoteNameCs;
    {0b0010100, 73, 0},
    // kNoteNameCs;
    {0b0010101, 73, 0},
    // kNoteNameCs;
    {0b0010110, 73, 0},
    // kNoteNameCs;
    {0b0010111, 73, 0},
    // kNoteNameCs;
    {0b0011000, 73, 0},
    // kNoteNameCs;
    {0b0011001, 73, 0},
    // kNoteNameCs;
    {0b0011010, 73, 0},
    // kNoteNameCs;
    {0b0011011, 73, 0},
    // kNoteNameCs;
    {0b0011100, 73, 0},
    // kNoteNameCs;
    {0b0011101, 73, 0},
    // kNoteNameCs;
    {0b0011110, 73, 0},
    // kNoteNameCs;
    {0b0011111, 73, 0},
    // kNoteNameCs;
    {0b0100000, 73, 0},
    // kNoteNameCs;
    {0b0100001, 73, 0},
    // kNoteNameCs;
    {0b0100010, 73, 0},
    // kNoteNameCs;
    {0b0100011, 73, 0},
    // kNoteNameCs;
    {0b0100100, 73, 0},
    // kNoteNameCs;
    {0b0100101, 73, 0},
    // kNoteNameCs;
    {0b0100110, 73, 0},
    // kNoteNameCs;
    {0b0100111, 73, 0},
    // kNoteNameCs;
    {0b0101000, 73, 0},
    // kNoteNameCs;
    {0b0101001, 73, 0},
    // kNoteNameCs;
    {0b0101010, 73, 0},
    // kNoteNameCs;
    {0b0101011, 73, 0},
    // kNoteNameCs;
    {0b0101100, 73, 0},
    // kNoteNameCs;
    {0b0101101, 73, 0},
    // kNoteNameCs;
    {0b0101110, 73, 0},
    // kNoteNameCs;
    {0b0101111, 73, 0},
    // kNoteNameCs;
    {0b0110000, 72, 0b000010},
    // kNoteNameC;
    {0b0110001, 72, 0b000010},
    // kNoteNameC
    {0b0110010, 72, 0},
    // kNoteNameC;
    {0b0110011, 72, 0b000010},
    // kNoteNameC;
    {0b0110100, 72, 1},
    // kNoteNameC | kVibratoMask;
    {0b0110101, 72, 1},
    // kNoteNameC | kVibratoMask;
    {0b0110110, 72, 0},
    // kNoteNameC;
    {0b0110111, 72, 1},
    // kNoteNameC | kVibratoMask;
    {0b0111000, 72, 0},
    // kNoteNameC;
    {0b0111001, 72, 0b000010},
    // kNoteNameC;
    {0b0111010, 72, 0},
    // kNoteNameC;
    {0b0111011, 72, 0},
    // kNoteNameC;
    {0b0111100, 72, 0},
    // kNoteNameC;
    {0b0111101, 72, 1},
    // kNoteNameC  | kVibratoMask;
    {0b0111110, 72, 0},
    // kNoteNameC;
    {0b0111111, 73, 0},
    // kNoteNameCs;
    {0b1000000, 71, 0},
    // kNoteNameB;
    {0b1000001, 71, 0b000010},
    // kNoteNameB;
    {0b1000010, 71, 0},
    // kNoteNameB;
    {0b1000011, 71, 0b000010},
    // kNoteNameB;
    {0b1000100, 71, 0},
    // kNoteNameB;
    {0b1000101, 71, 1},
    // kNoteNameB | kVibratoMask;
    {0b1000110, 71, 0},
    // kNoteNameB;
    {0b1000111, 71, 1},
    // kNoteNameB | kVibratoMask;
    {0b1001000, 70, 0},
    // kNoteNameAs;
    {0b1001001, 71, 0},
    // kNoteNameB;
    {0b1001010, 70, 0},
    // kNoteNameAs;
    {0b1001011, 71, 0},
    // kNoteNameB;
    {0b1001100, 71, 0},
    // kNoteNameB;
    {0b1001101, 71, 0},
    // kNoteNameB;
    {0b1001110, 71, 0},
    // kNoteNameB;
    {0b1001111, 71, 0},
    // kNoteNameB;
    {0b1010000, 71, 0},
    // kNoteNameB;
    {0b1010001, 71, 0},
    // kNoteNameB;
    {0b1010010, 71, 0},
    // kNoteNameB;
    {0b1010011, 71, 0},
    // kNoteNameB;
    {0b1010100, 71, 0},
    // kNoteNameB;
    {0b1010101, 71, 0},
    // kNoteNameB;
    {0b1010110, 71, 0},
    // kNoteNameB;
    {0b1010111, 71, 0},
    // kNoteNameB;
    {0b1011000, 71, 0},
    // kNoteNameB;
    {0b1011001, 71, 0},
    // kNoteNameB;
    {0b1011010, 71, 0},
    // kNoteNameB;
    {0b1011011, 71, 0},
    // kNoteNameB;
    {0b1011100, 71, 0},
    // kNoteNameB;
    {0b1011101, 71, 0},
    // kNoteNameB;
    {0b1011110, 71, 0},
    // kNoteNameB;
    {0b1011111, 71, 0},
    // kNoteNameB;
    {0b1100000, 69, 0},
    // kNoteNameA;
    {0b1100001, 69, 0b000010},
    // kNoteNameA;
    {0b1100010, 69, 0},
    // kNoteNameA;
    {0b1100011, 69, 0b000010},
    // kNoteNameA;
    {0b1100100, 69, 0},
    // kNoteNameA;
    {0b1100101, 69, 1},
    // kNoteNameA | kVibratoMask;
    {0b1100110, 69, 0},
    // kNoteNameA;
    {0b1100111, 69, 1},
    // kNoteNameA | kVibratoMask;
    {0b1101000, 68, 0},
    // kNoteNameGs;
    {0b1101001, 69, 0},
    // kNoteNameA;
    {0b1101010, 68, 0},
    // kNoteNameGs;
    {0b1101011, 69, 0},
    // kNoteNameA;
    {0b1101100, 69, 0},
    // kNoteNameA;
    {0b1101101, 69, 0},
    // kNoteNameA;
    {0b1101110, 69, 0},
    // kNoteNameA;
    {0b1101111, 69, 0},
    // kNoteNameA;
    {0b1110000, 67, 0},
    // kNoteNameG;
    {0b1110001, 67, 0b000010},
    // kNoteNameG;
    {0b1110010, 67, 0},
    // kNoteNameG;
    {0b1110011, 67, 0b000010},
    // kNoteNameG; 
    {0b1110100, 67, 0},
    // kNoteNameG; 
    {0b1110101, 67, 1},
    // kNoteNameG | kVibratoMask 
    {0b1110110, 67, 0},
    // kNoteNameG; 
    {0b1110111, 67, 1},
    // kNoteNameG | kVibratoMask;
    {0b1111000, 66, 0},
    // kNoteNameFs; 
    {0b1111001, 66, 0},
    // kNoteNameFs;
    {0b1111010, 65, 0},
    // kNoteNameF; 
    {0b1111011, 66, 0},
    // kNoteNameFs; 
    {0b1111100, 64, 0},
    // kNoteNameE; 
    {0b1111101, 64, 0},
    // kNoteNameE; 
    {0b1111110, 63, 0},
    // kNoteNameDs; 
    {0b1111111, 62, 0}
    // kNoteNameD; 
};


//GHB/Scottish smallpipes
struct GHB_explicit_entry {
  uint8_t keys;
  uint8_t midi_note;
};


const struct GHB_explicit_entry GHB_explicit[GHB_EXPLICIT_SIZE] PROGMEM = {  //explicit fingering patterns. Sensor numbers from left to right are L1,L2,L3,R1,R2,R3,R4. The bell sensor is ignored.

{0b0000000, 72}, //G//(as written, actual note sounded is Bb, like on real GHB) Noteshift variable is used to transpose from key of D (all fingerings are based on D) to Bb. Uses equal temperament, but temperament can be set in app (e.g. Universal Piper).
{0b0000001, 72}, //G
{0b0000010, 72}, //G
{0b0000011, 72}, //G
{0b0000100, 72}, //G
{0b0000101, 72}, //G
{0b0000110, 72}, //G
{0b0000111, 72}, //G
{0b0001000, 72}, //G
{0b0001001, 72}, //G
{0b0001010, 72}, //G
{0b0001011, 72}, //G
{0b0001100, 72}, //G
{0b0001101, 72}, //G
{0b0001110, 72}, //G
{0b0001111, 72}, //G
{0b0010000, 72}, //G
{0b0010001, 72}, //G
{0b0010010, 72}, //G
{0b0010011, 72}, //G
{0b0010100, 72}, //G
{0b0010101, 72}, //G
{0b0010110, 72}, //G
{0b0010111, 72}, //G
{0b0011000, 72}, //G
{0b0011001, 72}, //G
{0b0011010, 72}, //G
{0b0011011, 72}, //G
{0b0011100, 72}, //G
{0b0011101, 72}, //G
{0b0011110, 72}, //G
{0b0011111, 72}, //G
{0b0100000, 72}, //G
{0b0100001, 72}, //G
{0b0100010, 72}, //G
{0b0100011, 72}, //G
{0b0100100, 72}, //G
{0b0100101, 72}, //G
{0b0100110, 72}, //G
{0b0100111, 72}, //G
{0b0101000, 72}, //G
{0b0101001, 72}, //G
{0b0101010, 72}, //G
{0b0101011, 72}, //G
{0b0101100, 72}, //G
{0b0101101, 72}, //G
{0b0101110, 72}, //G
{0b0101111, 72}, //G
{0b0110000, 72}, //G
{0b0110001, 72}, //G
{0b0110010, 72}, //G
{0b0110011, 72}, //G
{0b0110100, 72}, //G
{0b0110101, 72}, //G
{0b0110110, 72}, //G
{0b0110111, 72}, //G
{0b0111000, 72}, //G
{0b0111001, 72}, //G
{0b0111010, 72}, //G
{0b0111011, 72}, //G
{0b0111100, 72}, //G
{0b0111101, 72}, //G
{0b0111110, 72}, //G
{0b0111111, 73}, //G#
{0b1000000, 71}, //F#
{0b1000001, 71}, //F#
{0b1000010, 71}, //F#
{0b1000011, 71}, //F#
{0b1000100, 71}, //F#
{0b1000101, 71}, //F#
{0b1000110, 71}, //F#
{0b1000111, 71}, //F#
{0b1001000, 71}, //F#
{0b1001001, 71}, //F#
{0b1001010, 71}, //F#
{0b1001011, 71}, //F#
{0b1001100, 71}, //F#
{0b1001101, 71}, //F#
{0b1001110, 71}, //F#
{0b1001111, 71}, //F#
{0b1010000, 71}, //F#
{0b1010001, 71}, //F#
{0b1010010, 71}, //F#
{0b1010011, 71}, //F#
{0b1010100, 71}, //F#
{0b1010101, 71}, //F#
{0b1010110, 71}, //F#
{0b1010111, 71}, //F#
{0b1011000, 71}, //F#
{0b1011001, 71}, //F#
{0b1011010, 70}, //F
{0b1011011, 70}, //F#
{0b1011100, 71}, //F#
{0b1011101, 71}, //F#
{0b1011110, 71}, //F#
{0b1011111, 71}, //F#
{0b1100000, 69}, //E
{0b1100001, 69}, //E
{0b1100010, 69}, //E
{0b1100011, 69}, //E
{0b1100100, 69}, //E
{0b1100101, 69}, //E
{0b1100110, 69}, //E
{0b1100111, 69}, //E
{0b1101000, 69}, //E
{0b1101001, 69}, //E
{0b1101010, 69}, //E
{0b1101011, 69}, //E
{0b1101100, 69}, //E
{0b1101101, 69}, //E
{0b1101110, 69}, //E
{0b1101111, 69}, //E
{0b1110000, 67}, //D
{0b1110001, 67}, //D
{0b1110010, 67}, //D
{0b1110011, 67}, //D
{0b1110100, 67}, //D
{0b1110101, 67}, //D
{0b1110110, 67}, //D
{0b1110111, 67}, //D
{0b1111000, 66}, //C#
{0b1111001, 66}, //C#
{0b1111010, 65}, //C
{0b1111011, 65}, //C
{0b1111100, 64}, //B
{0b1111101, 63}, //A#
{0b1111110, 62}, //A
{0b1111111, 60}, //G
};



//Northumbrian
struct northumbrian_explicit_entry {
  uint8_t keys;
  uint8_t midi_note;
  uint8_t expressive_hole; //this is the hole that can be partially covered to bend the given note down to the next lowest note on the scale.
  uint8_t distance; //the number of half steps (semitones) down to the next lowest note on the scale, so we know how far down to bend the current note to reach it.
};

const struct northumbrian_explicit_entry northumbrian_explicit[NORTHUMBRIAN_EXPLICIT_SIZE] PROGMEM = {  //explicit fingering patterns. Sensor numbers from left to right are L1,L2,L3,R1,R2,R3,R4. The bell sensor is ignored.

  //{0b01111111, 74, 9, 2} // high G

};

struct northumbrian_general_entry {
  uint8_t keys;
  uint8_t midi_note;
  uint8_t expressive_hole; //this is the hole that can be partially covered to bend the given note down to the next lowest note on the scale.
  uint8_t distance; //the number of half steps (semitones) down to the next lowest note on the scale, so we know how far down to bend the current note to reach it.
};

const struct northumbrian_general_entry northumbrian_general[NORTHUMBRIAN_GENERAL_SIZE]PROGMEM  = {  //general fingering pattern for each MIDI note. In this case, the index (position from right) of the leftmost uncovered hole is used to determine the MIDI note. This way all other holes below that one are ignored.
  {127, 60, 1, 1},//This will be interpreted as slient (closed pipe). The findleftmostunsetbit function returns 127 if all holes are covered.
  {0, 62, 2, 1},//G --- noteshift variable is set by default to lower the key to F, which is most common for Northumbrian.
  {1, 64, 3, 2},//A
  {2, 66, 4, 2},//B
  {3, 67, 5, 1},//C
  {4, 69, 6, 2},//D
  {5, 71, 7, 2},//E
  {6, 73, 8, 2},//F#
  {7, 74, 9, 2},//high G

};



//Gaita
struct gaita_explicit_entry {
  uint8_t keys;
  uint8_t midi_note;
};


const struct gaita_explicit_entry gaita_explicit[GAITA_EXPLICIT_SIZE]PROGMEM  = {  //general fingering pattern for each MIDI note. In this case, the index (position from right) of the leftmost uncovered hole is used to determine the MIDI note. This way all other holes below that one are ignored.
{0b00000000, 75}, //C  MIDI notes in the GAITA chart are shifted 1 note upward to bring the base note to a similar range as other instruments, because this base note is used in octave-shifting calculations. The notes are shifted back down again later to bring them in tune.
{0b00000001, 75}, //
{0b00000010, 75}, //C
{0b00000011, 75}, //
{0b00000100, 75}, //
{0b00000101, 75}, //
{0b00000110, 75}, //
{0b00000111, 75}, //
{0b00001000, 75}, //
{0b00001001, 75}, //C
{0b00001010, 75}, //
{0b00001011, 75}, //
{0b00001100, 75}, //
{0b00001101, 75}, //
{0b00001110, 75}, //
{0b00001111, 74}, //C?
{0b00010000, 75}, //
{0b00010001, 75}, //
{0b00010010, 75}, //
{0b00010011, 75}, //
{0b00010100, 75}, //
{0b00010101, 75}, //
{0b00010110, 75}, //
{0b00010111, 75}, //
{0b00011000, 75}, //
{0b00011001, 75}, //
{0b00011010, 75}, //
{0b00011011, 75}, //
{0b00011100, 75}, //
{0b00011101, 75}, //
{0b00011110, 75}, //
{0b00011111, 75}, //
{0b00100000, 75}, //
{0b00100001, 75}, //
{0b00100010, 75}, //
{0b00100011, 75}, //
{0b00100100, 75}, //
{0b00100101, 75}, //
{0b00100110, 75}, //
{0b00100111, 75}, //
{0b00101000, 75}, //
{0b00101001, 75}, //
{0b00101010, 75}, //
{0b00101011, 73}, //A#
{0b00101100, 73}, //
{0b00101101, 73}, //
{0b00101110, 73}, //
{0b00101111, 73}, //
{0b00110000, 80}, //F
{0b00110001, 74}, //B
{0b00110010, 74}, //
{0b00110011, 74}, //
{0b00110100, 74}, //
{0b00110101, 74}, //
{0b00110110, 74}, //
{0b00110111, 74}, //
{0b00111000, 79}, //E
{0b00111001, 74}, //
{0b00111010, 78}, //D#
{0b00111011, 74}, //
{0b00111100, 77}, //D
{0b00111101, 74}, //
{0b00111110, 75}, //
{0b00111111, 74}, //B
{0b01000000, 84}, //A
{0b01000001, 84}, //A
{0b01000010, 84}, //A
{0b01000011, 73}, //A#
{0b01000100, 73}, //
{0b01000101, 73}, //
{0b01000110, 84}, //A
{0b01000111, 84}, //
{0b01001000, 84}, //
{0b01001001, 73}, //A#
{0b01001010, 73}, //
{0b01001011, 73}, //
{0b01001100, 73}, //
{0b01001101, 73}, //
{0b01001110, 73}, //
{0b01001111, 73}, //
{0b01010000, 83}, //G#
{0b01010001, 83}, //
{0b01010010, 83}, //
{0b01010011, 83}, //
{0b01010100, 83}, //G#
{0b01010101, 83}, //
{0b01010110, 83}, //
{0b01010111, 75}, //C
{0b01011000, 75}, //
{0b01011001, 75}, //
{0b01011010, 75}, //
{0b01011011, 75}, //
{0b01011100, 75}, //
{0b01011101, 75}, //
{0b01011110, 83}, //G#
{0b01011111, 75}, //
{0b01100000, 82}, //G
{0b01100001, 82}, //G
{0b01100010, 82}, //G
{0b01100011, 82}, //G
{0b01100100, 82}, //
{0b01100101, 82}, //
{0b01100110, 82}, //
{0b01100111, 82}, //
{0b01101000, 82}, //
{0b01101001, 82}, //G
{0b01101010, 82}, //
{0b01101011, 82}, //
{0b01101100, 81}, //F#
{0b01101101, 81}, //
{0b01101110, 81}, //
{0b01101111, 82}, //G
{0b01110000, 80}, //F
{0b01110001, 80}, //F
{0b01110010, 80}, //F
{0b01110011, 80}, //
{0b01110100, 80}, //
{0b01110101, 80}, //
{0b01110110, 80}, //F
{0b01110111, 80}, //F
{0b01111000, 79}, //E
{0b01111001, 79}, //E
{0b01111010, 78}, //D#
{0b01111011, 78}, //D#
{0b01111100, 77}, //D
{0b01111101, 76}, //C#
{0b01111110, 75}, //C
{0b01111111, 74}, //B
{0b10000000, 74}, //B
{0b10000001, 74}, //
{0b10000010, 74}, //
{0b10000011, 74}, //B
{0b10000100, 74}, //
{0b10000101, 74}, //
{0b10000110, 74}, //B
{0b10000111, 74}, //
{0b10001000, 74}, //
{0b10001001, 74}, //B
{0b10001010, 74}, //B
{0b10001011, 74}, //
{0b10001100, 74}, //
{0b10001101, 74}, //
{0b10001110, 74}, //B
{0b10001111, 74}, //B
{0b10010000, 74}, //
{0b10010001, 74}, //
{0b10010010, 74}, //
{0b10010011, 74}, //
{0b10010100, 74}, //
{0b10010101, 74}, //
{0b10010110, 74}, //
{0b10010111, 74}, //
{0b10011000, 74}, //
{0b10011001, 74}, //
{0b10011010, 74}, //
{0b10011011, 74}, //
{0b10011100, 74}, //
{0b10011101, 74}, //
{0b10011110, 74}, //
{0b10011111, 74}, //
{0b10100000, 73}, //A#
{0b10100001, 73}, //A#
{0b10100010, 73}, //A#
{0b10100011, 73}, //A#
{0b10100100, 73}, //A#
{0b10100101, 73}, //A#
{0b10100110, 73}, //A#
{0b10100111, 73}, //A#
{0b10101000, 73}, //A#
{0b10101001, 73}, //A#
{0b10101010, 73}, //A#
{0b10101011, 73}, //
{0b10101100, 73}, //A#
{0b10101101, 73}, //A#
{0b10101110, 73}, //A#
{0b10101111, 73}, //A#
{0b10110000, 75}, //C
{0b10110001, 75}, //C
{0b10110010, 75}, //C
{0b10110011, 75}, //C
{0b10110100, 75}, //
{0b10110101, 75}, //C
{0b10110110, 75}, //C
{0b10110111, 73}, //A#
{0b10111000, 75}, //
{0b10111001, 75}, //C
{0b10111010, 75}, //C
{0b10111011, 75}, //C
{0b10111100, 75}, //C
{0b10111101, 75}, //
{0b10111110, 75}, //C
{0b10111111, 75}, //C
{0b11000000, 72}, //A
{0b11000001, 72}, //A
{0b11000010, 72}, //A
{0b11000011, 72}, //A
{0b11000100, 72}, //
{0b11000101, 72}, //
{0b11000110, 72}, //A
{0b11000111, 72}, //
{0b11001000, 72}, //
{0b11001001, 72}, //A
{0b11001010, 72}, //A
{0b11001011, 72}, //
{0b11001100, 72}, //
{0b11001101, 72}, //
{0b11001110, 72}, //A
{0b11001111, 72}, //A
{0b11010000, 71}, //G#
{0b11010001, 74}, //B
{0b11010010, 71}, //G#
{0b11010011, 71}, //G#
{0b11010100, 71}, //
{0b11010101, 71}, //
{0b11010110, 71}, //G#
{0b11010111, 71}, //
{0b11011000, 71}, //
{0b11011001, 71}, //G#
{0b11011010, 71}, //G#
{0b11011011, 71}, //
{0b11011100, 71}, //
{0b11011101, 71}, //
{0b11011110, 71}, //G#
{0b11011111, 71}, //G#
{0b11100000, 70}, //G
{0b11100001, 70}, //G
{0b11100010, 70}, //G
{0b11100011, 70}, //
{0b11100100, 70}, //
{0b11100101, 70}, //G
{0b11100110, 70}, //G
{0b11100111, 70}, //G
{0b11101000, 69}, //F#
{0b11101001, 70}, //G
{0b11101010, 70}, //G
{0b11101011, 70}, //
{0b11101100, 69}, //F#
{0b11101101, 69}, //
{0b11101110, 70}, //G
{0b11101111, 70}, //
{0b11110000, 68}, //F
{0b11110001, 68}, //F
{0b11110010, 68}, //F
{0b11110011, 68}, //F
{0b11110100, 68}, //
{0b11110101, 68}, //F
{0b11110110, 68}, //F
{0b11110111, 68}, //F
{0b11111000, 67}, //E
{0b11111001, 67}, //E
{0b11111010, 66}, //D#
{0b11111011, 66}, //D#
{0b11111100, 65}, //D
{0b11111101, 64}, //C#
{0b11111110, 63}, //C
{0b11111111, 62}, //B










};
