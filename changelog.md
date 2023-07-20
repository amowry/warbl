# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
## [Unreleased]

## [2.2] - 2022-7-20
### Changed

- Bombarde fingering X OXO OOOO now plays Ab instead of A (in the key of Bb)

### Added

- Medieval bagpipes fingering
- Bansuri fingering

## [Released]

## [2.1] - 2021-10-24
### Changed

- There is no longer a USBCore.cpp file included with the sketch, because the newest Arduino IDE reintroduced the ability to turn off the CDC class in the USBDesc.h file. 
- Changed uilleann fingering X OXXXXOO from C to C#
- Reworked register jump/drop algorithm. There are now only two controls: jump time and drop time.
- Set velocity to a minimum of 1 so that notes are still turned on at lowest pressures.
- Changed the "Vented" switch in the Config Tool to radio buttons for Bag vs. Breath. The functionality is the same, but this seems more logical, as the "Closed" vs. "Vented setting was a holdover from the old mouthpiece options.
- When connected to the WARBL, the "Connect" button now changes to read "Disconnect". Clicking it will force the WARBL to stop sending CC messages for communication with the Config Tool. Note that this does not release the MIDI ports, so it is still necessary to unplug the WARBL to release the ports (and, for example, use the WARBL with another app on Windows).
- Various bug fixes

### Added

- Hysteresis for overblowing
- Transient note filter (key delay) feature for filtering out brief notes. This is useful when using notation software or MIDI sounds that are unforgiving of very brief "crossing noise".
- Breton bombarde fingering chart 
- Baroque flute fingering chart 



## [2.0] - 2021-5-13
### Changed

- Changed the MIDI console in the Config Tool to display up to 300 messages (CC messages used for WARBL/Config Tool communication are filtered out.)
- Many other small improvements to Config Tool user interface
- Changed recorder fingering 0 111 0100 from G to F# to mimic real instrument
- Fixed error in velocity calculation
- Changed how restoring factory settings works, so that the settings no longer need to be stored in EEPROM, freeing up space. Restoring settings now requires/causes WARBL to reboot, after which the Configuration Tool attempts to reconnect to it. Chrome occasionally freezes when WARBL reboots because of a Chrome bug, so occasionally Chrome may need to be restarted after a factory reset.
- Improved pressure mapping calculations so that they are approximately ten times faster. 

### Added

- Added Säckpipa major and minor fingering charts
- Added the ability to make a custom fingering chart based on simple tin whistle fingering patterns (plus the left thumb hole). Any MIDI note can be assigned to each pattern in the chart, and the R4 finger can optionally be used to flatten any note one semitone. Both the thumb and overblowing can optionally be used for register control, for a maximum range of three registers.
- Added key pressure in addition to channel pressure
- Added the ability to map pressure independently to CC, velocity, channel pressure, and key pressure.
- Ability to select output MIDI channel for notes
- Added legato slide/vibrato, which allows continuous sliding over a range of multiple semitomes without retriggering notes
- Ability to select MIDI bend range semitones (used with above to select number of semitones of sliding without retrigerring notes)
- MPE support (select MIDI channel 2 and turn on channel pressure)
- Abilty to override the default pressure range for pitch bend expression if overblowing is not being used
- Added preset import/export and pressure graph to Configuration Tool


## [1.8] - 2020-5-6
### Changed

- Fixed a bug causing settings in EEPROM to be corrupted in certain instances
- This version will restore factory settings, helping to avoid possible bugs from copying over the old settings.


## [Released]

## [1.7] - 2020-4-24
### Added

- Added two "modified saxophone" fingering charts
- Added EVI (trumpet) fingering chart
- Added Japanese shakuhachi fingering chart
- Added the ability to set a default instrument, i.e. the one that will be active when WARBL is powered on.
- Added the ability to send aftertouch (channel pressure) messages based on pressure.
- There is now the option to "force max velocity", meaning that if pressure is not being sent as velocity data, you can force WARBL to always send a Note On velocity of 127. That will maximize the volume of many MIDI apps. If this option isn't selected, the default velocity will be 64.

### Changed
- With recorder fingering you can now reach the second register by uncovering the thumb hole, and the third register by using overblowing while uncovering the thumb hole. Some alternate fingerings have been added to avoid conflicts between the registers (see the fingering chart for more info).

## [Released]

## [1.6] - 2019-09-05
### Added

- Added an extended Gaita fingering chart that allows playing a third octave.
- Added the ability to assign a button combination to begin autocalibration.
- Added custom GHB and Northumbrian vibrato, designed for closed fingering systems. Turning on the "Custom" switch with GHB or Northumbrian fingering will now assign holes R2 and R3 for vibrato use, and raising either or both fingers from their closed position will sharpen the note, assuming that raising them doesn't also trigger a different note in the fingering chart.
- Added XOXXXX Bb, XOXXXO Bb, OXOXXX C, and XXOXOX G# to tin the whistle/Irish flute fingering chart
- Added X XXO XX-- G# to recorder chart, where the "-" holes are ignored. Also added X OOO OOOO C#.
- Fingering for Chinese Xiao
- Added the ability for the bell sensor to close off the pipe using any fingering pattern instead of just uilleann. Now, with any fingering pattern, closing all holes and the bell sensor will stop the sound, unless the bell sensor is instead being used to control the register.

### Changed

- You now have the option of using the "custom" vibrato simultaneously with "slide"
- Modified default settings for the vented mouthpiece slightly
- Changed NoteOn velocity calculation slightly, add some weight to the velocity on the initial note after silence (tonguing a note). This brings it closer to the subsequent velocity used for legato notes. 


##

## [Released]

## [1.5] - 2019-05-21
### Added

- Option to map pressure to NoteOn velocity. This uses the same mapping options as for sending pressure as CC.
- Notes are now sent as legato, simply meaning that NoteOff events are sent immediately after NoteOn events, which signals to some MIDI synths that a legato effect is dersired.
- Semitone shift up and semitone shift down have been added to the button assignment options, for changing key.
- The option to select "momentary" for both octave shift up/down and semitone shift up/down (above) has been added to the button configuration. This allows using the buttons as octave keys, extending the range to 4 octaves, or as accidental keys, sharpening or flattening any note on-the-fly
- An "Uilleann, standard" fingering pattern has been added, which does not include the cross fingerings for G# and Bb. The previous uilleann fingering pattern that includes those cross-fingerings is now called "Uilleann, chromatic".
- updated all documentation to refer to the WARBL software as "firmware" to help avoid confusion between the firmware and installer packages, WARBL app, etc.
