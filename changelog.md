# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
## [Released]

## [1.9] - 2020-8-12
### Changed

- Changed recorder fingering 0 111 0100 from G to F# to mimic real instrument


## [Released]

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
