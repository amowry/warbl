# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.5] - 2019-05-21
### Added

- Option to map pressure to NoteOn velocity. This uses the same mapping options as for sending pressure as CC.
- Notes are now sent as legato, simply meaning that NoteOff events are sent immediately after NoteOn events, which signals to some MIDI synths that a legato effect is dersired.
- Semitone shift up and semitone shift down have been added to the button assignment options, for changing key.
- The option to select "momentary" for both octave shift up/down and semitone shift up/down (above) has been added to the button configuration. This allows using the buttons as octave keys, extending the range to 4 octaves, or as accidental keys, sharpening or flattening any note on-the-fly
- An "Uilleann, standard" fingering pattern has been added, which does not include the cross fingerings for G# and Bb. The previous uilleann fingering pattern that includes those cross-fingerings is now called "Uilleann, chromatic".
