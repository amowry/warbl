
## WARBL is a USB MIDI wind controller.

Please see the web site for more info:

https://warbl.xyz

For updating the software, typical users would just install the .hex file using the installer that you can [download here](https://warbl.xyz/documentation.html).
 
Advanced users can use the Arduino IDE to modify and upload the code. WARBL uses some special settings for USB MIDI that make the initial IDE setup more complicated than with a normal Arduino. However, you only have to follow most of the steps once.

### How to modify, compile, and upload the code using the Arduino IDE:
•	Install the latest version of the Arduino IDE. [You can download the latest version of the Arduino IDE here](https://www.arduino.cc/en/Main/Software)


•	Next download the folder containing the latest WARBL code in Arduino format, and put in your Arduino “sketch” folder, which is in PC is Documents/Arduino by default. WARBL runs on 3.3V and at 8Mhz, which is nonstandard for Arduino boards with the same processor. I use the Adafruit ItsyBitsy 32u4-3.3V for prototyping WARBL, so the easiest way to set up the IDE is to install the Adafruit boards package, [following the instructions here](https://learn.adafruit.com/introducting-itsy-bitsy-32u4?view=all#arduino-ide-setup) **Windows 7 only:** You will also have to install the drivers, following the link partway down the above page.


•	Now, we need to change some USB settings, so we first edit the boards.txt file, which typically is found here: 

C:\Users\(username)\AppData\Local\Arduino15\packages\adafruit\hardware\avr\1.4.12

 I use the free NotePad++ app to edit this. You’ll change lines 296 and 297 to these to use the WARBL USB VID and PID:

itsybitsy32u4_3V.build.vid=0x04D8
itsybitsy32u4_3V.build.pid=0xEE87
 
Then change the product name on line 301:
itsybitsy32u4_3V.build.usb_product="WARBL"


•	It is also necessary to configure the USBCore to request only 20mA of power. Otherwise iOS devices will say that the device uses too much power, even though it doesn’t. It only matters how much it requests.

So, find this file:

C:\Program Files (x86)\Arduino\hardware\arduino\avr\cores\arduino\USBCore.h

**Please note:** You may want to make a backup copy of this file before changing it. Changing it will affect all USB boards that you program with the IDE. The setting that we’re changing probably won’t make a difference in most cases, but it’s important to know this. 

 
At the end of line 270, change the power consumption request to: USB_CONFIG_POWER_MA(20) 


•	Next, in Arduino IDE, you’ll need to install three libraries that aren’t installed by default. They are:
 
TimerOne
DIO2
MIDIUSB
 
To install them, go to Sketch > Include Library > Manage Libraries, then search for the name of each, one at a time. Then it will give you an option to install each one.


•	Now open the WARBL sketch that you saved to in your sketchbook folder. Four tabs should open. 
 
 
•	Next, tell it which board you have by going Tools > Board and select Adafruit ItsyBitsy 32u4 3V 8MHz.


•	Then turn on “show verbose output during upload” under File > Preferences. Now, if all went well, you should be able to click the upload button. It will compile first. 


•	Then, when it tries to upload, you should see this output repeating in the messages at the bottom of the screen:

PORTS {} / {} => {}
PORTS {} / {} => {}
PORTS {} / {} => {}
PORTS {} / {} => {}
PORTS {} / {} => {}


•	Now, use a toothpick to double-click the WARBL reset button. The LED should pulse and the code should upload. If the LED doesn’t light or stays solid instead if pulsing, try again. If the IDE stops trying to upload, click “Upload” again, and try double-clicking again. It can take a few tries to get the timing right.



**A few additional notes:**
By default, the serial CDC class on WARBL is turned off, which makes it a USB MIDI class-compliant device. This also means that you can’t use the serial monitor in Arduino IDE. To turn serial on you can comment out the following line in the USBCore.cpp tab: 
#define CDCCON_DISABLE 
Then you will be able to print to the serial monitor for debugging. Doing this may also make it so that you don’t have to double-click the reset button to upload code (though it doesn’t always work). 
Turning on the CDC class will also make so that Windows 7 will require the drivers to be installed for normal MIDI operation to work, and may interfere with MIDI modules (I’m not sure about this).

Have fun!
