FireStep: change log
====================

v0.1.14
-------
* NEW: "prb" command probes to specified coordinate, testing given pin
* NEW: "sd" search delay consolidated from individual axes into common "syssd"	
* CHANGE: "syssd" defaults to 800us for each homing/probing calibration pulse
* CHANGE: "syshp" defaults to 3 for coarse homing speed

v0.1.13
------
* NEW: homing pulses synchronized for smoothness
* NEW: "syshp" configures homing pulses, which controls homing speed
* NEW: homing pulses reduced from 32 to 8
* NEW: latchBackoff is now universal instead of per axis (i.e., "syslb")
* NEW: "hom" now double checks at a slower speed for higher accuracy
* NEW: latchBackoff default is now 200 vs 32 pulses
* FIX: renamed Libraries to libraries as per Arduino standards

v0.1.12
------
* FIX: homing will now ignore axes with no minimum limit switch

v0.1.11
------
* MERGE: Neil's changes for OpenPnP support and full EMC02 setup

v0.1.10
------
* FIX: direction pin cache did not track configuration direction inversion

v0.1.9
------
* FIX: stepper enabled flag now matches actual enable pin value on startup.
* FIX: steppers disabled/enabled during pin configuration change

v0.1.8
------
* EEPROM read/write
* EEPROM startup command

v0.1.7
------
* io for digital/analog pins
* remove idle whine

v0.1.6
------
* fix for individual axis homing bug 

v0.1.5
------
* JSON command array
* dvs scale now supported

v0.1.4
------
* EMC02 pins corrected

v0.1.3
------
* Travel minimum default is now -32000 since FirePick Delta has negative homing coordinate

v0.1
------
* Initial release

