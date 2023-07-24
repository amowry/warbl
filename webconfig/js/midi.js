
// WARBL Web Configuration Tool MIDI Support Routines
//

//debugger;


var mapSelection; //keeps track of which pressure output we're mapping (CC, vel, aftertouch, poly)
var curve = [0,0,0,0]; //which curve is selected for CC, vel, aftertouch, poly
var inputSliderMin = [0,0,0,0]; //settings for sliders
var inputSliderMax = [100,100,100,100];
var outputSliderMin = [0,0,0,0];
var outputSliderMax = [127,127,127,127];
var consoleEntries = 0; //number of lines in MIDI console
var customFingeringFills = [[null,null,null,null,null,null,null,null,null,null,null],[0,74,73,72,71,69,67,66,64,62,61],[0,74,72,72,70,68,67,65,64,62,61],[0,74,74,72,72,69,68,67,65,62,60]];
var communicationMode = false; //if we're communicating with WARBL

var midiNotes = [];

var currentVersion = 21

var midiAccess = null; // the MIDIAccess object.

var WARBLout = null; // WARBL output port

var gExportProxy = null; // Export received message proxy

// For pressure graphing
var gPressureGraphEnabled = false; // When true, proxy pressure messages to graphing code
var gCurrentPressure = 0.0;
var gGraphInterval = null;
var gNPressureReadings = 100;
var gMaxPressure = 25;
var gPressureReadings = [];

var on,off;

var deviceName;

var volume = 0;

var currentNote = 62;

var sensorValue = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];

var buttonRowWrite; //used to indicate which button command row we'll be writing to below.

var jumpFactorWrite; //to indicate which pressure variably is going to be sent or received

var fingeringWrite; //indicates the instrument for which a fingering pattern is being sent.

var version = "Unknown";

var instrument = 0; //currently selected instrument tab

var ping = 0;

var lsb = 0; //used for reassembling pressure bytes from WARBL

var defaultInstr = 0; //default instrument

var modals = new Array();

for (var w = 1; w <= 23; w++) {

	modals[w] = document.getElementById("open-modal" + w);

}

//hide some stuff for app version
if (platform == "app") {
	document.getElementById("myTopnav").style.display = "none";
	document.getElementById("topLogo").style.display = "none";
	document.getElementById("importexport").style.display = "none";
}

// When the user clicks anywhere outside of the modal, close it
window.onclick = function(event) {
	
	for (var i = 1; i <= 23; i++) {

		if (event.target == modals[i]) {

			modalclose(i);

		}
	}
	event.stopPropagation();
}

window.addEventListener('load', function() {
	
	//console.log("window onLoad");

	// Clear the WARBL output port
	WARBLout = null;

	updateCells(); // set selects and radio buttons to initial values and enabled/disabled states
	
	for (var i = 1; i < 13; i++) { //trigger input event for sliders to display values
		var k = document.getElementById("jumpFactor" + (i));
		k.dispatchEvent(new Event('input'));
		if (i == 10) {
			var k = document.getElementById("jumpFactor10b"); //special case for key delay slider
			k.dispatchEvent(new Event('input'));
		}
		
	}
	
	depth.dispatchEvent(new Event('input'));

	$(".volume").click(function() {
		toggleOn();
		$(this).find('i').toggleClass('fa-volume-up fa-volume-off')
	});

	
	if (navigator.requestMIDIAccess)
		navigator.requestMIDIAccess({
			sysex: false
		}).then(onMIDIInit, onMIDIReject);
	else
		alert("Your browser does not support MIDI. Please use Chrome or Opera, or the free WARBL iOS app.")

});

//
// Common function to show the "WARBL not detected" message
//
function showWARBLNotDetected(){

	communicationMode = false; //make sure we know we're not connected
	document.getElementById("status").innerHTML = "WARBL not detected.";
	document.getElementById("status").style.color = "#F78339";
	document.getElementById("version").style.color = "#F78339";
	document.getElementById("connect").innerHTML = "Connect to WARBL";	//make sure the connect button shows the correct text
	document.getElementById("pressure").innerHTML = "";
	for (var m = 0; m < 9; m++) { //turn off toneholes
		document.getElementById("dot" + m).style.backgroundColor = "#333";
		if (m == 0) {
			document.getElementById("dot0").style.opacity = "0";					
		}
	}

	
	// Disable the import preset button
	$('#importPreset').attr('disabled','disabled');

}

//
// Common function to show the "Unknown" version message
//
function showWARBLUnknown(){

	document.getElementById("version").innerHTML = "Unknown";
	document.getElementById("current").style.visibility = "hidden";
	document.getElementById("status").style.visibility = "visible";
	document.getElementById("connect").innerHTML = "Connect to WARBL";	//make sure the connect button shows the correct text
	

	// Disable the import preset button
	$('#importPreset').attr('disabled','disabled');

}

function connect() {
	
	if (communicationMode && version > 2.0) {sendToAll(102, 99); //tell WARBL to exit communications mode if the "connect" button currently reads "Disconnect"
		communicationMode = false;
		showWARBLNotDetected();
		showWARBLUnknown();
		WARBLout = null;
		document.getElementById("connect").innerHTML = "Connect to WARBL";	//make sure the connect button shows the correct text
			if (midiAccess){

		//console.log("connect: Have a midiAccess, clearing the receive callbacks");

		// Walk the inputs and clear any receive callbacks
		var inputs = midiAccess.inputs.values();

		for (var input = inputs.next(); input && !input.done; input = inputs.next()) {

			input.value.onmidimessage = null;

		}
	}

	midiAccess = null;

	// Clear the WARBL output port
	WARBLout = null;

	ping = 0;
	
		}
		
	else{

	//debugger;

	//console.log("connect");
	//alert("connect");
	// Clear the midiAccess object callbacks
	if (midiAccess){

		//console.log("connect: Have a midiAccess, clearing the receive callbacks");

		// Walk the inputs and clear any receive callbacks
		var inputs = midiAccess.inputs.values();

		for (var input = inputs.next(); input && !input.done; input = inputs.next()) {

			input.value.onmidimessage = null;

		}
	}

	midiAccess = null;

	// Clear the WARBL output port
	WARBLout = null;

	ping = 0;

	// Setup initial detection and version messages
	showWARBLNotDetected();
	showWARBLUnknown();

	// If availabe in the browser, request MIDI access with sysex support
	if (navigator.requestMIDIAccess)
		navigator.requestMIDIAccess({
			sysex: false
		}).then(onMIDIInit, onMIDIReject);
	else{
		alert("Your browser does not support MIDI. Please use Chrome or Opera, or the free WARBL iOS app.");
	}
	}

}

//
// Callback when first requesting WebMIDI support
//
function onMIDIInit(midi) {

	//debugger;

	//console.log("onMIDIInit");
//alert("onMiDIInitl");
	// Save off the MIDI access object
	midiAccess = midi;

	// Null the WARBL MIDI output port
	WARBLout = null;

	var foundMIDIInputDevice = false;
	
	// Walk the inputs and see if a WARBL is connected
	var inputs = midiAccess.inputs.values();

	for (var input = inputs.next(); input && !input.done; input = inputs.next()) {

		foundMIDIInputDevice = true;

		deviceName = input.value.name;

		//console.log("deviceName = "+deviceName);
		//alert("deviceName = "+deviceName);
			
		setPing(); //start checking to make sure WARBL is still connected   
		
		input.value.onmidimessage = WARBL_Receive;

	}

	if (foundMIDIInputDevice){
		
		if (!communicationMode || version < 2.1 || version == "Unknown"){

		sendToAll(102, 126); //tell WARBL to enter communications mode
		
		} 
		

	}
	else{

		showWARBLNotDetected();
		
		
	}

	midi.onstatechange = midiOnStateChange;

}

function onMIDIReject(err) {

	alert("The MIDI system failed to start. Please refresh the page.");

}

function midiOnStateChange(event) {
	
	//console.log("state change");

	if (ping == 1) {

		showWARBLNotDetected();
		showWARBLUnknown();
		WARBLout = null;	
		document.getElementById("connect").innerHTML = "Connect to WARBL";	//make sure the connect button shows the correct text

	}

}

function setPing() {

	//change ping to 1 after 3 seconds, after which we'll show a disconnnect if the MIDI state changes.
	setTimeout(function() {

		ping = 1;

	}, 3000);
}

//
// Common message handler for sendToAll and sendToWARBL to build send message
//
function buildMessage(byte2, byte3){

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

	return cc;
}

//
// Send to all connected MIDI devices
//
function sendToAll(byte2, byte3) {

	//console.log("sendToAll");

	var cc = buildMessage(byte2,byte3);

	// Send to all MIDI output ports
	var iter = midiAccess.outputs.values();

	for (var o = iter.next(); !o.done; o = iter.next()) {

		//console.log("o.value.name: "+o.value.name);

		o.value.send(cc, window.performance.now()); //send CC message

	}

}

// Send a command to only the WARBL output port
function sendToWARBL(byte2, byte3) {
	if(communicationMode){
	if (platform = "app") {
		var cc = buildMessage(byte2,byte3);
		var iter = midiAccess.outputs.values();
		for (var o = iter.next(); !o.done; o = iter.next()) {
		o.value.send(cc, window.performance.now()); //send CC message
		
	}}
	
	else {

	//console.log("sendToWARBL");

	// Make sure we have a WARBL output port
		if (!WARBLout){

			console.error("sendToWARBL: No MIDI port selected!")
		
			return;

		}

		var cc = buildMessage(byte2,byte3);

		// Send to only WARBL MIDI output port
		WARBLout.send(cc, window.performance.now()); //send CC message
	}
}
}


function WARBL_Receive(event) {
	
	//debugger;
	
	//alert("received");

	var data0 = event.data[0];
	var data1 = event.data[1];
	var data2 = event.data[2];
	
	//alert(WARBLout);

	// If we're exporting a preset, send the data to the exporter instead
	if (gExportProxy){
		gExportProxy(data0,data1,data2);
		return;
	}

	//console.log("WARBL_Receive");
	//console.log("WARBL_Receive target = "+event.target.name);
	//console.log("WARBL_Receive: "+data0+" "+data1+" "+data2);

	// If we haven't established the WARBL output port and we get a received message on channel 7
	// find the port by name by walking the output ports and matching the input port name
	if ((!WARBLout) && ((data0 & 0x0F) == 6)){
		//alert(data0 & 0x0F);
		
	if (platform == "web"){

		//alert(input.value.name);
	 	var inputName = event.target.name;


	 	// Strip any [] postfix
	 	
	 	var inputBracketIndex = inputName.indexOf("[");

	 	if (inputName.indexOf("[")!=-1){

	 		inputName = inputName.substr(0,inputBracketIndex);
	 		
	 	}

		//console.log("Searching for WARBL output port matching input port name: "+targetName);
//alert("Searching for WARBL output port matching input port name: "+targetName);
		// Send to all MIDI output ports
		var iter = midiAccess.outputs.values();

		for (var o = iter.next(); !o.done; o = iter.next()) {

			var outputName = o.value.name;

		 	// Strip any [] postfix
		 	
		 	var outputBracketIndex = outputName.indexOf("[");

		 	if (outputName.indexOf("[")!=-1){

		 		outputName = outputName.substr(0,outputBracketIndex);

		 	}

			if (outputName == inputName){

				//console.log("Found the matching WARBL output port!")

				WARBLout = o.value;
				
				break;
			}
		}	

		if (!WARBLout){

			//console.error("Failed to find the WARBL output port!")
			
			showWARBLNotDetected();

		} 	
	}
	
	else{ //app version
	WARBLout = 1; //for app version we don't worry about the device name or port, just that it's sending on channel 7.
	}
	
	
	}

	var e;
	var f;
	var g;
	
	if (isNaN(data2)) {
		f = " ";
	} else {
		f = data2;
	}

	if ((data0 & 0xf0) == 144) {
		e = "On";
	} //update the MIDI console
	if ((data0 & 0xf0) == 128) {
		e = "Off";
	}
	if ((data0 & 0xf0) == 176) {
		e = "CC";
	}
	if ((data0 & 0xf0) == 224) {
		e = "PB";
	}
	if ((data0 & 0xf0) == 192) {
		e = "PC";
	}
	if ((data0 & 0xf0) == 208) {
		e = "CP";
	}	
	if ((data0 & 0xf0) == 160) {
		e = "KP";
	}
	
	
	
	if (!(e == "CC" && ((parseFloat(data0 & 0x0f) == 6)))) { //as long as it's not a CC on channel 7, show in the MIDI console.
		consoleEntries++;
			if(consoleEntries < 301){
				document.getElementById("console").innerHTML += (e + " " + ((data0 & 0x0f) + 1) + " " + data1 + " " + f);
				document.getElementById("console").innerHTML += "<br>";	 
			}
			else if (consoleEntries == 301) {document.getElementById("console").innerHTML += "max lines reached";
			}
			var elem2 = document.getElementById("console");
		elem2.scrollTop = elem2.scrollHeight;	
	}

	// Mask off the lower nibble (MIDI channel, which we don't care about yet)
	switch (data0 & 0xf0) {
		case 0x90:
			if (data2 != 0) { // if velocity != 0, this is a note-on message
				noteOn(data1);
				logKeys;
				return;
			}
			// if velocity == 0, fall thru: it's a note-off.
		case 0x80:
			noteOff(data1);
			logKeys;
			return;

		case 0xB0: //incoming CC from WARBL

			if (parseFloat(data0 & 0x0f) == 6) { //if it's channel 7 it's from WARBL 



				// Enable the import preset button
				$('#importPreset').removeAttr('disabled')

				//setPing(); //start checking to make sure WARBL is still connected

				if (data1 == 115) { //hole covered info from WARBL

					var byteA = data2;
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

				if (data1 == 114) { //hole covered info from WARBL
					for (var n = 0; n < 2; n++) {
						var byteB = data2;
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
				} else if (data1 == 102) { //parse based on received CC
					if (data2 > 19 && data2 < 30) {
						document.getElementById("v" + (data2 - 19)).innerHTML = "MAX"; //set sensor value field to max if message is received from WARBL
						checkMax((data2 - 19));
					}

					for (var i = 0; i < 3; i++) { // update the three selected fingering patterns if prompted by the tool.
						if (data2 == 30 + i) {
							fingeringWrite = i;
						}

						if (data2 > 32 && data2 < 60) {
							if (fingeringWrite == i) {
								document.getElementById("fingeringSelect" + i).value = data2 - 33;
							}
							updateCells();
						} //update any dependant fields	
					}



					if (data2 == 60) {
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
						okayCCmap();
						okayOverride();
						advancedOkayPB();
						customFingeringOkay();
					}
					if (data2 == 61) {
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
						advancedOkayPB();
						okayCCmap();
						okayOverride();
						handleDefault();
						customFingeringOkay();
					}
					if (data2 == 62) {
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
						advancedOkayPB();
						okayCCmap();
						okayOverride();
						handleDefault();
						customFingeringOkay();
					}


					if (data2 == 85) { //receive and handle default instrument settings
						defaultInstr = 0;
						handleDefault();
					}
					if (data2 == 86) {
						defaultInstr = 1;
						handleDefault();
					}
					if (data2 == 87) {
						defaultInstr = 2;
						handleDefault();
					}

					if (data2 == 70) {
						document.getElementById("pitchbendradio0").checked = true;
						updateCustom();
						updateCustom();
					}
					if (data2 == 71) {
						document.getElementById("pitchbendradio1").checked = true;
						updateCustom();
						updateCustom();
					}
					if (data2 == 72) {
						document.getElementById("pitchbendradio2").checked = true;
						updateCustom();
						updateCustom();
					}
					if (data2 == 73) {
						document.getElementById("pitchbendradio3").checked = true;
						updateCustom();
						updateCustom();
					}

					if (data2 == 80) {
						document.getElementById("sensorradio0").checked = true;
					}
					if (data2 == 81) {
						document.getElementById("sensorradio1").checked = true;
					}
					if (data2 == 82) {
						document.getElementById("sensorradio2").checked = true;
					}
					if (data2 == 83) {
						document.getElementById("sensorradio3").checked = true;
					}

					if (data2 == 121) {
						document.getElementById("bellSensor").style.opacity = 1;
						document.getElementById("1").disabled = false;
						document.getElementById("2").disabled = false;
						document.getElementById("v1").classList.add("sensorValueEnabled");

					}
					if (data2 == 120) {
						document.getElementById("bellSensor").style.opacity = 0.1;
						document.getElementById("1").disabled = true;
						document.getElementById("2").disabled = true;
						document.getElementById("v1").classList.remove("sensorValueEnabled");
					}

					for (var i = 0; i < 8; i++) { //update button configuration	   
						if (data2 == 90 + i) {
							buttonRowWrite = i;
						}
					}

					for (var j = 0; j < 8; j++) { //update button configuration	
						if (buttonRowWrite == j) {

							if (data2 == 119) { //special case for initiating autocalibration
								document.getElementById("row" + (buttonRowWrite)).value = 19;
							}

							for (var k = 0; k < 12; k++) {
								if (data2 == (100 + k)) {
									document.getElementById("row" + (buttonRowWrite)).value = k;
								}
								if (k < 5 && data2 == 112 + k) {
									document.getElementById("MIDIrow" + (buttonRowWrite)).value = k;
									updateCells();
								}
							}
						}
					} //update any dependant fields}						


					for (var l = 0; l < 3; l++) { //update momentary switches
						if (buttonRowWrite == l) {
							if (data2 == 117) {
								document.getElementById("checkbox" + l).checked = false;
							}
							if (data2 == 118) {
								document.getElementById("checkbox" + l).checked = true;
							}

						}
					}

				} else if (data1 == 103) {
					document.getElementById("senseDistance").value = 0 - data2;
				} //set sensedistance  
				else if (data1 == 117) {
					document.getElementById("depth").value = data2 + 1;
					var output = document.getElementById("demo14");
					depth.dispatchEvent(new Event('input'));
					output.innerHTML = data2 + 1;
				} //set vibrato depth  
				else if (data1 == 104) {
					jumpFactorWrite = data2;
				} // so we know which pressure setting is going to be received.
				else if (data1 == 105 && jumpFactorWrite < 13) {	
					document.getElementById("jumpFactor" + jumpFactorWrite).value = data2;
					if (jumpFactorWrite == 10) {
						document.getElementById("jumpFactor10b").value = data2; //special case for hysteresis			
					}
					for (var i = 1; i < 13; i++) {
						var k = document.getElementById("jumpFactor" + (i));		
						k.dispatchEvent(new Event('input'));
						if (i == 10) {
							var k = document.getElementById("jumpFactor10b"); //special case for key delay slider
							k.dispatchEvent(new Event('input'));
						}
					}
				}

				if (data1 == 105) {

					if (jumpFactorWrite == 13) {
						document.getElementById("checkbox6").checked = data2;
					} else if (jumpFactorWrite == 14) {
						document.getElementById("expressionDepth").value = data2;
					} else if (jumpFactorWrite == 15) {
						document.getElementById("checkbox7").checked = data2;
						updateCustom();
					} else if (jumpFactorWrite == 16) {
						curve[0] = data2;
					} else if (jumpFactorWrite == 17) {
						document.getElementById("pressureChannel").value = data2;
					} else if (jumpFactorWrite == 18) {
						document.getElementById("pressureCC").value = data2;
					} else if (jumpFactorWrite == 19) {
						inputSliderMin[0] = data2;
						slider.noUiSlider.set([data2, null]);
					} else if (jumpFactorWrite == 20) {
						inputSliderMax[0] = data2;
						slider.noUiSlider.set([null, data2]);
					} else if (jumpFactorWrite == 21) {
						outputSliderMin[0] = data2;
						slider2.noUiSlider.set([data2, null]);
					} else if (jumpFactorWrite == 22) {
						outputSliderMax[0] = data2;
						slider2.noUiSlider.set([null, data2]);
					} else if (jumpFactorWrite == 23) {
						document.getElementById("dronesOnCommand").value = data2;
					} else if (jumpFactorWrite == 24) {
						document.getElementById("dronesOnChannel").value = data2;
					} else if (jumpFactorWrite == 25) {
						document.getElementById("dronesOnByte2").value = data2;
					} else if (jumpFactorWrite == 26) {
						document.getElementById("dronesOnByte3").value = data2;
					} else if (jumpFactorWrite == 27) {
						document.getElementById("dronesOffCommand").value = data2;
					} else if (jumpFactorWrite == 28) {
						document.getElementById("dronesOffChannel").value = data2;
					} else if (jumpFactorWrite == 29) {
						document.getElementById("dronesOffByte2").value = data2;
					} else if (jumpFactorWrite == 30) {
						document.getElementById("dronesOffByte3").value = data2;
					} else if (jumpFactorWrite == 31 && data2 == 0) {
						document.getElementById("dronesRadio0").checked = true;
					} else if (jumpFactorWrite == 31 && data2 == 1) {
						document.getElementById("dronesRadio1").checked = true;
					} else if (jumpFactorWrite == 31 && data2 == 2) {
						document.getElementById("dronesRadio2").checked = true;
					} else if (jumpFactorWrite == 31 && data2 == 3) {
						document.getElementById("dronesRadio3").checked = true;
					} else if (jumpFactorWrite == 32) {
						lsb = data2;
					} else if (jumpFactorWrite == 33) {
						var x = parseInt((data2 << 7) | lsb); //receive pressure between 100 and 900
						x = (x - 100) * 24 / 900; //convert to inches of water. 100 is the approximate minimum sensor value.
						var p = x.toFixed(1); //round to 1 decimal
						p = Math.min(Math.max(p, 0), 24); //constrain
						document.getElementById("dronesPressureInput").value = p;
					} else if (jumpFactorWrite == 34) {
						lsb = data2;

					} else if (jumpFactorWrite == 35) {
						var x = parseInt((data2 << 7) | lsb); //receive pressure between 100 and 900
						x = (x - 100) * 24 / 900; //convert to inches of water.  100 is the approximate minimum sensor value.
						var p = x.toFixed(1); //round to 1 decimal
						p = Math.min(Math.max(p, 0), 24); //constrain
						document.getElementById("octavePressureInput").value = p;
					} else if (jumpFactorWrite == 43) {
						document.getElementById("checkbox9").checked = data2;
					} //invert											
					else if (jumpFactorWrite == 44) {
						document.getElementById("checkbox5").checked = data2; //custom
						updateCustom();
					} else if (jumpFactorWrite == 42) {
						document.getElementById("checkbox3").checked = data2;
					} //secret										
					else if (jumpFactorWrite == 40) {
						updateSelected();
						document.getElementById("checkbox4").checked = data2;
						if (data2 == 0) {
							document.getElementById("bagBreath0").checked = true;
						} else {document.getElementById("bagBreath1").checked = true;}
					} //vented							 			
					else if (jumpFactorWrite == 41) {
						document.getElementById("checkbox8").checked = data2;
					} //bagless						 	
					else if (jumpFactorWrite == 45) {
						document.getElementById("checkbox10").checked = data2;
					} //velocity	
					else if (jumpFactorWrite == 46) {						
						document.getElementById("checkbox11").checked = (data2 & 0x1);
						document.getElementById("checkbox13").checked = (data2 & 0x2);
					} //aftertouch	
					else if (jumpFactorWrite == 47) {
						document.getElementById("checkbox12").checked = data2;
					} //force max velocity
					
					else if (jumpFactorWrite == 48) {
						document.getElementById("checkbox14").checked = data2;
					} //immediate pitchbend
					
					else if (jumpFactorWrite == 49) {
						document.getElementById("checkbox15").checked = data2;
					} //legato
	
					else if (jumpFactorWrite == 50) {
						document.getElementById("checkbox16").checked = data2;
					} //override expression pressure range
					
					else if (jumpFactorWrite == 51) {
						document.getElementById("checkbox17").checked = data2;
					} //both thumb and overblow
					
					else if (jumpFactorWrite == 52) {
						document.getElementById("checkbox18").checked = data2;
						updateCustom();
					} //R4 flattens
					
					else if (jumpFactorWrite == 53) {
						document.getElementById("checkbox19").checked = data2;
						updateCustom();
					} //R4 Invert
						
					else if (jumpFactorWrite == 61) {
						document.getElementById("midiBendRange").value = data2;
					}
					else if (jumpFactorWrite == 62) {
						document.getElementById("noteChannel").value = data2;
					}
					else if (jumpFactorWrite == 70) {
						inputSliderMin[1] = data2;
						slider.noUiSlider.set([data2, null]);
					}
					else if (jumpFactorWrite == 71) {
						inputSliderMax[1] = data2;
						slider.noUiSlider.set([null, data2]);
					}
					else if (jumpFactorWrite == 72) {
						outputSliderMin[1] = data2;
						slider2.noUiSlider.set([data2, null]);
					}
					else if (jumpFactorWrite == 73) {
						outputSliderMax[1] = data2;
						slider2.noUiSlider.set([null, data2]);
					}
					else if (jumpFactorWrite == 74) {
						inputSliderMin[2] = data2;
						slider.noUiSlider.set([data2, null]);
					}
					else if (jumpFactorWrite == 75) {
						inputSliderMax[2] = data2;
						slider.noUiSlider.set([null, data2]);
					}
					else if (jumpFactorWrite == 76) {
						outputSliderMin[2] = data2;
						slider2.noUiSlider.set([data2, null]);
					}
					else if (jumpFactorWrite == 77) {
						outputSliderMax[2] = data2;
						slider2.noUiSlider.set([null, data2]);
					}
					else if (jumpFactorWrite == 78) {
						inputSliderMin[3] = data2;
						slider.noUiSlider.set([data2, null]);
					}
					else if (jumpFactorWrite == 79) {
						inputSliderMax[3] = data2;
						slider.noUiSlider.set([null, data2]);
					}
					else if (jumpFactorWrite == 80) {
						outputSliderMin[3] = data2;
						slider2.noUiSlider.set([data2, null]);
					}
					else if (jumpFactorWrite == 81) {
						outputSliderMax[3] = data2;
						slider2.noUiSlider.set([null, data2]);
					}
					else if (jumpFactorWrite == 85) {
						slider3.noUiSlider.set([data2, null]);
					}
					else if (jumpFactorWrite == 86) {
						slider3.noUiSlider.set([null, data2]);
					}
					else if (jumpFactorWrite == 82) {
						curve[1] = data2;
					}
					else if (jumpFactorWrite == 83) {
						curve[2] = data2;
					}
					else if (jumpFactorWrite == 84) {
						curve[3] = data2;
					}
					
					else if (jumpFactorWrite > 86 && jumpFactorWrite < 98) { //custom fingering chart inputs
						document.getElementById("fingeringInput" + (jumpFactorWrite - 86)).value = data2;								
					}
					
				}
 	
				else if (data1 == 110) { //receiving firmware version from WARBL
				

					version = data2;
					
					if (version == currentVersion) { //display the appropriate messages
						document.getElementById("current").innerHTML = "Your firmware is up to date.";
						document.getElementById("current").style.left = "710px";
						document.getElementById("current").style.visibility = "visible";
						document.getElementById("status").style.visibility = "hidden";
						document.getElementById("current").style.color = "#f7c839";
					}
					else {
						document.getElementById("current").innerHTML = "There is a firmware update available.";				
						document.getElementById("current").style.left = "690px";
						document.getElementById("current").style.visibility = "visible";
						document.getElementById("status").style.visibility = "hidden";
						document.getElementById("current").style.color = "#F78339";
					}
					
					
					version = version / 10;
					var n = version.toFixed(1)
					document.getElementById("version").innerHTML = n;
					document.getElementById("version").style.color = "#f7c839";
					
					
				//show that the WARBL is connected when we receive the firmware version.
				document.getElementById("status").innerHTML = "WARBL Connected.";
				document.getElementById("status").style.color = "#f7c839";
				communicationMode = true;
				if(version > 2.0){
				document.getElementById("connect").innerHTML = "Disconnect";
				}
					
					
					//Lots of UI changes based on the current firmware version of the connected WARBL
					
						if (version < 2.1) {
						document.getElementById("jumpfactor10Container").style.visibility = "visible";
						document.getElementById("jumpfactor4Container").style.visibility = "visible";
						document.getElementById("jumpValue4").style.visibility = "visible";
						document.getElementById("jumpValue10").style.visibility = "visible";				
						document.getElementById("jumpfactor10bContainer").style.visibility = "hidden";
						document.getElementById("jumpLabel8").style.visibility = "hidden";
						document.getElementById("jumpLabel5").style.visibility = "visible";									
						document.getElementById("demo10b").style.visibility = "hidden";				
						document.getElementById("jumpLabel4").innerHTML = "Jump";
						document.getElementById("jumpfactor5Container").style.top = "240px";											
						document.getElementById("jumpfactor6Container").style.top = "275px";						
						document.getElementById("jumpfactor11Container").style.top = "240px";											
						document.getElementById("jumpfactor12Container").style.top = "275px";														
						document.getElementById("jumplabel6").style.top = "242px";						
						document.getElementById("jumplabel7").style.top = "277px";												
						document.getElementById("jumpValue5").style.top = "225px";						
						document.getElementById("jumpValue6").style.top = "260px";												
						document.getElementById("jumpValue11").style.top = "225px";						
						document.getElementById("jumpValue12").style.top = "260px";
						document.getElementById("jumpFactor3").min = "1";										
						document.getElementById("jumpFactor3").max = "175";		
						document.getElementById("jumpFactor9").min = "1";										
						document.getElementById("jumpFactor9").max = "175";	
						document.getElementById("jumpFactor5").min = "1";											
						document.getElementById("jumpFactor6").min = "1";											
						document.getElementById("jumpFactor11").min = "1";												
						document.getElementById("jumpFactor12").min = "1";										
						document.getElementById("breathSlider").classList.remove('slider2');
						document.getElementById("breathSlider").classList.add('slider');
						document.getElementById("pressureGroup").style.top = "0px";
						document.getElementById("ventedLabel").style.visibility = "visible";
						document.getElementById("switch4").style.visibility = "visible";
						document.getElementById("radioGroup10").style.visibility = "hidden";
						document.getElementById("jumplabel10").innerHTML = "Closed";
						document.getElementById("jumplabel11").innerHTML = "Vented";
						document.getElementById("jumplabel10").style.left = "170px";
						document.getElementById("jumplabel11").style.left = "340px";
						document.getElementById("advancedSliders").style.top = "-125px";
						document.getElementById("selectedSettings").style.visibility = "hidden";
							

											
						
						//disable bombarde and baroque flute fingering option
						document.getElementById("fingeringSelect0").options[19].disabled = true;
						document.getElementById("fingeringSelect1").options[19].disabled = true;
						document.getElementById("fingeringSelect2").options[19].disabled = true;
						document.getElementById("fingeringSelect0").options[20].disabled = true;
						document.getElementById("fingeringSelect1").options[20].disabled = true;
						document.getElementById("fingeringSelect2").options[20].disabled = true;
																																
					}
					
					

					if (version < 2.0) {
						document.getElementById("overrideExpression").disabled = true;
						document.getElementById("polyMapButton").disabled = true;
						document.getElementById("velocityMapButton").disabled = true;
						document.getElementById("aftertouchMapButton").disabled = true;
						document.getElementById("noteChannel").disabled = true;
						document.getElementById("checkbox13").disabled = true;
						document.getElementById("switch13").style.cursor = "default";
						document.getElementById("advancedPBbutton").disabled = true;
						document.getElementById("pitchbendradio3").disabled = true;
						document.getElementById("configureCustomFingering").disabled = true;
					}
					
					

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
					
					
					if (version < 2.0) {
						document.getElementById("fingeringSelect0").options[16].disabled = true;
						document.getElementById("fingeringSelect1").options[16].disabled = true;
						document.getElementById("fingeringSelect2").options[16].disabled = true;
												
						document.getElementById("fingeringSelect0").options[17].disabled = true;
						document.getElementById("fingeringSelect1").options[17].disabled = true;
						document.getElementById("fingeringSelect2").options[17].disabled = true;
						
						document.getElementById("fingeringSelect0").options[18].disabled = true;
						document.getElementById("fingeringSelect1").options[18].disabled = true;
						document.getElementById("fingeringSelect2").options[18].disabled = true;
					}
					
				
					

				} else if (data1 == 111) {
					document.getElementById("keySelect0").value = data2;
				} else if (data1 == 112) {
					document.getElementById("keySelect1").value = data2;
				} else if (data1 == 113) {
					document.getElementById("keySelect2").value = data2;
				} else if (data1 == 116) {
					lsb = data2;
				} else if (data1 == 118) {
					var x = parseInt((data2 << 7) | lsb); //receive pressure between 100 and 900
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
					}
				}

				for (var i = 0; i < 8; i++) {
					if (buttonRowWrite == i) {
						if (data1 == 106 && data2 < 16) {
							document.getElementById("channel" + (buttonRowWrite)).value = data2;
						}
						if (data1 == 107) {
							document.getElementById("byte2_" + (buttonRowWrite)).value = data2;
						}
						if (data1 == 108) {
							document.getElementById("byte3_" + (buttonRowWrite)).value = data2;
						}
					}
				}

				if (data1 == 106 && data2 > 15) {

					if (data2 > 19 && data2 < 29) {
						document.getElementById("vibratoCheckbox" + (data2 - 20)).checked = false;
					}

					if (data2 > 29 && data2 < 39) {
						document.getElementById("vibratoCheckbox" + (data2 - 30)).checked = true;
					}

					if (data2 == 39) {
						document.getElementById("calibrateradio0").checked = true;
					}
					if (data2 == 40) {
						document.getElementById("calibrateradio1").checked = true;
					}

					if (data2 == 53) { //add an option for the uilleann regulators fingering pattern if a message is received indicating that it is supported.

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


function sendCustomFingeringFill(){
	
	modalclose(22);	
	var value = document.getElementById("customFingeringFill").value;
	for (i = 1; i < 12; ++i) {
		document.getElementById("fingeringInput" + i).value = customFingeringFills[value][i - 1];
		fingeringInput(i, customFingeringFills[value][i - 1]);
	}
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
	else if (selection == 17 || selection == 18) {
		key = 2;
	} //Sackpipa
		else if (selection == 20) {
		key = 123;
	} //Bombarde
	else {
		key = 0;
	} //default key of D for many patterns
	document.getElementById("keySelect" + row).value = key; //set key menu

	//send the fingering pattern
	sendToWARBL(102, 30 + row);
	sendToWARBL(102, 33 + selection);

	sendKey(row, key);
}


function fingeringInput(input, selection) { 	//send the custom fingering input entry

		var x = document.getElementById("fingeringInput" + input).value;
		if (x < 0 || x > 127 || isNaN(x)) {
			alert("Value must be 0-127.");
			document.getElementById("fingeringInput" + input).value = null;
		}		
		else{
	blink(1);
	document.getElementById("customFingeringFill").value = "12";
	sendToWARBL(104, 86 + input);
	sendToWARBL(105, selection);
		}
}


function defaultInstrument() { //tell WARBL to set the current instrument as the default

	defaultInstr = instrument;
	handleDefault();
	sendToWARBL(102, 85);

}

function handleDefault() { //display correct default instrument and "set default" buttons	

	if (version > 1.6 || version == "Unknown") {

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
	sendToWARBL(111 + row, selection);
}

function sendFingeringRadio(tab) { //change instruments, showing the correct tab for each instrument.

	instrument = tab;
	updateCustom();
	advancedOkay(); //turn off the advanced tab
	pressureOkay();	
	handleDefault(); //display correct default instrument and "set default" buttons	
	customFingeringOkay();
	if (tab == 0) {
		document.getElementById("instrument0").style.display = "block";
		document.getElementById("instrument1").style.display = "none";
		document.getElementById("instrument2").style.display = "none";

		document.getElementById("key0").style.display = "block";
		document.getElementById("key1").style.display = "none";
		document.getElementById("key2").style.display = "none";
		sendToWARBL(102, 60);
	} else if (tab == 1) {
		document.getElementById("instrument0").style.display = "none";
		document.getElementById("instrument1").style.display = "block";
		document.getElementById("instrument2").style.display = "none";

		document.getElementById("key0").style.display = "none";
		document.getElementById("key1").style.display = "block";
		document.getElementById("key2").style.display = "none";
		blink(2);
		sendToWARBL(102, 61);
	} else if (tab == 2) {
		document.getElementById("instrument0").style.display = "none";
		document.getElementById("instrument1").style.display = "none";
		document.getElementById("instrument2").style.display = "block";

		document.getElementById("key0").style.display = "none";
		document.getElementById("key1").style.display = "none";
		document.getElementById("key2").style.display = "block";
		blink(3);
		sendToWARBL(102, 62);
	}
	updateCells();
	updateCustom();
}


//move selection box for advanced overblowing settings based on whether bag or breath is selected
function updateSelected() {
	if (document.getElementById("bagBreath0").checked == true) {
		document.getElementById("selectedSettings").style.left = "125px";
		document.getElementById("selectedSettings").style.width = "162px";
	} else {
		document.getElementById("selectedSettings").style.left = "285px";
		document.getElementById("selectedSettings").style.width = "162px";	
	}
}

						

function sendSenseDistance(selection) {
	blink(1);
	selection = parseFloat(selection);
	x = 0 - parseFloat(selection);
	sendToWARBL(103, x);
}


function sendDepth(selection) {
	blink(1);
	selection = parseFloat(selection);
	sendToWARBL(117, selection);
}

function sendExpressionDepth(selection) {
	blink(1);
	selection = parseFloat(selection);
	sendToWARBL(104, 14);
	sendToWARBL(105, selection);
}

function sendCurveRadio(selection) {
	blink(1);
	selection = parseFloat(selection);
	curve[mapSelection] = selection;
	if(mapSelection == 0){
	sendToWARBL(104, 16);
	}
	else{
		sendToWARBL(104, 81 + mapSelection);
	}
	sendToWARBL(105, selection);
}

function sendPressureChannel(selection) {
	blink(1);
	var x = parseFloat(selection);
	if (x < 1 || x > 16 || isNaN(x)) {
		alert("Value must be 1-16.");
		document.getElementById("pressureChannel").value = null;
	} else {
		sendToWARBL(104, 17);
		sendToWARBL(105, x);
	}
}

function sendPressureCC(selection) {
	blink(1);
	selection = parseFloat(selection);
	sendToWARBL(104, 18);
	sendToWARBL(105, selection);
}

function sendBendRange(selection) {
	blink(1);
	var x = parseFloat(selection);
	if (x < 1 || x > 96 || isNaN(x)) {
		alert("Value must be 1-96.");
		document.getElementById("midiBendRange").value = null;
	} else {
		sendToWARBL(104, 61);
		sendToWARBL(105, x);
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
	if(mapSelection == 0){
	inputSliderMin[0] = parseInt(values[0]);
	inputSliderMax[0] = parseInt(values[1]);	
	sendToWARBL(104, 19);
	sendToWARBL(105, parseInt(values[0]));
	sendToWARBL(104, 20);
	sendToWARBL(105, parseInt(values[1]));
	}
	else {
	inputSliderMin[mapSelection] = parseInt(values[0]);
	inputSliderMax[mapSelection] = parseInt(values[1]);
	sendToWARBL(104, 70 + ((mapSelection-1)*4));
	sendToWARBL(105, parseInt(values[0]));
	sendToWARBL(104, 71 + ((mapSelection-1)*4));
	sendToWARBL(105, parseInt(values[1]));		
	}
});

//pressure output slider
slider2.noUiSlider.on('change', function(values) {
	blink(1);
	if(mapSelection == 0){
	outputSliderMin[0] = parseInt(values[0]);
	outputSliderMax[0] = parseInt(values[1]);
	sendToWARBL(104, 21);
	sendToWARBL(105, parseInt(values[0]));
	sendToWARBL(104, 22);
	sendToWARBL(105, parseInt(values[1]));
	}
	else {
	outputSliderMin[mapSelection] = parseInt(values[0]);
	outputSliderMax[mapSelection] = parseInt(values[1]);
	sendToWARBL(104, 72 + ((mapSelection-1)*4));
	sendToWARBL(105, parseInt(values[0]));
	sendToWARBL(104, 73 + ((mapSelection-1)*4));
	sendToWARBL(105, parseInt(values[1]));		
	}
});

//expression override slider
slider3.noUiSlider.on('change', function(values) {
	blink(1);
	sendToWARBL(104, 85);
	sendToWARBL(105, parseInt(values[0]));
	sendToWARBL(104, 86);
	sendToWARBL(105, parseInt(values[1]));
});


slider.noUiSlider.on('update', function (values, handle) {
	var marginMin = document.getElementById('slider-value-min'),
    marginMax = document.getElementById('slider-value-max');

    if (handle) {
			var min = parseFloat(values[handle] * 0.24).toFixed(1);
        marginMax.innerHTML = min;
    } else {
			var max = parseFloat(values[handle] * 0.24).toFixed(1);
        marginMin.innerHTML = max;
    }

});

slider2.noUiSlider.on('update', function (values, handle) {
	var marginMin = document.getElementById('slider2-value-min'),
    marginMax = document.getElementById('slider2-value-max');
    if (handle) {
        marginMax.innerHTML = parseInt(values[handle]);
    } else {
        marginMin.innerHTML = parseInt(values[handle]);
    }
});

slider3.noUiSlider.on('update', function (values, handle) {
	var marginMin = document.getElementById('slider3-value-min'),
    marginMax = document.getElementById('slider3-value-max');

    if (handle) {
			var min = parseFloat(values[handle] * 0.24).toFixed(1);
        marginMax.innerHTML = min;
    } else {
			var max = parseFloat(values[handle] * 0.24).toFixed(1);
        marginMin.innerHTML = max;
    }

});


function sendDronesOnCommand(selection) {
	blink(1);
	selection = parseFloat(selection);
	sendToWARBL(104, 23);
	sendToWARBL(105, selection);
}

function sendDronesOnChannel(selection) {
	var x = parseFloat(selection);
	if (x < 0 || x > 16 || isNaN(x)) {
		alert("Value must be 1-16.");
		document.getElementById("dronesOnChannel").value = null;
	} else {
		blink(1);
		sendToWARBL(104, 24);
		sendToWARBL(105, x);
	}
}

function sendDronesOnByte2(selection) {
	var x = parseFloat(selection);
	if (x < 0 || x > 127 || isNaN(x)) {
		alert("Value must be 0 - 127.");
		document.getElementById("dronesOnByte2").value = null;
	} else {
		blink(1);
		sendToWARBL(104, 25);
		sendToWARBL(105, x);
	}
}

function sendDronesOnByte3(selection) {
	var x = parseFloat(selection);
	if (x < 0 || x > 127 || isNaN(x)) {
		alert("Value must be 0 - 127.");
		document.getElementById("dronesOnByte3").value = null;
	} else {
		blink(1);
		sendToWARBL(104, 26);
		sendToWARBL(105, x);
	}
}

function sendDronesOffCommand(selection) {
	blink(1);
	var y = parseFloat(selection);
	sendToWARBL(104, 27);
	sendToWARBL(105, y);
}

function sendDronesOffChannel(selection) {
	var x = parseFloat(selection);
	if (x < 0 || x > 16 || isNaN(x)) {
		alert("Value must be 1-16.");
		document.getElementById("dronesOffChannel").value = null;
	} else {
		blink(1);
		sendToWARBL(104, 28);
		sendToWARBL(105, x);
	}
}

function sendDronesOffByte2(selection) {
	var x = parseFloat(selection);
	if (x < 0 || x > 127 || isNaN(x)) {
		alert("Value must be 0 - 127.");
		document.getElementById("dronesOffByte2").value = null;
	} else {
		blink(1);
		sendToWARBL(104, 29);
		sendToWARBL(105, x);
	}
}

function sendDronesOffByte3(selection) {
	var x = parseFloat(selection);
	if (x < 0 || x > 127 || isNaN(x)) {
		alert("Value must be 0 - 127.");
		document.getElementById("dronesOffByte3").value = null;
	} else {
		blink(1);
		sendToWARBL(104, 30);
		sendToWARBL(105, x);
	}
}

function sendDronesRadio(selection) {
	blink(1);
	selection = parseFloat(selection);
	sendToWARBL(104, 31);
	sendToWARBL(105, selection);
}

function learnDrones() {
	blink(1);
	document.getElementById("dronesPressureInput").style.backgroundColor = "#32CD32";
	setTimeout(blinkDrones, 500);
	sendToWARBL(106, 43);

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
		sendToWARBL(104, 32);
		sendToWARBL(105, x & 0x7F);
		sendToWARBL(104, 33);
		sendToWARBL(105, x >> 7);
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
		sendToWARBL(104, 34);
		sendToWARBL(105, x & 0x7F);
		sendToWARBL(104, 35);
		sendToWARBL(105, x >> 7);
	}

}

function learn() {
	blink(1);
	document.getElementById("octavePressureInput").style.backgroundColor = "#32CD32";
	setTimeout(blinkOctave, 500);
	sendToWARBL(106, 41);
}

function sendCalibrateRadio(selection) {
	selection = parseFloat(selection);
	blink(1);
	sendToWARBL(106, 39 + selection);
}

function sendPitchbendRadio(selection) {
	updateCustom();
	updateCustom();
	selection = parseFloat(selection);
	if (selection > 0) {
		blink(selection + 1);
	}
	sendToWARBL(102, 70 + selection);
}

function sendVibratoHoles(holeNumber, selection) {
	selection = +selection; //convert true/false to 1/0
	sendToWARBL(106, (30 - (selection * 10)) + holeNumber);
}

function updateCustom() { //keep correct settings enabled/disabled with respect to the custom vibrato switch and send pressure as CC switches.


	var a = document.getElementById("fingeringSelect" + instrument).value;

	if (document.getElementById("pitchbendradio2").checked == false && (((a == 0 || a == 1 || a == 4 || a == 10) && ((document.getElementById("pitchbendradio1").checked == true && version < 1.6) || (version == "Unknown" || version > 1.5))) || (version == "Unknown" || version > 1.5) && (a == 3 || a == 2))) {
		document.getElementById("checkbox5").disabled = false;
		document.getElementById("switch5").style.cursor = "pointer";
	} else {
		//document.getElementById("checkbox5").checked = false;
		document.getElementById("checkbox5").disabled = true;
		document.getElementById("switch5").style.cursor = "default";
		//sendToWARBL(104,44); //if custom is disabled, tell WARBL to turn it off
		//sendToWARBL(105, 0);
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
	
		if (document.getElementById("checkbox18").checked == true) {
			document.getElementById("fingeringInput11").disabled = true;
			document.getElementById("fingeringInput11").style.cursor = "default";
		
	} else {
	
			document.getElementById("fingeringInput11").disabled = false;
			document.getElementById("fingeringInput11").style.cursor = "pointer";
		
	}
}

function sendBreathmodeRadio(selection) {
	selection = parseFloat(selection);
	if (selection > 0) {
		blink(selection + 1);
	}
	if(document.getElementById("sensorradio1").checked == true) { //update override
		document.getElementById("checkbox16").disabled = true;
	}
	else {
		document.getElementById("checkbox16").disabled = false;
	}
	sendToWARBL(102, 80 + selection);

}

function advanced() {
	document.getElementById("box2").style.display = "block";
	document.getElementById("box1").style.display = "none";
	updateSelected();
}

function advancedOkay() {
	document.getElementById("box2").style.display = "none";
	document.getElementById("box1").style.display = "block";
}

function advancedPB() {
	if(version > 1.8 || version == "Unknown"){
		document.getElementById("box4").style.display = "block";
		document.getElementById("box5").style.display = "none";
	}
}


function overRideExpression() {
	if(version > 1.8 || version == "Unknown"){
		if(document.getElementById("sensorradio1").checked == true) {
			document.getElementById("checkbox16").disabled = true;
		}
		else {
			document.getElementById("checkbox16").disabled = false;
		}
		document.getElementById("box8").style.display = "block";
		document.getElementById("box6").style.display = "none";
	}
}

function okayOverride() {
	document.getElementById("box8").style.display = "none";
	document.getElementById("box6").style.display = "block";
}


function advancedOkayPB() {
	document.getElementById("box4").style.display = "none";
	document.getElementById("box5").style.display = "block";
}

function configureCustomFingering() {
	document.getElementById("topControls").style.display = "none";
	document.getElementById("customControls").style.display = "block";
	document.getElementById("box1").style.top = "740px";
	document.getElementById("box2").style.top = "740px";
	document.getElementById("box4").style.top = "740px";
	document.getElementById("box5").style.top = "740px";
	document.getElementById("pressuregraph").style.top = "740px";
	document.getElementById("box3").style.top = "1200px";
	document.getElementById("box6").style.top = "1200px";
	document.getElementById("box7").style.top = "1200px";
	document.getElementById("box8").style.top = "1200px";
	document.getElementById("buttonBox").style.top = "1690px";
	document.getElementById("topcontrolbox").style.height = "2085px";

}

function customFingeringOkay() {
	document.getElementById("customControls").style.display = "none";
	document.getElementById("topControls").style.display = "block";
	document.getElementById("box1").style.top = "440px";
	document.getElementById("box2").style.top = "440px";
	document.getElementById("box4").style.top = "440px";
	document.getElementById("box5").style.top = "440px";
	document.getElementById("pressuregraph").style.top = "440px";
	document.getElementById("box3").style.top = "900px";
	document.getElementById("box6").style.top = "900px";
	document.getElementById("box7").style.top = "900px";
	document.getElementById("box8").style.top = "900px";
	document.getElementById("buttonBox").style.top = "1390px";
	document.getElementById("topcontrolbox").style.height = "1785px";
	document.getElementById("customFingeringFill").value = "12";
}


function mapCC() {
	mapSelection = 0;

	slider.noUiSlider.set([inputSliderMin[0], inputSliderMax[0]]);
	slider2.noUiSlider.set([outputSliderMin[0], outputSliderMax[0]]);
	document.getElementById("pressureChannel").style.visibility = "visible";
	document.getElementById("pressureCC").style.visibility = "visible";
	document.getElementById("expressionChannel").style.visibility = "visible";
	document.getElementById("highByte").style.visibility = "visible";	
	document.getElementById("box7").style.display = "block";
	document.getElementById("box6").style.display = "none";
	if(curve[0] < 3) {document.getElementById("curveRadio" + curve[0]).checked = true;
	}
	if (version > 1.8){
		document.getElementById("pressureMappingHeader").innerHTML="CC Mapping";
	}
}

function mapVelocity() {
	mapSelection = 1;
	slider.noUiSlider.set([inputSliderMin[1], inputSliderMax[1]]);
	slider2.noUiSlider.set([outputSliderMin[1], outputSliderMax[1]]);
	if(curve[1] < 3) {document.getElementById("curveRadio" + curve[1]).checked = true;
	}
	document.getElementById("box7").style.display = "block";
	document.getElementById("box6").style.display = "none";	
	document.getElementById("pressureMappingHeader").innerHTML="Velocity Mapping";
	//console.log(mapSelection);
}

function mapAftertouch() {
	mapSelection = 2;
	slider.noUiSlider.set([inputSliderMin[2], inputSliderMax[2]]);
	slider2.noUiSlider.set([outputSliderMin[2], outputSliderMax[2]]);
	if(curve[2] < 3) {document.getElementById("curveRadio" + curve[2]).checked = true;
	}
	document.getElementById("box7").style.display = "block";
	document.getElementById("box6").style.display = "none";
	document.getElementById("pressureMappingHeader").innerHTML="Channel Pressure Mapping";	
}

function mapPoly() {
	mapSelection = 3;
	slider.noUiSlider.set([inputSliderMin[3], inputSliderMax[3]]);
	slider2.noUiSlider.set([outputSliderMin[3], outputSliderMax[3]]);
	if(curve[3] < 3) {document.getElementById("curveRadio" + curve[3]).checked = true;
	}
	document.getElementById("box7").style.display = "block";
	document.getElementById("box6").style.display = "none";	
	document.getElementById("pressureMappingHeader").innerHTML="Key Pressure Mapping";
}


function okayCCmap() {
	mapSelection = 4;
	document.getElementById("box7").style.display = "none";
	document.getElementById("box6").style.display = "block";
	document.getElementById("pressureChannel").style.visibility = "hidden";
	document.getElementById("pressureCC").style.visibility = "hidden";
	document.getElementById("expressionChannel").style.visibility = "hidden";
	document.getElementById("highByte").style.visibility = "hidden";
}



function pressureGraph() {
    

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
	if (version > 2.0 || version == "Unknown"){
	document.getElementById("jumpFactor7").value = "3";
	document.getElementById("jumpFactor8").value = "7";
	document.getElementById("jumpFactor9").value = "20";
	document.getElementById("jumpFactor11").value = "12";
	document.getElementById("jumpFactor12").value = "50";
	for (var i = 7; i < 10; i++) {
		var k = document.getElementById("jumpFactor" + (i));
			k.dispatchEvent(new Event('input'));
		var j = document.getElementById("jumpFactor" + (i)).value;
		sendJumpFactor(i, j);
	}
	for (var i = 11; i < 13; i++) {
		var k = document.getElementById("jumpFactor" + (i));
			k.dispatchEvent(new Event('input'));
		var j = document.getElementById("jumpFactor" + (i)).value;
		sendJumpFactor(i, j);
	}		
	}
	
	else{
	document.getElementById("jumpFactor1").value = "25";
	document.getElementById("jumpFactor2").value = "15";
	document.getElementById("jumpFactor3").value = "15";
	document.getElementById("jumpFactor4").value = "15";
	document.getElementById("jumpFactor5").value = "30";
	document.getElementById("jumpFactor6").value = "60";
	document.getElementById("jumpFactor7").value = "3";
	document.getElementById("jumpFactor8").value = "7";
	document.getElementById("jumpFactor9").value = "100";
	document.getElementById("jumpFactor10").value = "7";
	document.getElementById("jumpFactor11").value = "9";
	document.getElementById("jumpFactor12").value = "9";

	for (var i = 1; i < 13; i++) {
		var k = document.getElementById("jumpFactor" + (i));
			k.dispatchEvent(new Event('input'));
		var j = document.getElementById("jumpFactor" + (i)).value;
		sendJumpFactor(i, j);
	}
	}
}

function advancedBagDefaults() {
	
	
	if (version > 2.0 || version == "Unknown"){
	document.getElementById("jumpFactor1").value = "50";
	document.getElementById("jumpFactor2").value = "20";
	document.getElementById("jumpFactor3").value = "20";
	document.getElementById("jumpFactor4").value = "50";
	document.getElementById("jumpFactor6").value = "75";

	for (var i = 1; i < 7; i++) {
		var k = document.getElementById("jumpFactor" + (i));
			k.dispatchEvent(new Event('input'));
		var j = document.getElementById("jumpFactor" + (i)).value;
		sendJumpFactor(i, j);
	}
	}
	
	
	else{
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


	for (var i = 1; i < 13; i++) {
		var k = document.getElementById("jumpFactor" + (i));
			k.dispatchEvent(new Event('input'));
		var j = document.getElementById("jumpFactor" + (i)).value;
		sendJumpFactor(i, j);

	}
	}
}


function sendJumpFactor(factor, selection) {
	selection = parseFloat(selection);
	blink(1);
	sendToWARBL(104, factor);
	sendToWARBL(105, selection);
}


function sendRow(rowNum) {
	blink(1);
	updateCells();
	sendToWARBL(102, 90 + rowNum);
	var y = (100) + parseFloat(document.getElementById("row" + rowNum).value);
	sendToWARBL(102, y);
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
	sendToWARBL(102, 90 + MIDIrowNum);
	var y = (112) + parseFloat(document.getElementById("MIDIrow" + MIDIrowNum).value);
	sendToWARBL(102, y);
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
	sendToWARBL(102, 90 + rowNum);
	sendToWARBL(106, y);
}

function sendByte2(rowNum) {
	blink(1);
	MIDIvalueChange();
	sendToWARBL(102, 90 + rowNum);
	var y = parseFloat(document.getElementById("byte2_" + (rowNum)).value);
	sendToWARBL(107, y);
}

function sendByte3(rowNum) {
	blink(1);
	MIDIvalueChange();
	sendToWARBL(102, 90 + rowNum);
	var y = parseFloat(document.getElementById("byte3_" + (rowNum)).value);
	sendToWARBL(108, y);
}

function sendMomentary(rowNum) { //send momentary
	blink(1);
	updateCells();
	var y = document.getElementById("checkbox" + rowNum).checked
	sendToWARBL(102, 90 + rowNum);
	if (y == false) {
		sendToWARBL(102, 117);
	}
	if (y == true) {
		sendToWARBL(102, 118);
	}
}

//switches

function sendVented(selection) {
	selection = +selection; //convert true/false to 1/0
	blink(1);
	sendToWARBL(104, 40);
	sendToWARBL(105, selection);
	updateSelected();
}

function sendBagless(selection) {
	selection = +selection; //convert true/false to 1/0
	blink(1);
	sendToWARBL(104, 41);
	sendToWARBL(105, selection);
}

function sendSecret(selection) {
	selection = +selection; //convert true/false to 1/0
	blink(1);
	sendToWARBL(104, 42);
	sendToWARBL(105, selection);
}

function sendInvert(selection) {
	selection = +selection; //convert true/false to 1/0
	blink(1);
	sendToWARBL(104, 43);
	sendToWARBL(105, selection);
}

function sendCustom(selection) {
	selection = +selection; //convert true/false to 1/0
	blink(1);
	updateCustom();
	sendToWARBL(104, 44);
	sendToWARBL(105, selection);
}


function sendExpression(selection) {
	selection = +selection; //convert true/false to 1/0
	blink(1);
	//if (selection == 1) {
		//document.getElementById("overrideExpression").disabled = false;
	//} else (document.getElementById("overrideExpression").disabled = true);
	sendToWARBL(104, 13);
	sendToWARBL(105, selection);
}

function sendRawPressure(selection) {
	selection = +selection; //convert true/false to 1/0
	updateCustom();
	blink(1);
	sendToWARBL(104, 15);
	sendToWARBL(105, selection);
}

function sendVelocity(selection) {
	selection = +selection; //convert true/false to 1/0
	blink(1);
	sendToWARBL(104, 45);
	sendToWARBL(105, selection);
	updateCells();
}

function sendAftertouch(selection, polyselection) {
	selection = +selection; //convert true/false to 1/0
    var val = selection | ((+polyselection) << 1);
	blink(1);
	sendToWARBL(104, 46);
	sendToWARBL(105, val);
}

function sendForceVelocity(selection) {
	selection = +selection; 
	blink(1);
	sendToWARBL(104, 47);
	sendToWARBL(105, selection);
}

function sendHack1(selection) {
	selection = +selection; 
	blink(1);
	sendToWARBL(104, 48);
	sendToWARBL(105, selection);
}

function sendHack2(selection) {
	selection = +selection; 
	blink(1);
	sendToWARBL(104, 49);
	sendToWARBL(105, selection);
}

function sendOverride(selection) {
	selection = +selection; 
	blink(1);
	sendToWARBL(104, 50);
	sendToWARBL(105, selection);
}

function sendBoth(selection) {
	selection = +selection; 
	blink(1);
	sendToWARBL(104, 51);
	sendToWARBL(105, selection);
}

function sendR4flatten(selection) {
	selection = +selection; 
	blink(1);
	updateCustom();
	sendToWARBL(104, 52);
	sendToWARBL(105, selection);
}

/* //curently unused--can be used for an additional switch.
function sendInvertR4(selection) {
	selection = +selection;
	blink(1);
	sendToWARBL(104, 53);
	sendToWARBL(105, selection);
}
*/


//end switches



function saveAsDefaults() {
	modalclose(2);
	blink(3);
	sendToWARBL(102, 123);
}

function saveAsDefaultsForAll() {
	modalclose(3);
	blink(3);
	sendToWARBL(102, 124);
}

function restoreAll() {
	modalclose(4);
	blink(3);
	sendToWARBL(102, 125);
	communicationMode = 0;
	if (version > 1.9) { //WARBL will restart, so try to reconnect to it.
		setTimeout(connect, 3000);
	}
}

function autoCalibrateBell() {
	if(communicationMode){
	LEDon();}
	setTimeout(LEDoff, 5000);
	sendToWARBL(106, 42);
}

function autoCalibrate() {
	if(communicationMode){
	LEDon();}
	setTimeout(LEDoff, 10000);
	sendToWARBL(102, 127);
}

function frequencyFromNoteNumber(note) {
	return 440 * Math.pow(2, (note - 69) / 12);
}

function modal(modalId) {
	document.getElementById("open-modal" + modalId).classList.add('modal-window-open');
	if(modalId == 18){clearConsole()};
}

function modalclose(modalId) {
	document.getElementById("open-modal" + modalId).classList.remove('modal-window-open');
}

function blink(blinkNum) {
	if (communicationMode){
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

var output14 = document.getElementById("demo14");
var depthSlider = document.getElementById('depth');
depthSlider.addEventListener('input', sliderChange);
function sliderChange() {
  output14.innerHTML = depthSlider.value;
}

var output1 = document.getElementById("demo1");
var jumpSlider1 = document.getElementById('jumpFactor1');
jumpSlider1.addEventListener('input', slider1Change);
function slider1Change() {
  output1.innerHTML = jumpSlider1.value;
}

var output2 = document.getElementById("demo2");
var jumpSlider2 = document.getElementById('jumpFactor2');
jumpSlider2.addEventListener('input', slider2Change);
function slider2Change() {
  output2.innerHTML = jumpSlider2.value;
}

var output3 = document.getElementById("demo3");
var jumpSlider3 = document.getElementById('jumpFactor3');
jumpSlider3.addEventListener('input', slider3Change);
function slider3Change() {
  output3.innerHTML = jumpSlider3.value;
}

var output4 = document.getElementById("demo4");
var jumpSlider4 = document.getElementById('jumpFactor4');
jumpSlider4.addEventListener('input', slider4Change);
function slider4Change() {
  output4.innerHTML = jumpSlider4.value;
}

var output5 = document.getElementById("demo5");
var jumpSlider5 = document.getElementById('jumpFactor5');
jumpSlider5.addEventListener('input', slider5Change);
function slider5Change() {
  output5.innerHTML = jumpSlider5.value;
}

var output6 = document.getElementById("demo6");
var jumpSlider6 = document.getElementById('jumpFactor6');
jumpSlider6.addEventListener('input', slider6Change);
function slider6Change() {
  output6.innerHTML = jumpSlider6.value;
}

var output7 = document.getElementById("demo7");
var jumpSlider7 = document.getElementById('jumpFactor7');
jumpSlider7.addEventListener('input', slider7Change);
function slider7Change() {
  output7.innerHTML = jumpSlider7.value;
}

var output8 = document.getElementById("demo8");
var jumpSlider8 = document.getElementById('jumpFactor8');
jumpSlider8.addEventListener('input', slider8Change);
function slider8Change() {
  output8.innerHTML = jumpSlider8.value;
}

var output9 = document.getElementById("demo9");
var jumpSlider9 = document.getElementById('jumpFactor9');
jumpSlider9.addEventListener('input', slider9Change);
function slider9Change() {
  output9.innerHTML = jumpSlider9.value;
}

var output10 = document.getElementById("demo10");
var jumpSlider10 = document.getElementById('jumpFactor10');
jumpSlider10.addEventListener('input', slider10Change);
function slider10Change() {
  output10.innerHTML = jumpSlider10.value;
}

var output10b = document.getElementById("demo10b");
var jumpSlider10b = document.getElementById('jumpFactor10b');
jumpSlider10b.addEventListener('input', slider10bChange);
function slider10bChange() {
  output10b.innerHTML = jumpSlider10b.value;
}

var output11 = document.getElementById("demo11");
var jumpSlider11 = document.getElementById('jumpFactor11');
jumpSlider11.addEventListener('input', slider11Change);
function slider11Change() {
  output11.innerHTML = jumpSlider11.value;
}

var output12 = document.getElementById("demo12");
var jumpSlider12 = document.getElementById('jumpFactor12');
jumpSlider12.addEventListener('input', slider12Change);
function slider12Change() {
  output12.innerHTML = jumpSlider12.value;
}



function clearConsole() { //clear MIDI console
document.getElementById("console").innerHTML = "";
consoleEntries = 0;	
}



//sets up initial values for selects/fields/radios and constantly keeps the proper options enabled/disabled
function updateCells() {

	if (version > 1.6) {
		var p = document.getElementById("checkbox10").checked;
		if (p == true) {
			document.getElementById("checkbox12").disabled = true;
			document.getElementById("switch12").style.cursor = "default";
		} else {
			document.getElementById("checkbox12").disabled = false;
			document.getElementById("switch12").style.cursor = "pointer";
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

//
// WebAudio related functionality
//
var AudioContextFunc = window.AudioContext || window.webkitAudioContext;

var audioContext = new AudioContextFunc();

var player = new WebAudioFontPlayer();

player.loader.decodeAfterLoading(audioContext, '_tone_0650_SBLive_sf2');

//
// Toggle between adding and removing the "responsive" class to topnav when the user clicks on the icon
//
function topNavFunction() {

	var x = document.getElementById("myTopnav");
	
	if (x.className === "topnav") {
		x.className += " responsive";
	} else {
		x.className = "topnav";
	}

}

//
// Preset import/export features
//

// 
// Synchronous sleep (returns Promise for async/await use)
//
function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

//
// Import a WARBL preset from a file
//


function importPreset(context){

	//console.log("importPreset");

	//console.log("current tab = "+instrument);

	// Put up error message if WARBL is not connected
	if (!WARBLout){
		
		document.getElementById("modal14-title").innerHTML="Preset import only allowed when WARBL is connected";

		document.getElementById("modal14-ok").style.opacity = 1.0;

		modal(14);

		return;

	}

	// For versioning preset files
	var maxSupportedPresetVersion = 1;

	// How long to wait in msec between sending commands
	var delay = 5;
     
    //debugger;

    // Make sure there is a file selected
    if (context.files && (context.files.length != 0)){

		// Show the import modal
		var fname = context.files[0].name;
		fname = fname.replace(".warbl","");
		fname = fname.replace(".WARBL","");

		document.getElementById("modal14-title").innerHTML="Importing: "+context.files[0].name;

		// Disable the OK button until after the import is complete
		document.getElementById("modal14-ok").style.opacity = 0.001;

		modal(14);

	  	var reader = new FileReader();

	    reader.onload = async function(){ 

	    	//debugger;

	    	// Parse the file
	    	try{

	    		var theImportObject = JSON.parse(reader.result);

	    	}catch(e){

				document.getElementById("modal14-title").innerHTML="Error: File is not a WARBL Preset";
				return;

	    	}

	    	// Sanity check the import file

	    	if (!theImportObject){

				document.getElementById("modal14-title").innerHTML="Error: File is not a WARBL Preset";
				return;

	    	}

	    	if ((!theImportObject.signature) || (theImportObject.signature != "WARBL")){

				document.getElementById("modal14-title").innerHTML="Error: File is not a WARBL Preset";
				return;

	    	}

	    	if (!theImportObject.version){

				document.getElementById("modal14-title").innerHTML="Error: File is not a WARBL Preset";
				return;

			}

	    	if (theImportObject.version > maxSupportedPresetVersion){

				document.getElementById("modal14-title").innerHTML="Error: WARBL Preset Version "+theImportObject.version+" not supported by this version of the configuration tool";
				return;

			}

			//debugger;

	    	// Send the data to the WARBL
	    	var nMessages = theImportObject.messages.length;

	    	var i;
	    	var byte0, byte1, byte2;

	    	// Send the firmware version
    		byte0 = theImportObject.messages[0][0];
    		byte1 = theImportObject.messages[0][1];
    		byte2 = theImportObject.messages[0][2];
	    	sendToWARBL(byte1, byte2);
	    	
    		// Synchronous sleep to allow command processing
    		await sleep(delay);

			//
	    	// Determine the current instrument that was set at export
	    	//
	  		var exportedInstrument =  theImportObject.messages[10][2] - 60;

	  		//
	  		// Determine the current target instrument tab
	  		//
	  		var targetInstrument = 30 + instrument;

	  		// console.log("exportedInstrument = "+exportedInstrument);
	  		// console.log("targetInstrument = "+(targetInstrument-30));
	  		
	  		//
	  		// Determine the current target key shift
	  		//
	  		var targetKey = 111 + instrument;

	    	// send the fingering and key shift for the current tab only
			switch (exportedInstrument){

				case 0:

		    		byte0 = theImportObject.messages[1][0];
		    		byte1 = theImportObject.messages[1][2];
		    		byte2 = targetInstrument;

			    	sendToWARBL(byte1, byte2);

		    		// Synchronous sleep to allow command processing
		    		await sleep(delay);
		    		
		    		byte0 = theImportObject.messages[2][0];
		    		byte1 = theImportObject.messages[2][1];
		    		byte2 = theImportObject.messages[2][2];
			    	
			    	sendToWARBL(byte1, byte2);

		    		// Synchronous sleep to allow command processing
		    		await sleep(delay);
		    		
		    		byte0 = theImportObject.messages[3][0];
		    		byte1 = targetKey;		
		    		byte2 = theImportObject.messages[3][2];
			    	
			    	sendToWARBL(byte1, byte2);

		    		// Synchronous sleep to allow command processing
		    		await sleep(delay);
					
					break;

				case 1:

		    		byte0 = theImportObject.messages[4][0];
		    		byte1 = theImportObject.messages[4][1];
		    		byte2 = targetInstrument;

			    	sendToWARBL(byte1, byte2);

		    		// Synchronous sleep to allow command processing
		    		await sleep(delay);
		    		
		    		byte0 = theImportObject.messages[5][0];
		    		byte1 = theImportObject.messages[5][1];
		    		byte2 = theImportObject.messages[5][2];
			    	
			    	sendToWARBL(byte1, byte2);

		    		// Synchronous sleep to allow command processing
		    		await sleep(delay);
		    		
		    		byte0 = theImportObject.messages[6][0];
		    		byte1 = targetKey;		
		    		byte2 = theImportObject.messages[6][2];
			    	
			    	sendToWARBL(byte1, byte2);

		    		// Synchronous sleep to allow command processing
		    		await sleep(delay);

					break;

				case 2:

		    		byte0 = theImportObject.messages[7][0];
		    		byte1 = theImportObject.messages[7][1];
		    		byte2 = targetInstrument;

			    	sendToWARBL(byte1, byte2);

		    		// Synchronous sleep to allow command processing
		    		await sleep(delay);
		    		
		    		byte0 = theImportObject.messages[8][0];
		    		byte1 = theImportObject.messages[8][1];
		    		byte2 = theImportObject.messages[8][2];
			    	
			    	sendToWARBL(byte1, byte2);

		    		// Synchronous sleep to allow command processing
		    		await sleep(delay);
		    		
		    		byte0 = theImportObject.messages[9][0];
		    		byte1 = targetKey;		
		    		byte2 = theImportObject.messages[9][2];
			    	
			    	sendToWARBL(byte1, byte2);

		    		// Synchronous sleep to allow command processing
		    		await sleep(delay);

					break;

			}
	    	
			// Skip command 10 - Selected instrument at save time

	    	// Skip command 11 - Sets default

	    	// Send the rest of the data

	    	for (i=12;i<nMessages;++i){

	    		byte0 = theImportObject.messages[i][0];
	    		byte1 = theImportObject.messages[i][1];
	    		byte2 = theImportObject.messages[i][2];

				//console.log("Sending Message #"+i+":"+byte0+" "+byte1+" "+byte2);

	    		sendToWARBL(byte1, byte2);

	    		// Synchronous sleep to allow command processing
	    		await sleep(delay);

	    	}

	    	// Refresh the UI after a short delay by putting the device back in communications mode
	    	setTimeout(function(){

	    		//console.log("refreshing UI");

				sendToWARBL(102, 126);

				// Show the import complete modal
				document.getElementById("modal14-title").innerHTML="Preset Import Complete!";

				// Show the OK button until after the import is complete
				document.getElementById("modal14-ok").style.opacity = 1.0;

				// Clear the file input selector after a delay
				setTimeout(function(){

					$('#importPreset').val('');

				},1000);

	    	},500);

	    } 

    	reader.readAsText(context.files[0]);

    } 

}

//
// Export the current WARBL preset to a file
//
function exportPreset(){

	//console.log("exportPreset");

	// Put up error message if WARBL is not connected
	if (!WARBLout){
		
		document.getElementById("modal14-title").innerHTML="Preset export only allowed when WARBL is connected";

		document.getElementById("modal14-ok").style.opacity = 1.0;

		modal(14);

		return;

	}

	var nMessages = 0;

	// Initialize the export structure
	var theExportObject = {
		signature:"WARBL",  // Used to sanity check imports
		version:1,			// In case we need to version the export structure
		messages:[]			// Preset messages
	};

	// 
	// Download the exported preset to a file
	//
	function downloadPreset(content, filename, contentType){

		const a = document.createElement('a');

		const file = new Blob([content], {type: contentType});

		a.href= URL.createObjectURL(file);
		a.download = filename;
		a.click();

		URL.revokeObjectURL(a.href);

		// Tell the user that the export is complete
		document.getElementById("modal14-title").innerHTML="Preset Export Complete!";
		
		// Make sure the OK button is showing
		document.getElementById("modal14-ok").style.opacity = 1.0;
		
		modal(14);

	};

	//
	// Export message proxy
	//
	function exportMessageHandler(byte0,byte1,byte2){

		//console.log("Message #"+nMessages+":"+byte0+" "+byte1+" "+byte2);

		// Add the message to the message array
		theExportObject.messages.push([byte0,byte1,byte2]);

		nMessages++;

		if (nMessages == 135){
			
			//debugger;

			//console.log("exportMessageHandler: All messages received");

			// Stringify the messsages array
			var theExportObjectJSON = JSON.stringify(theExportObject);

			// Clear the global export proxy
			gExportProxy = null;

			// Do the preset file export
			downloadPreset(theExportObjectJSON,"WARBL_Preset.warbl","text/plain");

		}
	}

	// Setup the global export message proxy
	gExportProxy = exportMessageHandler;

	// Tell WARBL to enter communications mode
	// Received bytes will be forwarded to the export message handler instead
	sendToWARBL(102, 126); 

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
	      .text("Inches of H₂O");      

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