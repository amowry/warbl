
## WARBL is a USB MIDI wind controller.

Please see the web site for more info:

https://warbl.xyz

For updating the software, typical users would just install the .hex file using the installer that you can [download here](https://warbl.xyz/documentation.html).
 
Advanced users can use the Arduino IDE to modify and upload the code. WARBL uses some special settings for USB MIDI that make the initial IDE setup more complicated than with a normal Arduino. However, you only have to follow most of the steps once.

### How to use WARBL with the Arduino IDE:

* Install the latest version of the Arduino IDE. [You can download the latest version of the Arduino IDE here](https://www.arduino.cc/en/Main/Software)


* If you plan to do editing, please note that we are using 4-space indentation with a specific formatting style. To use this style in the Arduino editor, go to the Arduino preferences, and at the bottom where it says `More preferences can be edited directly in the file`, open and edit the preferences.txt file mentioned there, and add or change the `editor.tabs.size=4` line as shown. Also, copy the file **formatter.conf** from this repository to the same directory that the preferences.txt is, this will let you use the Auto Format feature in the Arduino Tools menu to properly format the code.


* Next you can open the 'warbl_firmware' project from the Arduino IDE


* WARBL runs on 3.3V and at 8Mhz, which is nonstandard for Arduino boards with the same processor. I used the Adafruit ItsyBitsy 32u4-3.3V for prototyping WARBL, so the easiest way to set up the IDE is to install the Adafruit boards package, [following the instructions here](https://learn.adafruit.com/introducting-itsy-bitsy-32u4?view=all#arduino-ide-setup). **Windows 7 only:** You will also have to install the drivers, following the link partway down the above page.



* Now, we need to change some USB settings, so we first edit the boards.txt file, which typically is found here (on Windows--Mac locations of the files below may be similar): 

  `C:\Users\(username)\AppData\Local\Arduino15\packages\adafruit\hardware\avr\1.4.12` 

    I use the free NotePad++ app to edit this. First, change the VID and PID to the WARBL USB VID and PID:

       itsybitsy32u4_3V.build.vid=0x04D8
  
       itsybitsy32u4_3V.build.pid=0xEE87
  
    Next, make the same changes to these lines:
  
       itsybitsy32u4_3V.vid.0=0x04D8
  
       itsybitsy32u4_3V.pid.0=0xEE87
 
    Then change the product name:
  
       itsybitsy32u4_3V.build.usb_product="WARBL"
  
    And finally the manufacturer:
  
       itsybitsy32u4_3V.build.usb_manufacturer="Mowry Stringed Instruments"



*	It is also necessary to configure the USBCore to request only 20mA of power. Otherwise iOS devices will say that the device uses too much power, even though it doesn’t. It only matters how much it requests. 
    So, find this file:
  
    `C:\Program Files (x86)\Arduino\hardware\arduino\avr\cores\arduino\USBCore.h`
  
    > **Please note:** You may want to make a backup copy of this file before changing it. Changing it will affect all USB boards that you program with the IDE. The setting that we’re changing probably won’t make a difference in most cases, but it’s important to know this. 

    Find this line and change the power consumption request to 20: 
     
        USB_CONFIG_POWER_MA(20) 



*	Next, in Arduino IDE, you’ll need to install three libraries that aren’t installed by default. They are:
 
    TimerOne, DIO2, and MIDIUSB
 
    To install them, go to Sketch > Include Library > Manage Libraries, then search for the name of each, one at a time. Then it will give you an option to install each one.



*	Now open the WARBL sketch that you saved to in your sketchbook folder. Four tabs should open. 
 
 
 
*	Next, tell it which board you have by going Tools > Board and select `Adafruit ItsyBitsy 32u4 3V 8MHz`.



*	Then turn on “show verbose output during upload” under File > Preferences. Now, if all went well, you should be able to click the upload button. It will compile first. 



*	Then, when it tries to upload, you should see this output repeating in the messages at the bottom of the screen:

    ```
    PORTS {} / {} => {}   
    PORTS {} / {} => {}    
    PORTS {} / {} => {}    
    PORTS {} / {} => {}    
    PORTS {} / {} => {}
    ```



*	Now, use a toothpick to double-click the WARBL reset button. The LED should pulse and the code should upload. If the LED doesn’t light or stays solid instead if pulsing, try again. If the IDE stops trying to upload, click “Upload” again, and try double-clicking again. It can take a few tries to get the timing right.





### A few additional notes:
The serial CDC class on WARBL needs to be turned off to make it a USB MIDI class-compliant device. This also means that you won't be able to use the serial monitor in Arduino IDE, and you'll have to double-click the programming button to install firmware. To turn serial off on the newest version of the Arduino IDE, you can uncomment the following line in the USBDesc.h file:
 
 #define CDC_DISABLED
 
 The above file should be located here: C:\Program Files (x86)\Arduino\hardware\arduino\avr\cores\arduino

Note: turning off the CDC class will turn it off for any Arduino that you program with the IDE, so you'll need to remember to turn it back on after programming the WARBL. You can leave the USBDesc.h file open and comment/uncomment the line and save it as necessary without having to close/reopen the IDE. I keep a shortcut to USBDesc.h on my desktop for this purpose.

Turning off the CDC class will also make it so that Windows 7 won't require drivers to be installed for WARBL to work in normal MIDI mode.

Have fun!
