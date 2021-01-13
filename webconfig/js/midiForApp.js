var midiNotes = [];
var currentVersion = 18
var midiAccess = null; // the MIDIAccess object.
var on,
	off;
// For pressure graphing
var gPressureGraphEnabled = false; // When true, proxy pressure messages to graphing code
var gCurrentPressure = 0.0;
var gGraphInterval = null;
var gNPressureReadings = 100;
var gMaxPressure = 25;
var gPressureReadings = [];

var deviceName;
var volume = 0;
var currentNote = 62;
var sensorValue = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
var buttonRowWrite; //used to indicate which button command row we'll be writing to below.
var jumpFactorWrite; //to indicate which pressure variably is going to be sent or received
var fingeringWrite; //indicates the instrument for which a fingering pattern is being sent.
var version = "Unknown";
//var version = 10;
var instrument = 0; //currently selected instrument tab
var ping = 0;
var lsb = 0; //used for reassembling pressure bytes from WARBL
var defaultInstr = 0; //default instrument


var modals = new Array();
for (var i = 1; i <= 14; i++) {
	modals[i] = document.getElementById("open-modal" + i);
}


// When the user clicks anywhere outside of the modal, close it
window.onclick = function(event) {
	for (var i = 1; i <= 14; i++) {
		if (event.target == modals[i]) {
			modalclose(i);
		}
	}
	event.stopPropagation();
}



$(document).ready(function() {
	updateCells(); // set selects and radio buttons to initial values and enabled/disabled states
	updateSliders();


	$(".volume").click(function() {
		toggleOn();
		$(this).find('i').toggleClass('fa-volume-up fa-volume-off')
	});
});


window.addEventListener('load', function() {
	// patch up prefixes


	if (navigator.requestMIDIAccess)
		navigator.requestMIDIAccess({
			sysex: true
		}).then(onMIDIInit, onMIDIReject);
	else
		alert("Your browser does not support MIDI. Please use Chrome or Opera, or the free WARBL iOS app.")

});

function connect() {
	ping = 0;
	document.getElementById("status").style.color = "#F78339";
	document.getElementById("version").style.color = "#F78339";
	document.getElementById("status").innerHTML = "WARBL not detected.";
	document.getElementById("version").innerHTML = "Unknown";
	document.getElementById("current").style.visibility = "hidden";
	document.getElementById("status").style.visibility = "visible";
	if (navigator.requestMIDIAccess)
		navigator.requestMIDIAccess({
			sysex: true
		}).then(onMIDIInit, onMIDIReject);
	else
		alert("Your browser does not support MIDI. Please use Chrome or Opera, or the free WARBL iOS app.")
}

function onMIDIInit(midi) {
	midiAccess = midi;


	var haveAtLeastOneDevice = false;
	var inputs = midiAccess.inputs.values();
	for (var input = inputs.next(); input && !input.done; input = inputs.next()) {
		deviceName = input.value.name;
		send(102, 126); //tell WARBL to enter communications mode
		setPing(); //start checking to make sure WARBL is still connected   
		input.value.onmidimessage = MIDIMessageEventHandler;
		haveAtLeastOneDevice = true;
	}


	if (!haveAtLeastOneDevice) {
		document.getElementById("status").style.color = "#F78339";
		document.getElementById("version").style.color = "#F78339";
		document.getElementById("status").innerHTML = "WARBL not detected.";
	}


	midi.onstatechange = midiOnStateChange;

}


function midiOnStateChange(event) {

	if (ping == 1) {
		document.getElementById("status").style.color = "#F78339";
		document.getElementById("version").style.color = "#F78339";
		document.getElementById("status").innerHTML = "WARBL not detected.";
		document.getElementById("version").innerHTML = "Unknown";
		document.getElementById("current").style.visibility = "hidden";
		document.getElementById("status").style.visibility = "visible";
	}

}



function onMIDIReject(err) {
	alert("The MIDI system failed to start. Please refresh the page.");
}


function setPing() {
	setTimeout(function() {
		ping = 1;
	}, 3000); //change ping to 1 after 3 seconds, after which we'll show a disconnnect if the MIDI state changes.
}



function MIDIMessageEventHandler(event) {


	var e;
	var f;
	if (event.data[2] == "undefined") {
		f = " ";
	} else {
		f = event.data[2];
	}
	if ((event.data[0] & 0xf0) == 144) {
		e = "On";
	} //update the MIDI console
	if ((event.data[0] & 0xf0) == 128) {
		e = "Off";
	}
	if ((event.data[0] & 0xf0) == 176) {
		e = "CC";
	}
	if ((event.data[0] & 0xf0) == 224) {
		e = "PB";
	}
	if ((event.data[0] & 0xf0) == 192) {
		e = "PC";
	}
	if (!(e == "CC" && (event.data[1] > 101 || event.data[1] < 120))) {
		document.getElementById("console").innerHTML = (e + " " + ((event.data[0] & 0x0f) + 1) + " " + event.data[1] + " " + f);
	}



	// Mask off the lower nibble (MIDI channel, which we don't care about yet)
	switch (event.data[0] & 0xf0) {
		case 0x90:
			if (event.data[2] != 0) { // if velocity != 0, this is a note-on message
				noteOn(event.data[1]);
				logKeys;
				return;
			}
			// if velocity == 0, fall thru: it's a note-off.
		case 0x80:
			noteOff(event.data[1]);
			logKeys;
			return;

		case 0xB0: //incoming CC from WARBL


			if (parseFloat(event.data[0] & 0x0f) == 6) { //if it's channel 7 it's from WARBL 

				document.getElementById("status").innerHTML = "WARBL Connected.";
				document.getElementById("status").style.color = "#f7c839";
				//setPing(); //start checking to make sure WARBL is still connected

				if (event.data[1] == 115) { //hole covered info from WARBL


					var byteA = event.data[2];
					for (var m = 0; m < 7; m++) {
						if (bit_test(byteA, m) == 1) {
							document.getElementById("dot" + m).style.backgroundColor = "blue";
							if (m == 0) {
								document.getElementById("dot0").style.opacity = "0.8";
							}
						} else {
							document.getElementById("dot" + m).style.backgroundColor = "#333";
							if (m == 0) {
								document.getElementById("dot0").style.opacity = "0";
							}
						}
					}
				}

				if (event.data[1] == 114) { //hole covered info from WARBL
					for (var n = 0; n < 2; n++) {
						var byteB = event.data[2];
						if (bit_test(byteB, n) == 1) {
							document.getElementById("dot" + (7 + n)).style.backgroundColor = "blue";
						} else {
							if (n == 1) {
								document.getElementById("dot" + (7 + n)).style.backgroundColor = "#181818";
							}
							if (n == 0) {
								document.getElementById("dot" + (7 + n)).style.backgroundColor = "#333";
							}
						}
					}
				} else if (event.data[1] == 102) { //parse based on received CC
					if (event.data[2] > 19 && event.data[2] < 30) {
						document.getElementById("v" + (event.data[2] - 19)).innerHTML = "MAX"; //set sensor value field to max if message is received from WARBL
						checkMax((event.data[2] - 19));
					}

					for (var i = 0; i < 3; i++) { // update the three selected fingering patterns if prompted by the tool.
						if (event.data[2] == 30 + i) {
							fingeringWrite = i;
						}

						if (event.data[2] > 32 && event.data[2] < 60) {
							if (fingeringWrite == i) {
								document.getElementById("fingeringSelect" + i).value = event.data[2] - 33;
							}
							updateCells();
						} //update any dependant fields	
					}



					if (event.data[2] == 60) {
						document.getElementById("fingering0").checked = true;
						instrument = 0;
						document.getElementById("instrument0").style.display = "block";
						document.getElementById("instrument1").style.display = "none";
						document.getElementById("instrument2").style.display = "none";
						document.getElementById("key0").style.display = "block";
						document.getElementById("key1").style.display = "none";
						document.getElementById("key2").style.display = "none";
						handleDefault();
						advancedOkay(); //turn off the advanced tab
						pressureOkay();	
						updateCells();
					}
					if (event.data[2] == 61) {
						document.getElementById("fingering1").checked = true;
						instrument = 1;
						document.getElementById("instrument0").style.display = "none";
						document.getElementById("instrument1").style.display = "block";
						document.getElementById("instrument2").style.display = "none";
						document.getElementById("key0").style.display = "none";
						document.getElementById("key1").style.display = "block";
						document.getElementById("key2").style.display = "none";
						advancedOkay(); //turn off the advanced tab	
						pressureOkay();	
						updateCells();
						handleDefault();
					}
					if (event.data[2] == 62) {
						document.getElementById("fingering2").checked = true;
						instrument = 2;
						document.getElementById("instrument0").style.display = "none";
						document.getElementById("instrument1").style.display = "none";
						document.getElementById("instrument2").style.display = "block";
						document.getElementById("key0").style.display = "none";
						document.getElementById("key1").style.display = "none";
						document.getElementById("key2").style.display = "block";
						advancedOkay(); //turn off the advanced tab	
						pressureOkay();	
						updateCells();
						handleDefault();
					}


					if (event.data[2] == 85) { //receive and handle default instrument setting
						defaultInstr = 0;
						handleDefault();
					}
					if (event.data[2] == 86) {
						defaultInstr = 1;
						handleDefault();
					}
					if (event.data[2] == 87) {
						defaultInstr = 2;
						handleDefault();
					}


					if (event.data[2] == 70) {
						document.getElementById("pitchbendradio0").checked = true;
						updateCustom();
						updateCustom();
					}
					if (event.data[2] == 71) {
						document.getElementById("pitchbendradio1").checked = true;
						updateCustom();
						updateCustom();
					}
					if (event.data[2] == 72) {
						document.getElementById("pitchbendradio2").checked = true;
						updateCustom();
						updateCustom();
					}
					if (event.data[2] == 73) {
					    document.getElementById("pitchbendradio3").checked = true;
					    updateCustom();
					    updateCustom();
					}
 
					if (event.data[2] == 80) {
						document.getElementById("sensorradio0").checked = true;
					}
					if (event.data[2] == 81) {
						document.getElementById("sensorradio1").checked = true;
					}
					if (event.data[2] == 82) {
						document.getElementById("sensorradio2").checked = true;
					}
					if (event.data[2] == 83) {
						document.getElementById("sensorradio3").checked = true;
					}


					if (event.data[2] == 121) {
						document.getElementById("bellSensor").style.opacity = 1;
						document.getElementById("1").disabled = false;
						document.getElementById("2").disabled = false;
						document.getElementById("v1").classList.add("sensorValueEnabled");

					}
					if (event.data[2] == 120) {
						document.getElementById("bellSensor").style.opacity = 0.1;
						document.getElementById("1").disabled = true;
						document.getElementById("2").disabled = true;
						document.getElementById("v1").classList.remove("sensorValueEnabled");
					}

					for (var i = 0; i < 8; i++) { //update button configuration	   
						if (event.data[2] == 90 + i) {
							buttonRowWrite = i;
						}
					}


					for (var j = 0; j < 8; j++) { //update button configuration	
						if (buttonRowWrite == j) {

							if (event.data[2] == 119) { //special case for initiating autocalibration
								document.getElementById("row" + (buttonRowWrite)).value = 19;
							}

							for (var k = 0; k < 12; k++) {
								if (event.data[2] == (100 + k)) {
									document.getElementById("row" + (buttonRowWrite)).value = k;
								}
								if (k < 5 && event.data[2] == 112 + k) {
									document.getElementById("MIDIrow" + (buttonRowWrite)).value = k;
									updateCells();
								}
							}
						}
					} //update any dependant fields}						


					for (var l = 0; l < 3; l++) { //update momentary switches
						if (buttonRowWrite == l) {
							if (event.data[2] == 117) {
								document.getElementById("checkbox" + l).checked = false;
							}
							if (event.data[2] == 118) {
								document.getElementById("checkbox" + l).checked = true;
							}

						}
					}



				} else if (event.data[1] == 103) {
					document.getElementById("senseDistance").value = 0 - event.data[2];
				} //set sensedistance  
				else if (event.data[1] == 117) {
					document.getElementById("depth").value = event.data[2] + 1;
					var output = document.getElementById("demo14");
					output.innerHTML = event.data[2] + 1;
				} //set vibrato depth  
				else if (event.data[1] == 104) {
					jumpFactorWrite = event.data[2];
				} // so we know which pressure setting is going to be received.
				else if (event.data[1] == 105 && jumpFactorWrite < 13) {
					document.getElementById("jumpFactor" + jumpFactorWrite).value = event.data[2];
					var output = document.getElementById("demo" + jumpFactorWrite);
					output.innerHTML = event.data[2];
				}

				if (event.data[1] == 105) {


					if (jumpFactorWrite == 13) {
						document.getElementById("checkbox6").checked = event.data[2];
					} else if (jumpFactorWrite == 14) {
						document.getElementById("expressionDepth").value = event.data[2];
					} else if (jumpFactorWrite == 15) {
						document.getElementById("checkbox7").checked = event.data[2];
					} else if (jumpFactorWrite == 16 && event.data[2] == 0) {
						document.getElementById("curveRadio0").checked = true;
					} else if (jumpFactorWrite == 16 && event.data[2] == 1) {
						document.getElementById("curveRadio1").checked = true;
					} else if (jumpFactorWrite == 16 && event.data[2] == 2) {
						document.getElementById("curveRadio2").checked = true;
					} else if (jumpFactorWrite == 17) {
						document.getElementById("pressureChannel").value = event.data[2];
					} else if (jumpFactorWrite == 18) {
						document.getElementById("pressureCC").value = event.data[2];
					} else if (jumpFactorWrite == 19) {
						slider.noUiSlider.set([event.data[2], null]);
					} else if (jumpFactorWrite == 20) {
						slider.noUiSlider.set([null, event.data[2]]);
					} else if (jumpFactorWrite == 21) {
						slider2.noUiSlider.set([event.data[2], null]);
					} else if (jumpFactorWrite == 22) {
						slider2.noUiSlider.set([null, event.data[2]]);
					} else if (jumpFactorWrite == 23) {
						document.getElementById("dronesOnCommand").value = event.data[2];
					} else if (jumpFactorWrite == 24) {
						document.getElementById("dronesOnChannel").value = event.data[2];
					} else if (jumpFactorWrite == 25) {
						document.getElementById("dronesOnByte2").value = event.data[2];
					} else if (jumpFactorWrite == 26) {
						document.getElementById("dronesOnByte3").value = event.data[2];
					} else if (jumpFactorWrite == 27) {
						document.getElementById("dronesOffCommand").value = event.data[2];
					} else if (jumpFactorWrite == 28) {
						document.getElementById("dronesOffChannel").value = event.data[2];
					} else if (jumpFactorWrite == 29) {
						document.getElementById("dronesOffByte2").value = event.data[2];
					} else if (jumpFactorWrite == 30) {
						document.getElementById("dronesOffByte3").value = event.data[2];
					} else if (jumpFactorWrite == 31 && event.data[2] == 0) {
						document.getElementById("dronesRadio0").checked = true;
					} else if (jumpFactorWrite == 31 && event.data[2] == 1) {
						document.getElementById("dronesRadio1").checked = true;
					} else if (jumpFactorWrite == 31 && event.data[2] == 2) {
						document.getElementById("dronesRadio2").checked = true;
					} else if (jumpFactorWrite == 31 && event.data[2] == 3) {
						document.getElementById("dronesRadio3").checked = true;
					} else if (jumpFactorWrite == 32) {
						lsb = event.data[2];
					} else if (jumpFactorWrite == 33) {
						var x = parseInt((event.data[2] << 7) | lsb); //receive pressure between 100 and 900
						x = (x - 100) * 24 / 900; //convert to inches of water. 100 is the approximate minimum sensor value.
						var p = x.toFixed(1); //round to 1 decimal
						p = Math.min(Math.max(p, 0), 24); //constrain
						document.getElementById("dronesPressureInput").value = p;
					} else if (jumpFactorWrite == 34) {
						lsb = event.data[2];

					} else if (jumpFactorWrite == 35) {
						var x = parseInt((event.data[2] << 7) | lsb); //receive pressure between 100 and 900
						x = (x - 100) * 24 / 900; //convert to inches of water.  100 is the approximate minimum sensor value.
						var p = x.toFixed(1); //round to 1 decimal
						p = Math.min(Math.max(p, 0), 24); //constrain
						document.getElementById("octavePressureInput").value = p;
					} else if (jumpFactorWrite == 43) {
						document.getElementById("checkbox9").checked = event.data[2];
					} //invert											
					else if (jumpFactorWrite == 44) {
						document.getElementById("checkbox5").checked = event.data[2]; //custom
						updateCustom();
					} else if (jumpFactorWrite == 42) {
						document.getElementById("checkbox3").checked = event.data[2];
					} //secret										
					else if (jumpFactorWrite == 40) {
						document.getElementById("checkbox4").checked = event.data[2];
					} //vented							 			
					else if (jumpFactorWrite == 41) {
						document.getElementById("checkbox8").checked = event.data[2];
					} //bagless						 	
					else if (jumpFactorWrite == 45) {
						document.getElementById("checkbox10").checked = event.data[2];
					} //velocity	
					else if (jumpFactorWrite == 46) {
						document.getElementById("checkbox11").checked = (event.data[2] & 0x1);
						document.getElementById("checkbox13").checked = (event.data[2] & 0x2);
					} //aftertouch	
					else if (jumpFactorWrite == 47) {
						document.getElementById("checkbox12").checked = event.data[2];
					} //force max velocity	
					else if (jumpFactorWrite == 61) {
					    document.getElementById("midiBendRange").value = event.data[2];
					}
					else if (jumpFactorWrite == 62) {
					    document.getElementById("noteChannel").value = event.data[2];
					}


				}


				//else if (event.data[1] == 109){document.getElementById("hysteresis").value = event.data[2];
				//var output = document.getElementById("demo13");
				//output.innerHTML = event.data[2];}  	
				else if (event.data[1] == 110) {
					version = event.data[2];


					if (version >= currentVersion) {

						document.getElementById("current").innerHTML = "Your firmware is up to date.";
						document.getElementById("current").style.left = "710px";
						document.getElementById("current").style.visibility = "visible";
						document.getElementById("status").style.visibility = "hidden";
						document.getElementById("current").style.color = "#f7c839";
					} else {
						document.getElementById("current").innerHTML = "There is a firmware update available.";
						document.getElementById("current").style.left = "690px";
						document.getElementById("current").style.visibility = "visible";
						document.getElementById("status").style.visibility = "hidden";
						document.getElementById("current").style.color = "#F78339";
					}


					version = version / 10;
					document.getElementById("version").innerHTML = version;
					document.getElementById("version").style.color = "#f7c839";

					if (version < 1.5) {
						document.getElementById("fingeringSelect0").options[2].disabled = true;
						document.getElementById("fingeringSelect1").options[2].disabled = true;
						document.getElementById("fingeringSelect2").options[2].disabled = true;
						document.getElementById("checkbox10").disabled = true;
						for (var i = 0; i < 8; i++) {
							document.getElementById("row" + i).options[7].disabled = true;
							document.getElementById("row" + i).options[8].disabled = true;
						}
					}

					if (version < 1.6) {
						for (var i = 0; i < 8; i++) {
							document.getElementById("row" + i).options[12].disabled = true;
						}
						document.getElementById("fingeringSelect0").options[6].disabled = true;
						document.getElementById("fingeringSelect1").options[6].disabled = true;
						document.getElementById("fingeringSelect2").options[6].disabled = true;
						document.getElementById("fingeringSelect0").options[11].disabled = true;
						document.getElementById("fingeringSelect1").options[11].disabled = true;
						document.getElementById("fingeringSelect2").options[11].disabled = true;
					}


					if (version < 1.7) {
						document.getElementById("checkbox11").disabled = true;
						document.getElementById("checkbox12").disabled = true;

						document.getElementById("fingeringSelect0").options[12].disabled = true;
						document.getElementById("fingeringSelect1").options[12].disabled = true;
						document.getElementById("fingeringSelect2").options[12].disabled = true;

						document.getElementById("fingeringSelect0").options[13].disabled = true;
						document.getElementById("fingeringSelect1").options[13].disabled = true;
						document.getElementById("fingeringSelect2").options[13].disabled = true;

						document.getElementById("fingeringSelect0").options[14].disabled = true;
						document.getElementById("fingeringSelect1").options[14].disabled = true;
						document.getElementById("fingeringSelect2").options[14].disabled = true;

						document.getElementById("fingeringSelect0").options[15].disabled = true;
						document.getElementById("fingeringSelect1").options[15].disabled = true;
						document.getElementById("fingeringSelect2").options[15].disabled = true;
					}


					/*		
			if (version > 1.6){ //add new items that should only be visible with newer software versions
					
				
			
				//add new fingering options if version is newer
				for (i = 0; i < document.getElementById("fingeringSelect0").length; ++i){
    				if (document.getElementById("fingeringSelect0").options[i].value == "12"){
  				var a = 1;    
				}}


  				if (a != 1){
				var x = document.getElementById("fingeringSelect0"); 

			var optionb = document.createElement("option");
  			optionb.text = "Saxophone, extended";
  			optionb.value = 12;
  			x.add(optionb);
			
			var optionc = document.createElement("option");
  			optionc.text = "Saxophone, basic";
  			optionc.value = 14;
  			x.add(optionc);
			
			var optiond = document.createElement("option");
  			optiond.text = "Trumpet/EVI";
  			optiond.value = 15;
  			x.add(optiond);
			
			var optione = document.createElement("option");
  			optione.text = "Shakuhachi";
  			optione.value = 16;
  			x.add(optione);
						
						
			var y = document.getElementById("fingeringSelect1");
			var optionb = document.createElement("option");
  			optionb.text = "Saxophone, extended";
  			optionb.value = 12;
  			y.add(optionb);
			
			var optionc = document.createElement("option");
  			optionc.text = "Saxophone, basic";
  			optionc.value = 14;
  			y.add(optionc);
			
			var optiond = document.createElement("option");
  			optiond.text = "Trumpet/EVI";
  			optiond.value = 15;
  			y.add(optiond);
			
			var optione = document.createElement("option");
  			optione.text = "Shakuhachi";
  			optione.value = 16;
  			y.add(optione);
					
			var z = document.getElementById("fingeringSelect2");
			var optionb = document.createElement("option");
  			optionb.text = "Saxophone, extended";
  			optionb.value = 12;
  			z.add(optionb);
				
			var optionc = document.createElement("option");
  			optionc.text = "Saxophone, basic";
  			optionc.value = 14;
  			z.add(optionc);
			
			var optiond = document.createElement("option");
  			optiond.text = "Trumpet/EVI";
  			optiond.value = 15;
  			z.add(optiond);
			
			var optione = document.createElement("option");
  			optione.text = "Shakuhachi";
  			optione.value = 16;
  			z.add(optione);

				}
				*/

					/*		
			//add new options to button configuration for v. 1.6+	
			var a = 0;
				for (i = 0; i < document.getElementById("row0").length; ++i){
    				if (document.getElementById("row0").options[i].value == "19"){ //check to see if we've already added it
 					a = 1;    
					}}
					
				if (a != 1){
					for (var i=0; i< 8; i++) {	
					
						var x = document.getElementById("row" + i); 		
  						var option = document.createElement("option");
  						option.text = "Begin autocalibration";
  						option.value = 19;
  						x.add(option);

					}
				}
				
			


		}

*/

				} else if (event.data[1] == 111) {
					document.getElementById("keySelect0").value = event.data[2];
				} else if (event.data[1] == 112) {
					document.getElementById("keySelect1").value = event.data[2];
				} else if (event.data[1] == 113) {
					document.getElementById("keySelect2").value = event.data[2];
				} else if (event.data[1] == 116) {
					lsb = event.data[2];
				} else if (event.data[1] == 118) {
					var x = parseInt((event.data[2] << 7) | lsb); //receive pressure between 100 and 900
					x = (x - 100) * 24 / 900; //convert to inches of water.  105 is the approximate minimum sensor value.
					p = Math.min(Math.max(p, 0), 24); //constrain
					var p = x.toFixed(1); //round to 1 decimal
					if (p < 0.2) {
						p = 0
					};
					
					// Are we graphing pressure values?
					if (gPressureGraphEnabled){
						capturePressure(p);
					}
					else{
						document.getElementById("pressure").innerHTML = (p);
						document.getElementById("pressure1").innerHTML = (p);
					}				}


				for (var i = 0; i < 8; i++) {
					if (buttonRowWrite == i) {
						if (event.data[1] == 106 && event.data[2] < 16) {
							document.getElementById("channel" + (buttonRowWrite)).value = event.data[2];
						}
						if (event.data[1] == 107) {
							document.getElementById("byte2_" + (buttonRowWrite)).value = event.data[2];
						}
						if (event.data[1] == 108) {
							document.getElementById("byte3_" + (buttonRowWrite)).value = event.data[2];
						}
					}
				}

				if (event.data[1] == 106 && event.data[2] > 15) {



					if (event.data[2] > 19 && event.data[2] < 29) {
						document.getElementById("vibratoCheckbox" + (event.data[2] - 20)).checked = false;
					}

					if (event.data[2] > 29 && event.data[2] < 39) {
						document.getElementById("vibratoCheckbox" + (event.data[2] - 30)).checked = true;
					}

					if (event.data[2] == 39) {
						document.getElementById("calibrateradio0").checked = true;
					}
					if (event.data[2] == 40) {
						document.getElementById("calibrateradio1").checked = true;
					}

					if (event.data[2] == 53) { //add an option for the uilleann regulators fingering pattern if a message is received indicating that it is supported.

						for (i = 0; i < document.getElementById("fingeringSelect0").length; ++i) {
							if (document.getElementById("fingeringSelect0").options[i].value == "9") {
								var a = 1;
							}
						}

						if (a != 1) {
							var x = document.getElementById("fingeringSelect0");
							var option = document.createElement("option");
							option.text = "Uilleann regulators";
							option.value = 9;
							x.add(option);

							var y = document.getElementById("fingeringSelect1");
							var option = document.createElement("option");
							option.text = "Uilleann regulators";
							option.value = 9;
							y.add(option);

							var z = document.getElementById("fingeringSelect2");
							var option = document.createElement("option");
							option.text = "Uilleann regulators";
							option.value = 9;
							z.add(option);

						}
					}


				}

			}
	}
	updateCells(); //keep enabled/disabled cells updated.
}



function bit_test(num, bit) {
	return ((num >> bit) % 2 != 0)
}


function sendFingeringSelect(row, selection) {
	selection = parseFloat(selection);
	updateCells();
	updateCustom();
	blink(1);
	//default keys
	var key;
	if (selection == 2) {
		key = 8;
	} //GHB
	else if (selection == 3) {
		key = 3;
	} //Northumbrian
	else if (selection == 5 || selection == 13) {
		key = 125;
	} //Gaita
	else if (selection == 6) {
		key = 122;
	} //NAF
	else if (selection == 8) {
		key = 125;
	} //Recorder
	else if (selection == 11) {
		key = 113;
	} //Xiao
	else if (selection == 12 || selection == 14 || selection == 15) {
		key = 123;
	} //Sax
	else {
		key = 0;
	} //default key of D for many patterns
	document.getElementById("keySelect" + row).value = key; //set key menu

	//send the fingering pattern
	send(102, 30 + row);
	send(102, 33 + selection);

	sendKey(row, key);
}


function defaultInstrument() { //tell WARBL to set the current instrument as the default
	defaultInstr = instrument;
	handleDefault();
	send(102, 85);
}

function handleDefault() { //display correct default instrument and "set default" buttons	

	if (version > 1.6) {

		if (defaultInstr != 0) {
			document.getElementById("instrument1Label").innerHTML = "Instrument 1:";
		}
		if (defaultInstr != 1) {
			document.getElementById("instrument2Label").innerHTML = "Instrument 2:";
		}
		if (defaultInstr != 2) {
			document.getElementById("instrument3Label").innerHTML = "Instrument 3:";
		}

		if (defaultInstr == 0) {
			document.getElementById("instrument1Label").innerHTML = "Instrument 1 (default):";
		}
		if (defaultInstr == 1) {
			document.getElementById("instrument2Label").innerHTML = "Instrument 2 (default):";
		}
		if (defaultInstr == 2) {
			document.getElementById("instrument3Label").innerHTML = "Instrument 3 (default):";
		}

		if (defaultInstr != instrument) {
			document.getElementById("defaultInstrument").style.visibility = "visible";

			if (instrument == 0) {
				document.getElementById("defaultInstrument").style.left = "130px";
			}
			if (instrument == 1) {
				document.getElementById("defaultInstrument").style.left = "430px";
			}
			if (instrument == 2) {
				document.getElementById("defaultInstrument").style.left = "730px";
			}
		} else {
			document.getElementById("defaultInstrument").style.visibility = "hidden";

		}
	}
}


function sendKey(row, selection) {
	selection = parseFloat(selection);
	row = parseFloat(row);
	updateCells();
	blink(1);
	send(111 + row, selection);
}

function sendFingeringRadio(tab) { //change instruments, showing the correct tab for each instrument.

	instrument = tab;
	updateCustom();
	advancedOkay(); //turn off the advanced tab
	pressureOkay();	
	handleDefault(); //display correct default instrument and "set default" buttons	

	if (tab == 0) {
		document.getElementById("instrument0").style.display = "block";
		document.getElementById("instrument1").style.display = "none";
		document.getElementById("instrument2").style.display = "none";


		document.getElementById("key0").style.display = "block";
		document.getElementById("key1").style.display = "none";
		document.getElementById("key2").style.display = "none";
		send(102, 60);
	} else if (tab == 1) {
		document.getElementById("instrument0").style.display = "none";
		document.getElementById("instrument1").style.display = "block";
		document.getElementById("instrument2").style.display = "none";


		document.getElementById("key0").style.display = "none";
		document.getElementById("key1").style.display = "block";
		document.getElementById("key2").style.display = "none";
		blink(2);
		send(102, 61);
	} else if (tab == 2) {
		document.getElementById("instrument0").style.display = "none";
		document.getElementById("instrument1").style.display = "none";
		document.getElementById("instrument2").style.display = "block";


		document.getElementById("key0").style.display = "none";
		document.getElementById("key1").style.display = "none";
		document.getElementById("key2").style.display = "block";
		blink(3);
		send(102, 62);
	}
	updateCells();
	updateCustom();
}

function sendSenseDistance(selection) {
	blink(1);
	selection = parseFloat(selection);
	x = 0 - parseFloat(selection);
	send(103, x);
}


function sendDepth(selection) {
	blink(1);
	updateSliders();
	selection = parseFloat(selection);
	send(117, selection);
}


function sendExpressionDepth(selection) {
	blink(1);
	selection = parseFloat(selection);
	send(104, 14);
	send(105, selection);
}



function sendCurveRadio(selection) {
	blink(1);
	selection = parseFloat(selection);
	send(104, 16);
	send(105, selection);
}


function sendPressureChannel(selection) {
	blink(1);
	
	var x = parseFloat(selection);
	if (x < 1 || x > 16 || isNaN(x)) {
	    alert("Value must be 1-16.");
	    document.getElementById("pressureChannel").value = null;
	} else {
	    send(104, 17);
	    send(105, x);
	}
}

function sendPressureCC(selection) {
	blink(1);
	selection = parseFloat(selection);
	send(104, 18);
	send(105, selection);
}

function sendBendRange(selection) {
    blink(1);
    var x = parseFloat(selection);
    if (x < 1 || x > 96 || isNaN(x)) {
        alert("Value must be 1-96.");
        document.getElementById("midiBendRange").value = null;
    } else {
        send(104, 61);
        send(105, x);
    }
}

function sendNoteChannel(selection) {
       blink(1);
       var x = parseFloat(selection);
       if (x < 1 || x > 16 || isNaN(x)) {
               alert("Value must be 1-16.");
               document.getElementById("noteChannel").value = null;
       } else {
               sendToWARBL(104, 62);
               sendToWARBL(105, x);
       }
}



//pressure input slider
slider.noUiSlider.on('change', function(values) {
	blink(1);
	send(104, 19);
	send(105, parseInt(values[0]));
	send(104, 20);
	send(105, parseInt(values[1]));
});

//pressure output slider
slider2.noUiSlider.on('change', function(values) {
	blink(1);
	send(104, 21);
	send(105, parseInt(values[0]));
	send(104, 22);
	send(105, parseInt(values[1]));
});


function sendDronesOnCommand(selection) {
	blink(1);
	selection = parseFloat(selection);
	send(104, 23);
	send(105, selection);
}

function sendDronesOnChannel(selection) {
	var x = parseFloat(selection);
	if (x < 0 || x > 16 || isNaN(x)) {
		alert("Value must be 1-16.");
		document.getElementById("dronesOnChannel").value = null;
	} else {
		blink(1);
		send(104, 24);
		send(105, x);
	}
}


function sendDronesOnByte2(selection) {
	var x = parseFloat(selection);
	if (x < 0 || x > 127 || isNaN(x)) {
		alert("Value must be 0 - 127.");
		document.getElementById("dronesOnByte2").value = null;
	} else {
		blink(1);
		send(104, 25);
		send(105, x);
	}
}


function sendDronesOnByte3(selection) {
	var x = parseFloat(selection);
	if (x < 0 || x > 127 || isNaN(x)) {
		alert("Value must be 0 - 127.");
		document.getElementById("dronesOnByte3").value = null;
	} else {
		blink(1);
		send(104, 26);
		send(105, x);
	}
}


function sendDronesOffCommand(selection) {
	blink(1);
	var y = parseFloat(selection);
	send(104, 27);
	send(105, y);
}


function sendDronesOffChannel(selection) {
	var x = parseFloat(selection);
	if (x < 0 || x > 16 || isNaN(x)) {
		alert("Value must be 1-16.");
		document.getElementById("dronesOffChannel").value = null;
	} else {
		blink(1);
		send(104, 28);
		send(105, x);
	}
}


function sendDronesOffByte2(selection) {
	var x = parseFloat(selection);
	if (x < 0 || x > 127 || isNaN(x)) {
		alert("Value must be 0 - 127.");
		document.getElementById("dronesOffByte2").value = null;
	} else {
		blink(1);
		send(104, 29);
		send(105, x);
	}
}


function sendDronesOffByte3(selection) {
	var x = parseFloat(selection);
	if (x < 0 || x > 127 || isNaN(x)) {
		alert("Value must be 0 - 127.");
		document.getElementById("dronesOffByte3").value = null;
	} else {
		blink(1);
		send(104, 30);
		send(105, x);
	}
}


function sendDronesRadio(selection) {
	blink(1);
	selection = parseFloat(selection);
	send(104, 31);
	send(105, selection);
}

function learnDrones() {
	blink(1);
	document.getElementById("dronesPressureInput").style.backgroundColor = "#32CD32";
	setTimeout(blinkDrones, 500);
	send(106, 43);

}

function blinkDrones() {
	document.getElementById("dronesPressureInput").style.backgroundColor = "#FFF";
}


function blinkOctave() {
	document.getElementById("octavePressureInput").style.backgroundColor = "#FFF";
}

function sendDronesPressure(selection) {
	var x = parseFloat(selection);
	if (x < 0 || x > 24 || isNaN(x)) {
		alert("Value must be 0.0 - 24.0.");
		document.getElementById("dronesPressureInput").value = null;
	} else {
		x = parseInt(x * 900 / 24 + 100);
		blink(1);
		document.getElementById("dronesPressureInput").style.backgroundColor = "#32CD32";
		setTimeout(blinkDrones, 500);
		send(104, 32);
		send(105, x & 0x7F);
		send(104, 33);
		send(105, x >> 7);
	}

}


function sendOctavePressure(selection) {
	var x = parseFloat(selection);
	if (x < 0 || x > 24 || isNaN(x)) {
		alert("Value must be 0.0 - 24.0.");
		document.getElementById("octavePressureInput").value = null;
	} else {
		x = parseInt(x * 900 / 24 + 100);
		blink(1);
		document.getElementById("octavePressureInput").style.backgroundColor = "#32CD32";
		setTimeout(blinkOctave, 500);
		send(104, 34);
		send(105, x & 0x7F);
		send(104, 35);
		send(105, x >> 7);
	}

}


function learn() {
	blink(1);
	document.getElementById("octavePressureInput").style.backgroundColor = "#32CD32";
	setTimeout(blinkOctave, 500);
	send(106, 41);
}


function sendCalibrateRadio(selection) {
	selection = parseFloat(selection);
	blink(1);
	send(106, 39 + selection);
}


function sendPitchbendRadio(selection) {
	updateCustom();
	updateCustom();
	selection = parseFloat(selection);
	if (selection > 0) {
		blink(selection + 1);
	}
	send(102, 70 + selection);
}



function sendVibratoHoles(holeNumber, selection) {
	selection = +selection; //convert true/false to 1/0
	send(106, (30 - (selection * 10)) + holeNumber);
}


function updateCustom() { //keep correct settings enabled/disabled with respect to the custom vibrato switch.
	var a = document.getElementById("fingeringSelect" + instrument).value;


	if (document.getElementById("pitchbendradio2").checked == false && (((a == 0 || a == 1 || a == 4 || a == 10) && ((document.getElementById("pitchbendradio1").checked == true && version < 1.6) || version > 1.5)) || version > 1.5 && (a == 3 || a == 2))) {
		document.getElementById("checkbox5").disabled = false;
	} else {
		//document.getElementById("checkbox5").checked = false;
		document.getElementById("checkbox5").disabled = true;
		//send(104,44); //if custom is disabled, tell WARBL to turn it off
		//send(105, 0);
		//blink(1);
	}
	if (document.getElementById("checkbox5").checked == true && document.getElementById("checkbox5").disabled == false) {
		for (var i = 0; i < 9; i++) {
			document.getElementById("vibratoCheckbox" + i).disabled = true;
			document.getElementById("vibratoCheckbox" + i).style.cursor = "default";
		}
	} else {
		for (var i = 0; i < 9; i++) {
			document.getElementById("vibratoCheckbox" + i).disabled = false;
			document.getElementById("vibratoCheckbox" + i).style.cursor = "pointer";
		}
	}
}



function sendBreathmodeRadio(selection) {
	selection = parseFloat(selection);
	if (selection > 0) {
		blink(selection + 1);
	}
	send(102, 80 + selection);

}


function advanced() {
	document.getElementById("box2").style.display = "block";
	document.getElementById("box1").style.display = "none";
}

function advancedOkay() {
	document.getElementById("box2").style.display = "none";
	document.getElementById("box1").style.display = "block";
}

function pressureGraph() {

    // Not connected, disable graph
    if (!midiAccess){
    	return;
    }
    
	// Show the pressure graph
	document.getElementById("pressuregraph").style.display = "block";
	document.getElementById("box1").style.display = "none";

	// Start graphing the incoming pressure
	startPressureGraph();
}

function pressureOkay() {

	// Stop graphing the incoming pressure
	stopPressureGraph();

	// Hide the pressure
	document.getElementById("pressuregraph").style.display = "none";
	document.getElementById("box1").style.display = "block";
}

function advancedDefaults() {
	document.getElementById("jumpFactor1").value = "25";
	document.getElementById("jumpFactor2").value = "15";
	document.getElementById("jumpFactor3").value = "15";
	document.getElementById("jumpFactor4").value = "15";
	document.getElementById("jumpFactor5").value = "30";
	document.getElementById("jumpFactor6").value = "60";
	document.getElementById("jumpFactor7").value = "3";
	document.getElementById("jumpFactor8").value = "7";
	document.getElementById("jumpFactor9").value = "10";
	document.getElementById("jumpFactor10").value = "7";
	document.getElementById("jumpFactor11").value = "9";
	document.getElementById("jumpFactor12").value = "9";
	updateSliders();

	for (var i = 1; i < 13; i++) {
		var j = document.getElementById("jumpFactor" + (i)).value;
		sendJumpFactor(i, j);
	}
}


function advancedBagDefaults() {
	document.getElementById("jumpFactor1").value = "75";
	document.getElementById("jumpFactor2").value = "25";
	document.getElementById("jumpFactor3").value = "15";
	document.getElementById("jumpFactor4").value = "15";
	document.getElementById("jumpFactor5").value = "30";
	document.getElementById("jumpFactor6").value = "60";
	document.getElementById("jumpFactor7").value = "50";
	document.getElementById("jumpFactor8").value = "20";
	document.getElementById("jumpFactor9").value = "28";
	document.getElementById("jumpFactor10").value = "7";
	document.getElementById("jumpFactor11").value = "15";
	document.getElementById("jumpFactor12").value = "22";
	updateSliders();

	for (var i = 1; i < 13; i++) {
		var j = document.getElementById("jumpFactor" + (i)).value;
		sendJumpFactor(i, j);
	}
}



function sendJumpFactor(factor, selection) {
	selection = parseFloat(selection);
	blink(1);
	updateSliders();
	send(104, factor);
	send(105, selection);
}


//function sendHysteresis() {
//blink(1);
//var x = parseFloat(document.getElementById("hysteresis").value);	
//send(109,x);}

function sendRow(rowNum) {
	blink(1);
	updateCells();
	send(102, 90 + rowNum);
	var y = (100) + parseFloat(document.getElementById("row" + rowNum).value);
	send(102, y);
	sendMIDIrow(rowNum);
	sendChannel(rowNum);
	sendByte2(rowNum);
	sendByte3(rowNum);
	if (rowNum < 3) {
		sendMomentary(rowNum);
	}
}

function sendMIDIrow(MIDIrowNum) {
	blink(1);
	updateCells();
	send(102, 90 + MIDIrowNum);
	var y = (112) + parseFloat(document.getElementById("MIDIrow" + MIDIrowNum).value);
	send(102, y);
	sendChannel(MIDIrowNum);
	sendByte2(MIDIrowNum);
	sendByte3(MIDIrowNum);
	if (MIDIrowNum < 3) {
		sendMomentary(MIDIrowNum);
	}
}

function sendChannel(rowNum) {
	blink(1);
	MIDIvalueChange();
	var y = parseFloat(document.getElementById("channel" + (rowNum)).value);
	send(102, 90 + rowNum);
	send(106, y);
}

function sendByte2(rowNum) {
	blink(1);
	MIDIvalueChange();
	send(102, 90 + rowNum);
	var y = parseFloat(document.getElementById("byte2_" + (rowNum)).value);
	send(107, y);
}

function sendByte3(rowNum) {
	blink(1);
	MIDIvalueChange();
	send(102, 90 + rowNum);
	var y = parseFloat(document.getElementById("byte3_" + (rowNum)).value);
	send(108, y);
}

function sendMomentary(rowNum) { //send momentary
	blink(1);
	updateCells();
	var y = document.getElementById("checkbox" + rowNum).checked
	send(102, 90 + rowNum);
	if (y == false) {
		send(102, 117);
	}
	if (y == true) {
		send(102, 118);
	}
}



//switches

function sendVented(selection) {
	selection = +selection; //convert true/false to 1/0
	blink(1);
	send(104, 40);
	send(105, selection);
}

function sendBagless(selection) {
	selection = +selection; //convert true/false to 1/0
	blink(1);
	send(104, 41);
	send(105, selection);
}


function sendSecret(selection) {
	selection = +selection; //convert true/false to 1/0
	blink(1);
	send(104, 42);
	send(105, selection);
}


function sendInvert(selection) {
	selection = +selection; //convert true/false to 1/0
	blink(1);
	send(104, 43);
	send(105, selection);
}


function sendCustom(selection) {
	selection = +selection; //convert true/false to 1/0
	blink(1);
	updateCustom();
	send(104, 44);
	send(105, selection);
}

function sendExpression(selection) {
	selection = +selection; //convert true/false to 1/0
	blink(1);
	send(104, 13);
	send(105, selection);
}


function sendRawPressure(selection) {
	selection = +selection; //convert true/false to 1/0
	blink(1);
	send(104, 15);
	send(105, selection);
}


function sendVelocity(selection) {
	selection = +selection; //convert true/false to 1/0
	blink(1);
	send(104, 45);
	send(105, selection);
	updateCells();
}

function sendAftertouch(selection, polyselection) {
	selection = +selection; //convert true/false to 1/0
	var val = selection | ((+polyselection) << 1);
	blink(1);
	send(104, 46);
	send(105, val);
}

function sendForceVelocity(selection) {
	selection = +selection; //convert true/false to 1/0
	blink(1);
	send(104, 47);
	send(105, selection);
}


//end switches


function saveAsDefaults() {
	modalclose(2);
	blink(3);
	send(102, 123);
}

function saveAsDefaultsForAll() {
	modalclose(3);
	blink(3);
	send(102, 124);
}

function restoreAll() {
	modalclose(4);
	blink(3);
	send(102, 125);
}

function autoCalibrateBell() {
	LEDon();
	setTimeout(LEDoff, 5000);
	send(106, 42);
}

function autoCalibrate() {
	LEDon();
	setTimeout(LEDoff, 10000);
	send(102, 127);
}

function frequencyFromNoteNumber(note) {
	return 440 * Math.pow(2, (note - 69) / 12);
}

function modal(modalId) {
	document.getElementById("open-modal" + modalId).classList.add('modal-window-open');
}

function modalclose(modalId) {
	document.getElementById("open-modal" + modalId).classList.remove('modal-window-open');
}

function send(byte2, byte3) {
	if (!(byte2 == 102 && byte3 == 127) && !(byte2 == 106 && byte3 == 42)) {
		blink(1);
	} //blink once if we aren't doing autocalibration, which requires the LED to be on longer.

	if (byte2 == 102) {
		if (byte3 == 19) { //send message to save sensor calibration
			blink(3);
			for (var i = 1; i < 10; i++) {
				document.getElementById("v" + (i)).innerHTML = "0";
			}
			for (var i = 0; i < 19; i++) {
				sensorValue[i] = 0;
			}
		}


		if (isEven(byte3) && byte3 < 19) { //send sensor calibration values
			sensorValue[byte3 - 2]++;
			document.getElementById("v" + (byte3 >> 1)).innerHTML = sensorValue[byte3 - 2];
		}


		if (isOdd(byte3) && byte3 < 19) {
			sensorValue[byte3 - 1]--;
			document.getElementById("v" + ((byte3 + 1) >> 1)).innerHTML = sensorValue[byte3 - 1];
			checkMax((byte3 + 1) >> 1);
		}
	}

	cc = [0xB6, byte2, byte3]; //prepare message
	var iter = midiAccess.outputs.values();
	for (var o = iter.next(); !o.done; o = iter.next()) {
		o.value.send(cc, window.performance.now()); //send CC message
	}
}


function blink(blinkNum) {
	LEDon();
	setTimeout(LEDoff, 200);
	if (blinkNum > 1) { //blink twice
		setTimeout(LEDon, 400);
		setTimeout(LEDoff, 600);
	}
	if (blinkNum > 2) { // blink thrice
		setTimeout(LEDon, 800);
		setTimeout(LEDoff, 1000);
	}
	if (blinkNum > 3) { //blink four times
		setTimeout(LEDon, 1200);
		setTimeout(LEDoff, 1400);
	}
	if (blinkNum > 4) { //blink five times
		setTimeout(LEDon, 1600);
		setTimeout(LEDoff, 1800);
	}
}


function checkMax(field) { //disable the sensor value field if it has been set to MAX
	var x = document.getElementById("v" + field).textContent;
	if (x == "MAX") {
		document.getElementById(field * 2).disabled = true;
	}
	if (x != "MAX") {
		document.getElementById(field * 2).disabled = false;
	}
}


function isEven(n) {
	return n % 2 == 0;
}


function isOdd(n) {
	return Math.abs(n % 2) == 1;
}


function LEDoff() {
	//console.log("off");
	elements = document.getElementsByClassName("leddot");
	for (var i = 0; i < elements.length; i++) {
		elements[i].style.visibility = "hidden";
	}
}


function LEDon() {
	elements = document.getElementsByClassName("leddot");
	for (var i = 0; i < elements.length; i++) {
		elements[i].style.visibility = "visible";
	}
}


function toggleOn() {
	volume = +!volume;
	if (volume == 0) {
		noteOff(currentNote)
	}
	//else{context.resume();
	//}
}


function logKeys() {
	var s = 'Keys';
	for (var i = 0; i < midiNotes.length; i++) {
		s = s + ' ' + midiNotes[i].pitch;
	}
}



function noteOn(pitch) {
	noteOff(pitch);
	currentNote = pitch;
	if (volume) {
		for (var i = 0; i < _tone_0650_SBLive_sf2.zones.length; i++) {
			_tone_0650_SBLive_sf2.zones[i].ahdsr = false;
		}
		var envelope = player.queueWaveTable(audioContext, audioContext.destination, _tone_0650_SBLive_sf2, 0, pitch, 999, 0.5, true);
		var note = {
			pitch: pitch,
			envelope: envelope
		};
		midiNotes.push(note);
	}
}



function noteOff(pitch) {
	for (var i = 0; i < midiNotes.length; i++) {
		if (midiNotes[i].pitch == pitch) {
			if (midiNotes[i].envelope) {
				midiNotes[i].envelope.cancel();
			}
			midiNotes.splice(i, 1);
			return;
		}
	}
}



function updateSliders() {

	for (var i = 1; i < 13; i++) {
		var slider1 = document.getElementById("jumpFactor" + i);
		var output1 = document.getElementById("demo" + i);
		output1.innerHTML = slider1.value;
	}

	var slider14 = document.getElementById("depth");
	var output14 = document.getElementById("demo14");
	output14.innerHTML = slider14.value;
}



//sets up initial values for selects/fields/radios and constantly keeps the proper options enabled/disabled
function updateCells() {

	if (version > 1.6) {
		var p = document.getElementById("checkbox10").checked;
		if (p == true) {
			document.getElementById("checkbox12").disabled = true;
		} else {
			document.getElementById("checkbox12").disabled = false;
		}
	}

	var q = document.getElementById("checkbox0").checked;
	if (q == true) {
		document.getElementById("row5").disabled = true;
	} else {
		document.getElementById("row5").disabled = false;
	}

	var r = document.getElementById("checkbox1").checked;
	if (r == true) {
		document.getElementById("row6").disabled = true;
	} else {
		document.getElementById("row6").disabled = false;
	}

	if (q == true || r == true) {
		document.getElementById("row3").disabled = true;
	} else {
		document.getElementById("row3").disabled = false;
	}

	var s = document.getElementById("checkbox2").checked;
	if (s == true) {
		document.getElementById("row7").disabled = true;
	} else {
		document.getElementById("row7").disabled = false;
	}

	if (r == true || s == true) {
		document.getElementById("row4").disabled = true;
	} else {
		document.getElementById("row4").disabled = false;
	}



	for (var i = 0; i < 8; i++) {
		var x = document.getElementById("row" + i).value;
		var t = document.getElementById("row" + i).disabled;

		if ((x != 1 || t == true)) {
			document.getElementById("MIDIrow" + i).disabled = true;
			document.getElementById("channel" + i).disabled = true;
			document.getElementById("byte2_" + i).disabled = true;
			document.getElementById("byte3_" + i).disabled = true;
		} else {
			document.getElementById("MIDIrow" + i).disabled = false;
			document.getElementById("channel" + i).disabled = false;
			document.getElementById("byte2_" + i).disabled = false;
			document.getElementById("byte3_" + i).disabled = false;
		}


		var y = document.getElementById("MIDIrow" + i).value;
		if (y == 2) {
			document.getElementById("byte3_" + i).disabled = true;
		}
		if (y > 2) {
			document.getElementById("byte2_" + i).disabled = true;
			document.getElementById("byte3_" + i).disabled = true;
		}

		var z = document.getElementById("row" + i).value;


		if ((y == 0 && i < 3 && x == 1) || ((version > 1.4 || version == "Unknown") && i < 3 && (z == 5 || z == 6 || z == 10 || z == 11))) {
			document.getElementById("checkbox" + i).disabled = false;
			document.getElementById("switch" + i).style.cursor = "pointer";
		}

		if (((y != 0 && i < 3) || (i < 3 && (x == 0 || x > 1))) && !((version > 1.4 || version == "Unknown") && i < 3 && (z == 5 || z == 6 || z == 10 || z == 11))) {
			document.getElementById("checkbox" + i).disabled = true;
			document.getElementById("switch" + i).style.cursor = "default";
			document.getElementById("checkbox" + i).checked = false;
		}



		var q = document.getElementById("checkbox0").checked;
		if (q == true) {
			document.getElementById("row5").disabled = true;
		} else {
			document.getElementById("row5").disabled = false;
		}

		var r = document.getElementById("checkbox1").checked;
		if (r == true) {
			document.getElementById("row6").disabled = true;
		} else {
			document.getElementById("row6").disabled = false;
		}

		if (q == true || r == true) {
			document.getElementById("row3").disabled = true;
		} else {
			document.getElementById("row3").disabled = false;
		}

		var s = document.getElementById("checkbox2").checked;
		if (s == true) {
			document.getElementById("row7").disabled = true;
		} else {
			document.getElementById("row7").disabled = false;
		}

		if (r == true || s == true) {
			document.getElementById("row4").disabled = true;
		} else {
			document.getElementById("row4").disabled = false;
		}



	}



	var z = $("#fingeringSelect0 option:selected").text();
	document.getElementById("fingeringLabel1").innerHTML = z;

	var z = $("#fingeringSelect1 option:selected").text();
	document.getElementById("fingeringLabel2").innerHTML = z;

	var z = $("#fingeringSelect2 option:selected").text();
	document.getElementById("fingeringLabel3").innerHTML = z;



}



function MIDIvalueChange() {
	for (var i = 0; i < 8; i++) {
		var x = document.getElementById("channel" + i).value;
		if (((x < 0 || x > 16 || isNaN(x)) && !document.getElementById("channel" + i).disabled)) {
			alert("Value must be 1-16.");
			document.getElementById("channel" + i).value = null;
		}
	}
	for (var j = 0; j < 8; j++) {
		var y = document.getElementById("channel" + j).value;
		var x = document.getElementById("byte2_" + j).value;
		if (x < 0 || x > 127 || isNaN(x)) {
			alert("Value must be 0-127.");
			document.getElementById("byte2_" + j).value = null;
		}
		if (y == 7 && x > 101 && x < 120) {
			alert("This CC range on channel 7 is reserved for the Configuration Tool.");
			document.getElementById("byte2_" + j).value = null;

		}
	}

	for (var k = 0; k < 8; k++) {
		var x = document.getElementById("byte3_" + k).value;
		if (x < 0 || x > 127 || isNaN(x)) {
			alert("Value must be 0-127.");
			document.getElementById("byte3_" + k).value = null;
		}
	}

}



var AudioContextFunc = window.AudioContext || window.webkitAudioContext;
var audioContext = new AudioContextFunc();
var player = new WebAudioFontPlayer();
player.loader.decodeAfterLoading(audioContext, '_tone_0650_SBLive_sf2');
var midiNotes = [];



/* Toggle between adding and removing the "responsive" class to topnav when the user clicks on the icon */
function topNavFunction() {
	var x = document.getElementById("myTopnav");
	if (x.className === "topnav") {
		x.className += " responsive";
	} else {
		x.className = "topnav";
	}
}


//
// Capture the incoming pressure
//
function capturePressure(val){
	
	//console.log("capturePressure: "+val);

	gCurrentPressure = val;

}

//
// Graph the incoming pressure
//
function graphPressure(){

	//console.log("graphPressure: "+gCurrentPressure);
	//debugger;

	// Drop the last element of the array
	gPressureReadings.pop();
	
	// Add the new value to the front of the array
	gPressureReadings.unshift(gCurrentPressure);

	//debugger;

	// Clear the graph
   	$("#d3PressureGraph").empty();

	// set the dimensions and margins of the graph
	var margin = {top: 10, right: 10, bottom: 100, left: 50},
	    width = 390,
	    height = 320 - margin.top - margin.bottom;

	// append the svg object to the body of the page
	var svg = d3.select("#d3PressureGraph")
	  .append("svg")
	    .attr("width", width + margin.left + margin.right)
	    .attr("height", height + margin.top + margin.bottom)
	  // translate this svg element to leave some margin.
	  .append("g")
	    .attr("transform", "translate(" + margin.left + "," + margin.top + ")");

	// X scale and Axis
	var x = d3.scaleLinear()
	    .domain([0, width])       // This is the min and the max of the data
	    .range([0, width]);   
	
	svg.append('g')
	  .attr("transform", "translate(0," + height + ")")
	  .call(d3.axisBottom(x).tickSize(0).tickFormat(""));

	// Y scale and Axis
	var y = d3.scaleLinear()
	    .domain([0, gMaxPressure])  // This is the min and the max of the data: 0 to gMaxPressure
	    .range([height, 0]); 

	svg.append('g')
	  .call(d3.axisLeft(y));

	// text label for the x axis
	svg.append("text")
	      .attr("y", (height + 4))
	      .attr("x",(width / 2) - (margin.left/2) + 10)
	      .attr("dy", "1em")
	      .style("text-anchor", "middle")
	      .style("fill","white")
	      .text("Time");      

	// text label for the y axis
	svg.append("text")
	      .attr("transform", "rotate(-90)")
	      .attr("y", 0 - (margin.left - 5))
	      .attr("x",0 - (height / 2))
	      .attr("dy", "1em")
	      .style("text-anchor", "middle")
	      .style("fill","white")
	      .text("Inches of H2O");      

	// X scale will use the index of our data
	var xScale = d3.scaleLinear()
	    .domain([0, gNPressureReadings-1]) // input
	    .range([0, width]); // output

	// Y scale will use the pressure range from 0 - gMaxPressure 
	var yScale = d3.scaleLinear()
	    .domain([0, gMaxPressure]) // input 
	    .range([height, 0]); // output 

	// d3's line generator
	var line = d3.line()
	    .x(function(d, i) { return xScale(i); }) // set the x values for the line generator
	    .y(function(d) { return yScale(d.y); }) // set the y values for the line generator 
	    .curve(d3.curveMonotoneX); // apply smoothing to the line

	// An array of objects of length N. Each object has key -> value pair, the key being "y" and the value is a pressure reading
	var dataset = d3.range(gNPressureReadings).map(function(d) { return {"y":gPressureReadings[d] } });

	// Append the path, bind the data, and call the line generator 
	svg.append("path")
	    .datum(dataset) // Binds data to the line 
	    .attr("class", "pressureline") // Assign a class for styling 
	    .attr("d", line); // Calls the line generator 
}

//
// Start the pressure graph
//
function startPressureGraph(){

    //console.log("startPressureGraph");

    // Shunt the pressure messages to the graphing routine
    gPressureGraphEnabled = true;

    // Reset the pressure capture
    gCurrentPressure = 0.0;

    // Clear the pressure capture array
    var i;
    gPressureReadings = [];
    for (i=0;i<gNPressureReadings;++i){
    	gPressureReadings.push(0.0);
    }

    // Setup the graph redraw interval timer
    gGraphInterval = setInterval(graphPressure,100);

}

//
// Stop the pressure graph
//
function stopPressureGraph(){

	//console.log("stopPressureGraph");

	// Stop shunting the pressure messages to the graphing routine
    gPressureGraphEnabled = false;

    // Stop the interval timer
    clearInterval(gGraphInterval);

   	$("#d3PressureGraph").empty();

	return;
}